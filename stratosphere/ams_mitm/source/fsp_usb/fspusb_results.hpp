
#pragma once
#include <stratosphere.hpp>
#include "fatfs/ff.h"

namespace ams::mitm::fspusb {

    R_DEFINE_NAMESPACE_RESULT_MODULE(2);

    /* Use custom results from 8000 to 8999, since FS uses almost to 7000 :P */
    /* FATFS errors are converted below, those who aren't handled are returned as 8100 + the error */

    R_DEFINE_ERROR_RESULT(InvalidDriveInterfaceId,     8001);
    R_DEFINE_ERROR_RESULT(DriveUnavailable,            8002);
    R_DEFINE_ERROR_RESULT(DriveInitializationFailure,  8003);

    namespace result {

        NX_CONSTEXPR Result CreateFromFRESULT(FRESULT err) {
            switch(err) {
                case FR_OK:
                    return ResultSuccess();

                case FR_NO_FILE:
                case FR_NO_PATH:
                case FR_INVALID_NAME:
                    return fs::ResultPathNotFound();

                case FR_EXIST:
                    return fs::ResultPathAlreadyExists();

                case FR_WRITE_PROTECTED:
                    return fs::ResultUnsupportedOperation();

                case FR_INVALID_DRIVE:
                    return fs::ResultInvalidMountName();

                case FR_INVALID_PARAMETER:
                    return fs::ResultInvalidArgument();

                /* TODO: more FATFS errors */

                default:
                    return MAKERESULT(fspusb::impl::result::ResultModuleId, 8100 + err);
            }
        }

    }

}