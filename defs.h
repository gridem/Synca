// Copyright (c) 2013 Grigory Demchenko

#pragma once

#include <iostream>
#include <stdexcept>
#include <functional>
#include <string>

// Release log
#define RLOG(D_msg)                 std::cerr << D_msg << std::endl

// Null log
#define NLOG(D_msg)

#ifdef _DEBUG
#   define LOG                      RLOG
#else
#   define LOG                      NLOG
#endif
#define DUMP(D_value)               LOG(#D_value " = " << D_value)
#define RAISE(D_str)                throw std::runtime_error(D_str)
#define VERIFY(D_cond, D_str)       if (!(D_cond)) RAISE("Verification failed: " #D_cond ": " D_str)

#ifdef _WIN32
#   define TLS                      __declspec(thread)
#else
#   define TLS                      __thread
#endif

typedef std::function<void ()> Handler;
typedef std::string Buffer;

template<typename T>
T& single()
{
    static T t;
    return t;
}
