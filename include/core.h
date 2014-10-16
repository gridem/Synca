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

#include <boost/optional.hpp>
#include <atomic>

#include "mt.h"
#include "goer.h"

#define  JLOG(D_msg)             TLOG("[" << synca::index() << "] " << D_msg)
#define RJLOG(D_msg)            RTLOG("[" << synca::index() << "] " << D_msg)

namespace synca {

typedef std::function<void(Handler)> ProceedHandler;

int index();
Goer go(Handler handler, mt::IScheduler& scheduler);
Goer go(Handler handler);
void goN(int n, Handler handler);

void teleport(mt::IScheduler& scheduler);
void handleEvents();
void disableEvents();
void enableEvents();
void waitForAll();
void defer(Handler handler);
void deferProceed(ProceedHandler proceed);
void goWait(std::initializer_list<Handler> handlers);

struct EventsGuard
{
    EventsGuard();
    ~EventsGuard();
};

struct Waiter
{
    Waiter();
    ~Waiter();
    
    Waiter& go(Handler h);
    void wait();
    
private:
    void init0();
    
    Handler proceed;
    std::shared_ptr<Waiter> proceeder;
};

size_t goAnyWait(std::initializer_list<Handler> handlers);

template<typename T_result>
boost::optional<T_result> goAnyResult(std::initializer_list<std::function<boost::optional<T_result>()>> handlers)
{
    typedef boost::optional<T_result> Result;
    typedef std::function<void(Result&&)> ResultHandler;
    
    struct Counter
    {
        Counter(ResultHandler proceed_) : proceed(std::move(proceed_)) {}
        ~Counter()
        {
            tryProceed(Result());
        }
        
        void tryProceed(Result&& result)
        {
            if (++ counter == 1)
                proceed(std::move(result));
        }
        
    private:
        Atomic<int> counter;
        ResultHandler proceed;
    };

    Result result;
    deferProceed([&handlers, &result](Handler proceed) {
        std::shared_ptr<Counter> counter = std::make_shared<Counter>(
        [&result, proceed](Result&& res) {
            result = std::move(res);
            proceed();
        });
        for (const auto& handler: handlers)
        {
            go([counter, &handler] {
                Result result = handler();
                if (result)
                    counter->tryProceed(std::move(result));
            });
        }
    });
    return result;
}

struct Alone : mt::IScheduler
{
    Alone(mt::IService& service, const char* name = "alone");

    void schedule(Handler handler);
    const char* name() const;

private:
    boost::asio::io_service::strand strand;
    const char* strandName;
};

struct TimeoutTag;
struct Timeout
{
    Timeout(int ms);
    ~Timeout();
    
private:
    boost::asio::deadline_timer timer;
};

struct Service
{
    Service() : service(nullptr) {}
    
    void attach(mt::IService&);
    void detach();
    
    operator mt::IoService&() const;
    
private:
    mt::IoService* service;
};

template<typename T_tag>
Service& service()
{
    return single<Service, T_tag>();
}

struct Scheduler
{
    Scheduler();
    
    void attach(mt::IScheduler& s);
    void detach();

    operator mt::IScheduler&() const;

private:
    mt::IScheduler* scheduler;
};

struct DefaultTag;

template<typename T_tag>
Scheduler& scheduler()
{
    return single<Scheduler, T_tag>();
}

}
