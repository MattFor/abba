//
// Created by Grzegorz on 8/16/2026.
//

#ifndef ABBA_TESTRUNNER_H
#define ABBA_TESTRUNNER_H

#include <span>
#include <string_view>

#include "utilities/Logger.h"

struct Test
{
    std::string_view name;
    void (*          function)();
};

inline bool runTests(std::span<const Test> tests, std::size_t& passed, std::size_t& failed, std::size_t& total, std::size_t& current)
{
    total += tests.size();

    int i = 0;
    for (const auto& [name, function] : tests)
    {
        p("{}|{}. Testing {}", ++current, ++i, name);

        try
        {
            function();

            p("[PASS] {}\n\n", name);
            ++passed;
        }
        catch (const std::exception& e)
        {
            p("[FAIL] {}\n\n", e.what());

            ++failed;

            return true;
        }
        catch (...)
        {
            p("[FAIL] Unknown exception\n\n");

            ++failed;

            return true;
        }
    }
    return false;
}

#endif //ABBA_TESTRUNNER_H
