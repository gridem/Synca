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

#include <boost/asio.hpp>

#include "common.h"
#include "mt.h"

namespace synca {

struct NetworkTag;

namespace net {

struct Acceptor;
struct Socket
{
    friend struct Acceptor;
    typedef boost::asio::ip::tcp::endpoint EndPoint;
    
    Socket();
    void read(Buffer&);
    void partialRead(Buffer&);
    void write(const Buffer&);
    void connect(const std::string& ip, int port);
    void connect(const EndPoint& e);
    void close();

private:
    boost::asio::ip::tcp::socket socket;
};

typedef std::function<void(Socket&)> SocketHandler;
struct Acceptor
{
    explicit Acceptor(int port);

    Socket accept();
    void goAccept(SocketHandler);

private:
    boost::asio::ip::tcp::acceptor acceptor;
};

struct Resolver
{
    Resolver();
    typedef boost::asio::ip::tcp::resolver::iterator EndPoints;

    EndPoints resolve(const std::string& hostname, int port);
private:
    boost::asio::ip::tcp::resolver resolver;
};

}}
