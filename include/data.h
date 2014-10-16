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

#include "mt.h"
#include "channel.h"
#include "helpers.h"

namespace synca {
namespace data {

template<typename T_channel>
void closeAndWait(mt::ThreadPool& tp, T_channel& c)
{
    tp.wait();
    c.close();
    tp.wait();
}

template<typename T>
struct Closer
{
    Closer(T& t_) : t(t_) {}
    ~Closer()             { t.close(); }
private:
    T& t;
};

template<typename T>
Closer<T> closer(T& t)
{
    return {t};
}

template<typename T_src, typename T_dst, typename F_pipe>
void piping(T_src& s, T_dst& d, F_pipe f, int n = 1)
{
    goN(n, [&s, &d, f] {
        auto c = closer(d);
        f(s, d);
    });
}

template<typename T_src, typename T_dst, typename F_pipe>
void piping1toMany(T_src& s, T_dst& d, F_pipe f, int n = 1)
{
    piping(s, d, [f] (T_src& s, T_dst& d) {
        for (auto&& v: s)
        {
            try
            {
                f(v, d);
            }
            catch (std::exception& e)
            {
                RJLOG("Error: " << e.what());
            }
        }
    }, n);
}

template<typename T_src, typename T_dst, typename F_pipe>
void piping1to1(T_src& s, T_dst& d, F_pipe f, int n = 1)
{
    piping(s, d, [f] (T_src& s, T_dst& d) {
        for (auto&& v: s)
        {
            try
            {
                d.put(std::move(f(v)));
            }
            catch (std::exception& e)
            {
                RJLOG("Error: " << e.what());
            }
        }
    }, n);
}

template<typename T_src, typename T_dst, typename F_pipe>
void piping1to01(T_src& s, T_dst& d, F_pipe f, int n = 1)
{
    piping(s, d, [f] (T_src& s, T_dst& d) {
        for (auto&& v: s)
        {
            try
            {
                auto&& r = f(v);
                if (!isEmpty(r))
                    d.put(std::move(r));
            }
            catch (std::exception& e)
            {
                RJLOG("Error: " << e.what());
            }
        }
    }, n);
}

}}