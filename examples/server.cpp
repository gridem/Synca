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
#include "mt.h"
#include "core.h"
#include "helpers.h"

namespace server {

using namespace mt;
using namespace synca;
using namespace synca::net;

void serve(int port)
{
    ThreadPool net(4, "net");
    service<NetworkTag>().attach(net);
    scheduler<DefaultTag>().attach(net);
    go([port] {
        Acceptor acceptor(port);
        while (true)
        {
            JLOG("accepting");
            acceptor.goAccept([](Socket& socket) {
                JLOG("accepted");
                while (true)
                {
                    Buffer sz(1, 0);
                    socket.read(sz);
                    Buffer str(size_t(sz[0]), 0);
                    socket.read(str);
                    JLOG("read: " << str);
                    str += " world!";
                    sz[0] = char(str.size());
                    socket.write(sz);
                    socket.write(str);
                    JLOG("written: " << str);
                }
            });
        }
    }, net);
}

}

int main(int argc, char* argv[])
{
    try
    {
        server::serve(8800);
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
    RLOG("main ended");
    return 0;
}
