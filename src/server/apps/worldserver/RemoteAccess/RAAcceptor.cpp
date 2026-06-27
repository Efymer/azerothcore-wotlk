/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// brick E1: the Remote Access (RA) acceptor lives on the LEGACY src/server/shared/Network stack
// (global AsyncAcceptor / IoContextTcpSocket). The modern Acore::Net world socket stack lives in
// src/common/network and uses the same bare header names. Those two stacks cannot coexist in one
// translation unit via include ordering, so the RA acceptor is isolated here while worldserver's
// Main.cpp stays purely on the modern Acore::Net stack. The factory returns an opaque keep-alive
// handle so Main.cpp never needs the legacy AsyncAcceptor type.

#include "RAAcceptor.h"
#include "Config.h"
#include "Log.h"
#include "Network/AsyncAcceptor.h"
#include "RASession.h"

std::shared_ptr<void> StartRaSocketAcceptor(Acore::Asio::IoContext& ioContext)
{
    uint16 raPort = uint16(sConfigMgr->GetOption<int32>("Ra.Port", 3443));
    std::string raListener = sConfigMgr->GetOption<std::string>("Ra.IP", "0.0.0.0");

    AsyncAcceptor* acceptor = new AsyncAcceptor(ioContext, raListener, raPort);
    if (!acceptor->Bind())
    {
        LOG_ERROR("server.worldserver", "Failed to bind RA socket acceptor");
        delete acceptor;
        return nullptr;
    }

    acceptor->AsyncAccept<RASession>();

    // The acceptor destructor closes the listener; return it as an opaque keep-alive handle.
    return std::shared_ptr<void>(acceptor, [](void* ptr)
    {
        delete static_cast<AsyncAcceptor*>(ptr);
    });
}
