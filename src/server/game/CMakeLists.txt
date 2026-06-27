#
# This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

CU_RUN_HOOK(BEFORE_GAME_LIBRARY)

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if (USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/gamePCH.h)
endif ()

# Group sources
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(game-interface INTERFACE)

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(game-interface
  INTERFACE
    ${PUBLIC_INCLUDES})

target_link_libraries(game-interface
  INTERFACE
    shared)

add_library(game STATIC
  ${PRIVATE_SOURCES})

add_dependencies(game revision.h)

# brick E1: WorldSocket/WorldSocketMgr build on the modern Acore::Net networking stack
# (src/common/network). Those headers share bare file names (Socket.h, SocketMgr.h, NetworkThread.h,
# AsyncAcceptor.h, SocketConnectionInitializer.h) with the legacy global-namespace stack in
# src/server/shared/Network pulled in transitively via `shared`. Prepend the modern roots so the
# Acore::Net versions win bare-name resolution for game's own translation units.
target_include_directories(game
  BEFORE
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/common/network
    ${CMAKE_SOURCE_DIR}/src/common/network/ConnectionInitializers
    ${CMAKE_SOURCE_DIR}/src/server/shared/Networking/ConnectionInitializers)

target_include_directories(game
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(game
  PRIVATE
    acore-core-interface
  PUBLIC
    game-interface)

set_target_properties(game
  PROPERTIES
    FOLDER
      "server")

# Generate precompiled header
if (USE_COREPCH)
  add_cxx_pch(game ${PRIVATE_PCH_HEADER})
endif ()

CU_RUN_HOOK(AFTER_GAME_LIBRARY)
