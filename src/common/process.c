// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "process.h"
#include "bb_array.h"
#include "bb_criticalsection.h"
#include "bb_thread.h"
#include "bb_time.h"
#include "dlist.h"
#include "sb.h"
#include "time_utils.h"
#include <stdlib.h>

static bb_thread_return_t process_io_thread(void *);

typedef struct win32Process_s {
	process_t base;

	struct win32Process_s *next;
	struct win32Process_s *prev;

	SYSTEMTIME startLocalTime;
	SYSTEMTIME endLocalTime;
	char startLocalTimeStr[256];
	char endLocalTimeStr[256];
	UINT64 startMS;
	u64 endMS;

	HANDLE hProcess;
	HANDLE hOutputRead;
	HANDLE hErrorRead;
	HANDLE hInputWrite;

	bb_critical_section cs;
	bb_thread_handle_t hThread;
	processIO stdoutThread;
	processIO stderrThread;
	b32 threadWanted;
	b32 threadDone;
} win32Process_t;
win32Process_t sentinelSubprocess;

void process_init(void)
{
	DLIST_INIT(&sentinelSubprocess);
}

static void process_report_error(const char *apiName, const char *cmdline)
{
	char *errorMessage = "Unable to format error message";
	DWORD lastError = GetLastError();
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorMessage, 0, NULL);
	BB_ERROR("process", "Failed to create process (%s):\n  %s\n  Error %u (0x%8.8X): %s",
	         apiName, cmdline, lastError, lastError, errorMessage);
	LocalFree(errorMessage);
}

processSpawnResult_t process_spawn(const char *dir, const char *cmdline, processSpawnType_t processSpawnType)
{
	processSpawnResult_t result = { 0 };

	if(!cmdline || !*cmdline)
		return result;

	if(!dir || !*dir)
		return result;

	if(processSpawnType == kProcessSpawn_OneShot) {
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		char *cmdlineDup = _strdup(cmdline);
		result.success = CreateProcessA(NULL, cmdlineDup, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi);
		free(cmdlineDup);

		if(result.success) {
			BB_LOG("process", "Created process: %s\n", cmdline);
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			return result;
		} else {
			process_report_error("CreateProcess", cmdline);
			return result;
		}
	}

	if(processSpawnType == kProcessSpawn_Tracked) {
		HANDLE hOutputReadTmp, hOutputRead, hOutputWrite;
		HANDLE hInputWriteTmp, hInputRead, hInputWrite;
		HANDLE hErrorReadTmp, hErrorRead, hErrorWrite;
		SECURITY_ATTRIBUTES sa;

		// Set up the security attributes struct.
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;

		// Create the child stdout pipe.
		if(CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0)) {
			// Create the child stderr pipe.
			if(CreatePipe(&hErrorReadTmp, &hErrorWrite, &sa, 0)) {
				// Create the child input pipe.
				if(CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0)) {
					// Create new output read handle and the input write handles. Set
					// the Properties to FALSE. Otherwise, the child inherits the
					// properties and, as a result, non-closeable handles to the pipes
					// are created.
					if(DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
					                   GetCurrentProcess(),
					                   &hOutputRead, // Address of new handle.
					                   0, FALSE,     // Make it uninheritable.
					                   DUPLICATE_SAME_ACCESS)) {
						if(DuplicateHandle(GetCurrentProcess(), hErrorReadTmp,
						                   GetCurrentProcess(),
						                   &hErrorRead, // Address of new handle.
						                   0, FALSE,    // Make it uninheritable.
						                   DUPLICATE_SAME_ACCESS)) {
							if(DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
							                   GetCurrentProcess(),
							                   &hInputWrite, // Address of new handle.
							                   0, FALSE,     // Make it uninheritable.
							                   DUPLICATE_SAME_ACCESS)) {
								PROCESS_INFORMATION pi;
								STARTUPINFO si;

								// Close inheritable copies of the handles you do not want to be
								// inherited.
								CloseHandle(hOutputReadTmp);
								CloseHandle(hInputWriteTmp);
								CloseHandle(hErrorReadTmp);

								ZeroMemory(&si, sizeof(STARTUPINFO));
								si.cb = sizeof(STARTUPINFO);
								si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
								si.hStdOutput = hOutputWrite;
								si.hStdInput = hInputRead;
								si.hStdError = hErrorWrite;
								si.wShowWindow = SW_HIDE;

								char *cmdlineDup = _strdup(cmdline);
								result.success = CreateProcess(NULL, cmdlineDup, NULL, NULL, TRUE,
								                               CREATE_NEW_CONSOLE, NULL, dir, &si, &pi);
								free(cmdlineDup);
								if(result.success) {
									BB_LOG("process", "Created tracked process: %s\n", cmdline);
									result.process = malloc(sizeof(win32Process_t));
									win32Process_t *process = (win32Process_t *)result.process;
									if(process) {
										memset(process, 0, sizeof(*process));

										process->base.command = _strdup(cmdline);
										process->base.dir = _strdup(dir);
										DLIST_INSERT_AFTER(&sentinelSubprocess, process);

										process->startMS = GetTickCount64();
										GetLocalTime(&process->startLocalTime);
										GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &process->startLocalTime, "h':'mm':'ss tt",
										               process->startLocalTimeStr, sizeof(process->startLocalTimeStr));

										process->hProcess = pi.hProcess;
										process->hOutputRead = hOutputRead;
										process->hInputWrite = hInputWrite;
										process->hErrorRead = hErrorRead;

										bb_critical_section_init(&process->cs);
										process->hThread = bbthread_create(process_io_thread, process);
										process->threadWanted = true;
									}
								} else {
									process_report_error("CreateProcess", cmdline);
								}

								// Close any unnecessary handles.
								CloseHandle(pi.hThread);

								// Close pipe handles (do not continue to modify the parent).
								// You need to make sure that no handles to the write end of the
								// output pipe are maintained in this process or else the pipe will
								// not close when the child process exits and the ReadFile will hang.
								CloseHandle(hOutputWrite);
								CloseHandle(hInputRead);
								CloseHandle(hErrorWrite);
							} else {
								process_report_error("DuplicateHandle", cmdline);
							}
						} else {
							process_report_error("DuplicateHandle", cmdline);
						}
					} else {
						process_report_error("DuplicateHandle", cmdline);
					}
				} else {
					process_report_error("CreatePipe", cmdline);
				}
			} else {
				process_report_error("CreatePipe", cmdline);
			}
		} else {
			process_report_error("CreatePipe", cmdline);
		}
	}

	return result;
}

