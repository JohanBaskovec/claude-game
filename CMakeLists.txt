cmake_minimum_required(VERSION 3.24)
project(FugueInAmadeusMinor LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Type de build" FORCE)
endif()

# --- SDL3 ---------------------------------------------------------------
# On utilise un SDL3 installé sur le système s'il existe, sinon on le
# télécharge et on le compile avec le projet (lien statique).
option(FUGUE_VENDORED_SDL "Toujours compiler SDL3 depuis les sources" OFF)

if(NOT FUGUE_VENDORED_SDL)
    find_package(SDL3 CONFIG QUIET)
endif()

if(NOT SDL3_FOUND)
    message(STATUS "SDL3 introuvable sur le système : téléchargement via FetchContent")
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.4.20
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(SDL3)
endif()

# --- Jeu ----------------------------------------------------------------
add_executable(fugue
    src/main.c
    src/config.c
    src/debug_menu.c
    src/enemy.c
    src/floor.c
    src/game.c
    src/hud.c
    src/input.c
    src/minimap.c
    src/player.c
    src/projectile.c
    src/room.c
)

target_link_libraries(fugue PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_options(fugue PRIVATE /W4)
    # Application fenêtrée sans console en Release.
    set_target_properties(fugue PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
else()
    target_compile_options(fugue PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
    target_link_libraries(fugue PRIVATE m)
endif()

if(EMSCRIPTEN)
    # Build web : produit fugue.html + fugue.js + fugue.wasm.
    # Les assets seront ajoutés plus tard avec --preload-file.
    set_target_properties(fugue PROPERTIES SUFFIX ".html")
endif()
