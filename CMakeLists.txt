#
# Copyright 2016 James S. Waller
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

project(wrparsecxx CXX C)
cmake_minimum_required(VERSION 2.6)

include(CheckCXXSourceRuns)

set(WRPARSECXX_VERSION_MAJOR 0)
set(WRPARSECXX_VERSION_MINOR 1)
set(WRPARSECXX_VERSION_PATCH 0)

if (NOT TOP_BUILD_DIR)
        set(TOP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

#########################################
#
# Dependencies
#
if (WRUTIL_ROOT)
        # fall through
elseif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/wrutil/)
        set(WRUTIL_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/wrutil)
        file(MAKE_DIRECTORY ${WRUTIL_BUILD_DIR})
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/wrutil ${WRUTIL_BUILD_DIR})

        include(${CMAKE_CURRENT_SOURCE_DIR}/wrutil/cmake/compiler-config.cmake)
        include(${WRUTIL_BUILD_DIR}/wrutil-config-internal.cmake)
elseif (UNIX)
        if (EXISTS /usr/share/wrutil/wrutil-targets.cmake)
                set(WRUTIL_ROOT /usr)
        elseif (EXISTS /usr/local/share/wrutil/wrutil-targets.cmake)
                set(WRUTIL_ROOT /usr/local)
        endif()
endif()

if (NOT WRUTIL_BUILD_DIR)
        if (WRUTIL_ROOT)
                include(${WRUTIL_ROOT}/share/wrutil/compiler-config.cmake)
                include(${WRUTIL_ROOT}/share/wrutil/wrutil-config.cmake)
                include(${WRUTIL_ROOT}/share/wrutil/wrutil-targets.cmake)

                set(CMAKE_REQUIRED_LIBRARIES wrutil)
                set(CMAKE_REQUIRED_DEFINITIONS "-Dwrutil_IMPORTS")
                set(CHECK_CXX_CODE "#include <wrutil/uiostream.h>\nint main() { wr::uout << \"Hello\" << std::endl\; return 0\; }\n")
                check_cxx_source_runs(${CHECK_CXX_CODE} HAVE_WRUTIL)
        endif()
        if (NOT HAVE_WRUTIL)
                message(SEND_ERROR "wrutil library not found. Please either specify -DWRUTIL_ROOT=<dir> where <dir> is the base directory of a pre-installed wrutil package, or place a copy of the wrutil source tree (with directory name \"wrutil\") under the top directory of wrparse.")
        endif()
endif()

if (WRPARSE_ROOT)
        # fall through
elseif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/wrparse/)
        set(WRPARSE_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/wrparse)
        file(MAKE_DIRECTORY ${WRPARSE_BUILD_DIR})
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/wrparse
                         ${WRPARSE_BUILD_DIR})
        include(${WRPARSE_BUILD_DIR}/wrparse-config-internal.cmake)
elseif (UNIX)
        if (EXISTS /usr/share/wrparse/wrparse-targets.cmake)
                set(WRPARSE_ROOT /usr)
        elseif (EXISTS /usr/local/share/wrparse/wrparse-targets.cmake)
                set(WRPARSE_ROOT /usr/local)
        endif()
endif()

if (NOT WRPARSE_BUILD_DIR)
        if (WRPARSE_ROOT)
                include(${WRPARSE_ROOT}/share/wrparse/wrparse-config.cmake)
                include(${WRPARSE_ROOT}/share/wrparse/wrparse-targets.cmake)
                list(APPEND CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})
                set(CMAKE_REQUIRED_LIBRARIES wrparse)
                set(CMAKE_REQUIRED_DEFINITIONS "-Dwrutil_IMPORTS -Dwrparse_IMPORTS")
                set(CHECK_CXX_CODE "#include <wrparse/Parser.h>\nint main() { wr::parse::Parser parser\; (void) parser\; return 0\; }\n")
                check_cxx_source_runs(${CHECK_CXX_CODE} HAVE_WRPARSE)
        endif()
        if (NOT HAVE_WRPARSE)
                message(SEND_ERROR "wrparse library not found. Please either specify -DWRPARSE_ROOT=<dir> where <dir> is the base directory of a pre-installed wrparse package, or place a copy of the wrparse source tree (with directory name \"wrparse\") under the top directory of wrparsecxx.")
        endif()
endif()

########################################
#
# Target Definitions
#
include_directories(include)

set(WRPARSECXX_SOURCES
        src/CXXLexer.cxx
        src/CXXOptions.cxx
        src/CXXParser.cxx
        src/CXXTokenKinds.cxx
        src/ExprMatch.cxx
)

set(WRPARSECXX_HEADERS
        include/wrparse/cxx/CXXLexer.h
        include/wrparse/cxx/CXXOptions.h
        include/wrparse/cxx/CXXParser.h
        include/wrparse/cxx/CXXTokenKinds.h
        include/wrparse/cxx/ExprMatch.h
)

add_library(wrparsecxx SHARED ${WRPARSECXX_SOURCES} ${WRPARSECXX_HEADERS})
target_link_libraries(wrparsecxx wrparse)
set_target_properties(wrparsecxx PROPERTIES
        COMPILE_FLAGS "-Dwrutil_IMPORTS -Dwrparse_IMPORTS ${WR_SOFLAGS}"
        SOVERSION ${WRPARSECXX_VERSION_MAJOR}
        VERSION ${WRPARSECXX_VERSION_MAJOR}.${WRPARSECXX_VERSION_MINOR}.${WRPARSECXX_VERSION_PATCH}
)

add_library(wrparsecxx_static STATIC ${WRPARSECXX_SOURCES} ${WRPARSECXX_HEADERS})

add_executable(lexcxx example/lexcxx.cxx example/lex_parse_options.cxx)
target_link_libraries(lexcxx wrparsecxx wrparse wrutil)

add_executable(parsecxx example/parsecxx.cxx example/lex_parse_options.cxx)
target_link_libraries(parsecxx wrparsecxx wrparse wrutil)
set_target_properties(lexcxx parsecxx
        PROPERTIES COMPILE_FLAGS "-Dwrutil_IMPORTS -Dwrparse_IMPORTS"
)

########################################
#
# Output Directories
#
set_target_properties(wrparsecxx wrparsecxx_static
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TOP_BUILD_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${TOP_BUILD_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${TOP_BUILD_DIR}/lib
)

set_target_properties(lexcxx parsecxx
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY example
)

########################################
#
# Installation
#
install(TARGETS wrparsecxx wrparsecxx_static
        EXPORT wrparsecxx-targets
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)

install(DIRECTORY include/wrparse/cxx
        DESTINATION include
        DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                              GROUP_READ GROUP_EXECUTE
                              WORLD_READ WORLD_EXECUTE
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE
                         GROUP_READ
                         WORLD_READ
)

install(EXPORT wrparsecxx-targets DESTINATION share/wrparse)

configure_file(cmake/wrparsecxx-config.cmake.in wrparsecxx-config.cmake @ONLY)
configure_file(cmake/wrparsecxx-config-internal.cmake.in
               wrparsecxx-config-internal.cmake @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wrparsecxx-config.cmake
        DESTINATION share/wrparse
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ
                    WORLD_READ
)
