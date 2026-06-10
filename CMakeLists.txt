cmake_minimum_required(VERSION 3.15)
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

add_library(AIEngineCore STATIC 
    src/AIEngine.cpp
)

set_target_properties(AIEngineCore PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(AIEnginePlugin SHARED 
    src/AIEngineAPI.cpp
)
target_link_libraries(AIEnginePlugin PRIVATE AIEngineCore)

set_target_properties(AIEnginePlugin PROPERTIES 
     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../AIEngineDemo/AI Engine Demo/Assets/Plugins"
     RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../AIEngineDemo/AI Engine Demo/Assets/Plugins"
 )

add_executable(AIEngineTest 
    src/main.cpp
)
target_link_libraries(AIEngineTest PRIVATE AIEngineCore)