static b32 process_thread_tick_io_buffer(win32Process_t *process, HANDLE handle, processIO *io)
{
	DWORD nBytesAvailable = 0;
	BOOL bAnyBytesAvailable = PeekNamedPipe(handle, NULL, 0, NULL, &nBytesAvailable, NULL);
	if(bAnyBytesAvailable) {
		if(nBytesAvailable) {
			bb_critical_section_lock(&process->cs);
			if(bba_reserve(*io, nBytesAvailable)) {
				char *buffer = io->data + io->count;
				DWORD nBytesRead = 0;
				BOOL ok = ReadFile(handle, buffer, nBytesAvailable, &nBytesRead, NULL);
				if(ok) {
					if(nBytesRead) {
						io->count += nBytesRead;
						bb_critical_section_unlock(&process->cs);
						return true;
					} else {
						process->threadDone = true;
					}
				} else {
					DWORD err = GetLastError();
					if(err == ERROR_BROKEN_PIPE) {
						process->threadDone = true;
					}
				}
			}
			bb_critical_section_unlock(&process->cs);
		}
	} else {
		DWORD err = GetLastError();
		if(err == ERROR_BROKEN_PIPE) {
			process->threadDone = true;
		}
	}
	return false;
}

static bb_thread_return_t process_io_thread(void *_process)
{
	win32Process_t *process = _process;
	while(process->threadWanted) {
		b32 readData = false;
		readData = readData || process_thread_tick_io_buffer(process, process->hOutputRead, &process->stdoutThread);
		readData = readData || process_thread_tick_io_buffer(process, process->hErrorRead, &process->stderrThread);
		if(!readData) {
			bb_sleep_ms(16);
		}
	}
	process->threadDone = true;
	return 0;
}

static void process_tick_io_buffer(win32Process_t *process, processIOPtr *ioOut, processIO *ioMain, processIO *ioThread)
{
	if(ioThread->count) {
		bb_critical_section_lock(&process->cs);

		if(bba_reserve(*ioMain, ioMain->count + ioThread->count + 1)) {
			memcpy(ioMain->data + ioMain->count, ioThread->data, ioThread->count);
			ioOut->buffer = ioMain->data + ioMain->count;
			ioOut->nBytes = ioThread->count;
			ioMain->count += ioThread->count;
			ioMain->data[ioMain->count] = '\0';
			ioThread->count = 0;
		}

		bb_critical_section_unlock(&process->cs);
	}
}

processTickResult_t process_tick(process_t *base)
{
	processTickResult_t result = { 0, 0, 0 };
	win32Process_t *process = (win32Process_t *)(base);
	bool wasDone = process->base.done;
	if(process->threadDone) {
		process->base.done = true;
	}

	process_tick_io_buffer(process, &result.stdoutIO, &process->base.stdoutBuffer, &process->stdoutThread);
	process_tick_io_buffer(process, &result.stderrIO, &process->base.stderrBuffer, &process->stderrThread);

	if(process->base.done && !wasDone) {
		process->endMS = GetTickCount64();
		GetLocalTime(&process->endLocalTime);
		GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &process->endLocalTime, "h':'mm':'ss tt",
		               process->endLocalTimeStr, sizeof(process->endLocalTimeStr));
	}

	result.done = process->base.done;
	return result;
}

void process_free(process_t *base)
{
	win32Process_t *process = (win32Process_t *)(base);

	if(process->hThread) {
		process->threadWanted = false;
		bbthread_join(process->hThread);
	}

	bb_critical_section_shutdown(&process->cs);

	bba_free(process->stdoutThread);
	bba_free(process->stderrThread);
	bba_free(process->base.stdoutBuffer);
	bba_free(process->base.stderrBuffer);

	CloseHandle(process->hOutputRead);
	CloseHandle(process->hErrorRead);
	CloseHandle(process->hInputWrite);
	CloseHandle(process->hProcess);

	DLIST_REMOVE(process);
	free(process->base.command);
	free(process->base.dir);
	free(process);
}

void process_get_timings(process_t *base, const char **start, const char **end, u64 *elapsed)
{
	win32Process_t *process = (win32Process_t *)(base);
	*start = process->startLocalTimeStr;
	*end = process->endLocalTimeStr;
	*elapsed = (process->base.done) ? process->endMS - process->startMS : GetTickCount64() - process->startMS;
}
