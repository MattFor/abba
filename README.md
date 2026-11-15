# ABBA

[![CI](https://github.com/MattFor/abba/actions/workflows/ci.yml/badge.svg)](https://github.com/MattFor/abba/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Arbitrary Binary Behavioural Anticheat written in C++.

## Requirements

- CMake 4.2+
- A C++26 compiler with `libstdc++exp`, e.g. GCC 14

## Building

```sh
git clone --recurse-submodules https://github.com/MattFor/abba
cd abba
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target abba
```

## License

MIT, see [LICENSE](LICENSE). By [MattFor](https://github.com/MattFor) and [Gekimaru](https://github.com/Gekimaru).
