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

#include <unordered_map>

#include "core.h"
#include "network.h"
#include "portal.h"
#include "helpers.h"

namespace client {

using namespace mt;
using namespace synca;

struct DiskCache
{
    boost::optional<std::string> get(const std::string& key)
    {
        JLOG("get: " << key);
        return boost::optional<std::string>();
    }
    
    void set(const std::string& key, const std::string& val)
    {
        JLOG("set: " << key << ";" << val);
    }
};

struct MemCache
{
    boost::optional<std::string> get(const std::string& key)
    {
        auto it = map.find(key);
        return it == map.end() ? boost::optional<std::string>() : boost::optional<std::string>(it->second);
    }
    
    void set(const std::string& key, const std::string& val)
    {
        map[key] = val;
    }
    
private:
    std::unordered_map<std::string, std::string> map;
};

struct Network
{
    Network() : port(-1) {}
    
    // gets the object from network
    std::string get(const std::string& key)
    {
        VERIFY(port > 0, "Port must be assigned");
        net::Socket socket;
        JLOG("connecting");
        socket.connect("127.0.0.1", port);
        // first byte is the string size
        Buffer sz(1, char(key.size()));
        socket.write(sz);
        // then the string itself
        socket.write(key);
        // gets the result size
        socket.read(sz);
        Buffer val(size_t(sz[0]), 0);
        // gets the actual result
        socket.read(val);
        JLOG("val received");
        return val;
    }
    
    int port;
};

struct UI : IScheduler
{
    void schedule(Handler handler)
    {
        // scheduler emulation: executes handler in separate thread
        struct UIScheduleTag;
        createThread(handler, atomic<UIScheduleTag>() ++, name()).detach();
    }

    const char* name() const
    {
        return "ui";
    }

    Goer handleKey(const std::string& key)
    {
        return go([key] {
            // to implement wait correctly
            ++ atomic<UI>();
            
            // timeout for all operations: 1s
            Timeout t(1000);
            std::string val;
            // gets the results from caches parallel
            boost::optional<std::string> result = goAnyResult<std::string>({
                [&key] {
                    return portal<DiskCache>()->get(key);
                }, [&key] {
                    return portal<MemCache>()->get(key);
                }
            });
            if (result)
            {
                // the result has been found
                val = std::move(*result);
                JLOG("cache val: " << val);
            }
            else
            {
                // caches don't contain the result
                // loading object from network
                {
                    // network timeout: 0.5s
                    Timeout tNet(500);
                    val = portal<Network>()->get(key);
                }
                JLOG("net val: " << val);
                // starting from this point
                // external events are disabled
                EventsGuard guard;
                // writing the data parallel
                goWait({
                    [&key, &val] {
                        portal<DiskCache>()->set(key, val);
                    }, [&key, &val] {
                        portal<MemCache>()->set(key, val);
                    }
                });
                JLOG("cache updated");
            }
            // switches to UI thread to handle it
            portal<UI>()->handleResult(key, val);
        });
    }
    
    void handleResult(const std::string& key, const std::string& val)
    {
        TLOG("UI result: " << key << ";" << val);
        // TODO: add some actions
    }
    
    void wait()
    {
        WAIT_FOR(atomic<UI>() != 0);
        waitForAll();
        atomic<UI>() = 0;
    }
    
    void performHandleKey(const std::string& key)
    {
        schedule([this, key] {
            handleKey(key);
        });
        wait();
    }
    
    void performHandleKeyCancel(const std::string& key)
    {
        schedule([this, key] {
            Goer gr = handleKey(key);
            gr.cancel();
        });
        wait();
    }
};

void perform(int port)
{
    single<Network>().port = port;
    
    // creates thread pool for common tasks
    ThreadPool cpu(3, "cpu");
    // creates thread pool for network operations
    ThreadPool net(2, "net");
    
    // scheduler to serialize disk actions
    Alone diskStorage(cpu, "disk storage");
    // scheduler to serialize memory actions
    Alone memStorage(cpu, "mem storage");
    
    // sets the default scheduler
    scheduler<DefaultTag>().attach(cpu);
    // attaches network service to network thread pool
    service<NetworkTag>().attach(net);
    // attaches timeout service to common thread pool
    service<TimeoutTag>().attach(cpu);
    
    // attaches disk cache portal to disk scheduler
    portal<DiskCache>().attach(diskStorage);
    // attaches memory cache portal to memory scheduler
    portal<MemCache>().attach(memStorage);
    // attaches network portal to network scheduler
    portal<Network>().attach(net);
    
    UI& ui = single<UI>();
    // attaches UI portal to UI scheduler
    portal<UI>().attach(ui);
    
    ui.performHandleKey("Hello");
    ui.performHandleKey("Hello");
    ui.performHandleKeyCancel("Hello");
}

}

int main(int argc, char* argv[])
{
    try
    {
        client::perform(8800);
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
