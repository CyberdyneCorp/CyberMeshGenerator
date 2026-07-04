# Foundation test suite. Zero external dependencies so the CPU subset builds and
# passes offline (iOS/Android + no-network CI contract). The Catch2 + live TetGen
# oracle scaffold is added on top of this in the tetgen-oracle work when those
# deps are available.
file(GLOB CMG_IO_TESTS CONFIGURE_DEPENDS io/test_*.cpp)
add_executable(cmg_tests
    test_foundation.cpp test_delaunay.cpp test_constrained.cpp test_quality.cpp
    test_sizing.cpp test_region.cpp test_voronoi.cpp test_recovery.cpp
    test_optimize.cpp test_constraints.cpp test_voronoi_io.cpp
    test_cli.cpp test_reconstruct.cpp test_coarsen.cpp test_simplify.cpp
    test_voxelize.cpp
    test_metrics.cpp test_detect.cpp test_backend.cpp
    ${CMG_IO_TESTS} oracle/test_oracle.cpp)
target_link_libraries(cmg_tests PRIVATE cmg::cmg)
target_include_directories(cmg_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# Frozen TetGen oracle golden data (no TetGen build needed at test time).
target_compile_definitions(cmg_tests PRIVATE
    CMG_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/oracle/golden")

add_test(NAME cmg_foundation COMMAND cmg_tests)
