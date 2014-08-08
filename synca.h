// Copyright (c) 2013 Grigory Demchenko

#pragma once

#include <string>
#include <functional>
#include <boost/asio.hpp>
#include "defs.h"
#include "async.h"

namespace io { namespace synca {

void go(Handler);
void dispatch(int threadCount = 1);

struct Acceptor;
struct Socket
{
    friend struct Acceptor;
    
    Socket();
    Socket(Socket&&);
    
    void read(Buffer&);
    void readSome(Buffer&);
    void readUntil(Buffer&, Buffer until);
    void write(const Buffer&);

private:
    async::Socket socket;
};

struct Acceptor
{
    typedef std::function<void(Socket&)> Handler;
    
    explicit Acceptor(int port);
    
    void accept(Socket& socket);
    void goAccept(Handler);

private:
    async::Acceptor acceptor;
};

}}
