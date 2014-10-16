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

// дисковый кеш
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

// кеш в памяти
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
    
    // получение объекта по сети
    std::string get(const std::string& key)
    {
        VERIFY(port > 0, "Port must be assigned");
        net::Socket socket;
        JLOG("connecting");
        socket.connect("127.0.0.1", port);
        // первый байт - размер строки
        Buffer sz(1, char(key.size()));
        socket.write(sz);
        // затем - строка
        socket.write(key);
        // получаем размер результата
        socket.read(sz);
        Buffer val(size_t(sz[0]), 0);
        // получаем сам результат
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
        // эмуляция шедулинга: запуск действий в новом потоке
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
            
            // timeout для всех операций: 1с
            Timeout t(1000);
            std::string val;
            // получить результат из кешей параллельно
            boost::optional<std::string> result = goAnyResult<std::string>({
                [&key] {
                    return portal<DiskCache>()->get(key);
                }, [&key] {
                    return portal<MemCache>()->get(key);
                }
            });
            if (result)
            {
                // результат найден
                val = std::move(*result);
                JLOG("cache val: " << val);
            }
            else
            {
                // кеши не содержат результата
                // получаем объект по сети
                {
                    // таймаут на сетевую обработку: 0.5с
                    Timeout tNet(500);
                    val = portal<Network>()->get(key);
                }
                JLOG("net val: " << val);
                // начиная с этого момента и до конца блока
                // отмена (и таймауты) отключены
                EventsGuard guard;
                // параллельно записываем в оба кеша
                goWait({
                    [&key, &val] {
                        portal<DiskCache>()->set(key, val);
                    }, [&key, &val] {
                        portal<MemCache>()->set(key, val);
                    }
                });
                JLOG("cache updated");
            }
            // переходим в UI и обрабатываем результат
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
    
    // создаем пул потоков для общих действий
    ThreadPool cpu(3, "cpu");
    // создаем пул потоков для сетевых действий
    ThreadPool net(2, "net");
    
    // шедулер для сериализации действий с диском
    Alone diskStorage(cpu, "disk storage");
    // шедулер для сериализации действий с памятью
    Alone memStorage(cpu, "mem storage");
    
    // задание шедулера по умолчанию
    scheduler<DefaultTag>().attach(cpu);
    // привязка сетевого сервиса к сетевому пулу
    service<NetworkTag>().attach(net);
    // привязка обработки таймаутов к общему пулу
    service<TimeoutTag>().attach(cpu);
    
    // привязка дискового портала к дисковому шедулеру
    portal<DiskCache>().attach(diskStorage);
    // привязка портала памяти к шедулеру
    portal<MemCache>().attach(memStorage);
    // привязка сетевого портала к сетевому пулу
    portal<Network>().attach(net);
    
    UI& ui = single<UI>();
    // привязка UI портала к UI шедулеру
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
