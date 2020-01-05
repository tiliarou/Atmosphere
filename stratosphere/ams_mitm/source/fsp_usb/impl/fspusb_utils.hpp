
#pragma once
#include "../fspusb_results.hpp"

namespace ams::mitm::fspusb::impl {

    inline void FormatDriveMountName(char *str, u32 drive_idx) {
        std::memset(str, 0, strlen(str));
        sprintf(str, "%d:", drive_idx);
    }

}