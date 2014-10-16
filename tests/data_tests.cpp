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

#include "data.h"
#include "channel.h"
#include "mt.h"
#include "helpers.h"

namespace data {

using namespace synca;
using namespace synca::data;
using namespace mt;

typedef std::string Str;
typedef std::pair<Str, Str> StrPair;
typedef Channel<Str> ChanStr;
typedef Channel<StrPair> ChanStrPair;
typedef Channel<int> ChanInt;

template<typename T>
Str toStr(const T& t)
{
    return Str(t.first, t.second);
}

bool isEmpty(const Str& s)
{
    return s.empty();
}

bool isEmpty(const StrPair& s)
{
    return isEmpty(s.first);
}

void pipe1()
{
    ThreadPool tp(3, "tp");
    scheduler<DefaultTag>().attach(tp);
    Channel<int> c;
    Channel<int> aggr;
    c.put(5);
    go([&] {
        for (int v: c)
        {
            aggr.put(v - 1);
            aggr.put(v - 2);
        }
        aggr.close();
    });
    std::vector<int> vs;
    go([&] {
        for (int v: aggr)
        {
            vs.push_back(v);
            if (v > 0)
                c.put(v);
        }
        c.close();
    });
    closeAndWait(tp, c);
    for (int v: vs)
    {
        TLOG("v: " << v);
    }
}

void pipe2()
{
    ThreadPool tp(3, "tp");
    scheduler<DefaultTag>().attach(tp);
    Channel<int> c;
    Channel<int> aggr;
    piping1toMany(c, aggr, [](int v, Channel<int>& dst) {
        dst.put(v - 1);
        dst.put(v - 2);
    });
    std::vector<int> vs;
    piping1toMany(aggr, c, [&vs](int v, Channel<int>& dst) {
        vs.push_back(v);
        if (v > 0)
            dst.put(v);
    });
    RTLOG("starting");
    c.put(25);
    closeAndWait(tp, c);
    RTLOG("completed, size: " << vs.size());
}

void pipe3()
{
    ThreadPool tp(3, "tp");
    scheduler<DefaultTag>().attach(tp);
    Channel<int> c1;
    Channel<int> c2;
    piping(c1, c2, [](Channel<int>& src, Channel<int>& dst) {
        for (int v: src)
            dst.put(v + 1);
    });
    c1.put(15);
    closeAndWait(tp, c1);
    int v = c2.get();
    TLOG("v: " << v);
}

void pipe4()
{
    ThreadPool tp(3, "tp");
    scheduler<DefaultTag>().attach(tp);
    Channel<int> c1;
    Channel<int> c2;
    piping1to1(c1, c2, [](int v) {
        return v + 1;
    });
    c1.put(15);
    closeAndWait(tp, c1);
    int v = c2.get();
    TLOG("v: " << v);
}

void cycle1()
{
    int threads = std::thread::hardware_concurrency();

    ThreadPool tp(threads, "tp");
    scheduler<DefaultTag>().attach(tp);

    std::vector<Channel<int>> channels(4);
    auto& chFirst = channels.front();
    auto& chLast = channels.back();
    for (int i = 0; i < 10000; ++ i)
        chFirst.put(1);

    int counter = 0;
    for (size_t i = 0; i < channels.size() - 1; ++ i)
    {
        auto& ch = channels[i];
        auto& chNext = channels[i+1];
        go([&ch, &chNext] {
            while (true)
                chNext.put(ch.get());
        });
    }
    go([&chFirst, &chLast, &counter] {
        for (;; ++ counter)
            chFirst.put(chLast.get());
    });
    for (int cur = counter, last = cur;; cur = last)
    {
        sleepFor(1000);
        last = counter;
        RLOG("counted: " << last-cur);
    }
}

}
