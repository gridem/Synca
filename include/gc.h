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

#include <vector>

#include "common.h"

struct GC
{
    ~GC();
    
    template<typename T>
    T* add(T* t)
    {
        deleters.emplace_back([t] { delete t; });
        return t;
    }
    
private:
    std::vector<Handler> deleters;
};

GC& gc();

template<typename T, typename... V>
T* gcnew(V&&... v)
{
    return gc().add(new T(std::forward(v)...));
}
