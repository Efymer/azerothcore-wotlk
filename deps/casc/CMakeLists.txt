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

# CascLib is required by the client-data extraction toolchain (map/DB2/vmap
# extractors) to read the modern WoW client's CASC storage, replacing the
# MPQ-based access used for the 3.3.5a client. We prefer a system/package-manager
# provided CascLib and wrap it as an interface target, consistent with how the
# other dependencies (openssl, protobuf, mysql) are exposed.

find_package(CascLib CONFIG REQUIRED)

if(NOT TARGET CascLib::casc)
  message(FATAL_ERROR "CascLib wasn't found on your system but it's required to build the CASC-based client extraction tools! Install it (e.g. via your package manager or vcpkg) and make sure CMake can locate it.")
endif()

add_library(casc INTERFACE)

target_link_libraries(casc
  INTERFACE
    CascLib::casc)

message(STATUS "Found CascLib: ${CascLib_VERSION}")
