// Copyright (c) 2013 Grigory Demchenko

#include <string>
#include <sstream>
#include "defs.h"
#include "coro.h"
#include "sync.h"
#include "async.h"
#include "synca.h"

#define HTTP_DELIM          "\r\n"
#define HTTP_DELIM_BODY     HTTP_DELIM HTTP_DELIM

namespace coro {

int s;

void coro1()
{
    CLOG("CORO: " << ++s);
    yield();
    CLOG("CORO: yielded");
}

void test1()
{
    CLOG("before");
    Coro c(coro1);
    CLOG("after coro1");
    c.resume();
    CLOG("after resume");
    c.start(coro1);
    CLOG("started again");
    c.resume();
    CLOG("after resume");
}

void coro2()
{
    yield();
    throw std::runtime_error("coro2: generated exception");
}

void test2()
{
    Coro c(coro2);
    c.resume();
    c.resume();
}

void coro3()
{
    std::cout << '2';
    yield();
    std::cout << '4';
}

void test3()
{
    std::cout << '1';
    Coro c(coro3);
    std::cout << '3';
    c.resume();
    std::cout << '5';
}

}

Buffer httpContent(const Buffer& body)
{
    std::ostringstream o;
    o << "HTTP/1.1 200 Ok" HTTP_DELIM
        "Content-Type: text/html" HTTP_DELIM
        "Content-Length: " << body.size() << HTTP_DELIM_BODY
        << body;
    return o.str();
}

namespace io {

namespace sync {

void test1()
{
    Acceptor acceptor(8800);
    LOG("accepting");
    while (true)
    {
        Socket socket;
        acceptor.accept(socket);
        try
        {
            LOG("accepted");
            Buffer buffer(4000, 0);
            socket.readUntil(buffer, HTTP_DELIM_BODY);
            socket.write(httpContent("<h1>Hello sync singlethread!</h1>"));
            socket.close();
        }
        catch (std::exception& e)
        {
            (void) e;
            LOG("error: " << e.what());
        }
    }
}

void test2()
{
    Acceptor acceptor(8800);
    LOG("accepting");
    while (true)
    {
        Socket* toAccept = new Socket;
        acceptor.accept(*toAccept);
        LOG("accepted");
        go([toAccept] {
            try
            {
                Socket socket = std::move(*toAccept);
                delete toAccept;
                Buffer buffer;
                while (true)
                {
                    buffer.resize(4000);
                    socket.readUntil(buffer, HTTP_DELIM_BODY);
                    socket.write(httpContent("<h1>Hello sync multithread!</h1>"));
                }
            }
            catch (std::exception& e)
            {
                (void) e;
                LOG("error: " << e.what());
            }
        });
    }
}

}

namespace async {

void threads(int threads)
{
    Acceptor acceptor(8800);
    LOG("accepting");
    Handler accepting = [&acceptor, &accepting] {
        struct Connection
        {
            Buffer buffer;
            Socket socket;
            
            void handling()
            {
                buffer.resize(4000);
                socket.readUntil(buffer, HTTP_DELIM_BODY, [this](const Error& error) {
                    if (!!error)
                    {
                        LOG("error on reading: " << error.message());
                        delete this;
                        return;
                    }
                    LOG("read");
                    buffer = httpContent("<h1>Hello async!</h1>");
                    socket.write(buffer, [this](const Error& error) {
                        if (!!error)
                        {
                            LOG("error on writing: " << error.message());
                            delete this;
                            return;
                        }
                        LOG("written");
                        handling();
                    });
                });
            }
        };
        
        Connection* conn = new Connection;
        acceptor.accept(conn->socket, [conn, &accepting](const Error& error) {
            if (!!error)
            {
                LOG("error on accepting: " << error.message());
                delete conn;
                return;
            }
            LOG("accepted");
            conn->handling();
            accepting();
        });
    };
    
    accepting();
    dispatch(threads);
}

void test1()
{
    threads(1);
}

void test2()
{
    threads(16);
}

void test3()
{
    threads(0);
}

}

namespace synca {

void test0()
{
    Acceptor acceptor(8800);
    LOG("accepting");
    go([&acceptor] {
        while (true)
        {
            Socket* toAccept = new Socket;
            acceptor.accept(*toAccept);
            LOG("accepted");
            go([toAccept] {
                try
                {
                    Socket socket = std::move(*toAccept);
                    delete toAccept;
                    Buffer buffer;
                    while (true)
                    {
                        buffer.resize(4000);
                        socket.readUntil(buffer, HTTP_DELIM_BODY);
                        socket.write(httpContent("<h1>Hello synca!</h1>"));
                    }
                }
                catch (std::exception& e)
                {
                    (void) e;
                    LOG("error: " << e.what());
                }
            });
        }
    });
    dispatch(1);
}

void threads(int threads)
{
    Acceptor acceptor(8800);
    LOG("accepting");
    go([&acceptor] {
        while (true)
        {
            acceptor.goAccept([](Socket& socket) {
                try
                {
                    Buffer buffer;
                    while (true)
                    {
                        buffer.resize(4000);
                        socket.readUntil(buffer, HTTP_DELIM_BODY);
                        socket.write(httpContent("<h1>Hello synca!</h1>"));
                    }
                }
                catch (std::exception& e)
                {
                    (void) e;
                    LOG("error: " << e.what());
                }
            });
        }
    });
    dispatch(threads);
}

void test1()
{
    threads(1);
}

void test2()
{
    threads(16);
}

void test3()
{
    threads(0);
}

}}

int main(int argc, char* argv[])
{
    using namespace io;
    try
    {
        VERIFY(argc == 2, "Usage: <test name>");
        std::string name = argv[1];
        RLOG("Starting test: " << name);
        #define TEST(D_name) if (name == #D_name) D_name(); else
        TEST(coro::test1)
        TEST(coro::test2)
        TEST(coro::test3)
        TEST(sync::test1)
        TEST(sync::test2)
        TEST(async::test1)
        TEST(async::test2)
        TEST(async::test3)
        TEST(synca::test0)
        TEST(synca::test1)
        TEST(synca::test2)
        TEST(synca::test3)
        RAISE("Unknown test name: " + name);
        #undef TEST
    }
    catch (std::exception& e)
    {
        RLOG("Error: " << e.what());
        return 1;
    }
    catch (...)
    {
        RLOG("Unknown error");
        return 2;
    }
    return 0;
}
