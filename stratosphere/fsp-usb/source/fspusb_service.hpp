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

#include "fspusb_drive.hpp"
#include "fspusb_drivefilesystem.hpp"

enum FspUsbCmd {
    FspUsbCmd_UpdateDrives = 0,
    FspUsbCmd_OpenDriveFileSystem = 1,
};

class FspUsbService final : public IServiceObject {
    protected:
        Result UpdateDrives(Out<u32> count);
        Result OpenDriveFileSystem(u32 index, Out<std::shared_ptr<IFileSystemInterface>> out);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<FspUsbCmd_UpdateDrives, &FspUsbService::UpdateDrives>(),
            MakeServiceCommandMeta<FspUsbCmd_OpenDriveFileSystem, &FspUsbService::OpenDriveFileSystem>(),
        };
};
