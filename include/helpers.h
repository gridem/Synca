/*
 * Copyright 2014 Grigory Demchenko (aka gridem)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <iostream>
#include <stdexcept>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "common.h"

// Internal macro
#define SLOG__(D_msg)               std::cerr << boost::posix_time::microsec_clock::local_time() << ": " << D_msg << std::endl

// Release log
#ifdef flagLOG_MUTEX
#   include <mutex>
#   define RLOG(D_msg)              do { std::lock_guard<std::mutex> _(single<std::mutex>()); SLOG__(D_msg); } while(false)
#else
#   define RLOG                     SLOG__
#endif

// Null log
#define NLOG(D_msg)

#ifdef flagLOG_DEBUG
#   define LOG                      RLOG
#else
#   define LOG                      NLOG
#endif
#define DUMP(D_value)               LOG(#D_value " = " << D_value)
#define RAISE(D_str)                throw std::runtime_error(D_str)
#define VERIFY(D_cond, D_str)       if (!(D_cond)) RAISE("Verification failed: " #D_cond ": " D_str)

#ifdef flagMSC
#   define TLS                      __declspec(thread)
#else
#   define TLS                      __thread
#endif

#define WAIT_FOR(D_condition)       while (!(D_condition)) std::this_thread::yield()

inline void sleepFor(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
