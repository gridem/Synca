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

#include <thread>
#include <atomic>

#include "journey.h"
#include "helpers.h"

namespace synca {

TLS Journey* t_journey = nullptr;

struct JourneyCreateTag;
struct JourneyDestroyTag;

Journey::Journey(mt::IScheduler& s) :
    eventsAllowed(true), sched(&s), indx(++ atomic<JourneyCreateTag>())
{
}

Journey::~Journey()
{
    ++ atomic<JourneyDestroyTag>();
}

void Journey::proceed()
{
    schedule0([this] {
        proceed0();
    });
}

Handler Journey::proceedHandler()
{
    return [this] {
        proceed();
    };
}

void Journey::defer(Handler handler)
{
    handleEvents();
    deferHandler = handler;
    coro::yield();
    handleEvents();
}

void Journey::deferProceed(ProceedHandler proceed)
{
    defer([this, proceed] {
        proceed(proceedHandler());
    });
}

void Journey::teleport(mt::IScheduler& s)
{
    if (&s == sched)
    {
        JLOG("the same destination, skipping teleport <-> " << s.name());
        return;
    }
    JLOG("teleport " << sched->name() << " -> " << s.name());
    sched = &s;
    defer(proceedHandler());
}

void Journey::handleEvents()
{
    if (!eventsAllowed || std::uncaught_exception())
        return;
    auto s = gr.reset();
    if (s == ES_NORMAL)
        return;
    throw EventException(s);
}

void Journey::disableEvents()
{
    handleEvents();
    eventsAllowed = false;
}

void Journey::enableEvents()
{
    eventsAllowed = true;
    handleEvents();
}

mt::IScheduler& Journey::scheduler() const
{
    return *sched;
}

int Journey::index() const
{
    return indx;
}

Goer Journey::goer() const
{
    return gr;
}

Goer Journey::create(Handler handler, mt::IScheduler& s)
{
    return (new Journey(s))->start0(std::move(handler));
}

Goer Journey::start0(Handler handler)
{
    Goer gr = goer();
    schedule0([handler, this] {
        guardedCoro0()->start([handler] {
            JLOG("started");
            try
            {
                handler();
            }
            catch (std::exception& e)
            {
                (void) e;
                JLOG("exception in coro: " << e.what());
            }
            JLOG("ended");
        });
    });
    return gr;
}

void Journey::schedule0(Handler handler)
{
    VERIFY(sched != nullptr, "Scheduler must be set in journey");
    sched->schedule(std::move(handler));
}

Journey::CoroGuard Journey::guardedCoro0()
{
    return CoroGuard(*this);
}

void Journey::proceed0()
{
    guardedCoro0()->resume();
}

void Journey::onEnter0()
{
    t_journey = this;
}

void Journey::onExit0()
{
    if (deferHandler == nullptr)
    {
        delete this;
    }
    else
    {
        Handler handler = std::move(deferHandler);
        handler();
    }
    t_journey = nullptr;
}

Journey& journey()
{
    VERIFY(t_journey != nullptr, "There is no current journey executed");
    return *t_journey;
}

void waitForAll()
{
    TLOG("waiting for journeys to complete");
    WAIT_FOR(atomic<JourneyCreateTag>() == atomic<JourneyDestroyTag>());
    TLOG("waiting for journeys completed");
}

}

GC& gc()
{
    return synca::journey().gc;
}

