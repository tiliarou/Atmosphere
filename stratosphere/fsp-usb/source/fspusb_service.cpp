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
 
#include <switch.h>
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>

#include "fspusb_service.hpp"

extern HosMutex usb_lock;

Result FspUsbService::UpdateDrives(Out<u32> count) {
    Result rc = USBDriveSystem::Update();
    if(rc == 0) {
        u32 drvcount = USBDriveSystem::GetDriveCount();
        count.SetValue(drvcount);
    }
    return rc;
}

Result FspUsbService::OpenDriveFileSystem(u32 index, Out<std::shared_ptr<IFileSystemInterface>> out) {
    if(index >= USBDriveSystem::GetDriveCount()) {
        return MAKERESULT(455, 10);
    }
    out.SetValue(std::make_shared<IFileSystemInterface>(std::make_shared<DriveFileSystem>(index)));
    return 0;
}