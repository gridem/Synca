// Copyright (c) 2013 Grigory Demchenko

#include <algorithm>
#include <thread>
#include "async.h"
#include "sync.h"

namespace io { namespace async {

boost::asio::io_service& service()
{
    return single<boost::asio::io_service>();
}

void go(Handler handler)
{
    LOG("async::go");
    service().post(std::move(handler));
}

void run()
{
    service().run();
}

void dispatch(int threadCount)
{
    int threads = threadCount > 0 ? threadCount : int(std::thread::hardware_concurrency());
    RLOG("Threads: " << threads);
    for (int i = 1; i < threads; ++ i)
        sync::go(run);
    run();
}

Socket::Socket() :
    socket(service())
{
}

Socket::Socket(Socket&& s) :
    socket(std::move(s.socket))
{
}

void Socket::read(Buffer& buffer, IoHandler handler)
{
    boost::asio::async_read(socket, boost::asio::buffer(&buffer[0], buffer.size()),
        [&buffer, handler](const Error& error, std::size_t) {
            handler(error);
    }); 
}

void Socket::readSome(Buffer& buffer, IoHandler handler)
{
    socket.async_read_some(boost::asio::buffer(&buffer[0], buffer.size()),
        [&buffer, handler](const Error& error, std::size_t bytes) {
            buffer.resize(bytes);
            handler(error);
    });
}

bool hasEnd(size_t posEnd, const Buffer& b, const Buffer& end)
{
    return posEnd >= end.size() &&
        b.rfind(end, posEnd - end.size()) != std::string::npos;
}

void Socket::readUntil(Buffer& buffer, Buffer until, IoHandler handler)
{
    VERIFY(buffer.size() >= until.size(), "Buffer size is smaller than expected");
    struct UntilHandler
    {
        UntilHandler(Socket& socket_, Buffer& buffer_, Buffer until_, IoHandler handler_) :
            offset(0),
            socket(socket_),
            buffer(buffer_),
            until(std::move(until_)),
            handler(std::move(handler_))
        {
        }
        
        void read()
        {
            LOG("read at offset: " << offset);
            socket.socket.async_read_some(boost::asio::buffer(&buffer[offset], buffer.size() - offset), *this);
        }
        
        void complete(const Error& error)
        {
            handler(error);
        }
        
        void operator()(const Error& error, std::size_t bytes)
        {
            if (!!error)
            {
                return complete(error);
            }
            offset += bytes;
            VERIFY(offset <= buffer.size(), "Offset outside buffer size");
            LOG("buffer: '" << buffer.substr(0, offset) << "'");
            if (hasEnd(offset, buffer, until))
            {
                // found end
                buffer.resize(offset);
                return complete(error);
            }
            if (offset == buffer.size())
            {
                LOG("not enough size: " << buffer.size());
                buffer.resize(buffer.size() * 2);
            }
            read();
        }
        
    private:
        size_t offset;
        Socket& socket;
        Buffer& buffer;
        Buffer until;
        IoHandler handler;
    };
    UntilHandler(*this, buffer, std::move(until), std::move(handler)).read();

#ifdef ALTERNATIVE_IMPLEMENTATION_USING_PTR
    struct UntilHandler
    {
        UntilHandler(Socket& socket_, Buffer& buffer_, Buffer until_, IoHandler handler_) :
            offset(0),
            socket(socket_),
            buffer(buffer_),
            until(std::move(until_)),
            handler(std::move(handler_))
        {
            read();
        }
        
        void read()
        {
            LOG("read at offset: " << offset);
            socket.socket.async_read_some(boost::asio::buffer(&buffer[offset], buffer.size() - offset),
                [this](const Error& error, std::size_t bytes) {
                    handle(error, bytes);
                }
            );
        }
        
        void complete(const Error& error)
        {
            handler(error);
            delete this;
        }
        
        void handle(const Error& error, std::size_t bytes)
        {
            if (!!error)
            {
                return complete(error);
            }
            offset += bytes;
            VERIFY(offset <= buffer.size(), "Offset outside buffer size");
            LOG("buffer: '" << buffer.substr(0, offset) << "'");
            if (hasEnd(offset, buffer, until))
            {
                // found end
                buffer.resize(offset);
                return complete(error);
            }
            if (offset == buffer.size())
            {
                LOG("not enough size: " << buffer.size());
                buffer.resize(buffer.size() * 2);
            }
            read();
        }
        
    private:
        size_t offset;
        Socket& socket;
        Buffer& buffer;
        Buffer until;
        IoHandler handler;
    };
    new UntilHandler(*this, buffer, std::move(until), std::move(handler));
#endif
}

void Socket::write(const Buffer& buffer, IoHandler handler)
{
    boost::asio::async_write(socket, boost::asio::buffer(&buffer[0], buffer.size()),
        [&buffer, handler](const Error& error, std::size_t) {
            handler(error);
    }); 
}

void Socket::close()
{
    socket.close();
}

Acceptor::Acceptor(int port) :
    acceptor(service(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
}

void Acceptor::accept(Socket& socket, IoHandler handler)
{
    acceptor.async_accept(socket.socket, handler);
}

}}
