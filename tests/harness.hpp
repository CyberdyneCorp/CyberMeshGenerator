// CyberMeshGenerator — minimal zero-dependency test harness.
//
// The foundation ships a tiny self-contained runner so the CPU test subset builds
// and passes offline with no external test framework (important for the
// iOS/Android and no-network CI contract). The richer Catch2 + live TetGen oracle
// scaffold layers on top of this and is enabled where those deps are present.
#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace cmgtest {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> fn) {
        registry().push_back({std::move(name), std::move(fn)});
    }
};

struct Failure {
    std::string message;
};

inline void check(bool cond, const std::string& expr) {
    if (!cond) throw Failure{"check failed: " + expr};
}

inline int run_all() {
    int failed = 0;
    for (const auto& c : registry()) {
        try {
            c.fn();
            std::printf("  ✓ %s\n", c.name.c_str());
        } catch (const Failure& f) {
            std::printf("  ✗ %s — %s\n", c.name.c_str(),
                        f.message.c_str());
            ++failed;
        } catch (const std::exception& e) {
            std::printf("  ✗ %s — unexpected exception: %s\n",
                        c.name.c_str(), e.what());
            ++failed;
        }
    }
    std::printf("\n%zu tests, %d failed\n", registry().size(), failed);
    return failed == 0 ? 0 : 1;
}

} // namespace cmgtest

#define CMG_CONCAT_(a, b) a##b
#define CMG_CONCAT(a, b) CMG_CONCAT_(a, b)
#define CMG_TEST(NAME)                                                          \
    static void CMG_CONCAT(cmg_test_fn_, __LINE__)();                          \
    static ::cmgtest::Registrar CMG_CONCAT(cmg_test_reg_, __LINE__)(           \
        NAME, &CMG_CONCAT(cmg_test_fn_, __LINE__));                            \
    static void CMG_CONCAT(cmg_test_fn_, __LINE__)()
#define CMG_CHECK(COND) ::cmgtest::check((COND), #COND)
