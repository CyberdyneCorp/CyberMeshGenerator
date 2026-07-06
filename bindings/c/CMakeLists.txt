# Stable C ABI shim — the foundation both the Python and Swift bindings sit on.
# Built when either binding is requested (or CMG_BUILD_C_ABI is forced on).
add_library(cmg_c src/cmg_c.cpp)
add_library(cmg::c ALIAS cmg_c)

target_include_directories(cmg_c PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(cmg_c PUBLIC cmg::cmg)
set_target_properties(cmg_c PROPERTIES C_VISIBILITY_PRESET default)

# Stamp a semver-tracked SONAME on the shared C ABI so downstream consumers can
# pin against a compatible ABI (SOVERSION = major). No-op for a static build.
set_target_properties(cmg_c PROPERTIES
    EXPORT_NAME c
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

if(CMG_BUILD_TESTS)
    add_executable(cmg_c_smoke test/smoke.c)
    target_link_libraries(cmg_c_smoke PRIVATE cmg::c)
    add_test(NAME cmg_c_abi COMMAND cmg_c_smoke)
endif()
