#include "fspusb_usb_manager.hpp"
#include <vector>
#include <array>
#include <memory>

namespace ams::mitm::fspusb::impl {

    namespace {

        os::Mutex g_usb_manager_lock;
        os::Thread g_usb_update_thread;
        std::vector<DrivePointer> g_usb_manager_drives;
        Event g_usb_manager_interface_available_event;
        Event g_usb_manager_thread_exit_event;
        UsbHsInterfaceFilter g_usb_manager_device_filter = {};
        bool g_usb_manager_initialized = false;

        std::array<bool, DriveMax> g_usb_manager_mounted_index_array; 

        void UpdateDrives() {
            std::scoped_lock lk(g_usb_manager_lock);

            UsbHsInterface iface_block[DriveMax];
            size_t iface_block_size = DriveMax * sizeof(UsbHsInterface);
            memset(iface_block, 0, iface_block_size);
            s32 iface_count = 0;

            if (!g_usb_manager_drives.empty()) {
                auto rc = usbHsQueryAcquiredInterfaces(iface_block, iface_block_size, &iface_count);
                std::vector<DrivePointer> valid_drives;
                if (R_SUCCEEDED(rc)) {
                    for (auto &drive: g_usb_manager_drives) {
                        /* For each drive in our list, check whether it is still available (by looping through actual acquired interfaces) */
                        bool ok = false;
                        for (s32 i = 0; i < iface_count; i++) {
                            if (iface_block[i].inf.ID == drive->GetInterfaceId()) {
                                ok = true;
                                break;
                            }
                        }

                        if (ok) {
                            valid_drives.push_back(std::move(drive));
                        }
                        else {
                            drive->Unmount();
                            drive->Dispose();
                        }
                    }
                }
                g_usb_manager_drives.clear();
                for (auto &drive: valid_drives) {
                    g_usb_manager_drives.push_back(std::move(drive));
                }
                valid_drives.clear();
            }
            memset(iface_block, 0, iface_block_size);
            auto rc = usbHsQueryAvailableInterfaces(&g_usb_manager_device_filter, iface_block, iface_block_size, &iface_count);

            /* Check new ones and (try to) acquire them */
            if (R_SUCCEEDED(rc)) {
                for (s32 i = 0; i < iface_count; i++) {
                    UsbHsClientIfSession iface;
                    UsbHsClientEpSession inep;
                    UsbHsClientEpSession outep;
                    rc = usbHsAcquireUsbIf(&iface, &iface_block[i]);
                    if (R_SUCCEEDED(rc)) {
                        for (u32 j = 0; j < 15; j++) {
                            auto epd = &iface.inf.inf.input_endpoint_descs[j];
                            if (epd->bLength > 0) {
                                rc = usbHsIfOpenUsbEp(&iface, &outep, 1, epd->wMaxPacketSize, epd);
                                break;
                            }
                        }
                        if (R_SUCCEEDED(rc)) {
                            for (u32 j = 0; j < 15; j++) {
                                auto epd = &iface.inf.inf.output_endpoint_descs[j];
                                if (epd->bLength > 0) {
                                    rc = usbHsIfOpenUsbEp(&iface, &inep, 1, epd->wMaxPacketSize, epd);
                                    break;
                                }
                            }
                            if (R_SUCCEEDED(rc)) {

                                /* Since FATFS reads from drives in the vector and we need to mount it, push it to the vector first */
                                /* Then, if it didn't mount correctly, pop from the vector and close the interface */
                                auto drv = std::make_unique<Drive>(iface, inep, outep);
                                g_usb_manager_drives.push_back(std::move(drv));

                                auto &drive_ref = g_usb_manager_drives.back();
                                auto rc = drive_ref->Mount();
                                if (R_FAILED(rc)) {
                                    drive_ref->Dispose();
                                    g_usb_manager_drives.pop_back();
                                }
                            }
                        }
                    }
                }
            }
        }

        void ManagerUpdateThread(void *arg) {
            Result rc;
            s32 idx;
            
            while(true) {
                // Wait until one of our events is triggered
                idx = 0;
                rc = waitMulti(&idx, -1, waiterForEvent(usbHsGetInterfaceStateChangeEvent()), waiterForEvent(&g_usb_manager_interface_available_event), waiterForEvent(&g_usb_manager_thread_exit_event));
                if (R_SUCCEEDED(rc)) {
                    /* Clear InterfaceStateChangeEvent if it was triggered (not an autoclear event) */
                    if (idx == 0) {
                        eventClear(usbHsGetInterfaceStateChangeEvent());
                    }
                    
                    /* Break out of the loop if the thread exit user event was fired */
                    if (idx == 2) {
                        break;
                    }
                    
                    /* Update drives */
                    UpdateDrives();
                }
            }
        }

        u32 FindNextMountableIndex() {
            for (u32 i = 0; i < DriveMax; i++) {
                if (!g_usb_manager_mounted_index_array[i]) {
                    return i;
                }
            }
            /* All mounted */
            return InvalidMountedIndex;
        }

        bool MountAtIndex(u32 idx) {
            if (idx >= DriveMax) {
                /* Invalid index */
                return false;
            }
            if (g_usb_manager_mounted_index_array[idx]) {
                /* Already mounted here */
                return false;
            }
            g_usb_manager_mounted_index_array[idx] = true;
            return true;
        }
    }

