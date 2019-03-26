/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#pragma once
#include <switch.h>

static constexpr u32 Module_Fs = 2;

static constexpr Result ResultFsNotImplemented       = MAKERESULT(Module_Fs, 3001);
static constexpr Result ResultFsOutOfRange           = MAKERESULT(Module_Fs, 3005);

static constexpr Result ResultFsAllocationFailureInSubDirectoryFileSystem = MAKERESULT(Module_Fs, 3355);

static constexpr Result ResultFsInvalidArgument       = MAKERESULT(Module_Fs, 6001);
static constexpr Result ResultFsInvalidPath           = MAKERESULT(Module_Fs, 6002);
static constexpr Result ResultFsTooLongPath           = MAKERESULT(Module_Fs, 6003);
static constexpr Result ResultFsInvalidCharacter      = MAKERESULT(Module_Fs, 6004);
static constexpr Result ResultFsInvalidPathFormat     = MAKERESULT(Module_Fs, 6005);
static constexpr Result ResultFsDirectoryUnobtainable = MAKERESULT(Module_Fs, 6006);
static constexpr Result ResultFsNotNormalized         = MAKERESULT(Module_Fs, 6007);

static constexpr Result ResultFsInvalidOffset        = MAKERESULT(Module_Fs, 6061);
static constexpr Result ResultFsInvalidSize          = MAKERESULT(Module_Fs, 6062);
static constexpr Result ResultFsNullptrArgument      = MAKERESULT(Module_Fs, 6063);

static constexpr Result ResultFsUnsupportedOperation = MAKERESULT(Module_Fs, 6300);
