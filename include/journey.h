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

#include <stdexcept>
#include "coro.h"
#include "mt.h"
#include "goer.h"
#include "core.h"
#include "gc.h"

namespace synca {

struct Journey
{
    ~Journey();
    
    void proceed();
    Handler proceedHandler();
    void defer(Handler handler);
    void deferProceed(ProceedHandler proceed);
    void teleport(mt::IScheduler& s);
    
    void handleEvents();
    void disableEvents();
    void enableEvents();

    mt::IScheduler& scheduler() const;
    int index() const;
    Goer goer() const;

    static Goer create(Handler handler, mt::IScheduler& s);
    
private:
    Journey(mt::IScheduler& s);

    struct CoroGuard
    {
        CoroGuard(Journey& j_) : j(j_)  { j.onEnter0();   }
        ~CoroGuard()                    { j.onExit0();    }
        
        coro::Coro* operator->()        { return &j.coro; }
    private:
        Journey& j;
    };
    
    Goer start0(Handler handler);
    void schedule0(Handler handler);
    CoroGuard guardedCoro0();
    void proceed0();
    void onEnter0();
    void onExit0();
    
    Goer gr;
    bool eventsAllowed;
    mt::IScheduler* sched;
    coro::Coro coro;
    Handler deferHandler;
    int indx;

    friend GC& ::gc();
    GC gc;
};

Journey& journey();

}
