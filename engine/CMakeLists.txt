cmake_minimum_required(VERSION 3.20)
project(AIEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Configurant el mode a 'Release' per defecte per a Unity.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

include_directories(include) 
include_directories(src)     
include_directories(schema)  

# =====================================================================
#  NUCLI: biblioteca independent del motor de joc.
#  No conté ni main() ni la capa P/Invoke, de manera que tant el plugin
#  com la suite de tests hi poden enllaçar directament.
# =====================================================================
add_library(AIEngineCore STATIC 
    src/AIEngine.cpp
)

set_target_properties(AIEngineCore PROPERTIES POSITION_INDEPENDENT_CODE ON)

# =====================================================================
#  PLUGIN: la DLL que consumeix Unity (nucli + capa d'interop en C).
# =====================================================================
add_library(AIEnginePlugin SHARED 
    src/AIEngineAPI.cpp
)
target_link_libraries(AIEnginePlugin PRIVATE AIEngineCore)

# Opcional: carpeta Assets/Plugins d'un projecte de Unity on deixar la DLL
# directament en compilar. Exemple:
# cmake -S . -B build -DAIENGINE_UNITY_PLUGINS_DIR="C:/.../Assets/Plugins"
set(AIENGINE_UNITY_PLUGINS_DIR "" CACHE PATH
    "Carpeta Assets/Plugins d'un projecte de Unity on copiar la DLL (opcional)")

if(AIENGINE_UNITY_PLUGINS_DIR)
    set_target_properties(AIEnginePlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${AIENGINE_UNITY_PLUGINS_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${AIENGINE_UNITY_PLUGINS_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${AIENGINE_UNITY_PLUGINS_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${AIENGINE_UNITY_PLUGINS_DIR}"
    )
endif()

# =====================================================================
#  DEMO: executable de consola per a proves manuals (main.cpp).
# =====================================================================
add_executable(AIEngineDemo 
    src/main.cpp
)
target_link_libraries(AIEngineDemo PRIVATE AIEngineCore)

# =====================================================================
#  TESTS: suite automatitzada. Es pot desactivar amb
#  -DAIENGINE_BUILD_TESTS=OFF.
# =====================================================================
option(AIENGINE_BUILD_TESTS "Compilar la suite de tests" ON)

if(AIENGINE_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.5.3
    )
    FetchContent_MakeAvailable(doctest)

    enable_testing()
    add_subdirectory(tests)
endif()