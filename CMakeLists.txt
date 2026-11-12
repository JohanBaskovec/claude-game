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
    src/bomb.c
    src/config.c
    src/debug_menu.c
    src/decor.c
    src/draw.c
    src/enemy.c
    src/floor.c
    src/font.c
    src/game.c
    src/hazard.c
    src/hud.c
    src/input.c
    src/item.c
    src/minimap.c
    src/pickup.c
    src/player.c
    src/projectile.c
    src/room.c
    src/sprite.c
    src/text.c
    src/weapon.c
)

target_link_libraries(fugue PRIVATE SDL3::SDL3)

# Assets (police, planches de sprites) copiés à côté de l'exécutable : le jeu les cherche via
# SDL_GetBasePath(). Cible à part, recopiée à chaque build : changer un PNG ne recompile rien.
add_custom_target(fugue_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:fugue>/assets
    COMMENT "Copie des assets")
add_dependencies(fugue_assets fugue)

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
    set_target_properties(fugue PROPERTIES SUFFIX ".html")
    target_link_options(fugue PRIVATE "--preload-file=${CMAKE_SOURCE_DIR}/assets@/assets"
                                      "-sINITIAL_MEMORY=64MB" "-sALLOW_MEMORY_GROWTH=1")
endif()
