# Switch homebrew packaging — DRAFT, untested.
#
# cwr_switch_package(<target>) adds two targets:
#
#   <target>_nro  Homebrew .nro (launch from hbmenu, or stream with `nxlink -s`).
#                 Needs only devkitPro tools. This is the dev-loop format.
#
#   <target>_nsp  Installable .nsp, built only when CWR_SWITCH_KEYS points at the
#                 builder's own keys file.
#
# Tool lookups expect $DEVKITPRO/tools/bin (nacptool, elf2nro, elf2nso, npdmtool).
# hacbrewpack is a separate community tool and must be on PATH for the NSP target.

set(CWR_SWITCH_APP_NAME    "CWR-RE"           CACHE STRING "Title name shown on the Switch (no Bohemia trademarks)")
set(CWR_SWITCH_APP_AUTHOR  "CWR-RE contributors" CACHE STRING "Author shown on the Switch")
set(CWR_SWITCH_APP_VERSION "${PROJECT_VERSION}" CACHE STRING "Version shown on the Switch")
set(CWR_SWITCH_TITLE_ID    "05C3A0C0DE000000" CACHE STRING "16-hex-digit title/program ID (homebrew 05xx range)")
set(CWR_SWITCH_ICON        "${CMAKE_SOURCE_DIR}/resources/switch/icon.jpg" CACHE FILEPATH "256x256 JPEG icon")
set(CWR_SWITCH_KEYS        "" CACHE FILEPATH "Builder's own prod.keys; enables the NSP target when set")

find_program(NACPTOOL    nacptool    HINTS "${DEVKITPRO}/tools/bin" REQUIRED)
find_program(ELF2NRO     elf2nro     HINTS "${DEVKITPRO}/tools/bin" REQUIRED)
find_program(ELF2NSO     elf2nso     HINTS "${DEVKITPRO}/tools/bin")
find_program(NPDMTOOL    npdmtool    HINTS "${DEVKITPRO}/tools/bin")
find_program(HACBREWPACK hacbrewpack)

function(cwr_switch_package target)
    set(out "${CMAKE_BINARY_DIR}/switch/${target}")
    set(elf "$<TARGET_FILE:${target}>")
    set(nacp "${out}/control.nacp")

    if(NOT EXISTS "${CWR_SWITCH_ICON}")
        message(WARNING "Switch icon not found at ${CWR_SWITCH_ICON}; NRO/NSP will use the default icon. "
                        "Do not use Arma/Operation Flashpoint logos.")
    endif()

    # ── NRO ──────────────────────────────────────────────────────────────────
    add_custom_command(
        OUTPUT "${nacp}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${out}"
        COMMAND "${NACPTOOL}" --create "${CWR_SWITCH_APP_NAME}" "${CWR_SWITCH_APP_AUTHOR}"
                "${CWR_SWITCH_APP_VERSION}" "${nacp}" --titleid=${CWR_SWITCH_TITLE_ID}
        VERBATIM)

    set(icon_arg "")
    if(EXISTS "${CWR_SWITCH_ICON}")
        set(icon_arg "--icon=${CWR_SWITCH_ICON}")
    endif()

    add_custom_command(
        OUTPUT "${out}/${target}.nro"
        COMMAND "${ELF2NRO}" "${elf}" "${out}/${target}.nro" ${icon_arg} "--nacp=${nacp}"
        DEPENDS ${target} "${nacp}"
        COMMENT "Packing ${target}.nro"
        VERBATIM)
    add_custom_target(${target}_nro ALL DEPENDS "${out}/${target}.nro")

    # ── NSP (optional, builder-supplied keys) ───────────────────────────────
    if(NOT CWR_SWITCH_KEYS)
        message(STATUS "Switch: CWR_SWITCH_KEYS not set — NSP target disabled (NRO only)")
        return()
    endif()
    if(NOT ELF2NSO OR NOT NPDMTOOL OR NOT HACBREWPACK)
        message(WARNING "Switch: NSP needs elf2nso, npdmtool and hacbrewpack — NSP target disabled")
        return()
    endif()

    # NPDM: start from libnx's default application permissions, set our ID/name.
    file(READ "${DEVKITPRO}/libnx/default.json" npdm_json)
    string(JSON npdm_json SET "${npdm_json}" name "\"${CWR_SWITCH_APP_NAME}\"")
    string(JSON _has_program_id ERROR_VARIABLE _no_program_id GET "${npdm_json}" program_id)
    if(_no_program_id)
        string(JSON npdm_json SET "${npdm_json}" title_id "\"0x${CWR_SWITCH_TITLE_ID}\"")
    else()
        string(JSON npdm_json SET "${npdm_json}" program_id "\"0x${CWR_SWITCH_TITLE_ID}\"")
    endif()
    file(WRITE "${out}/npdm.json" "${npdm_json}")

    set(exefs   "${out}/exefs")
    set(control "${out}/control")
    set(nspdir  "${out}/nsp")

    set(icon_copy "")
    if(EXISTS "${CWR_SWITCH_ICON}")
        set(icon_copy COMMAND ${CMAKE_COMMAND} -E copy "${CWR_SWITCH_ICON}" "${control}/icon_AmericanEnglish.dat")
    endif()

    add_custom_command(
        OUTPUT "${nspdir}/${CWR_SWITCH_TITLE_ID}.nsp"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${exefs}" "${control}" "${nspdir}"
        COMMAND "${ELF2NSO}" "${elf}" "${exefs}/main"
        COMMAND "${NPDMTOOL}" "${out}/npdm.json" "${exefs}/main.npdm"
        COMMAND ${CMAKE_COMMAND} -E copy "${nacp}" "${control}/control.nacp"
        ${icon_copy}
        # TODO(verify): hacbrewpack flag names vary between releases.
        COMMAND "${HACBREWPACK}" -k "${CWR_SWITCH_KEYS}" --titleid ${CWR_SWITCH_TITLE_ID}
                --exefsdir "${exefs}" --controldir "${control}" --noromfs --nologo
                --nspdir "${nspdir}"
        DEPENDS ${target} "${nacp}"
        COMMENT "Packing ${target} NSP (title ${CWR_SWITCH_TITLE_ID})"
        VERBATIM)
    add_custom_target(${target}_nsp DEPENDS "${nspdir}/${CWR_SWITCH_TITLE_ID}.nsp")
endfunction()
