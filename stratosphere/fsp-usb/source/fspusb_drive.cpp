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

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>

#include "fspusb_drive.hpp"

static bool init = false;
static UsbHsInterfaceFilter ifilter;
static Event ifaceavailable;
HosMutex drive_lock;
std::vector<DriveData> drives;

Result USBDriveSystem::Initialize() {
    if(init) return 0;
    Result rc = usbHsInitialize();
    if(rc == 0)
    {
        memset(&ifilter, 0, sizeof(ifilter));
        ifilter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass | UsbHsInterfaceFilterFlags_bInterfaceProtocol;
        ifilter.bInterfaceClass = 8;
        ifilter.bInterfaceSubClass = 6;
        ifilter.bInterfaceProtocol = 80;
        rc = usbHsCreateInterfaceAvailableEvent(&ifaceavailable, false, 0, &ifilter);
        if(rc == 0) {
            init = true;
        }
    }
    return rc;
}

bool USBDriveSystem::IsInitialized() {
    return init;
}

Result USBDriveSystem::WaitForDrives(s64 timeout) {
    if(!init) return LibnxError_NotInitialized;
    return eventWait(&ifaceavailable, timeout);
}

Result USBDriveSystem::Update() {
    UsbHsInterface iface_block[DRIVE_MAX_VALUE];
    memset(iface_block, 0, sizeof(iface_block));
    s32 iface_count = 0;
    Result rc = 0;
    if(!drives.empty()) {
        rc = usbHsQueryAcquiredInterfaces(iface_block, sizeof(iface_block), &iface_count);
        for(u32 i = 0; i < drives.size(); i++) {
            bool ok = false;
            for(s32 j = 0; j < iface_count; j++) {
                if(iface_block[j].inf.ID == drives[i].scsi->device->client->ID) {
                    ok = true;
                    break;
                }
            }
            if(!ok) {
                f_mount(NULL, drives[i].mountname, 1);
                usbHsEpClose(&drives[i].usbinep);
                usbHsEpClose(&drives[i].usboutep);
                usbHsIfResetDevice(&drives[i].usbif);
                usbHsIfClose(&drives[i].usbif);
                drives.erase(drives.begin() + i);
            }
        }
    }
    memset(iface_block, 0, sizeof(iface_block));
    rc = usbHsQueryAvailableInterfaces(&ifilter, iface_block, sizeof(iface_block), &iface_count);
    if(rc == 0) {
        for(s32 i = 0; i < iface_count; i++) {
            DriveData dt;
            rc = usbHsAcquireUsbIf(&dt.usbif, &iface_block[i]);
            if(rc == 0) {
                for(u32 j = 0; j < 15; j++) {
                    auto epd = &dt.usbif.inf.inf.input_endpoint_descs[j];
                    if(epd->bLength > 0) {
                        rc = usbHsIfOpenUsbEp(&dt.usbif, &dt.usbinep, 1, epd->wMaxPacketSize, epd);
                        break;
                    }
                }
                if(rc == 0) {
                    for(u32 j = 0; j < 15; j++) {
                        auto epd = &dt.usbif.inf.inf.output_endpoint_descs[j];
                        if(epd->bLength > 0) {
                            rc = usbHsIfOpenUsbEp(&dt.usbif, &dt.usboutep, 1, epd->wMaxPacketSize, epd);
                            break;
                        }
                    }
                    if(rc == 0) {
                        dt.device = std::make_shared<SCSIDevice>(&dt.usbif, &dt.usbinep, &dt.usboutep);
                        dt.scsi = std::make_shared<SCSIBlock>(dt.device);
                        memset(dt.mountname, 0, 0x10);
                        u32 idx = drives.size();
                        sprintf(dt.mountname, "usb-%d", idx);
                        dt.fatfs = (FATFS*)malloc(sizeof(FATFS));
                        memset(dt.fatfs, 0, sizeof(FATFS));
                        drives.push_back(dt);
                        auto fres = f_mount(dt.fatfs, dt.mountname, 1);
                        if(fres != FR_OK) {
                            drives.pop_back();
                        }
                    }
                }
            }
        }
    }
    return rc;
}

void USBDriveSystem::Finalize() {
    if(!init) return;
    usbHsDestroyInterfaceAvailableEvent(&ifaceavailable, 0);
    usbHsExit();
    init = false;
}

u32 USBDriveSystem::GetDriveCount() {
    return drives.size();
}