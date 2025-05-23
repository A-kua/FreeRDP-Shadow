# WinPR: Windows Portable Runtime
# winpr cmake build script
#
# Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

cmake_minimum_required(VERSION 3.5)

project(WinPR C)

set(CMAKE_COLOR_MAKEFILE ON)

option(WITH_VERBOSE_WINPR_ASSERT "Compile with verbose WINPR_ASSERT." ON)

if (WITH_VERBOSE_WINPR_ASSERT)
    add_definitions(-DWITH_VERBOSE_WINPR_ASSERT)
endif()

if(FREERDP_VERSION)
	set(FREERDP_BUILD 1)
endif()

# Include cmake modules
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(FindPkgConfig)
include(TestBigEndian)

# Include our extra modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/)

# Check for cmake compatibility (enable/disable features)
include(CheckCmakeCompat)
include(FindFeature)
include(ConfigOptions)
include(ComplexLibrary)
include(FeatureSummary)
include(CheckCCompilerFlag)
include(GNUInstallDirsWrapper)
include(CMakePackageConfigHelpers)

if (NOT WIN32)
    add_definitions(-DWINPR_CRITICAL_SECTION_DISABLE_SPINCOUNT)
endif()

# Soname versioning
set(RAW_VERSION_STRING "2.11.8-dev")
if(EXISTS "${CMAKE_SOURCE_DIR}/.source_tag")
	file(READ ${CMAKE_SOURCE_DIR}/.source_tag RAW_VERSION_STRING)
elseif(USE_VERSION_FROM_GIT_TAG)
	git_get_exact_tag(_GIT_TAG --tags --always)
	if (NOT ${_GIT_TAG} STREQUAL "n/a")
		set(RAW_VERSION_STRING ${_GIT_TAG})
	endif()
endif()
string(STRIP ${RAW_VERSION_STRING} RAW_VERSION_STRING)

set(VERSION_REGEX "^.?([0-9]+)\\.([0-9]+)\\.([0-9]+)-?(.*)")
string(REGEX REPLACE "${VERSION_REGEX}" "\\1" WINPR_VERSION_MAJOR "${RAW_VERSION_STRING}")
string(REGEX REPLACE "${VERSION_REGEX}" "\\2" WINPR_VERSION_MINOR "${RAW_VERSION_STRING}")
string(REGEX REPLACE "${VERSION_REGEX}" "\\3" WINPR_VERSION_REVISION "${RAW_VERSION_STRING}")
string(REGEX REPLACE "${VERSION_REGEX}" "\\4" WINPR_VERSION_SUFFIX "${RAW_VERSION_STRING}")

set(WINPR_VERSION "${WINPR_VERSION_MAJOR}.${WINPR_VERSION_MINOR}.${WINPR_VERSION_REVISION}")
set(WINPR_API_VERSION "${WINPR_VERSION_MAJOR}")
if (WINPR_VERSION_SUFFIX)
	set(WINPR_VERSION_FULL "${WINPR_VERSION}-${WINPR_VERSION_SUFFIX}")
else()
	set(WINPR_VERSION_FULL "${WINPR_VERSION}")
endif()

option(WITH_INTERNAL_MD4 "Use compiled in md4 hash functions instead of OpenSSL/MBedTLS" OFF)
option(WITH_INTERNAL_MD5 "Use compiled in md5 hash functions instead of OpenSSL/MBedTLS" OFF)
if(NOT IOS)
	check_include_files(stdbool.h WINPR_HAVE_STDBOOL_H)
	check_include_files(stdint.h WINPR_HAVE_STDINT_H)
	check_include_files(inttypes.h WINPR_HAVE_INTTYPES_H)
	check_include_files(execinfo.h HAVE_EXECINFO_HEADER)
	if (HAVE_EXECINFO_HEADER)
		check_symbol_exists(backtrace execinfo.h HAVE_EXECINFO_BACKTRACE)
		check_symbol_exists(backtrace_symbols execinfo.h HAVE_EXECINFO_BACKTRACE_SYMBOLS)
		check_symbol_exists(backtrace_symbols_fd execinfo.h HAVE_EXECINFO_BACKTRACE_SYMBOLS_FD)

		# Some implementations (e.g. Android NDK API < 33) provide execinfo.h but do not define
		# the backtrace functions. Disable detection for these cases
		if (HAVE_EXECINFO_BACKTRACE AND HAVE_EXECINFO_BACKTRACE_SYMBOLS AND HAVE_EXECINFO_BACKTRACE_SYMBOLS_FD)
			set(HAVE_EXECINFO_H ON)
		endif()
	endif()
else(NOT IOS)
	set(WINPR_HAVE_STDBOOL_H 1)
	set(WINPR_HAVE_STDINT_H 1)
	set(WINPR_HAVE_INTTYPES_H 1)
endif(NOT IOS)

if(FREERDP_BUILD)
	set(WINPR_VERSION_FULL ${WINPR_VERSION_FULL} PARENT_SCOPE)
	set(WINPR_VERSION ${WINPR_VERSION} PARENT_SCOPE)
	set(WINPR_API_VERSION ${WINPR_API_VERSION} PARENT_SCOPE)
