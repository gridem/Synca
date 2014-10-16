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

#include <string>

#include "goer.h"
#include "helpers.h"

namespace synca {

std::string statusToString(EventStatus s)
{
    switch (s)
    {
    case ES_NORMAL:
        return "Normal";

    case ES_CANCELLED:
        return "Cancelled";

    case ES_TIMEDOUT:
        return "Timed out";

    default:
        return "<unknown>";
    }
}

EventException::EventException(EventStatus s) :
    std::runtime_error("Journey event received: " + statusToString(s)),
    st(s)
{
}

EventStatus EventException::status()
{
    return st;
}

Goer::Goer() : state(std::make_shared<State>())
{
}

EventStatus Goer::reset()
{
    EventStatus s = state0().status;
    state0().status = ES_NORMAL;
    return s;
}

bool Goer::cancel()
{
    return setStatus0(ES_CANCELLED);
}

bool Goer::timedout()
{
    return setStatus0(ES_TIMEDOUT);
}

bool Goer::setStatus0(EventStatus s)
{
    auto& ss = state0().status;
    if (ss != ES_NORMAL)
        return false;
    ss = s;
    return true;
}

Goer::State& Goer::state0()
{
    State* s = state.get();
    VERIFY(s != nullptr, "Goer state is null");
    return *s;
}

}
