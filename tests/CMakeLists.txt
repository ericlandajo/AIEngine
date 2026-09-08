add_executable(AIEngineTests
    test_smoke.cpp
    test_mocks.cpp
    test_decorators.cpp
    test_parallel.cpp
    # test_conditions.cpp
    # test_fsm.cpp
    # test_loading.cpp
)

target_link_libraries(AIEngineTests PRIVATE
    AIEngineCore
    doctest::doctest
)

target_include_directories(AIEngineTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(AIEngineTests PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(AIEngineTests)