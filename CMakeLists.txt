cmake_minimum_required(VERSION 3.21)

# ------------------------------------------------------------------------------
#  RENAME THESE for every new mod (README -> "New mod from this template")
# ------------------------------------------------------------------------------
set(PRODUCT_NAME   "MyPlugin")     # DLL base name + mod folder name, no spaces
set(BEAUTIFUL_NAME "My Plugin")    # display name: menu section, log line, .rc
set(MOD_AUTHOR     "Daminikov")
set(MOD_VERSION    "1.0.0")

option(PLUGIN_AUTO_DEPLOY "Copy the built plugin into the mod manager folder" ON)

if(NOT DEFINED ENV{COMMONLIB_SSE_FOLDER})
  message(FATAL_ERROR "COMMONLIB_SSE_FOLDER is not set - point it at a CommonLibSSE-NG checkout (v8.x).")
endif()

project(${PRODUCT_NAME} VERSION ${MOD_VERSION} LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# --------------------------------------------------------------- dependencies
# spdlog + directxtk + rapidcsv are required by CommonLibSSE-NG itself.
find_package(spdlog CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)

# ----------------------------------------------------------- CommonLibSSE-NG
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory("$ENV{COMMONLIB_SSE_FOLDER}" "${CMAKE_BINARY_DIR}/_deps/clib-build" EXCLUDE_FROM_ALL)
include("$ENV{COMMONLIB_SSE_FOLDER}/cmake/CommonLibSSE.cmake")

# ------------------------------------------------------------------- sources
file(GLOB_RECURSE PLUGIN_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE PLUGIN_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

# ---------------------------------------------------------- version resource
# NOTE: the .rc has to be compiled by CMake's RC language (project(... LANGUAGES CXX RC)).
# Feeding a pre-built .res through a custom command silently ends up as an order-only
# dependency: the DLL links, but FileVersion stays empty.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
list(APPEND PLUGIN_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/version.rc")

add_commonlibsse_plugin(
    ${PROJECT_NAME}
    AUTHOR "${MOD_AUTHOR}"
    SOURCES ${PLUGIN_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/vendor")

# NOTE: CommonLibSSE-NG v8 generates a TU that includes only REL/Relocation.h and
# SKSE/SKSE.h while REL/Version.h relies on std headers. Without this PCH the
# build dies inside REL/Version.h ("std::uint16_t is not a member of std").
target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/PCH.h")

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PRODUCT_NAME="${PRODUCT_NAME}"
    BEAUTIFUL_NAME="${BEAUTIFUL_NAME}"
    MOD_AUTHOR="${MOD_AUTHOR}"
    MOD_VERSION="${MOD_VERSION}")

# -------------------------------------------------------------------- deploy
if(PLUGIN_AUTO_DEPLOY AND DEFINED ENV{SKYRIM_MODS_FOLDER})
    set(MOD_DIR "$ENV{SKYRIM_MODS_FOLDER}/${PRODUCT_NAME}")
    message(STATUS "Deploy: ${MOD_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MOD_DIR}/SKSE/Plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${PROJECT_NAME}>" "${MOD_DIR}/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MOD_DIR}/PrismaUI/views/${PRODUCT_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/view" "${MOD_DIR}/PrismaUI/views/${PRODUCT_NAME}"
        VERBATIM)
    if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${MOD_DIR}/SKSE/Plugins/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM)
    endif()
elseif(PLUGIN_AUTO_DEPLOY)
    message(STATUS "Deploy skipped: SKYRIM_MODS_FOLDER is not set")
endif()