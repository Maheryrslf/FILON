cmake_minimum_required(VERSION 3.24)
project(Filon VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(FILON_BUILD_APP   "Construire le jeu (fenêtre raylib)" ON)
option(FILON_BUILD_TESTS "Construire les tests de parité avec la version JS" ON)

# Cœur du jeu : C++ pur, sans dépendance graphique
add_subdirectory(core)

if(FILON_BUILD_APP)
  set(BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND ON  CACHE BOOL "" FORCE)
  set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)
  add_subdirectory(external/raylib EXCLUDE_FROM_ALL)
  add_subdirectory(app)
endif()

if(FILON_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
