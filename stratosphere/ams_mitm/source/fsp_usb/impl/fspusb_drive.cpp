#include "fspusb_drive.hpp"
#include "fspusb_usb_manager.hpp"

namespace ams::mitm::fspusb::impl {

    Drive::Drive(UsbHsClientIfSession interface, UsbHsClientEpSession in_ep, UsbHsClientEpSession out_ep) : usb_interface(interface), usb_in_endpoint(in_ep), usb_out_endpoint(out_ep), scsi_context(nullptr), mounted_idx(0xFF), mounted(false) {
        this->scsi_context = new SCSIDriveContext(&this->usb_interface, &this->usb_in_endpoint, &this->usb_out_endpoint);
    }

    Result Drive::Mount() {
        if(!this->mounted) {
            if(!this->scsi_context->Ok()) {
                return fspusb::ResultDriveInitializationFailure().GetValue();
            }

            /* Try to find a mountable index */
            if(FindAndMountAtIndex(&this->mounted_idx)) {
                FormatDriveMountName(this->mount_name, this->mounted_idx);
                auto ffrc = f_mount(&this->fat_fs, this->mount_name, 1);
                
                R_TRY(fspusb::result::CreateFromFRESULT(ffrc));
                this->mounted = true;
            }
        }
        return ResultSuccess();
    }

    void Drive::Unmount() {
        if(this->mounted) {
            UnmountAtIndex(this->mounted_idx);
            f_mount(nullptr, this->mount_name, 1);
            memset(&this->fat_fs, 0, sizeof(this->fat_fs));
            memset(this->mount_name, 0, 0x10);
            this->mounted = false;
        }
    }

    void Drive::Dispose() {
        if(this->scsi_context != nullptr) {
            delete this->scsi_context;
            this->scsi_context = nullptr;
        }
        usbHsIfResetDevice(&this->usb_interface);
        usbHsEpClose(&this->usb_in_endpoint);
        usbHsEpClose(&this->usb_out_endpoint);
        usbHsIfClose(&this->usb_interface);
    }

}