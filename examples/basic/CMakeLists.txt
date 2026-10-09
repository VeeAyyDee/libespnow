# SPDX-License-Identifier: MPL-2.0
cmake_minimum_required(VERSION 3.16)
get_filename_component(LIBESPNOW_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(EXTRA_COMPONENT_DIRS "${LIBESPNOW_SOURCE}")
set(COMPONENTS main libespnow)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(libespnow_basic)
include("${LIBESPNOW_SOURCE}/cmake/replace_vendor_archive.cmake")
