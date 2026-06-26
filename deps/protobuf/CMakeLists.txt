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

# Protocol Buffers is required by the modern Battle.net login server (bnetserver),
# which speaks protobuf over its session. We prefer a system/package-manager
# provided Protobuf and wrap it as an interface target, consistent with how the
# other dependencies (openssl, boost, mysql) are exposed.

# Do not pin a version. Modern Protobuf (abseil-based, e.g. 6.x / libprotoc
# 33.x as provided by vcpkg) ships a CONFIG package whose version-compat is
# SameMajorVersion, so any explicit "minimum" major (e.g. 3.0.0) is rejected
# against a higher installed major. We only need the modern imported targets
# (protobuf::libprotobuf, protobuf::protoc), which all supported versions ship.
find_package(Protobuf REQUIRED)

if(NOT Protobuf_FOUND)
  message(FATAL_ERROR "Protobuf wasn't found on your system but it's required to build the Battle.net login server (bnetserver)! Install it (e.g. via your package manager or vcpkg) and make sure CMake can locate it.")
endif()

add_library(protobuf INTERFACE)

target_link_libraries(protobuf
  INTERFACE
    protobuf::libprotobuf)

target_include_directories(protobuf
  INTERFACE
    ${Protobuf_INCLUDE_DIRS})

# Expose the protoc compiler to later CMake code that generates .pb.cc/.pb.h
# from the vendored proto definitions. FindProtobuf populates the
# Protobuf_PROTOC_EXECUTABLE cache variable and, where available, the
# protobuf::protoc imported target; surface both for downstream consumers.
if(NOT Protobuf_PROTOC_EXECUTABLE AND TARGET protobuf::protoc)
  get_target_property(Protobuf_PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION_RELEASE)
endif()

set(PROTOBUF_PROTOC_EXECUTABLE "${Protobuf_PROTOC_EXECUTABLE}"
  CACHE FILEPATH "Path to the protoc Protocol Buffers compiler executable" FORCE)

message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
message(STATUS "  * protoc compiler: ${PROTOBUF_PROTOC_EXECUTABLE}")
