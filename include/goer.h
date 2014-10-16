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

#include <stdexcept>
#include <memory>

namespace synca {

enum EventStatus
{
    ES_NORMAL,
    ES_CANCELLED,
    ES_TIMEDOUT,
};

struct EventException : std::runtime_error
{
    EventException(EventStatus s);
    EventStatus status();

private:
    EventStatus st;
};

struct Goer
{
    Goer();
    EventStatus reset();
    bool cancel();
    bool timedout();
    
private:
    struct State
    {
        State() : status(ES_NORMAL) {}
        EventStatus status;
    };

    bool setStatus0(EventStatus s);
    State& state0();

    std::shared_ptr<State> state;
};

}

