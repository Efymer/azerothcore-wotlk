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

# Make the tools list available in the current scope
GetToolsList(TOOLS_BUILD_LIST)

if (TOOLS_BUILD STREQUAL "none")
  set(TOOLS_DEFAULT_BUILD "disabled")
else()
  set(TOOLS_DEFAULT_BUILD "enabled")
endif()

# Sets BUILD_TOOLS_USE_WHITELIST
# Sets BUILD_TOOLS_WHITELIST
if (TOOLS_BUILD MATCHES "-only")
  set(BUILD_TOOLS_USE_WHITELIST ON)

  if (TOOLS_BUILD STREQUAL "maps-only")
    list(APPEND BUILD_TOOLS_WHITELIST extractor_common map_extractor mmaps_generator vmap4_assembler vmap4_extractor)
  endif()

  if (TOOLS_BUILD STREQUAL "db-only")
    list(APPEND BUILD_TOOLS_WHITELIST dbimport)
  endif()
endif()

# Set the TOOL_${TOOL_BUILD_NAME} variables from the
# variables set above
foreach(TOOL_BUILD_NAME ${TOOLS_BUILD_LIST})
  ToolNameToVariable(${TOOL_BUILD_NAME} TOOL_BUILD_VARIABLE)

  if(${TOOL_BUILD_VARIABLE} STREQUAL "default")
    if(BUILD_TOOLS_USE_WHITELIST)
      list(FIND BUILD_TOOLS_WHITELIST "${TOOL_BUILD_NAME}" INDEX)
      if(${INDEX} GREATER -1)
        set(${TOOL_BUILD_VARIABLE} ${TOOLS_DEFAULT_BUILD})
      else()
        set(${TOOL_BUILD_VARIABLE} "disabled")
      endif()
    else()
      set(${TOOL_BUILD_VARIABLE} ${TOOLS_DEFAULT_BUILD})
    endif()
  endif()

  # Build the Graph values
  if(${TOOL_BUILD_VARIABLE} MATCHES "enabled")
    list(APPEND TOOL_BUILD_GRAPH_KEYS tools)
    set(TOOL_BUILD_VALUE_DISPLAY_tools tools)
    list(APPEND TOOL_BUILD_VALUE_CONTAINS_tools ${TOOL_BUILD_NAME})
  else()
    list(APPEND TOOL_BUILD_GRAPH_KEYS disabled)
    set(TOOL_BUILD_VALUE_DISPLAY_disabled disabled)
    list(APPEND TOOL_BUILD_VALUE_CONTAINS_disabled ${TOOL_BUILD_NAME})
  endif()
endforeach()

list(SORT TOOL_BUILD_GRAPH_KEYS)
list(REMOVE_DUPLICATES TOOL_BUILD_GRAPH_KEYS)

# Display the graphs
message("")
message("* Tools build list (${TOOLS_BUILD}):")
message("  |")

foreach(TOOL_BUILD_GRAPH_KEY ${TOOL_BUILD_GRAPH_KEYS})
  if(NOT TOOL_BUILD_GRAPH_KEY STREQUAL "disabled")
    message("  +- ${TOOL_BUILD_VALUE_DISPLAY_${TOOL_BUILD_GRAPH_KEY}}")
  else()
    message("  |  ${TOOL_BUILD_VALUE_DISPLAY_${TOOL_BUILD_GRAPH_KEY}}")
  endif()
  foreach(TOOL_BUILD_GRAPH_ENTRY ${TOOL_BUILD_VALUE_CONTAINS_${TOOL_BUILD_GRAPH_KEY}})
    message("  |   +- ${TOOL_BUILD_GRAPH_ENTRY}")
  endforeach()
  message("  |")
endforeach()

message("")

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