else()
	set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

	if(NOT IOS)
		find_package(Threads REQUIRED)
	endif()

	# Include files
	if(NOT IOS)
		check_include_files(fcntl.h HAVE_FCNTL_H)
		check_include_files(unistd.h HAVE_UNISTD_H)
		check_include_files(inttypes.h HAVE_INTTYPES_H)
		check_include_files(sys/modem.h HAVE_SYS_MODEM_H)
		check_include_files(sys/filio.h HAVE_SYS_FILIO_H)
		check_include_files(sys/sockio.h HAVE_SYS_SOCKIO_H)
		check_include_files(sys/strtio.h HAVE_SYS_STRTIO_H)
		check_include_files(sys/select.h HAVE_SYS_SELECT_H)
		check_include_files(unwind.h HAVE_UNWIND_H)
        if (HAVE_UNWIND_H)
            add_definitions(-DHAVE_UNWIND_H)
        endif()
	else()
		set(HAVE_FCNTL_H 1)
		set(HAVE_UNISTD_H 1)
		set(HAVE_INTTYPES_H 1)
		set(HAVE_SYS_FILIO_H 1)
	endif()

	if(NOT IOS)
		check_struct_has_member("struct tm" tm_gmtoff time.h HAVE_TM_GMTOFF)
	else()
		set(HAVE_TM_GMTOFF 1)
	endif()

	if(NOT WIN32 AND NOT IOS)
		CHECK_SYMBOL_EXISTS(pthread_mutex_timedlock pthread.h HAVE_PTHREAD_MUTEX_TIMEDLOCK_SYMBOL)
		if (NOT HAVE_PTHREAD_MUTEX_TIMEDLOCK_SYMBOL)
			CHECK_LIBRARY_EXISTS(pthread pthread_mutex_timedlock "" HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIB)
		endif (NOT HAVE_PTHREAD_MUTEX_TIMEDLOCK_SYMBOL)
		if (NOT HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIB)
			CHECK_LIBRARY_EXISTS(pthreads pthread_mutex_timedlock "" HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIBS)
		endif (NOT HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIB)

		if (HAVE_PTHREAD_MUTEX_TIMEDLOCK_SYMBOL OR HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIB OR HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIBS)
			set(HAVE_PTHREAD_MUTEX_TIMEDLOCK ON)
		endif (HAVE_PTHREAD_MUTEX_TIMEDLOCK_SYMBOL OR HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIB OR HAVE_PTHREAD_MUTEX_TIMEDLOCK_LIBS)
	endif()

	set(OPENSSL_FEATURE_TYPE "OPTIONAL")
	set(OPENSSL_FEATURE_PURPOSE "cryptography")
	set(OPENSSL_FEATURE_DESCRIPTION "encryption, certificate validation, hashing functions")

	set(MBEDTLS_FEATURE_TYPE "OPTIONAL")
	set(MBEDTLS_FEATURE_PURPOSE "cryptography")
	set(MBEDTLS_FEATURE_DESCRIPTION "encryption, certificate validation, hashing functions")

	find_feature(OpenSSL ${OPENSSL_FEATURE_TYPE} ${OPENSSL_FEATURE_PURPOSE} ${OPENSSL_FEATURE_DESCRIPTION})
	find_feature(MbedTLS ${MBEDTLS_FEATURE_TYPE} ${MBEDTLS_FEATURE_PURPOSE} ${MBEDTLS_FEATURE_DESCRIPTION})

	if(OPENSSL_FOUND)
		add_definitions("-DWITH_OPENSSL")
	endif()

	if(MBEDTLS_FOUND)
		add_definitions("-DWITH_MBEDTLS")
	endif()

	# Include directories
	include_directories(${CMAKE_CURRENT_BINARY_DIR})

	# Configure files
	add_definitions("-DHAVE_CONFIG_H")
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

	enable_testing()

	if(MSVC)
		set(TESTING_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	else()
		set(TESTING_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Testing")
	endif()
endif()

# Default to release build type
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

# Default to build shared libs
if(NOT DEFINED BUILD_SHARED_LIBS)
	set(BUILD_SHARED_LIBS ON)
endif()

if(BUILD_SHARED_LIBS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWINPR_DLL")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWINPR_EXPORTS")

# Enable 64bit file support on linux and FreeBSD.
if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" OR FREEBSD)
	add_definitions("-D_FILE_OFFSET_BITS=64")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

add_subdirectory(include)

add_subdirectory(libwinpr)

if(NOT ANDROID AND NOT IOS AND NOT UWP AND WITH_WINPR_TOOLS)
	add_subdirectory(tools)
endif()

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

install_freerdp_man(wlog.7 7)
# Exporting

if(${CMAKE_VERSION} VERSION_GREATER "2.8.10")

	export(PACKAGE winpr)

	SetFreeRDPCMakeInstallDir(WINPR_CMAKE_INSTALL_DIR "WinPR${WINPR_VERSION_MAJOR}")

	set(WINPR_INCLUDE_DIR "include/winpr${WINPR_VERSION_MAJOR}")

	configure_package_config_file(WinPRConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/WinPRConfig.cmake
		INSTALL_DESTINATION ${WINPR_CMAKE_INSTALL_DIR}
		PATH_VARS WINPR_INCLUDE_DIR)

	write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/WinPRConfigVersion.cmake
		VERSION ${WINPR_VERSION} COMPATIBILITY SameMajorVersion)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WinPRConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/WinPRConfigVersion.cmake
		DESTINATION ${WINPR_CMAKE_INSTALL_DIR})

	install(EXPORT WinPRTargets DESTINATION ${WINPR_CMAKE_INSTALL_DIR})

endif()

if(FREERDP_BUILD)
	set(WINPR_PKG_CONFIG_FILENAME winpr${WINPR_VERSION_MAJOR} PARENT_SCOPE)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/winpr.pc.in ${CMAKE_CURRENT_BINARY_DIR}/winpr${WINPR_VERSION_MAJOR}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/winpr${WINPR_VERSION_MAJOR}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
