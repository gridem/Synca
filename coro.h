// Copyright (c) 2013 Grigory Demchenko

#pragma once

#include <functional>
#include <vector>
#include <boost/context/all.hpp>
#include "defs.h"

#define CLOG(D_msg)                 LOG(coro::isInsideCoro() << ": " << D_msg)

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

    boost::context::fcontext_t* context;
    boost::context::fcontext_t savedContext;
    std::vector<unsigned char> stack;
    std::exception_ptr exc;
};

}
