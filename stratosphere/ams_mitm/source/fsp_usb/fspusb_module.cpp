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
#include "fspusb_module.hpp"
#include "fspusb_service.hpp"

namespace ams::mitm::fspusb {

    namespace {

        constexpr sm::ServiceName ServiceName = sm::ServiceName::Encode("fsp-usb");

            struct ServerOptions {
                static constexpr size_t PointerBufferSize = 0x800;
                static constexpr size_t MaxDomains = 0x40;
                static constexpr size_t MaxDomainObjects = 0x4000;
            };
            /* Same options as fsp-srv, this will be a service with similar behaviour */

            constexpr size_t MaxServers = 1;
            constexpr size_t MaxSessions = 61;
            sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

    }

    void MitmModule::ThreadFunction(void *arg) {
        R_ASSERT(impl::InitializeManager());

        os::Thread usb_thread;
        R_ASSERT(usb_thread.Initialize(&impl::ManagerUpdateThread, nullptr, 0x4000, 0x15));
        R_ASSERT(usb_thread.Start());

        R_ASSERT(g_server_manager.RegisterServer<Service>(ServiceName, MaxSessions));

        g_server_manager.LoopProcess();

        impl::FinalizeManager();
    }

}
