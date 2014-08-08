// Copyright (c) 2013 Grigory Demchenko

#include <algorithm>
#include "synca.h"
#include "coro.h"

namespace io { namespace synca {

typedef async::Error Error;
typedef std::function<void(coro::Coro*)> CoroHandler;

TLS CoroHandler* t_deferHandler;
TLS const Error* t_error;

void handleError()
{
    if (t_error)
        throw boost::system::system_error(*t_error, "synca");
}

void defer(CoroHandler handler)
{
    VERIFY(coro::isInsideCoro(), "defer() outside coro");
    VERIFY(t_deferHandler == nullptr, "There is unexecuted defer handler");
    t_deferHandler = &handler;
    coro::yield();
    handleError();
}

void onCoroComplete(coro::Coro* coro)
{
    VERIFY(!coro::isInsideCoro(), "Complete inside coro");
    VERIFY(coro->isStarted() == (t_deferHandler != nullptr), "Unexpected condition in defer/started state");
    if (t_deferHandler != nullptr)
    {
        LOG("invoking defer handler");
        (*t_deferHandler)(coro);
        t_deferHandler = nullptr;
        LOG("completed defer handler");
    }
    else
    {
        LOG("nothing to do, deleting coro");
        delete coro;
    }
}

void go(Handler handler)
{
    LOG("synca::go");
    async::go([handler] {
        coro::Coro* coro = new coro::Coro(std::move(handler));
        onCoroComplete(coro);
    });
}

void dispatch(int threadCount)
{
    async::dispatch(threadCount);
}

void onComplete(coro::Coro* coro, const Error& error)
{
    LOG("async completed, coro: " << coro << ", error: " << error.message());
    VERIFY(coro != nullptr, "Coro is null");
    VERIFY(!coro::isInsideCoro(), "Completion inside coro");
    t_error = error ? &error : nullptr;
    coro->resume();
    LOG("after resume");
    onCoroComplete(coro);
}

async::IoHandler onCompleteHandler(coro::Coro* coro)
{
    return [coro](const Error& error) {
        onComplete(coro, error);
    };
}

async::IoHandler onCompleteGoHandler(coro::Coro* coro, Handler handler)
{
    return [coro, handler](const Error& error) {
        if (!error)
            go(std::move(handler));
        onComplete(coro, error);
    };
}

Socket::Socket()
{
}

Socket::Socket(Socket&& socket_) :
    socket(std::move(socket_.socket))
{
}

void Socket::readSome(Buffer& buffer)
{
    VERIFY(coro::isInsideCoro(), "readSome must be called inside coro");
    defer([this, &buffer](coro::Coro* coro) {
        VERIFY(!coro::isInsideCoro(), "readSome completion must be called outside coro");
        socket.readSome(buffer, onCompleteHandler(coro));
        LOG("readSome scheduled");
    });
}

void Socket::readUntil(Buffer& buffer, Buffer until)
{
    VERIFY(coro::isInsideCoro(), "readUntil must be called inside coro");
    defer([this, &buffer, until](coro::Coro* coro) {
        VERIFY(!coro::isInsideCoro(), "readUntil completion must be called outside coro");
        socket.readUntil(buffer, std::move(until), onCompleteHandler(coro));
        LOG("readUntil scheduled");
    });
}

void Socket::write(const Buffer& buffer)
{
    VERIFY(coro::isInsideCoro(), "write must be called inside coro");
    defer([this, &buffer](coro::Coro* coro) {
        VERIFY(!coro::isInsideCoro(), "write completion must be called outside coro");
        socket.write(buffer, onCompleteHandler(coro));
        LOG("write scheduled");
    });
}

Acceptor::Acceptor(int port) :
    acceptor(port)
{
}

void Acceptor::accept(Socket& socket)
{
    VERIFY(coro::isInsideCoro(), "accept must be called inside coro");
    defer([this, &socket](coro::Coro* coro) {
        VERIFY(!coro::isInsideCoro(), "accept completion must be called outside coro");
        acceptor.accept(socket.socket, onCompleteHandler(coro));
        LOG("accept scheduled");
    });
}

void Acceptor::goAccept(Handler handler)
{
    VERIFY(coro::isInsideCoro(), "goAccept must be called inside coro");
    defer([this, handler](coro::Coro* coro) {
        VERIFY(!coro::isInsideCoro(), "goAccept completion must be called outside coro");
        Socket* socket = new Socket;
        acceptor.accept(socket->socket, onCompleteGoHandler(coro, [socket, handler] {
            Socket s = std::move(*socket);
            delete socket;
            handler(s);
        }));
        LOG("accept scheduled");
    });
}

}}
