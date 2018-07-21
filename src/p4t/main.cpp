// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "app.h"
#include "config.h"
#include "keys.h"
#include "time_utils.h"
#include "va.h"

#include <windowsx.h>

#ifdef _DEBUG
#define LEAK_CHECK
#endif // #ifdef _DEBUG

#ifdef LEAK_CHECK
#include <crtdbg.h>
#endif // #ifdef LEAK_CHECK

namespace ImGui
{
	IMGUI_API void EndFrame();
}

// Data
static LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
static D3DPRESENT_PARAMETERS g_d3dpp;
static bool g_hasFocus;
static bool g_trackingMouse;
static int g_dpi = USER_DEFAULT_SCREEN_DPI;
static ImGuiStyle g_defaultStyle;

static void ResetD3D()
{
	if(!g_pd3dDevice)
		return;
	ImGui_ImplDX9_InvalidateDeviceObjects();
#ifdef NDEBUG
	g_pd3dDevice->Reset(&g_d3dpp);
#else
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	IM_ASSERT(hr != D3DERR_INVALIDCALL);
#endif
	ImGui_ImplDX9_CreateDeviceObjects();
}

static void UpdateDpiDependentStyle()
{
	ImGuiStyle &s = ImGui::GetStyle();
	s = g_defaultStyle;
	s.WindowPadding.x *= g_config.dpiScale;
	s.WindowPadding.y *= g_config.dpiScale;
	s.WindowMinSize.x *= g_config.dpiScale;
	s.WindowMinSize.y *= g_config.dpiScale;
	s.ChildWindowRounding *= g_config.dpiScale;
	s.FramePadding.x *= g_config.dpiScale;
	s.FramePadding.y *= g_config.dpiScale;
	s.FrameRounding *= g_config.dpiScale;
	s.ItemSpacing.x *= g_config.dpiScale;
	s.ItemSpacing.y *= g_config.dpiScale;
	s.ItemInnerSpacing.x *= g_config.dpiScale;
	s.ItemInnerSpacing.y *= g_config.dpiScale;
	s.TouchExtraPadding.x *= g_config.dpiScale;
	s.TouchExtraPadding.y *= g_config.dpiScale;
	s.IndentSpacing *= g_config.dpiScale;
	s.ColumnsMinSpacing *= g_config.dpiScale;
	s.ScrollbarSize *= g_config.dpiScale;
	s.ScrollbarRounding *= g_config.dpiScale;
	s.GrabMinSize *= g_config.dpiScale;
	s.GrabRounding *= g_config.dpiScale;
	s.DisplayWindowPadding.x *= g_config.dpiScale;
	s.DisplayWindowPadding.y *= g_config.dpiScale;
	s.DisplaySafeAreaPadding.x *= g_config.dpiScale;
	s.DisplaySafeAreaPadding.y *= g_config.dpiScale;
}

static void InitFonts()
{
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();
	if(g_config.uiFontConfig.enabled && g_config.uiFontConfig.size > 0 && *sb_get(&g_config.uiFontConfig.path)) {
		io.Fonts->AddFontFromFileTTF(sb_get(&g_config.uiFontConfig.path), g_config.uiFontConfig.size * g_config.dpiScale);
	} else {
		io.Fonts->AddFontDefault();
	}
	if(g_config.logFontConfig.enabled && g_config.logFontConfig.size > 0 && *sb_get(&g_config.logFontConfig.path)) {
		io.Fonts->AddFontFromFileTTF(sb_get(&g_config.logFontConfig.path), g_config.logFontConfig.size * g_config.dpiScale);
	} else {
		io.Fonts->AddFontDefault();
	}
}

void UpdateDpiDependentResources()
{
	InitFonts();
	ResetD3D();
	UpdateDpiDependentStyle();
}

bool g_needUpdateDpiDependentResources;
void QueueUpdateDpiDependentResources()
{
	g_needUpdateDpiDependentResources = true;
}

BOOL EnableNonClientDpiScalingShim(_In_ HWND hwnd)
{
	if(g_config.dpiAware) {
		HMODULE hModule = GetModuleHandleA("User32.dll");
		if(hModule) {
			typedef BOOL(WINAPI * Proc)(_In_ HWND hwnd);
			Proc proc = (Proc)(void *)(GetProcAddress(hModule, "EnableNonClientDpiScaling"));
			if(proc) {
				return proc(hwnd);
			}
		}
	}
	return FALSE;
}

