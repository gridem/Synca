Synca
=====

# Description

Synca is a small but efficient library to perform asynchronous operations in synchronous manner (async -> synca). This significantly simplifies the writing of network applications or other nontrivial concurrent algorithms. This library demonstrates how using the coroutines allows to achieve described simplifications. The code itself looks like synchronous invocations while internally it uses asynchronous scheduling.

## Features ##
1. Thread Pools. You can use different pools for different purposes.
2. Asynchronous mutexes aka Alone. So called nonblocking mutexes.
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

* Supported compilers:
    * GCC
    * Clang
    * MSVC
* Libraries: boost version >= 1.56