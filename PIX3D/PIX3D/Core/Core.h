#pragma once
#include <iostream>
#include <Utils/rang.h>
#include <format>

#if defined(_WIN32) || defined(_WIN64)
#ifdef PIX_DEBUG
#define PIX_ASSERT(cond) if(cond){} else { std::cout << "Assertion Failed!\n"; __debugbreak(); }
#define PIX_ASSERT_MSG(cond, msg) if(cond){} else { std::cout << msg << "\n"; __debugbreak(); }

// Macros without formatting
#define PIX_DEBUG_INFO(msg) \
        do { \
            std::cout << rang::fg::blue << rang::style::bold << "INFO: " \
                      << msg << rang::style::reset << std::endl; \
        } while(0)

#define PIX_DEBUG_WARNING(msg) \
        do { \
            std::cout << rang::fg::yellow << rang::style::bold << "WARNING: " \
                      << msg << rang::style::reset << std::endl; \
        } while(0)

#define PIX_DEBUG_ERROR(msg) \
        do { \
            std::cout << rang::fg::red << rang::style::bold << "ERROR: " \
                      << msg << rang::style::reset << std::endl; \
        } while(0)

#define PIX_DEBUG_SUCCESS(msg) \
        do { \
            std::cout << rang::fg::green << rang::style::bold << "SUCCESS: " \
                      << msg << rang::style::reset << std::endl; \
        } while(0)

// Macros with `std::format`
#define PIX_DEBUG_INFO_FORMAT(fmt, ...) \
        do { \
            std::cout << rang::fg::green << rang::style::bold << "INFO: " \
                      << std::format(fmt, __VA_ARGS__) << rang::style::reset << std::endl; \
        } while(0)

#define PIX_DEBUG_WARNING_FORMAT(fmt, ...) \
        do { \
            std::cout << rang::fg::yellow << rang::style::bold << "WARNING: " \
                      << std::format(fmt, __VA_ARGS__) << rang::style::reset << std::endl; \
        } while(0)

#define PIX_DEBUG_ERROR_FORMAT(fmt, ...) \
        do { \
            std::cout << rang::fg::red << rang::style::bold << "ERROR: " \
                      << std::format(fmt, __VA_ARGS__) << rang::style::reset << std::endl; \
        } while(0)

#define PIX_DEBUG_SUCCESS_FORMAT(fmt, ...) \
        do { \
            std::cout << rang::fg::green << rang::style::bold << "SUCCESS: " \
                      << std::format(fmt, __VA_ARGS__) << rang::style::reset << std::endl; \
        } while(0)

#elif PIX_RELEASE
#define PIX_ASSERT(cond)
#define PIX_ASSERT_MSG(cond, msg)
#define PIX_DEBUG_INFO(msg)
#define PIX_DEBUG_WARNING(msg)
#define PIX_DEBUG_ERROR(msg)
#define PIX_DEBUG_SUCCESS(msg)
#define PIX_DEBUG_INFO_FORMAT(fmt, ...)
#define PIX_DEBUG_WARNING_FORMAT(fmt, ...)
#define PIX_DEBUG_ERROR_FORMAT(fmt, ...)
#define PIX_DEBUG_SUCCESS_FORMAT(fmt, ...)
#endif
#else
#error This Engine Is Currently Supports Windows Platform Only!
#endif
