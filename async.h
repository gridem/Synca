// Copyright (c) 2013 Grigory Demchenko

#pragma once

#include <boost/asio.hpp>
#include "defs.h"

namespace io { namespace async {

void go(Handler);
void dispatch(int threadCount = 0);

typedef boost::system::error_code Error;
typedef std::function<void(const Error&)> IoHandler;

struct Acceptor;
struct Socket
{
    friend struct Acceptor;
    
    Socket();
    Socket(Socket&&);
    
    void read(Buffer&, IoHandler);
    void readSome(Buffer&, IoHandler);
    void readUntil(Buffer&, Buffer until, IoHandler);
    void write(const Buffer&, IoHandler);
    void close();
    
private:
    boost::asio::ip::tcp::socket socket;
};

struct Acceptor
{
    explicit Acceptor(int port);
    
    void accept(Socket&, IoHandler);

private:
    boost::asio::ip::tcp::acceptor acceptor;
};

}}
