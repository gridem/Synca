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

#include "core.h"
#include "portal.h"
#include "helpers.h"
#include "gc.h"

namespace test {

using namespace mt;
using namespace synca;

void teleport1()
{
    ThreadPool tp1(1, "tp1");
    ThreadPool tp2(1, "tp2");
    scheduler<DefaultTag>().attach(tp1);

    go([&tp2] {
        JLOG("TELEPORT 1");
        teleport(tp2);
        JLOG("TELEPORT 2");
    });
    waitForAll();
}

void teleport2()
{
    ThreadPool tp1(3, "tp1");
    ThreadPool tp2(2, "tp2");
    scheduler<DefaultTag>().attach(tp1);

    go([&tp1, &tp2] {
        JLOG("TELEPORT 1");
        teleport(tp2);
        JLOG("TELEPORT 2");
        teleport(tp1);
        JLOG("TELEPORT 3");
        teleport(tp1);
        JLOG("TELEPORT 4");
    });
    waitForAll();
}

void teleport_exception1()
{
    ThreadPool tp1(1, "tp1");
    ThreadPool tp2(1, "tp2");
    scheduler<DefaultTag>().attach(tp1);

    go([&tp1, &tp2] {
        while (true)
        {
            try
            {
                throw std::runtime_error("Test");
            }
            catch (std::exception& e)
            {
                (void) e;
                JLOG("Exception: " << e.what());
                teleport(tp2);
            }
            teleport(tp1);
        }
    });
    waitForAll();
}

void wait1()
{
    ThreadPool tp(3, "wait");
    scheduler<DefaultTag>().attach(tp);
    go([] {
        JLOG("+");
        goWait({
            [] {
                JLOG("+300");
                sleepFor(300);
                JLOG("-300");
            }, [] {
                JLOG("+500");
                sleepFor(500);
                JLOG("-500");
            }
        });
        JLOG("-");
    });
    waitForAll();
}

void wait2()
{
    ThreadPool tp(3, "wait");
    scheduler<DefaultTag>().attach(tp);
    go([] {
        Waiter w;
        w.wait();
        w.go([] {
            JLOG("+300");
            sleepFor(300);
            JLOG("-300");
        }).go([] {
            JLOG("+500");
            sleepFor(500);
            JLOG("-500");
        });
        JLOG("before wait");
        w.wait();
        JLOG("after wait");
        w.wait();
    });
    waitForAll();
}

void waitAny1()
{
    ThreadPool tp(3, "any");
    scheduler<DefaultTag>().attach(tp);
    go([] {
        size_t i = goAnyWait({
            [] {
                sleepFor(500);
            }, [] {
                sleepFor(100);
            }
        });
        JLOG("index: " << i);
    });
    waitForAll();
}

void resultAny1()
{
    ThreadPool tp(3, "result");
    scheduler<DefaultTag>().attach(tp);
    go([] {
        boost::optional<int> i = goAnyResult<int>({
            [] {
                sleepFor(500);
                return boost::optional<int>(500);
            }, [] {
                sleepFor(100);
                return boost::optional<int>(100);
            }
        });
        JLOG("value: " << *i);
    });
    waitForAll();
}

void resultAny2()
{
    ThreadPool tp(1, "result");
    scheduler<DefaultTag>().attach(tp);
    go([] {
        boost::optional<int> i = goAnyResult<int>({
            [] {
                sleepFor(500);
                return 500;
            }, [] {
                sleepFor(100);
                return 100;
            }
        });
        JLOG("value: " << *i);
    });
    waitForAll();
}

void resultAny3()
{
    ThreadPool tp(3, "result");
    scheduler<DefaultTag>().attach(tp);
    go([] {
        boost::optional<int> i = goAnyResult<int>({
            [] {
                sleepFor(100);
                return boost::optional<int>();
            }, [] {
                sleepFor(500);
                return 500;
            }
        });
        JLOG("value: " << *i);
    });
    waitForAll();
}

void resultAny4()
{
    ThreadPool tp(3, "result");
    scheduler<DefaultTag>().attach(tp);
    go([] {
        boost::optional<int> i = goAnyResult<int>({
            [] {
                return boost::optional<int>();
            }, [] {
                return boost::optional<int>();
            }
        });
        JLOG("has value: " << !!i);
    });
    waitForAll();
}

void alone1()
{
    ThreadPool tp(3, "tp");
    Alone a(tp);
    go([&a] {
        JLOG("1");
        go([] {
            JLOG("A1");
            sleepFor(1000);
            JLOG("A2");
        }, a);
        JLOG("2");
        go([] {
            JLOG("B1");
            sleepFor(1000);
            JLOG("B2");
        }, a);
        JLOG("3");
        teleport(a);
        JLOG("4");
    }, tp);
    waitForAll();
}

void timeout1()
{
    ThreadPool tp(3, "tp");
    service<TimeoutTag>().attach(tp);
    go([] {
        Timeout t(1000);
        handleEvents();
        JLOG("before sleep");
        sleepFor(200);
        JLOG("after sleep");
        handleEvents();
        JLOG("after handle events");
    }, tp);
    go([] {
        Timeout t(100);
        handleEvents();
        JLOG("before sleep");
        sleepFor(200);
        JLOG("after sleep");
        handleEvents();
        JLOG("after handle events");
    }, tp);
}

void timeout2()
{
    ThreadPool tp(3, "tp");
    service<TimeoutTag>().attach(tp);
    go([] {
        Timeout t(100);
        handleEvents();
        JLOG("before sleep");
        sleepFor(200);
        JLOG("after sleep");
        throw std::runtime_error("MY EXC");
        handleEvents();
        JLOG("after handle events");
    }, tp);
}

void portal1()
{
    ThreadPool tp1(1, "tp1");
    ThreadPool tp2(1, "tp2");
    
    struct X
    {
        void op() {}
    };
    
    portal<X>().attach(tp2);
    go([] {
        portal<X>()->op();
    }, tp1);
    waitForAll();
}

void portal2()
{
    ThreadPool tp1(1, "tp1");
    ThreadPool tp2(1, "tp2");
    
    go([&tp2] {
        Portal p(tp2);
        JLOG("throwing exception");
        throw std::runtime_error("exception occur");
    }, tp1);
    waitForAll();
}

void gc1()
{
    struct A   { ~A() { TLOG("~A"); } };
    struct B:A { ~B() { TLOG("~B"); } };
    struct C   { ~C() { TLOG("~C"); } };
    
    ThreadPool tp(1, "tp");
    go([] {
        A* a = gcnew<B>();
        C* c = gcnew<C>();
    }, tp);
}

void tp1()
{
    ThreadPool tp(3, "tp");
    scheduler<DefaultTag>().attach(tp);
    TLOG("0");
    tp.wait();
    TLOG("1");
    go([] {
        TLOG("go 1");
        sleepFor(500);
        TLOG("go 2");
    });
    TLOG("2");
    tp.wait();
    TLOG("3");
    tp.wait();
    TLOG("4");
    go([] {
        TLOG("go 3");
        sleepFor(500);
        TLOG("go 4");
    });
    TLOG("5");
    tp.wait();
    TLOG("6");
    tp.wait();
    TLOG("7");
}

}
