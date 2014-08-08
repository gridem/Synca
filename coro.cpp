// Copyright (c) 2013 Grigory Demchenko

#include "coro.h"
#include "defs.h"

namespace coro {

TLS Coro* t_coro;
const size_t STACK_SIZE = 1024*32;

// switch context from coroutine
void yield()
{
    VERIFY(isInsideCoro(), "yield() outside coro");
    t_coro->yield0();
}

// checking that we are inside coroutine
bool isInsideCoro()
{
    return t_coro != nullptr;
}

Coro::Coro()
{
    init0();
}

Coro::Coro(Handler handler)
{
    init0();
    start(std::move(handler));
}

Coro::~Coro()
{
    VERIFY(!isStarted() || std::uncaught_exception(), "Destroying started coro");
}

void Coro::start(Handler handler)
{
    VERIFY(!isStarted(), "Trying to start already started coro");
    context = boost::context::make_fcontext(&stack.back(), stack.size(), &starterWrapper0);
    jump0(reinterpret_cast<intptr_t>(&handler));
}

// continue coroutine execution after yield
void Coro::resume()
{
    VERIFY(started, "Cannot resume: not started");
    VERIFY(!running, "Cannot resume: in running state");
    jump0();
}

// is coroutine was started and not completed
bool Coro::isStarted() const
{
    return started || running;
}

void Coro::init0()
{
    started = false;
    running = false;
    context = nullptr;
    stack.resize(STACK_SIZE);
}

// returns to saved context
void Coro::yield0()
{
    boost::context::jump_fcontext(context, &savedContext, 0);
}

void Coro::jump0(intptr_t p)
{
    Coro* old = this;
    std::swap(old, t_coro);
    running = true;
    boost::context::jump_fcontext(&savedContext, context, p);
    running = false;
    std::swap(old, t_coro);
    if (exc != std::exception_ptr())
        std::rethrow_exception(exc);
}

void Coro::starterWrapper0(intptr_t p)
{
    t_coro->starter0(p);
}

void Coro::starter0(intptr_t p)
{
    started = true;
    try
    {
        Handler handler = std::move(*reinterpret_cast<Handler*>(p));
        handler();
    }
    catch (...)
    {
        exc = std::current_exception();
    }
    started = false;
    yield0();
}

}
