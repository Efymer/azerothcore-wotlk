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

add_subdirectory(apps)

if ((APPS_BUILD AND NOT APPS_BUILD STREQUAL "none") OR BUILD_TOOLS_DB_IMPORT)
  add_subdirectory(database)
endif()

if (BUILD_APPLICATION_AUTHSERVER OR BUILD_APPLICATION_WORLDSERVER)
  # proto only depends on the vendored protobuf + common, so it is added before
  # shared/game. Building any application implies APPS_BUILD is enabled, which is
  # also the guard that pulls in the protobuf dependency target.
  add_subdirectory(proto)
  add_subdirectory(shared)
endif()

if (BUILD_APPLICATION_WORLDSERVER)
  add_subdirectory(game)
  add_subdirectory(scripts)
endif()
