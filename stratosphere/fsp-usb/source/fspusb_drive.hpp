/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "fatfs/ff.h"
#include "fspusb_scsi_context.hpp"

enum class DriveFileSystemType {
    Invalid,
    FAT12,
    FAT16,
    FAT32,
    exFAT,
};

class Drive {
    public:
        Drive(UsbHsClientIfSession client, UsbHsClientEpSession in, UsbHsClientEpSession out);
        Result Mount(u32 index);
        Result Unmount();
        bool IsMounted();
        DriveFileSystemType GetFSType();
        bool IsOk();
        bool IsOpened();
        void Close();

    private:
        SCSIDevice device;
        SCSIBlock block;
        UsbHsClientIfSession client;
        UsbHsClientEpSession in;
        UsbHsClientEpSession out;
        bool open;
        bool mounted;
        char mountname[10];
        FATFS fs;
};

class USBDriveSystem {
    public:
        static Result Initialize();
        static bool IsInitialized();
        static Result WaitForDrives(s64 timeout = -1);
        static Result UpdateAvailableInterfaces(s64 timeout);
        static void Finalize();
        static Result CountDrives(s32 *out);
        static Drive OpenDrive(s32 idx);
};