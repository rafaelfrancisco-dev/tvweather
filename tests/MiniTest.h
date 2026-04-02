#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tvtest {

struct Failure : std::exception {};

struct TestCase {
    const char* name;
    void (*func)();
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failureCount()
{
    static int count = 0;
    return count;
}

inline void reportFailure(const char* expr, const char* file, int line)
{
    ++failureCount();
    std::cerr << file << ':' << line << ": check failed: " << expr << '\n';
}

inline void requireFailure(const char* expr, const char* file, int line)
{
    reportFailure(expr, file, line);
    throw Failure {};
}

struct Registrar {
    Registrar(const char* name, void (*func)())
    {
        registry().push_back({name, func});
    }
};

inline int runAllTests()
{
    int failedTests = 0;

    for (const auto& test : registry()) {
        const int failuresBefore = failureCount();
        try {
            test.func();
        } catch (const Failure&) {
        } catch (const std::exception& ex) {
            ++failureCount();
            std::cerr << "Unhandled exception in " << test.name << ": "
                      << ex.what() << '\n';
        } catch (...) {
            ++failureCount();
            std::cerr << "Unhandled non-standard exception in " << test.name << '\n';
        }

        if (failureCount() != failuresBefore)
            ++failedTests;
    }

    if (failedTests == 0) {
        std::cout << "All tests passed (" << registry().size() << " cases)." << '\n';
        return 0;
    }

    std::cerr << failedTests << " test case(s) failed." << '\n';
    return 1;
}

} // namespace tvtest

#define TVTEST_CONCAT_INNER(a, b) a##b
#define TVTEST_CONCAT(a, b) TVTEST_CONCAT_INNER(a, b)

#define TEST_CASE(name) \
    static void TVTEST_CONCAT(tvtest_case_, __LINE__)(); \
    static ::tvtest::Registrar TVTEST_CONCAT(tvtest_registrar_, __LINE__)( \
        name, TVTEST_CONCAT(tvtest_case_, __LINE__)); \
    static void TVTEST_CONCAT(tvtest_case_, __LINE__)()

#define CHECK(expr) \
    do { \
        if (!(expr)) \
            ::tvtest::reportFailure(#expr, __FILE__, __LINE__); \
    } while (false)

#define REQUIRE(expr) \
    do { \
        if (!(expr)) \
            ::tvtest::requireFailure(#expr, __FILE__, __LINE__); \
    } while (false)

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
int main()
{
    return ::tvtest::runAllTests();
}
#endif
