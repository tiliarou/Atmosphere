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
static UsbHsInterface ifaces[8];
static s32 icount = 0;
HosMutex usb_lock;

extern SCSIBlockPartition *drive_blocks[8];

Drive::Drive(UsbHsClientIfSession client, UsbHsClientEpSession in, UsbHsClientEpSession out) : client(client), in(in), out(out), open(false), mounted(false) {
    if((strlen(client.inf.pathstr) > 0) && (usbHsIfGetID(&client) >= 0)) {
        device = SCSIDevice(&client, &in, &out);
        open = true;
    }
}

Result Drive::Mount(u32 index) {
    if(!open) return MAKERESULT(199, 1);
    if(mounted) return 0;
    block = SCSIBlock(&device);
    drive_blocks[index] = &block.partitions[0];
    memset(&fs, 0, sizeof(FATFS));
    char name[16] = {0};
    sprintf(name, "usb-%d", index);
    FRESULT rc = f_mount(&fs, name, 1);
    if(rc != FR_OK) return MAKERESULT(199, rc + 100);
    memset(mountname, 0, 10);
    strcpy(mountname, name);
    mounted = true;
    return 0;
}

Result Drive::Unmount() {
    if(!open) return MAKERESULT(199, 1);
    if(!mounted) return 0;
    f_mount(NULL, mountname, 1);
    mounted = false;
    return 0;
}

bool Drive::IsMounted() {
    return mounted;
}

DriveFileSystemType Drive::GetFSType() {
    u8 rawtype = fs.fs_type;
    DriveFileSystemType fst = DriveFileSystemType::Invalid;
    if(rawtype == FS_FAT12) fst = DriveFileSystemType::FAT12;
    else if(rawtype == FS_FAT16) fst = DriveFileSystemType::FAT16;
    else if(rawtype == FS_FAT32) fst = DriveFileSystemType::FAT32;
    else if(rawtype == FS_EXFAT) fst = DriveFileSystemType::exFAT;
    return fst;
}

bool Drive::IsOk() {
    if(!open) return false;
    UsbHsInterface aqifaces[8];
    memset(aqifaces, 0, sizeof(aqifaces));
    s32 aqicount = 0;
    Result rc = usbHsQueryAcquiredInterfaces(aqifaces, sizeof(aqifaces), &aqicount);
    if(rc == 0)
    {
        for(s32 i = 0; i < aqicount; i++)
        {
            UsbHsInterface *iface = &aqifaces[i];
            if(iface->inf.ID == client.inf.inf.ID) {
                return true;
            }
        }
    }
    return false;
}

bool Drive::IsOpened() {
    return open;
}

void Drive::Close() {
    if(!open) return;
    Unmount();
    usbHsEpClose(&in);
    usbHsEpClose(&out);
    usbHsIfResetDevice(&client);
    usbHsIfClose(&client);
    open = false;
}

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
            for(u32 i = 0; i < 8; i++) {
                drive_blocks[i] = nullptr;
            }
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
    return UpdateAvailableInterfaces(timeout);
}

Result USBDriveSystem::UpdateAvailableInterfaces(s64 timeout) {
    Result rc = eventWait(&ifaceavailable, timeout);
    if(rc != 0) return rc;
    memset(ifaces, 0, sizeof(ifaces));
    rc = usbHsQueryAvailableInterfaces(&ifilter, ifaces, sizeof(ifaces), &icount);
    return rc;
}

void USBDriveSystem::Finalize() {
    if(!init) return;
    usbHsDestroyInterfaceAvailableEvent(&ifaceavailable, 0);
    usbHsExit();
    init = false;
}

Result USBDriveSystem::CountDrives(s32 *out) {
    *out = icount;
    return 0;
}

Drive USBDriveSystem::OpenDrive(s32 idx) {
    UsbHsClientIfSession client;
    UsbHsClientEpSession inep;
    UsbHsClientEpSession outep;
    memset(&client, 0, sizeof(UsbHsClientIfSession));
    memset(&inep, 0, sizeof(UsbHsClientEpSession));
    memset(&outep, 0, sizeof(UsbHsClientEpSession));
    if((idx + 1) > icount) return Drive(client, inep, outep);
    Result rc = usbHsAcquireUsbIf(&client, &ifaces[idx]);
    if(rc == 0)
    {
        for(u32 i = 0; i < 15; i++)
        {
            auto epd = &client.inf.inf.input_endpoint_descs[i];
            if(epd->bLength > 0)
            {
                rc = usbHsIfOpenUsbEp(&client, &inep, 1, epd->wMaxPacketSize, epd);
                break;
            }
        }
        for(u32 i = 0; i < 15; i++)
        {
            auto epd = &client.inf.inf.output_endpoint_descs[i];
            if(epd->bLength > 0)
            {
                rc = usbHsIfOpenUsbEp(&client, &outep, 1, epd->wMaxPacketSize, epd);
                break;
            }
        }
    }
    return Drive(client, inep, outep);
}

void USBMainThread(void *arg) {
    Result rc = 0;

    do
    {
        std::scoped_lock<HosMutex> lck(usb_lock);
        rc = USBDriveSystem::Initialize();
        svcSleepThread(100000000L);
    } while(rc != 0); 

    while(true) {
        svcSleepThread(100000000L);
    }

    USBDriveSystem::Finalize();
}