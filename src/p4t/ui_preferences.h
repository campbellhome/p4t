// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#pragma once

#include "common.h"
#include "config.h"

void Preferences_Open(config_t *config);
void Preferences_Update(config_t *config);
void Preferences_Reset();
bool Preferences_IsOpen();
