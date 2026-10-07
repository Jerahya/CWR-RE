add_library(SDL3Compat STATIC src/SDL3Compat.cpp)
target_include_directories(SDL3Compat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(SDL3Compat SYSTEM PUBLIC ${DEVKITPRO}/portlibs/switch/include)
target_compile_features(SDL3Compat PUBLIC cxx_std_20)

find_package(SDL2 CONFIG REQUIRED)

target_link_libraries(SDL3Compat PRIVATE SDL2::SDL2-static)

add_library(SDL3::SDL3 ALIAS SDL3Compat)
