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

#include <atomic>

#include "core.h"
#include "journey.h"
#include "helpers.h"

namespace synca {

typedef boost::system::error_code Error;

int index()
{
    return journey().index();
}

Goer go(Handler handler, mt::IScheduler& scheduler)
{
    return Journey::create(std::move(handler), scheduler);
}

Goer go(Handler handler)
{
    return Journey::create(std::move(handler), scheduler<DefaultTag>());
}

void goN(int n, Handler h)
{
    go(n == 1 ? h : [n, h] {
        for (int i = 0; i < n; ++ i)
            go(h);
    });
}

void teleport(mt::IScheduler& scheduler)
{
    journey().teleport(scheduler);
}

void handleEvents()
{
    journey().handleEvents();
}

void disableEvents()
{
    journey().disableEvents();
}

void enableEvents()
{
    journey().enableEvents();
}

void defer(Handler handler)
{
    journey().defer(handler);
}

void deferProceed(ProceedHandler proceed)
{
    journey().deferProceed(proceed);
}

void goWait(std::initializer_list<Handler> handlers)
{
    deferProceed([&handlers](Handler proceed) {
        std::shared_ptr<void> proceeder(nullptr, [proceed](void*) { proceed(); });
        for (const auto& handler: handlers)
        {
            go([proceeder, &handler] {
                handler();
            });
        }
    });
}

EventsGuard::EventsGuard()
{
    disableEvents();
}

EventsGuard::~EventsGuard()
{
    enableEvents();
}

Waiter::Waiter()
{
    proceed = journey().proceedHandler();
    init0();
}

Waiter::~Waiter()
{
    proceed = nullptr; // to avoid unnecessary proceeding
}

Waiter& Waiter::go(Handler handler)
{
    auto& holder = proceeder;
    synca::go([holder, handler] { handler(); });
    return *this;
}

void Waiter::wait()
{
    if (proceeder.unique())
    {
        JLOG("everything done, nothing to do");
        return;
    }
    defer([this] {
        auto toDestroy = std::move(proceeder);
    });
    init0();
}

void Waiter::init0()
{
    proceeder.reset(this, [](Waiter* w) {
        if (w->proceed != nullptr)
        {
            TLOG("wait completed, proceeding");
            w->proceed();
        }
    });
}

size_t goAnyWait(std::initializer_list<Handler> handlers)
{
    VERIFY(handlers.size() >= 1, "Handlers amount must be positive");

    size_t index = static_cast<size_t>(-1);
    deferProceed([&handlers, &index](Handler proceed) {
        std::shared_ptr<Atomic<int>> counter = std::make_shared<Atomic<int>>();
        size_t i = 0;
        for (const auto& handler: handlers)
        {
            go([counter, proceed, &handler, i, &index] {
                handler();
                if (++ *counter == 1)
                {
                    index = i;
                    proceed();
                }
            });
            ++ i;
        }
    });
    VERIFY(index < handlers.size(), "Incorrect index returned");
    return index;
}

Alone::Alone(mt::IService& service, const char* name) :
    strand(service.ioService()), strandName(name)
{
}

void Alone::schedule(Handler handler)
{
    strand.post(std::move(handler));
}

const char* Alone::name() const
{
    return strandName;
}

Timeout::Timeout(int ms) :
    timer(service<TimeoutTag>(), boost::posix_time::milliseconds(ms))
{
    Goer goer = journey().goer();
    timer.async_wait([goer](const Error& error) mutable {
        if (!error)
            goer.timedout();
    });
}

Timeout::~Timeout()
{
    timer.cancel_one();
    handleEvents();
}

void Service::attach(mt::IService& s)
{
    service = &s.ioService();
}

void Service::detach()
{
    service = nullptr;
}

Service::operator mt::IoService&() const
{
    VERIFY(service != nullptr, "Service is not attached");
    return *service;
}

Scheduler::Scheduler() : scheduler(nullptr)
{
}

void Scheduler::attach(mt::IScheduler& s)
{
    scheduler = &s;
}

void Scheduler::detach()
{
    scheduler = nullptr;
}

Scheduler::operator mt::IScheduler&() const
{
    VERIFY(scheduler != nullptr, "Scheduler is not attached");
    return *scheduler;
}

}