# Generates the actual tools projects
foreach(TOOL_NAME ${TOOLS_BUILD_LIST})
  GetPathToTool(${TOOL_NAME} SOURCE_TOOL_PATH)
  ToolNameToVariable(${TOOL_NAME} TOOL_BUILD_VARIABLE)

  if (${TOOL_BUILD_VARIABLE} STREQUAL "disabled")
    continue()
  endif()

  # extractor_common is the shared CASC-access static library consumed by the
  # maps/DB2 extractors, not a standalone executable. Build it as a library and
  # skip the executable generation below.
  if (${TOOL_NAME} STREQUAL "extractor_common")
    unset(EXTRACTOR_COMMON_SOURCES)
    CollectSourceFiles(
      ${SOURCE_TOOL_PATH}
      EXTRACTOR_COMMON_SOURCES)

    add_library(extractor_common STATIC ${EXTRACTOR_COMMON_SOURCES})

    target_link_libraries(extractor_common
      PRIVATE
        acore-dependency-interface
      PUBLIC
        casc
        common)

    target_include_directories(extractor_common
      PUBLIC
        ${SOURCE_TOOL_PATH})

    set_target_properties(extractor_common
      PROPERTIES
        FOLDER
          "tools")

    continue()
  endif()

  # map_extractor reads the CASC-based 3.4.x client (CascLib + the DB2 reader),
  # not the legacy MPQ archives, so it must NOT link the mpq/Recast/g3dlib stack
  # used by the other tools. Build it as a standalone executable wired against the
  # CASC extraction toolchain (casc + extractor_common + common) and skip the
  # generic executable generation below.
  if (${TOOL_NAME} STREQUAL "map_extractor")
    GetProjectNameOfToolName(${TOOL_NAME} TOOL_PROJECT_NAME)

    unset(MAP_EXTRACTOR_SOURCES)
    CollectSourceFiles(
      ${SOURCE_TOOL_PATH}
      MAP_EXTRACTOR_SOURCES)

    if (WIN32)
      list(APPEND MAP_EXTRACTOR_SOURCES ${winDebugging})
    endif()

    add_executable(${TOOL_PROJECT_NAME}
      ${MAP_EXTRACTOR_SOURCES})

    add_dependencies(${TOOL_PROJECT_NAME} revision.h)

    target_link_libraries(${TOOL_PROJECT_NAME}
      PRIVATE
        acore-dependency-interface
      PUBLIC
        casc
        extractor_common
        common
        zlib)

    target_include_directories(${TOOL_PROJECT_NAME}
      PUBLIC
        ${SOURCE_TOOL_PATH}
        ${SOURCE_TOOL_PATH}/loadlib
      PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/${TOOL_NAME})

    set_target_properties(${TOOL_PROJECT_NAME}
      PROPERTIES
        FOLDER
          "tools")

    if (UNIX)
      install(TARGETS ${TOOL_PROJECT_NAME} DESTINATION bin)
    elseif (WIN32)
      install(TARGETS ${TOOL_PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif()

    continue()
  endif()

  unset(TOOL_PRIVATE_SOURCES)
  CollectSourceFiles(
    ${SOURCE_TOOL_PATH}
    TOOL_PRIVATE_SOURCES)

  if (WIN32)
    list(APPEND TOOL_PRIVATE_SOURCES ${winDebugging})
  endif()

  GetProjectNameOfToolName(${TOOL_NAME} TOOL_PROJECT_NAME)

  # Create the application project
  add_executable(${TOOL_PROJECT_NAME}
    ${TOOL_PRIVATE_SOURCES})

  add_dependencies(${TOOL_PROJECT_NAME} revision.h)

  # Need fix errors in maps tools
  # target_link_libraries(${TOOL_PROJECT_NAME}
  #   PRIVATE
  #     acore-dependency-interface)

  if (${TOOL_PROJECT_NAME} MATCHES "dbimport")
    target_link_libraries(${TOOL_PROJECT_NAME}
      PUBLIC
        database
      PRIVATE
        modules
        scripts
        acore-core-interface)

    # Install config
    CopyToolConfig(${TOOL_PROJECT_NAME} ${TOOL_NAME})
  else()

    target_link_libraries(${TOOL_PROJECT_NAME}
    PRIVATE
      acore-dependency-interface)

    target_link_libraries(${TOOL_PROJECT_NAME}
      PUBLIC
        common
        mpq
        zlib
        Recast
        g3dlib
        fkYAML)
  endif()

  unset(TOOL_PUBLIC_INCLUDES)
  CollectIncludeDirectories(
    ${SOURCE_TOOL_PATH}
    TOOL_PUBLIC_INCLUDES)

  target_include_directories(${TOOL_PROJECT_NAME}
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR})

  target_include_directories(${TOOL_PROJECT_NAME}
    PUBLIC
      ${TOOL_PUBLIC_INCLUDES}
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/modules
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/${TOOL_NAME})

  set_target_properties(${TOOL_PROJECT_NAME}
    PROPERTIES
      FOLDER
        "tools")

  if (UNIX)
    install(TARGETS ${TOOL_PROJECT_NAME} DESTINATION bin)
  elseif (WIN32)
    install(TARGETS ${TOOL_PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()

  if (${TOOL_PROJECT_NAME} STREQUAL "mmaps_generator")
    if(WIN32)
      if("${CMAKE_MAKE_PROGRAM}" MATCHES "MSBuild")
        add_custom_command(TARGET ${TOOL_PROJECT_NAME}
          POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SOURCE_TOOL_PATH}/mmaps-config.yaml" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/mmaps-config.yaml")
      elseif(MINGW)
        add_custom_command(TARGET ${TOOL_PROJECT_NAME}
          POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SOURCE_TOOL_PATH}/mmaps-config.yaml" "${CMAKE_BINARY_DIR}/bin/mmaps-config.yaml")
      endif()
    endif()

    if (UNIX)
      install(FILES ${SOURCE_TOOL_PATH}/mmaps-config.yaml DESTINATION bin)
    elseif (WIN32)
      install(FILES ${SOURCE_TOOL_PATH}/mmaps-config.yaml DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif()
  endif()
endforeach()
