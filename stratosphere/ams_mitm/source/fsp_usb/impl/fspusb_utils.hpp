
#pragma once
#include "../fspusb_results.hpp"

#include <cstdio>

#define TMP_LOG(fmt, ...) { \
    FILE *f = fopen("sdmc:/fspusb.log", "a+"); \
    if(f) { \
        fprintf(f, fmt "\n", ##__VA_ARGS__); \
        fclose(f); \
    } \
}

namespace ams::mitm::fspusb::impl {

    inline void FormatDriveMountName(char *str, u32 drive_idx) {
        std::memset(str, 0, strlen(str));
        sprintf(str, "%d:", drive_idx);
    }

}