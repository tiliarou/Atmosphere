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

#define DRIVE_MAX_VALUE 10 // Way more that actual max count, in order to avoid issues

struct DriveData {
    char mountname[0x10];
    UsbHsClientIfSession usbif;
    UsbHsClientEpSession usbinep;
    UsbHsClientEpSession usboutep;
    std::shared_ptr<SCSIDevice> device;
    std::shared_ptr<SCSIBlock> scsi;
    FATFS *fatfs;
};

class USBDriveSystem {
    public:
        static Result Initialize();
        static bool IsInitialized();
        static Result Update();
        static Result WaitForDrives(s64 timeout = -1);
        static void Finalize();
        static u32 GetDriveCount();
};