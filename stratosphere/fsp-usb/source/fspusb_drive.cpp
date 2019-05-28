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

static bool g_usbdrive_init = false;
static UsbHsInterfaceFilter g_usbdrive_device_filter;
static Event g_usbdrive_interface_available_event;
HosMutex g_usbdrive_drives_lock;
std::vector<DriveData> g_usbdrive_drives;

Result USBDriveSystem::Initialize() {
    if(g_usbdrive_init) return 0;
    Result rc = usbHsInitialize();
    if(rc == 0)
    {
        memset(&g_usbdrive_device_filter, 0, sizeof(g_usbdrive_device_filter));
        g_usbdrive_device_filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass | UsbHsInterfaceFilterFlags_bInterfaceProtocol;
        g_usbdrive_device_filter.bInterfaceClass = 8;
        g_usbdrive_device_filter.bInterfaceSubClass = 6;
        g_usbdrive_device_filter.bInterfaceProtocol = 80;
        rc = usbHsCreateInterfaceAvailableEvent(&g_usbdrive_interface_available_event, false, 0, &g_usbdrive_device_filter);
        if(rc == 0) {
            g_usbdrive_init = true;
        }
    }
    return rc;
}

bool USBDriveSystem::IsInitialized() {
    return g_usbdrive_init;
}

Result USBDriveSystem::WaitForDrives(s64 timeout) {
    if(!g_usbdrive_init) return LibnxError_NotInitialized;
    return eventWait(&g_usbdrive_interface_available_event, timeout);
}

void USBDriveSystem::Update() {
    DRIVES_SCOPE_GUARD;
    UsbHsInterface iface_block[DriveMax];
    memset(iface_block, 0, sizeof(iface_block));
    s32 iface_count = 0;
    Result rc = 0;
    if(!g_usbdrive_drives.empty()) {
        rc = usbHsQueryAcquiredInterfaces(iface_block, sizeof(iface_block), &iface_count);
        for(u32 i = 0; i < g_usbdrive_drives.size(); i++) {
            bool ok = false;
            for(s32 j = 0; j < iface_count; j++) {
                if(iface_block[j].inf.ID == g_usbdrive_drives[i].usbif.ID) {
                    ok = true;
                    break;
                }
            }
            if(!ok) {
                printf("Deleting drive: %d, ID: %d\n", i, g_usbdrive_drives[i].usbif.ID);
                f_mount(NULL, g_usbdrive_drives[i].mountname, 1);
                usbHsEpClose(&g_usbdrive_drives[i].usbinep);
                usbHsEpClose(&g_usbdrive_drives[i].usboutep);
                usbHsIfResetDevice(&g_usbdrive_drives[i].usbif);
                usbHsIfClose(&g_usbdrive_drives[i].usbif);
                g_usbdrive_drives.erase(g_usbdrive_drives.begin() + i);
            }
        }
    }
    memset(iface_block, 0, sizeof(iface_block));
    rc = usbHsQueryAvailableInterfaces(&g_usbdrive_device_filter, iface_block, sizeof(iface_block), &iface_count);
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
                        printf("Adding new drive: %d, ID: %d\n", g_usbdrive_drives.size(), dt.usbif.ID);
                        dt.device = std::make_shared<SCSIDevice>(&dt.usbif, &dt.usbinep, &dt.usboutep);
                        dt.scsi = std::make_shared<SCSIBlock>(dt.device);
                        memset(dt.mountname, 0, 0x10);
                        u32 idx = g_usbdrive_drives.size();
                        sprintf(dt.mountname, "usb-%d:", idx);
                        memset(&dt.fatfs, 0, sizeof(FATFS));
                        g_usbdrive_drives.push_back(dt);
                        auto fres = f_mount(&dt.fatfs, dt.mountname, 1);
                        if(fres != FR_OK) {
                            printf("Failed to add new drive: %d (pdrv: %d) (%d)\n", g_usbdrive_drives.size(), dt.fatfs.pdrv, fres);
                            g_usbdrive_drives.pop_back();
                        }
                        else {
                            printf("Correctly added new drive: %d (pdrv: %d)\n", g_usbdrive_drives.size(), dt.fatfs.pdrv);
                        }
                    }
                }
            }
        }
    }
}

void USBDriveSystem::Finalize() {
    if(!g_usbdrive_init) return;
    usbHsDestroyInterfaceAvailableEvent(&g_usbdrive_interface_available_event, 0);
    usbHsExit();
    g_usbdrive_init = false;
}

u32 USBDriveSystem::GetDriveCount() {
    DRIVES_SCOPE_GUARD;
    return g_usbdrive_drives.size();
}

u8 USBDriveSystem::GetFSType(u32 index) {
    DRIVES_SCOPE_GUARD;
    if(index >= g_usbdrive_drives.size()) {
        return 0;
    }
    return g_usbdrive_drives[index].fatfs.fs_type;
}