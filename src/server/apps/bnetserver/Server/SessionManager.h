/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AZEROTHCORE_BNET_SESSION_MANAGER_H
#define AZEROTHCORE_BNET_SESSION_MANAGER_H

#include "SocketMgr.h"
#include "Session.h"

namespace Battlenet
{
    class SessionManager : public Acore::Net::SocketMgr<Session>
    {
        typedef SocketMgr<Session> BaseSocketMgr;

    public:
        static SessionManager& Instance();

        bool StartNetwork(Acore::Asio::IoContext& ioContext, std::string const& bindIp, uint16 port, int threadCount = 1) override;

    protected:
        Acore::Net::NetworkThread<Session>* CreateThreads() const override;
    };
}

#define sSessionMgr Battlenet::SessionManager::Instance()

#endif // AZEROTHCORE_BNET_SESSION_MANAGER_H
