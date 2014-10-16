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

#pragma once

#include <queue>
#include <mutex>

#include "core.h"
#include "helpers.h"

namespace synca {

// with iterators
template<typename T>
struct Channel
{
private:
    struct Waiters;
    struct Waiter
    {
        friend struct Waiters;
        
        Waiter(T& v) : val(&v) {}
        
        void proceed(T&& v)
        {
            *val = std::move(v);
            proc();
        }

        void proceed()
        {
            val = nullptr;
            proc();
        }
        
        void setProceed(Handler&& proceed)
        {
            proc = std::move(proceed);
        }
        
        bool hasValue() const
        {
            return val != nullptr;
        }
        
    private:
        Handler proc;
        Waiter* next = nullptr;
        T* val;
    };
    
    struct Waiters
    {
        Waiter* pop()
        {
            if (!root)
                return nullptr;
            Waiter* w = root;
            root = root->next;
            return w;
        }
        
        Waiters popAll()
        {
            Waiters w;
            w.root = root;
            root = nullptr;
            return w;
        }
        
        void push(Waiter& w)
        {
            w.next = root;
            root = &w;
        }
        
    private:
        Waiter* root = nullptr;
    };
    
    typedef std::unique_lock<std::mutex> Lock;
    
public:
    struct Iterator
    {
        Iterator() = default;
        Iterator(Channel& c) : ch(&c)            { ++*this; }

        T& operator*()                           { return val; }
        Iterator& operator++()                   { if (!ch->get(val)) ch = nullptr; return *this; }
        bool operator!=(const Iterator& i) const { return ch != i.ch; }
    private:
        T val;
        Channel* ch = nullptr;
    };
    
    Iterator begin()                             { return {*this}; }
    static Iterator end()                        { return {}; }
    
    void put(T val)
    {
        Lock lock(mutex);
        Waiter* w = waiters.pop();
        if (w) 
        {
            lock.unlock();
            w->proceed(std::move(val));
        }
        else
        {
            queue.emplace(std::move(val));
        }
    }
    
    bool get(T& val)
    {
        Lock lock(mutex);
        if (!queue.empty())
        {
            val = std::move(queue.front());
            queue.pop();
            return true;
        }
        if (closed)
            return false;
        Waiter w(val);
        waiters.push(w);
        lock.release();
        deferProceed([this, &w](Handler proceed) {
            w.setProceed(std::move(proceed));
            mutex.unlock();
        });
        return w.hasValue();
    }
    
    bool empty() const
    {
        Lock lock(mutex);
        return queue.empty();
    }
    
    T get()
    {
        T val;
        get(val);
        return val;
    }
    
    void open()
    {
        Lock lock(mutex);
        closed = false;
    }
    
    void close()
    {
        Lock lock(mutex);
        if (closed)
            return;
        closed = true;
        Waiters ws = waiters.popAll();
        lock.unlock();
        for (Waiter* w = ws.pop(); w; w = ws.pop())
            w->proceed();
    }
    
private:
    Waiters waiters;
    mutable std::mutex mutex;
    std::queue<T> queue;
    bool closed = false;
};

}
