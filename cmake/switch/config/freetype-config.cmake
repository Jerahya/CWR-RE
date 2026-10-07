# Switch: devkitPro's freetype config hardcodes /opt/devkitpro paths (MSYS2 view),
# which native Windows CMake/ld can't resolve. Same targets, Windows paths.
set(_p "${DEVKITPRO}/portlibs/switch")
if(NOT TARGET freetype)
    add_library(freetype STATIC IMPORTED)
    set_target_properties(freetype PROPERTIES
        IMPORTED_LOCATION "${_p}/lib/libfreetype.a"
        INTERFACE_INCLUDE_DIRECTORIES "${_p}/include/freetype2"
        INTERFACE_LINK_LIBRARIES "${_p}/lib/libharfbuzz.a;${_p}/lib/libpng.a;${_p}/lib/libbz2.a;${_p}/lib/libz.a")
    # harfbuzz and freetype reference each other.
    set_property(TARGET freetype PROPERTY IMPORTED_LINK_INTERFACE_MULTIPLICITY 2)
endif()
if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype INTERFACE IMPORTED)
    set_target_properties(Freetype::Freetype PROPERTIES INTERFACE_LINK_LIBRARIES freetype)
endif()
set(freetype_FOUND TRUE)