    Result InitializeManager() {
        std::scoped_lock lk(g_usb_manager_lock);

        R_UNLESS(!g_usb_manager_initialized, ResultSuccess());

        /* No drives mounted, set all the mounted indexes' status to false */
        for (u32 i = 0; i < DriveMax; i++) {
            g_usb_manager_mounted_index_array[i] = false;
        }

        memset(&g_usb_manager_device_filter, 0, sizeof(UsbHsInterfaceFilter));
        memset(&g_usb_manager_interface_available_event, 0, sizeof(Event));
        memset(&g_usb_manager_thread_exit_event, 0, sizeof(Event));

        R_TRY(usbHsInitialize());
        
        g_usb_manager_device_filter = {};
        g_usb_manager_device_filter.Flags = UsbHsInterfaceFilterFlags_bInterfaceClass | UsbHsInterfaceFilterFlags_bInterfaceSubClass | UsbHsInterfaceFilterFlags_bInterfaceProtocol;
        g_usb_manager_device_filter.bInterfaceClass = USB_CLASS_MASS_STORAGE;
        g_usb_manager_device_filter.bInterfaceSubClass = MASS_STORAGE_SCSI_COMMANDS;
        g_usb_manager_device_filter.bInterfaceProtocol = MASS_STORAGE_BULK_ONLY;
            
        R_TRY(usbHsCreateInterfaceAvailableEvent(&g_usb_manager_interface_available_event, true, 0, &g_usb_manager_device_filter));
        R_TRY(eventCreate(&g_usb_manager_thread_exit_event, true));
        R_TRY(g_usb_update_thread.Initialize(&ManagerUpdateThread, nullptr, 0x4000, 0x15));
        R_TRY(g_usb_update_thread.Start());
                    
        g_usb_manager_initialized = true;

        return ResultSuccess();
    }

    void FinalizeManager() {
        if (!g_usb_manager_initialized) {
            return;
        }
        
        for (auto &drive: g_usb_manager_drives) {
            drive->Unmount();
            drive->Dispose();
        }
        
        g_usb_manager_drives.clear();
        
        /* Fire thread exit user event, wait until the thread exits and close the event */
        eventFire(&g_usb_manager_thread_exit_event);
        g_usb_update_thread.Join();
        eventClose(&g_usb_manager_thread_exit_event);
        
        usbHsDestroyInterfaceAvailableEvent(&g_usb_manager_interface_available_event, 0);
        usbHsExit();
        
        g_usb_manager_initialized = false;
    }

    void DoUpdateDrives() {
        UpdateDrives();
    }

    size_t GetAcquiredDriveCount() {
        std::scoped_lock lk(g_usb_manager_lock);
        return g_usb_manager_drives.size();
    }

    bool FindAndMountAtIndex(u32 *out_mounted_idx) {
        u32 idx = FindNextMountableIndex();
        /* If FindNextMountableIndex returns InvalidMountedIndex (unable to find index), it will fail here. */
        bool ok = MountAtIndex(idx);
        if (ok) {
            *out_mounted_idx = idx;
        }
        return ok;
    }

    void UnmountAtIndex(u32 mounted_idx) {
        if (mounted_idx < DriveMax) {
            g_usb_manager_mounted_index_array[mounted_idx] = false;
        }
    }

    bool IsDriveInterfaceIdValid(s32 drive_interface_id) {
        std::scoped_lock lk(g_usb_manager_lock);
        for (auto &drive: g_usb_manager_drives) {
            if (drive_interface_id == drive->GetInterfaceId()) {
                return true;
            }
        }
        return false;
    }

    u32 GetDriveMountedIndex(s32 drive_interface_id) {
        std::scoped_lock lk(g_usb_manager_lock);
        for (u32 i = 0; i < g_usb_manager_drives.size(); i++) {
            auto &drive = g_usb_manager_drives.at(i);
            if (drive_interface_id == drive->GetInterfaceId()) {
                return drive->GetMountedIndex();
            }
        }
        return InvalidMountedIndex;
    }

    s32 GetDriveInterfaceId(u32 drive_idx) {
        std::scoped_lock lk(g_usb_manager_lock);
        if (drive_idx < g_usb_manager_drives.size()) {
            auto &drive = g_usb_manager_drives.at(drive_idx);
            return drive->GetInterfaceId();
        }
        return 0;
    }

    void DoWithDrive(s32 drive_interface_id, std::function<void(DrivePointer&)> fn) {
        std::scoped_lock lk(g_usb_manager_lock);
        for (auto &drive: g_usb_manager_drives) {
            if (drive_interface_id == drive->GetInterfaceId()) {
                fn(drive);
            }
        }
    }

    void DoWithDriveMountedIndex(u32 drive_mounted_idx, std::function<void(DrivePointer&)> fn) {
        std::scoped_lock lk(g_usb_manager_lock);
        for (auto &drive: g_usb_manager_drives) {
            if (drive_mounted_idx == drive->GetMountedIndex()) {
                fn(drive);
            }
        }
    }

    void DoWithDriveFATFS(s32 drive_interface_id, std::function<void(FATFS*)> fn) {
        DoWithDrive(drive_interface_id, [&](DrivePointer &drive_ptr) {
            drive_ptr->DoWithFATFS(fn);
        });
    }
}