// Copyright (c) 2013 Grigory Demchenko

#include <algorithm>
#include <thread>
#include "sync.h"

namespace io { namespace sync {

void go(Handler handler)
{
    LOG("sync::go");
    std::thread([handler] {
        try
        {
            LOG("new thread had been created");
            handler();
            LOG("thread was ended successfully");
        }
        catch (std::exception& e)
        {
            (void) e;
            LOG("thread was ended with error: " << e.what());
        }
    }).detach();
}

boost::asio::io_service& service()
{
    return single<boost::asio::io_service>();
}

Socket::Socket() :
    socket(service())
{
}

Socket::Socket(Socket&& s) :
    socket(std::move(s.socket))
{
}

void Socket::read(Buffer& buffer)
{
    boost::asio::read(socket, boost::asio::buffer(&buffer[0], buffer.size()));
}

void Socket::readSome(Buffer& buffer)
{
    buffer.resize(socket.read_some(boost::asio::buffer(&buffer[0], buffer.size())));
}

bool hasEnd(size_t posEnd, const Buffer& b, const Buffer& end)
{
    return posEnd >= end.size() &&
        b.rfind(end, posEnd - end.size()) != std::string::npos;
}

int Socket::readUntil(Buffer& buffer, const Buffer& until)
{
    size_t offset = 0;
    while (true)
    {
        size_t bytes = socket.read_some(boost::asio::buffer(&buffer[offset], buffer.size() - offset));
        offset += bytes;
        if (hasEnd(offset, buffer, until))
        {
            buffer.resize(offset);
            return offset;
        }
        if (offset == buffer.size())
        {
            LOG("not enough size: " << buffer.size());
            buffer.resize(buffer.size() * 2);
        }
    }
}

void Socket::write(const Buffer& buffer)
{
    boost::asio::write(socket, boost::asio::buffer(&buffer[0], buffer.size()));
}

void Socket::close()
{
    socket.close();
}

Acceptor::Acceptor(int port) :
    acceptor(service(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
}

void Acceptor::accept(Socket& socket)
{
    acceptor.accept(socket.socket);
}

}}
