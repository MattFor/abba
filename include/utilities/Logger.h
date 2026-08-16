//
// Created by mattfor on 8/16/26.
//

#ifndef ABBA_LOGS_H
#define ABBA_LOGS_H

#include <print>
#include <chrono>

#ifndef LOG
#define LOG 0
#endif

/**
 * Standard way to log with a timestamp.
 *
 * @tparam Args Anything to log.
 * @param text Main log string.
 * @param args Arguments for formatted text.
 */
template <typename... Args>
void p(std::format_string<Args...> text, Args&&... args)
{
    if constexpr (LOG)
    {
        std::print("[{:%Y-%m-%d %H:%M:%S}] {}\n", std::chrono::system_clock::now(), std::format(text, std::forward<Args>(args)...));
    }
}

#endif //ABBA_LOGS_H
