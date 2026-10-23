cmake_minimum_required(VERSION 3.13)
project(dmd_rpi CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_options(-Wall -Wextra)

# Pilote des dalles : rpi-rgb-led-matrix (hzeller, GPL 2.0+), compile a part sur le Pi (make -C lib).
option(WITH_HZELLER "Sortie sur les dalles via rpi-rgb-led-matrix" ON)
set(RGBMATRIX_DIR "$ENV{HOME}/rpi-rgb-led-matrix" CACHE PATH "Dossier de rpi-rgb-led-matrix")

# --- affichage ---------------------------------------------------------------
add_library(dmd_display STATIC
  src/display/display.cpp
  src/display/backend_png.cpp)
target_include_directories(dmd_display PUBLIC src/display)

# --- coeur (config, caches, medias, moteur) ------------------------------------
add_library(dmd_core STATIC
  src/core/sdfs.cpp
  src/core/config.cpp
  src/core/engine.cpp
  src/media/syscache.cpp
  src/media/gamescache.cpp
  src/media/pack.cpp
  src/media/stdgif.cpp
  src/media/pngdecode.cpp
  src/media/playlist.cpp
  src/media/themelogos.cpp
  src/ui/score.cpp
  src/ui/bootanim.cpp
  src/ui/clock.cpp
  src/ui/widgets.cpp
  src/net/udpserver.cpp
  src/net/zedmd.cpp
  src/net/webserver.cpp
  src/net/netutil.cpp
  src/net/wifimanager.cpp
  src/hw/button.cpp
  src/web/webconfig.cpp
  src/web/webmedia.cpp)
target_include_directories(dmd_core PUBLIC src/core src/media src/net src/ui src/web src/hw)
# clock.cpp compile, sans modifier le depot, des fichiers du firmware (clock_themes.h dans le dossier parent) : couche compat/
# (Arduino.h, ESP32-HUB75-MatrixPanel-I2S-DMA.h) visible seulement par ce fichier.
#
# Copie CORRIGEE de clock_themes.h (generee a chaque configuration, jamais commitee) : le generateur declare
# `static uint16_t rowbuf[16]` pour des sprites de 17 pixels de large (Mario : run0, run1, jump) -> memcpy et lecture d'UNE case
# apres la fin du tableau. Sur l'ESP32 le depassement est sans consequence visible ; sur le Pi, GCC le traite comme un
# comportement indefini (-Waggressive-loop-optimizations). La source de verite reste le fichier du firmware.
set(CT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../clock_themes.h")
set(CT_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(READ "${CT_SRC}" CT_TEXT)
string(REPLACE "static uint16_t rowbuf[16];" "static uint16_t rowbuf[32];" CT_TEXT "${CT_TEXT}")
set(CT_OLD "")
if(EXISTS "${CT_GEN_DIR}/clock_themes.h")
  file(READ "${CT_GEN_DIR}/clock_themes.h" CT_OLD)
endif()
if(NOT "${CT_OLD}" STREQUAL "${CT_TEXT}")
  file(WRITE "${CT_GEN_DIR}/clock_themes.h" "${CT_TEXT}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CT_SRC}")
set_source_files_properties(src/ui/clock.cpp src/ui/widgets.cpp PROPERTIES INCLUDE_DIRECTORIES
  "${CMAKE_CURRENT_SOURCE_DIR}/compat;${CT_GEN_DIR};${CMAKE_CURRENT_SOURCE_DIR}/src/core;${CMAKE_CURRENT_SOURCE_DIR}/src/display;${CMAKE_CURRENT_SOURCE_DIR}/src/ui;${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(dmd_core PUBLIC dmd_display gif png z cpp-httplib pthread atomic)  # atomic : std::atomic<uint64_t> sur ARM 32 bits

# --- outils ------------------------------------------------------------------
add_executable(dmd_show tools/dmd_show.cpp)
target_link_libraries(dmd_show dmd_display)

add_executable(dmd_play tools/dmd_play.cpp)
target_link_libraries(dmd_play dmd_core)

add_executable(dmd_daemon src/main.cpp)
target_link_libraries(dmd_daemon dmd_core)

if(WITH_HZELLER)
  add_library(dmd_hzeller STATIC src/display/backend_hzeller.cpp src/display/sched_tune.cpp)
  target_include_directories(dmd_hzeller PUBLIC src/display src/core)
  # SYSTEM : masque les avertissements des en-tetes de hzeller, qui ne sont pas les notres
  target_include_directories(dmd_hzeller SYSTEM PUBLIC ${RGBMATRIX_DIR}/include)
  target_link_libraries(dmd_hzeller PUBLIC dmd_display
    ${RGBMATRIX_DIR}/lib/librgbmatrix.a rt m pthread)
  foreach(t dmd_show dmd_play dmd_daemon)
    target_compile_definitions(${t} PRIVATE WITH_HZELLER)
    target_link_libraries(${t} dmd_hzeller)
  endforeach()
endif()
