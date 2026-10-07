# Nintendo Switch (homebrew) toolchain — DRAFT, untested.
#
# Wraps devkitPro's official Switch toolchain (devkitA64 GCC + libnx + portlibs)
# so we inherit its CPU flags (-march=armv8-a+crc+crypto -mtune=cortex-a57
# -mtp=soft -fPIE), sysroot, pkg-config setup and the nx_* packaging helpers.
#
# Requires the devkitPro packages: switch-dev switch-mesa switch-glad
# switch-openal-soft
# switch-curl switch-freetype switch-libvorbis switch-libogg switch-libopus
# switch-libzstd switch-mbedtls switch-zlib switch-pkg-config.
#
# Used both as the main toolchain and as the vcpkg chainload toolchain for the
# arm64-switch triplet.

if(DEFINED ENV{DEVKITPRO})
    set(DEVKITPRO "$ENV{DEVKITPRO}")
elseif(CMAKE_HOST_WIN32)
    set(DEVKITPRO "C:/devkitPro")
else()
    set(DEVKITPRO "/opt/devkitpro")
endif()
file(TO_CMAKE_PATH "${DEVKITPRO}" DEVKITPRO)

set(DEVKITPRO "${DEVKITPRO}" CACHE PATH "devkitPro root" FORCE)

set(ENV{DEVKITPRO} "${DEVKITPRO}")

if(NOT EXISTS "${DEVKITPRO}/cmake/Switch.cmake")
    message(FATAL_ERROR "devkitPro Switch toolchain not found at ${DEVKITPRO}/cmake/Switch.cmake "
                        "(install devkitPro with the switch-dev group, or set DEVKITPRO)")
endif()

if(CMAKE_HOST_WIN32)
    list(APPEND CMAKE_MODULE_PATH "${DEVKITPRO}/cmake")
    include(dkp-toolchain-common)
    set(CMAKE_SYSTEM_NAME NintendoSwitch)
    set(DKP_BIN2S_ALIGNMENT 8)
    # __dkp_toolchain is a macro: its CMAKE_CURRENT_LIST_DIR would resolve to this folder.
    set(CMAKE_USER_MAKE_RULES_OVERRIDE "${DEVKITPRO}/cmake/dkp-rule-overrides.cmake")
    __dkp_toolchain(devkitA64 aarch64 aarch64-none-elf)

    # __dkp_toolchain splits PATH on ':' (Unix); redo it with Windows separators.
    file(TO_CMAKE_PATH "$ENV{PATH}" CMAKE_SYSTEM_PROGRAM_PATH)

    set(NX_ROOT ${DEVKITPRO}/libnx CACHE PATH "libnx root" FORCE)
    set(DKP_INSTALL_PREFIX_INIT ${DEVKITPRO}/portlibs/switch)
    __dkp_platform_prefix(${DEVKITPRO}/portlibs/switch ${NX_ROOT})

    foreach(_tool ELF2NRO ELF2KIP ELF2NSO BUILD_PFS0 NACPTOOL NPDMTOOL UAM)
        string(TOLOWER "${_tool}" _name)
        find_program(NX_${_tool}_EXE NAMES ${_name} HINTS "${DEVKITPRO}/tools/bin")
    endforeach()
    find_file(NX_DEFAULT_ICON NAMES default_icon.jpg HINTS "${NX_ROOT}" NO_CMAKE_FIND_ROOT_PATH)

    list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../switch/config")
else()
    include("${DEVKITPRO}/cmake/Switch.cmake")
endif()

foreach(_var CFLAGS CXXFLAGS)
    if(NOT "$ENV{${_var}}" MATCHES "_GNU_SOURCE")
        set(ENV{${_var}} "$ENV{${_var}} -D_GNU_SOURCE")
    endif()
endforeach()

set(CWR_PLATFORM_SWITCH ON CACHE BOOL "Building for Nintendo Switch homebrew" FORCE)

# Experimental: compile with clang against the devkitA64 sysroot instead of GCC.
# The codebase is clang-first (clang-cl on Windows, clang on Linux/macOS), so
# GCC may surface new diagnostics. Flip this if the GCC build fights back.
option(CWR_SWITCH_USE_CLANG "Use clang (aarch64-none-elf) with the devkitA64 sysroot" OFF)
if(CWR_SWITCH_USE_CLANG)
    find_program(_cwr_clang NAMES clang REQUIRED)
    find_program(_cwr_clangxx NAMES clang++ REQUIRED)
    set(CMAKE_C_COMPILER   "${_cwr_clang}"   CACHE FILEPATH "" FORCE)
    set(CMAKE_CXX_COMPILER "${_cwr_clangxx}" CACHE FILEPATH "" FORCE)
    set(_cwr_target_flags
        "--target=aarch64-none-elf --gcc-toolchain=${DEVKITPRO}/devkitA64 --sysroot=${DEVKITPRO}/devkitA64/aarch64-none-elf")
    string(APPEND CMAKE_C_FLAGS_INIT   " ${_cwr_target_flags}")
    string(APPEND CMAKE_CXX_FLAGS_INIT " ${_cwr_target_flags}")
endif()
