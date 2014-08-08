Synca
=====

# Description
The code shows how to use coroutines to use asynchronous code in synchronous manner. It significanlty simplifies the logic and allows to concentrate on the task itself with performance degradation. So it includes the advantages of both asynchronous and synchronous approaches:

1. Asynchronous advantage: performance.
2. Synchronous advantage: simplicity.

The name 'synca' was taken using the following conversion: a+sync -> sync+a.

For more information you may read the article: [Asynchronous: back to the future](http://habrahabr.ru/post/201826/) (in Russian).

# Requirements

* Compiler: GCC: -std=c++11, Visual Studio: version >= 11.0 (Visual C++ 2012)
* Libraries: boost_system and boost_context, boost version >= 1.55