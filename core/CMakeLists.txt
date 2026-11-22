add_library(filon-core STATIC
  src/random.cpp
  src/world.cpp
  src/terrain.cpp
  src/texture.cpp
  src/physics.cpp
  src/drill.cpp
  src/nuggets.cpp
  src/economy.cpp
  src/json.cpp
  src/save.cpp
  src/creatures.cpp
)
target_include_directories(filon-core PUBLIC include)

if(MSVC)
  target_compile_options(filon-core PRIVATE /W4 /fp:precise)
else()
  # pas de contraction en FMA : les calculs doivent donner exactement les mêmes doubles que la version JS
  target_compile_options(filon-core PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
endif()
