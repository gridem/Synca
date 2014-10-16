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

#include "mt.h"
#include "helpers.h"

// ThreadPool log: inside ThreadPool functionality
#define PLOG(D_msg)             TLOG("@" << this->name() << ": " << D_msg)

namespace mt {

TLS int t_number = 0;
TLS const char* t_name = "main";

const char* name()
{
    return t_name;
}

int number()
{
    return t_number;
}

std::thread createThread(Handler handler, int number, const char* name)
{
    return std::thread([handler, number, name] {
        t_number = number + 1;
        t_name = name;
        try
        {
            TLOG("thread created");
            handler();
            TLOG("thread ended");
        }
        catch (std::exception& e)
        {
            (void) e;
            TLOG("thread ended with error: " << e.what());
        }
    });
}

ThreadPool::ThreadPool(size_t threadCount, const char* name) : tpName(name)
{
    work.reset(new boost::asio::io_service::work(service));
    threads.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++ i)
        threads.emplace_back(createThread([this] {
            while (true)
            {
                service.run();
                std::unique_lock<std::mutex> lock(mutex);
                if (toStop)
                    break;
                if (!work)
                {
                    work.reset(new boost::asio::io_service::work(service));
                    service.reset();
                    lock.unlock();
                    cond.notify_all();
                }
            }
        }, i, tpName));
    PLOG("thread pool created with threads: " << threadCount);
}

ThreadPool::~ThreadPool()
{
    mutex.lock();
    toStop = true;
    work.reset();
    mutex.unlock();
    PLOG("stopping thread pool");
    for (size_t i = 0; i < threads.size(); ++ i)
        threads[i].join();
    PLOG("thread pool stopped");
}

void ThreadPool::schedule(Handler handler)
{
    service.post(std::move(handler));
}

void ThreadPool::wait()
{
    std::unique_lock<std::mutex> lock(mutex);
    work.reset();
    while (true)
    {
        cond.wait(lock);
        TLOG("WAIT: waitCompleted: " << (work != nullptr));
        if (work)
            break;
    }
}

const char* ThreadPool::name() const
{
    return tpName;
}

IoService& ThreadPool::ioService()
{
    return service;
}

}