UINT GetDpiForWindowShim(_In_ HWND hwnd)
{
	if(g_config.dpiAware) {
		HMODULE hModule = GetModuleHandleA("User32.dll");
		if(hModule) {
			typedef UINT(WINAPI * Proc)(_In_ HWND hwnd);
			Proc proc = (Proc)(void *)(GetProcAddress(hModule, "GetDpiForWindow"));
			if(proc) {
				return proc(hwnd);
			}
		}
	}
	return USER_DEFAULT_SCREEN_DPI;
}

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
	case WM_NCCREATE:
		EnableNonClientDpiScalingShim(hWnd);
		g_dpi = (int)GetDpiForWindowShim(hWnd);
		g_config.dpiScale = g_dpi / (float)USER_DEFAULT_SCREEN_DPI;
		UpdateDpiDependentStyle();
		break;
	case WM_DPICHANGED: {
		g_dpi = HIWORD(wParam);
		g_config.dpiScale = g_dpi / (float)USER_DEFAULT_SCREEN_DPI;
		UpdateDpiDependentResources();

		RECT *const prcNewWindow = (RECT *)lParam;
		SetWindowPos(hWnd,
		             NULL,
		             prcNewWindow->left,
		             prcNewWindow->top,
		             prcNewWindow->right - prcNewWindow->left,
		             prcNewWindow->bottom - prcNewWindow->top,
		             SWP_NOZORDER | SWP_NOACTIVATE);
		break;
	}
	case WM_SETFOCUS:
		g_hasFocus = true;
		break;
	case WM_KILLFOCUS: {
		g_hasFocus = false;
		key_clear_all();
		auto &keysDown = ImGui::GetIO().KeysDown;
		memset(&keysDown, 0, sizeof(keysDown));
	} break;
	case WM_MOUSEMOVE:
		App_RequestRender();
		if(!g_trackingMouse) {
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.hwndTrack = hWnd;
			tme.dwFlags = TME_LEAVE;
			tme.dwHoverTime = HOVER_DEFAULT;
			TrackMouseEvent(&tme);
			g_trackingMouse = true;
		}
		break;
	case WM_MOUSELEAVE:
		App_RequestRender();
		g_trackingMouse = false;
		ImGui::GetIO().MousePos = ImVec2(-1, -1);
		for(int i = 0; i < BB_ARRAYSIZE(ImGui::GetIO().MouseDown); ++i) {
			ImGui::GetIO().MouseDown[i] = false;
		}
		break;
	case WM_KEYDOWN:
		if(wParam >= VK_F1 && wParam <= VK_F12) {
			key_on_pressed((key_e)(Key_F1 + wParam - VK_F1));
		}
		break;
	case WM_KEYUP:
		if(wParam >= VK_F1 && wParam <= VK_F12) {
			key_on_released((key_e)(Key_F1 + wParam - VK_F1));
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_CHAR:
		App_RequestRender();
		break;
	default:
		break;
	}

	if(ImGui_ImplDX9_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch(msg) {
	case WM_MOVE:
		App_RequestRender();
		config_getwindowplacement(hWnd);
		break;
	case WM_SIZE:
		App_RequestRender();
		config_getwindowplacement(hWnd);
		if(wParam != SIZE_MINIMIZED) {
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetD3D();
		}
		return 0;
	case WM_SYSCOMMAND:
		if((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		config_getwindowplacement(hWnd);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int CALLBACK WinMain(_In_ HINSTANCE /*Instance*/, _In_opt_ HINSTANCE /*PrevInstance*/, _In_ LPSTR CommandLine, _In_ int /*ShowCode*/)
{
#ifdef LEAK_CHECK
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif // #ifdef LEAK_CHECK

	g_defaultStyle = ImGui::GetStyle();

	if(!App_Init(CommandLine))
		return 0;

	// Initialize Direct3D
	LPDIRECT3D9 pD3D;
	if((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
		App_Shutdown();
		return 0;
	}
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // Present without vsync, maximum unthrottled framerate

	// Create the D3DDevice
	if(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, globals.hwnd, D3DCREATE_MIXED_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) {
		pD3D->Release();
		App_Shutdown();
		return 0;
	}

	// Setup ImGui binding
	ImGui_ImplDX9_Init(globals.hwnd, g_pd3dDevice);

	// Load Fonts
	// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
	//ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
	//io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Consola.ttf", 14.0f);
	//ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 14.0f);
	//ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\FantasqueSansMono-Regular.ttf", 14.0f);

	InitFonts();

	ImVec4 clear_col = ImColor(34, 35, 34);

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	ShowWindow(globals.hwnd, g_config.wp.showCmd ? (int)g_config.wp.showCmd : SW_SHOWDEFAULT);
	UpdateWindow(globals.hwnd);
	Time_StartNewFrame();
	while(msg.message != WM_QUIT && !App_IsShuttingDown()) {
		if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		if(g_needUpdateDpiDependentResources) {
			g_needUpdateDpiDependentResources = false;
			UpdateDpiDependentResources();
		}
		ImGui_ImplDX9_NewFrame();

		App_Update();

		ImGuiIO &io = ImGui::GetIO();
		bool requestRender = App_GetAndClearRequestRender() || key_is_any_down_or_released_this_frame();
		if(g_hasFocus) {
			if(io.InputCharacters[0]) {
				requestRender = true;
			} else {
				for(bool mouseDown : io.MouseDown) {
					if(mouseDown) {
						requestRender = true;
						break;
					}
				}
				if(!requestRender) {
					for(bool keyDown : io.KeysDown) {
						if(keyDown) {
							requestRender = true;
							break;
						}
					}
				}
			}
		}

		// ImGui Rendering
		if(requestRender) {
			g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
			D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_col.x * 255.0f), (int)(clear_col.y * 255.0f), (int)(clear_col.z * 255.0f), (int)(clear_col.w * 255.0f));
			g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
			if(g_pd3dDevice->BeginScene() >= 0) {
				ImGui::Render();
				g_pd3dDevice->EndScene();
			} else {
				ImGui::EndFrame();
				App_RequestRender();
			}
			HRESULT hr = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
			if(FAILED(hr)) {
				bb_sleep_ms(100);
				ResetD3D();
				App_RequestRender();
			}
		} else {
			ImGui::EndFrame();
			bb_sleep_ms(15);
		}
		Time_StartNewFrame();
	}

	ImGui_ImplDX9_Shutdown();
	if(g_pd3dDevice)
		g_pd3dDevice->Release();
	if(pD3D)
		pD3D->Release();

	App_Shutdown();

	return 0;
}
