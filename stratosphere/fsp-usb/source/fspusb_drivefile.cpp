#include "fspusb_drivefile.hpp"

extern HosMutex drive_lock;
extern std::vector<DriveData> drives;

bool DriveFile::IsOk() {
    DRIVES_SCOPE_GUARD;
    for(u32 i = 0; i < drives.size(); i++) {
        if(drives[i].usbif.ID == usbid) {
            return true;
        }
    }
    return false;
}

Result DriveFile::ReadImpl(u64 *out, u64 offset, void *buffer, u64 size) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_lseek(&fp, offset);
    if(res == FR_OK) {
        res = f_read(&fp, buffer, size, (UINT*)out);
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFile::GetSizeImpl(u64 *out) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    *out = f_size(&fp);
    return 0;
}

Result DriveFile::FlushImpl() {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    return 0;
}

Result DriveFile::WriteImpl(u64 offset, void *buffer, u64 size, bool flush) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_lseek(&fp, offset);
    if(res == FR_OK) {
        u64 out;
        res = f_write(&fp, buffer, size, (UINT*)&out);
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFile::SetSizeImpl(u64 size) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_lseek(&fp, size);
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFile::OperateRangeImpl(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    return 0;
}