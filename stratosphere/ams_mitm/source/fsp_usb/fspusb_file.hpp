
#pragma once
#include "impl/fspusb_usb_manager.hpp"

namespace ams::mitm::fspusb {

    class DriveFile : public fs::fsa::IFile {

        private:
            u32 idx;
            s32 usb_iface_id;
            FIL file;

            bool IsDriveOk() {
                return impl::IsDriveOk(this->usb_iface_id);
            }

        public:
            DriveFile(s32 iface_id, FIL fil) : usb_iface_id(iface_id), file(fil) {}

            ~DriveFile() {
                f_close(&this->file);
            }

            virtual Result ReadImpl(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                auto ffrc = f_lseek(&this->file, (u64)offset);
                if(ffrc == FR_OK) {
                    UINT btr = (UINT)size;
                    UINT br = 0;
                    ffrc = f_read(&this->file, buffer, btr, &br);
                    if(ffrc == FR_OK) {
                        *out = (size_t)br;
                    }
                }

                return result::CreateFromFRESULT(ffrc);
            }

            virtual Result GetSizeImpl(s64 *out) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                *out = (s64)f_size(&this->file);

                return ResultSuccess();
            }

            virtual Result FlushImpl() override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());
                return ResultSuccess();
            }

            virtual Result WriteImpl(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                auto ffrc = f_lseek(&this->file, (u64)offset);
                if(ffrc == FR_OK) {
                    UINT btw = (UINT)size;
                    UINT bw = 0;
                    ffrc = f_write(&this->file, buffer, btw, &bw);
                }

                /* We ignore the flush flag since everything is written right away */
                return result::CreateFromFRESULT(ffrc);
            }

            virtual Result SetSizeImpl(s64 size) override final {
                R_UNLESS(this->IsDriveOk(), ResultDriveUnavailable());

                u64 new_size = (u64)size;
                u64 cur_size = f_size(&this->file);

                auto ffrc = f_lseek(&this->file, new_size);

                /* f_lseek takes care of expanding the file if new_size > cur_size */
                /* However, if new_size < cur_size, we must also call f_truncate */
                if(new_size < cur_size) {
                    ffrc = f_truncate(&this->file);
                }

                return result::CreateFromFRESULT(ffrc);
            }

            virtual Result OperateRangeImpl(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override final {
                /* TODO: How should this be handled? */
                return fs::ResultNotImplemented();
            }

            virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                return sf::cmif::InvalidDomainObjectId;
            }
    };

}