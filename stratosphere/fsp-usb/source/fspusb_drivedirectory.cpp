#include "fspusb_drivedirectory.hpp"

extern HosMutex drive_lock;
extern std::vector<DriveData> drives;

bool DriveDirectory::IsOk() {
    DRIVES_SCOPE_GUARD;
    for(u32 i = 0; i < drives.size(); i++) {
        if(drives[i].usbif.ID == usbid) {
            return true;
        }
    }
    return false;
}

Result DriveDirectory::ReadImpl(uint64_t *out_count, FsDirectoryEntry *out_entries, uint64_t max_entries) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    FsDirectoryEntry ent;
    memset(&ent, 0, sizeof(FsDirectoryEntry));
    FRESULT res = FR_OK;
    FILINFO info;
    u32 count = 0;
    while(true) {
        if(count >= max_entries) {
            break;
        }
        res = f_readdir(&dp, &info);
        if((res != FR_OK) || (info.fname[0] == 0)) {
            break;
        }
        strcpy(ent.name, info.fname);
        if(info.fattrib & AM_DIR) {
            ent.type = ENTRYTYPE_DIR;
        }
        else {
            ent.type = ENTRYTYPE_FILE;
            ent.fileSize = info.fsize;
        }
        memcpy(&out_entries[count], &ent, sizeof(ent));
        count++;
    }
    *out_count = count;
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveDirectory::GetEntryCountImpl(uint64_t *count) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    u64 entcount = 0;
    FRESULT res = FR_OK;
    FILINFO info;
    while(true) {
        res = f_readdir(&dp, &info);
        if((res != FR_OK) || (info.fname[0] == 0)) {
            break;
        }
        entcount++;
    }
    *count = entcount;
    return FspUsbResults::MakeFATFSErrorResult(res);
}