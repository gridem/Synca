// Copyright (c) 2013 Grigory Demchenko

#pragma once

#include <string>
#include <boost/asio.hpp>
#include "defs.h"

namespace io { namespace sync {

void go(Handler);

// forward declaration
struct Acceptor;
struct Socket
{
    friend struct Acceptor;
    
    Socket();
    Socket(Socket&& s);
    
    // read the data of fixed size
    void read(Buffer&);
    
    // partial read the data not more than buffer size
    void readSome(Buffer&);
    
    // read until the string 'until'
    int readUntil(Buffer&, const Buffer& until);
    
    // write the data with fixed buffer size
    void write(const Buffer&);
    
    // close the socket
    void close();

private:
    boost::asio::ip::tcp::socket socket;
};

struct Acceptor
{
    // listen port to accept the client connections
    explicit Acceptor(int port);
    
    // initializes socket on new connection
    void accept(Socket& socket);

private:
    boost::asio::ip::tcp::acceptor acceptor;
};

}}
