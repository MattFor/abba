//
// Created by Grzegorz on 8/16/2026.
//

#ifndef ABBA_TESTRUNNER_H
#define ABBA_TESTRUNNER_H

#include <span>
#include <print>
#include <string_view>


struct Test
{
    std::string_view name;
    void (*          function)();
};

inline bool runTests(std::span<const Test> tests, std::size_t& passed, std::size_t& failed, std::size_t& total)
{
    total += tests.size();
    for (const auto& [name, function] : tests)
    {
        try
        {
            function();

            std::print("[PASS] {}\n", name);
            ++passed;
        }
        catch (const std::exception& e)
        {
            std::print("[FAIL] {}\n", e.what());

            ++failed;

            return true;
        }
        catch (...)
        {
            std::print("[FAIL] Unknown exception\n");

            ++failed;

            return true;
        }
    }
    return false;
}

#endif //ABBA_TESTRUNNER_H
