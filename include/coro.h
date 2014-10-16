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

#include <vector>
#include <boost/context/all.hpp>

#include "common.h"

namespace coro {

// switch context from coroutine
void yield();

// checking that we are inside coroutine
bool isInsideCoro();

// сопрограмма
struct Coro
{
    friend void yield();
    
    Coro();
    
    // create and start coroutine
    Coro(Handler);
    
    ~Coro();
    
    // start coroutine using handler
    void start(Handler);

    // continue coroutine execution after yield
    void resume();
    
    // is coroutine was started and not completed
    bool isStarted() const;

private:
    void init0();
    void yield0();
    void jump0(intptr_t p = 0);
    static void starterWrapper0(intptr_t p);
    void starter0(intptr_t p);

    bool started;
    bool running;

    boost::context::fcontext_t context;
    boost::context::fcontext_t savedContext;
    std::vector<unsigned char> stack;
    std::exception_ptr exc;
};

}
