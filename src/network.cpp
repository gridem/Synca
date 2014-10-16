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

#include "network.h"
#include "core.h"

namespace synca { namespace net {

typedef boost::system::error_code Error;
typedef std::function<void(const Error&)> IoHandler;
typedef std::function<void(const Error&, size_t)> BufferIoHandler;
typedef std::function<void(IoHandler)> CallbackIoHandler;

void deferIo(CallbackIoHandler cb)
{
    Error error;
    deferProceed([cb, &error](Handler proceed) {
        cb([proceed, &error](const Error& e) {
            error = e;
            proceed();
        });
    });
    if (!!error)
        throw boost::system::system_error(error, "synca");
}

BufferIoHandler bufferIoHandler(Buffer& buffer, IoHandler proceed)
{
    return [&buffer, proceed](const Error& error, size_t size) {
        if (!error)
            buffer.resize(size);
        proceed(error);
    };
}

BufferIoHandler bufferIoHandler(IoHandler proceed)
{
    return [proceed](const Error& error, size_t size) {
        proceed(error);
    };
}

Socket::Socket() : socket(service<NetworkTag>()) {}

void Socket::read(Buffer& buffer)
{
    deferIo([&buffer, this](IoHandler proceed) {
        boost::asio::async_read(
            socket,
            boost::asio::buffer(&buffer[0], buffer.size()),
            bufferIoHandler(buffer, std::move(proceed))
        ); 
    });
}

void Socket::partialRead(Buffer& buffer)
{
    deferIo([&buffer, this](IoHandler proceed) {
        socket.async_read_some(
            boost::asio::buffer(&buffer[0], buffer.size()),
            bufferIoHandler(buffer, std::move(proceed))
        ); 
    });
}

void Socket::write(const Buffer& buffer)
{
    deferIo([&buffer, this](IoHandler proceed) {
        boost::asio::async_write(
            socket,
            boost::asio::buffer(&buffer[0], buffer.size()),
            bufferIoHandler(std::move(proceed))
        ); 
    });
}

void Socket::connect(const std::string& ip, int port)
{
    deferIo([&ip, port, this](IoHandler proceed) {
        socket.async_connect(
            boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port),
            proceed);
    });
}

void Socket::connect(const EndPoint& e)
{
    deferIo([&e, this](IoHandler proceed) {
        socket.async_connect(e, proceed);
    });
}

void Socket::close()
{
    socket.close();
}

Acceptor::Acceptor(int port) :
    acceptor(service<NetworkTag>(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
}

Socket Acceptor::accept()
{
    Socket socket;
    deferIo([this, &socket](IoHandler proceed) {
        acceptor.async_accept(socket.socket, proceed);
    });
    return socket;
}

void Acceptor::goAccept(SocketHandler handler)
{
    std::unique_ptr<Socket> holder(new Socket(std::move(accept())));
    Socket* socket = holder.get();
    go([socket, handler] {
        std::unique_ptr<Socket> goHolder(socket);
        handler(*goHolder);
    });
    holder.release();
}

Resolver::Resolver() : resolver(service<NetworkTag>()) {}

Resolver::EndPoints Resolver::resolve(const std::string& hostname, int port)
{
    boost::asio::ip::tcp::resolver::query query(hostname, std::to_string(port));
    EndPoints ends;
    deferIo([this, &query, &ends](IoHandler proceed) {
        resolver.async_resolve(query, [proceed, &ends](const Error& e, EndPoints es) {
            if (!e)
                ends = es;
            proceed(e);
        });
    });
    return ends;
}

}}
