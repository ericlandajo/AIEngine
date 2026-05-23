cmake_minimum_required(VERSION 3.15)
project(AIEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include) 
include_directories(src)     
include_directories(schema)  

add_library(AIEngineCore STATIC 
    src/AIEngine.cpp
)

add_library(AIEnginePlugin SHARED 
    src/AIEngineAPI.cpp
)
target_link_libraries(AIEnginePlugin PRIVATE AIEngineCore)

add_executable(AIEngineTest 
    src/main.cpp
)
target_link_libraries(AIEngineTest PRIVATE AIEngineCore)