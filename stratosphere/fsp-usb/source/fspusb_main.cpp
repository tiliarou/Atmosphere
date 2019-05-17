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
#include <atmosphere.h>
#include <stratosphere.hpp>

#include "fspusb_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x100000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    u64 __stratosphere_title_id = 0x0100000000000376;
}

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    StratosphereCrashHandler(ctx);
}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    Result rc;

    SetFirmwareVersionForLibnx();

    DoWithSmSession([&]() {
        rc = fsInitialize();
        if (R_FAILED(rc)) {
            std::abort();
        }

        rc = fsdevMountSdmc();
        if (R_FAILED(rc)) {
            std::abort();
        }

        do
        {
            rc = USBDriveSystem::Initialize();
        } while(rc != 0);
    });

    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    fsdevUnmountAll();
    fsExit();
}

struct FspUsbManagerOptions { // fsp-srv's ones, but without domains (need this specific buf size as we're using fs-compatible fss)
    static const size_t PointerBufferSize = 0x800;
    static const size_t MaxDomains = 0;
    static const size_t MaxDomainObjects = 0;
};

using FspUsbManager = WaitableManager<FspUsbManagerOptions>;

int main(int argc, char **argv)
{
    /* Static server manager. */
    static auto s_server_manager = FspUsbManager(2);

    /* Create service. */
    s_server_manager.AddWaitable(new ServiceServer<FspUsbService>("fsp-usb", 0x40));

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    /* Cleanup */
    return 0;
}