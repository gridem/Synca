Synca
=====

# Description

Synca is a small but efficient library to perform asynchronous operations in synchronous manner (async -> synca). This significantly simplifies the writing of network applications or other nontrivial concurrent algorithms. This library demonstrates how using the coroutines allows to achieve described simplifications. The code itself looks like synchronous invocations while internally it uses asynchronous scheduling.

## Features

1. Thread Pools. You can use different pools for different purposes.
2. Asynchronous mutexes aka Alone. So called non-blocking mutexes.
3. Teleports and Portals. To transfer execution context across different thread pools.
4. Basic network support.
5. Different waiting primitives to implement scatter-gather algorithms.
6. Non-blocking asynchronous channels.
7. Other interesting things.

Some of the ideas was inspired by Go language.

For more information you may read the articles:

1. [Asynchronous: back to the future](http://habrahabr.ru/post/201826/) (in Russian).
2. [Asynchronous 2: teleportation through the portals](http://habrahabr.ru/company/yandex/blog/240525/) (in Russian).

# Requirements

* Supported compilers (must support c++11):
    * GCC
    * Clang
    * MSVC (2013 CTP1)
* Libraries: BOOST, version >= 1.56

# Library Documentation

This section provides the description of synca API.

## Multi-threaded Functionality

The multi-threaded layer describes the functionality operating with different threads and scheduling.

### Scheduler

Scheduler is responsible to schedule handlers for execution. Scheduler interface:

``` cpp
typedef std::function<void ()> Handler;

struct IScheduler : IObject
{
    virtual void schedule(Handler handler) = 0;
    virtual const char* name() const { return "<unknown>"; }
};
```

`ThreadPool` class implements this interface:

``` cpp
struct ThreadPool : IScheduler
{
    ThreadPool(size_t threadCount, const char* name = "");
    ~ThreadPool();
    
    void schedule(Handler handler);
    void wait();
    const char* name() const;
};
```

* `ThreadPool constructor`: creates thread pool using specified number of threads `threadCount` with corresponding `name`. `name` is intended for logging output only.
* `schedule`: schedules `handler` in the available thread inside thread pool.
* `wait`: blocks until all handlers complete their execution inside all threads.

## Basic Functionality

### go

Executes specified handler asynchronously inside newly created coroutine using default scheduler. See later how to assign default scheduler.

**Example**

``` cpp
go([] {
    std::cout << "Hello world!" << std::endl;
});
```

### go Through Particular Scheduler

Executes specified handler asynchronously inside newly created coroutine though particular scheduler. Scheduler must implement `IScheduler` interface.

**Example**

``` cpp
ThreadPool tp("tp");
go([] {
    std::cout << "Hello world!" << std::endl;
}, tp);
```

## Waiting Functions

### goWait

Executes specified list of handlers asynchronously inside newly created coroutines using default scheduler and waits until all handlers complete.

**Example: Fibonacci numbers**

``` cpp
int fibo(int v)
{
    if (v < 2)
        return v;
    int v1, v2;
    goWait({
        [v, &v1] { v1 = fibo (v-1); },
        [v, &v2] { v2 = fibo (v-2); }
    });
    return v1 + v2;
}
```

### goAnyWait

Executes specified list of handlers asynchronously inside newly created coroutines using default scheduler and waits until at least one handler completes. Returns index of the triggered handler (started from 0)

**Example**

``` cpp
go([] {
    size_t i = goAnyWait({
        [] {
            sleepFor(500);
        }, [] {
            sleepFor(100);
        }
    });
    // outputs: 'index: 1'
    std::cout << "index: " << i << std::endl;
});
```

### goAnyResult

Executes specified list of handlers asynchronously inside newly created coroutines using default scheduler and waits until at least one handler returns nonempty result. Returns the result of this handler. If all handlers don't return nonempty result then the returned result is empty. Handlers must use the following syntax:

``` cpp
boost::optional<ResultType> handler()
{
    // presudocode
    if (hasResult)
        return result;
    else
        return {};
}
```

**Example**

``` cpp
boost::optional<std::string> result = goAnyResult<std::string>({
    [&key] {
        return portal<DiskCache>()->get(key);
    }, [&key] {
        return portal<MemCache>()->get(key);
    }
});
if (result)
    processReturnedKey(*result);
```

## Networking Support

Library provides basic networking support. All operations in this section are asynchronous and don't block the thread.

### Socket

Provides asynchronous socket operations in synchronous manner.

``` cpp
struct Socket
{
    typedef /* unspecified type */ EndPoint;
    
    Socket();
    void read(Buffer&);
    void partialRead(Buffer&);
    void write(const Buffer&);
    void connect(const std::string& ip, int port);
    void connect(const EndPoint& e);
    void close();
};
```

* `read` - reads data from socket using buffer size.
* `partialRead` - reads available data from socket using buffer size. The amount of buffer data will less or equal to the initial buffer size.
* `write` - writes the whole buffer data to the socket.
* `connect` - connects to the server using specified ip, port or endpoint from `Resolver`.
* `close` - closes the socket and terminates current executed operations.

### Acceptor

Accepts the connects from the clients.

``` cpp
typedef std::function<void(Socket&)> SocketHandler;
struct Acceptor
{
    explicit Acceptor(int port);

    Socket accept();
    void goAccept(SocketHandler);
};
```

* `Acceptor::Acceptor` - listens the connects on specified port.
* `accept` - waits until client connects to the acceptor port.
* `goAccept` - syntax sugar to execute accepted client socket in new coroutine using `go`.

### Resolver

Resolves DNS name.

``` cpp
struct Resolver
{
    Resolver();
    typedef /* unspecified type */ EndPoints;

    EndPoints resolve(const std::string& hostname, int port);
};
```

* `resolve` - resolves the hostname and returns the `Endpoint` iterator: `Endpoints`.

## Interactions With Different Schedulers

This section provides the description of entities interacting with schedulers. This allows to decouple the entire system and provides non-blocking synchronization.

### Teleports

Switches the coroutine from one thread pool to another.

**Example**

``` cpp
ThreadPool tp1(1, "tp1");
ThreadPool tp2(1, "tp2");

go([&tp2] {
    JLOG("TELEPORT 1");
    teleport(tp2);
    JLOG("TELEPORT 2");
}, tp1);
waitForAll();
```

### Portals

Teleports to destination thread pool and automatically teleports back on scope exit. Uses RAII idiom.

**Example 1**

Portals with exception on-the-fly

``` cpp
ThreadPool tp1(1, "tp1");
ThreadPool tp2(1, "tp2");

go([&tp2] {
    Portal p(tp2);
    JLOG("throwing exception");
    throw std::runtime_error("exception occur");
}, tp1);
```

In this example the coroutine started in `tp1`, then switches to `tp2` during the Portal creation, and throws the exception. Due to stack unwinding portal teleports from `tp2` to `tp1` while thrown exception is in progress.

**Example 2**

Portals as an attached class method invocation.

``` cpp
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
```

In this example class `X` attached to `tp2` using the portal's functionality. On `op` method invocation coroutine automagically teleports to `tp2`. When method `op` ends, portal switches back to `tp1`, automagically as well.

### Alone

Alone is a non-blocking mutex.

**Example**

Executes sequentially several coroutines through `go`

``` cpp
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
```

## External Events Handling

The library supports 2 types of external events handling:
1. Timeouts. You may specify the timeout for the scoped set of operations.
2. Cancels. User may cancel the coroutine at any time.

These events handle at the time of context switches i.e. on any asynchronous operation. To provide more responsiveness user may add `handleEvents()` function call to check for external event existent. If there is such event the function `handleEvents` throws appropriate exception (or any asynchronous function call including networking and waiting functionality). The exception can be handled later using `try`/`catch` statements.

### Timeouts Handling

Allows to handle nested timeouts.

**Example**

``` cpp
ThreadPool tp(3, "tp");
// need to attach to this service
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
```

### Cancellation Handling

The user may cancel the coroutine at any time.

**Example**

``` cpp
Goer op = go(myMegaHandler);
// …
If (weDontNeedMegaHandlerAnymore)
    op.cancel();
```

## Miscellaneous

### Default Scheduler

To specify default scheduler you should use the following:

``` cpp
ThreadPool tp(3, "tp");
scheduler<DefaultTag>().attach(tp);
```

So the following lines become equivalent:

``` cpp
go(handler); // uses attached default ThreadPool
go(handler, tp);
```

### Network Thread Pool

To deal with the networking you must attach corresponding service via tag `NetworkTag`:

``` cpp
ThreadPool commonThreadPool(10, "cpu");
ThreadPool networkThreadPool(1, "net");
service<NetworkTag>().attach(networkThreadPool);
scheduler<DefaultTag>().attach(commonThreadPool);
go([] {
    Socket socket; // uses attached network service
    socket.connect("1.2.3.4", 1234);
    Buffer buf = "hello world";
    socket.write(buf);
}); // uses attached default scheduler
```

### Timeout Thread Pool

To deal with the timeout functionality you must attach corresponding service via tag `TimeoutTag`:

``` cpp
ThreadPool tp(3, "tp");
service<TimeoutTag>().attach(tp);
scheduler<DefaultTag>().attach(tp);
go([] {
    Timeout t(100); // uses attached timeout service
    handleEvents();
    JLOG("before sleep");
    sleepFor(200);
    JLOG("after sleep");
    throw std::runtime_error("MY EXC");
    handleEvents();
    JLOG("after handle events");
}); // uses attached default scheduler
```

## Simple Garbage Collector

Here is a simple garbage collector. Is collects only local allocations inside the coroutine.

**Example**

``` cpp
struct A   { ~A() { TLOG("~A"); } };
struct B:A { ~B() { TLOG("~B"); } };
struct C   { ~C() { TLOG("~C"); } };

ThreadPool tp(1, "tp");
go([] {
    A* a = gcnew<B>();
    C* c = gcnew<C>();
}, tp);
```
Outputs:
```
tp#1: ~C
tp#1: ~B
tp#1: ~A
```

Please note that `B` doesn't contain the virtual destructor!
