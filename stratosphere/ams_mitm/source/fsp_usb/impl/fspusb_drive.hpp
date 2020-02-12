
#pragma once
#include <thread>
#include <memory>
#include "../fatfs/ff.h"
#include "../fatfs/diskio.h"
#include "fspusb_utils.hpp"
#include "fspusb_scsi.hpp"

namespace ams::mitm::fspusb::impl {

    /* Maximum amount of drives, basically FATFS's volume number */
    constexpr u32 DriveMax = FF_VOLUMES;

    /* Used for drive mounting as an invalid index (since 0 is a valid one) */
    constexpr u32 InvalidMountedIndex = 0xFF;

    class Drive {
            NON_COPYABLE(Drive);
            NON_MOVEABLE(Drive);
        private:
            ams::os::Mutex fs_lock;
            UsbHsClientIfSession usb_interface;
            UsbHsClientEpSession usb_in_endpoint;
            UsbHsClientEpSession usb_out_endpoint;
            FATFS fat_fs;
            u32 mounted_idx;
            char mount_name[0x10];
            SCSIDriveContext *scsi_context;
            bool mounted;

        public:
            Drive(UsbHsClientIfSession interface, UsbHsClientEpSession in_ep, UsbHsClientEpSession out_ep, u8 lun);
            Result Mount();
            void Unmount();
            void Dispose(bool close_usbhs);

            s32 GetInterfaceId() {
                return this->usb_interface.ID;
            }

            SCSIDriveContext *GetSCSIContext() {
                return this->scsi_context;
            }

            u32 GetMountedIndex() {
                return this->mounted_idx;
            }

            u32 GetBlockSize()
            {
                if (this->scsi_context != nullptr) {
                    return this->scsi_context->GetBlock()->GetBlockSize();
                }
                return 0;
            }

            bool IsSCSIOk() {
                if (this->scsi_context != nullptr) {
                    return this->scsi_context->Ok();
                }
                return false;
            }

            DRESULT DoReadSectors(u8 *buffer, u64 sector_offset, u32 num_sectors) {
                if (this->scsi_context != nullptr) {
                    int res = this->scsi_context->GetBlock()->ReadSectors(buffer, sector_offset, num_sectors);
                    if (res != 0) {
                        return RES_OK;
                    }
                }
                return RES_PARERR;
            }

            DRESULT DoWriteSectors(const u8 *buffer, u64 sector_offset, u32 num_sectors) {
                if (this->scsi_context != nullptr) {
                    int res = this->scsi_context->GetBlock()->WriteSectors(buffer, sector_offset, num_sectors);
                    if (res != 0) {
                        return RES_OK;
                    }
                }
                return RES_PARERR;
            }

            void DoWithFATFS(std::function<void(FATFS*)> fn) {
                std::scoped_lock lk(this->fs_lock);
                fn(&this->fat_fs);
            }

            const char *GetMountName() {
                return this->mount_name;
            }
    };

    /* For convenience :P */
    using DrivePointer = std::unique_ptr<Drive>;
}