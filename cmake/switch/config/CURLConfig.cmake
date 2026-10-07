# Switch: devkitPro's switch-curl ships pkg-config only (run by a bash wrapper that
# native CMake can't use). Provide the CURL::libcurl target find_package expects.
set(_p "${DEVKITPRO}/portlibs/switch")
if(NOT TARGET CURL::libcurl)
    add_library(CURL::libcurl STATIC IMPORTED)
    set_target_properties(CURL::libcurl PROPERTIES
        IMPORTED_LOCATION "${_p}/lib/libcurl.a"
        INTERFACE_INCLUDE_DIRECTORIES "${_p}/include"
        INTERFACE_COMPILE_DEFINITIONS "CURL_STATICLIB"
        INTERFACE_LINK_LIBRARIES "${_p}/lib/libmbedtls.a;${_p}/lib/libmbedx509.a;${_p}/lib/libmbedcrypto.a;${_p}/lib/libz.a")
endif()
set(CURL_FOUND TRUE)
