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

#ifndef __RAACCEPTOR_H__
#define __RAACCEPTOR_H__

#include "IoContext.h"
#include <memory>

// brick E1: starts the legacy Remote Access listener and returns an opaque keep-alive handle
// (destroying it closes the acceptor). Defined in RAAcceptor.cpp so the legacy network stack stays
// out of worldserver's Main.cpp translation unit, which is now purely Acore::Net.
std::shared_ptr<void> StartRaSocketAcceptor(Acore::Asio::IoContext& ioContext);

#endif // __RAACCEPTOR_H__
