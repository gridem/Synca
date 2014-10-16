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

#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "common.h"

// thread log: outside coro
#define  TLOG(D_msg)             LOG(mt::name() << "#" << mt::number() << ": " << D_msg)
#define RTLOG(D_msg)            RLOG(mt::name() << "#" << mt::number() << ": " << D_msg)

// multithreading
namespace mt {

const char* name();
int number();

std::thread createThread(Handler handler, int number, const char* name = "");

struct IScheduler : IObject
{
    virtual void schedule(Handler handler) = 0;
    virtual const char* name() const { return "<unknown>"; }
};

typedef boost::asio::io_service IoService;
struct IService : IObject
{
    virtual IoService& ioService() = 0;
};

struct ThreadPool : IScheduler, IService
{
    ThreadPool(size_t threadCount, const char* name = "");
    ~ThreadPool();
    
    void schedule(Handler handler);
    void wait();
    const char* name() const;
    
private:
    IoService& ioService();

    const char* tpName;
    std::unique_ptr<boost::asio::io_service::work> work;
    boost::asio::io_service service;
    std::vector<std::thread> threads;
    std::mutex mutex;
    std::condition_variable cond;
    bool toStop = false;
};

}
