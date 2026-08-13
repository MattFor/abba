#include <print>
#include <chrono>

#define LOG 1

void p(const std::string& text)
{
    if constexpr (LOG)
    {
        const auto now = std::chrono::system_clock::now();
        std::print("[{:%Y-%m-%d %H:%M:%S}] {}\n", now, text);
    }
}

int main()
{
    p("test");

    // 1. Load settings

    // 2. Attach to binary

    // 3. Create scanner loops
}
