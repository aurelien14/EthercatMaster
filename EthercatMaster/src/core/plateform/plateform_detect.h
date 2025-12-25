#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
/*
#elif defined(__linux__)
#define PLATFORM_LINUX
#elif defined(__APPLE__)
#define PLATFORM_MACOS
*/
#else
#error "Unsupported platform"
#endif