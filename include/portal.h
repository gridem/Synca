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

#include "core.h"

namespace synca {

struct Portal
{
    Portal(mt::IScheduler& destination);
    ~Portal();
    
private:
    mt::IScheduler& source;
};

template<typename T>
struct WithPortal : Scheduler
{
    struct Access : Portal
    {
        Access(Scheduler& s) : Portal(s) {}
        T* operator->()             { return &single<T>(); }
    };
    
    Access operator->()             { return *this; }
};

template<typename T>
WithPortal<T>& portal()
{
    return single<WithPortal<T>>();
}

}
