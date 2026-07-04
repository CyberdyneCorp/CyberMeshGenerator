# Foundation test suite. Zero external dependencies so the CPU subset builds and
# passes offline (iOS/Android + no-network CI contract). The Catch2 + live TetGen
# oracle scaffold is added on top of this in the tetgen-oracle work when those
# deps are available.
add_executable(cmg_tests test_foundation.cpp)
target_link_libraries(cmg_tests PRIVATE cmg::cmg)
target_include_directories(cmg_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME cmg_foundation COMMAND cmg_tests)
