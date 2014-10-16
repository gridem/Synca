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

#include <stdexcept>

#include "synca_tests.h"
#include "data_tests.h"
#include "helpers.h"

#define TESTS()   \
    TEST_ITERATOR(test::teleport1) \
    TEST_ITERATOR(test::teleport2) \
    TEST_ITERATOR(test::teleport_exception1)   \
    TEST_ITERATOR(test::wait1) \
    TEST_ITERATOR(test::wait2) \
    TEST_ITERATOR(test::waitAny1)  \
    TEST_ITERATOR(test::resultAny1)    \
    TEST_ITERATOR(test::resultAny2)    \
    TEST_ITERATOR(test::resultAny3)    \
    TEST_ITERATOR(test::resultAny4)    \
    TEST_ITERATOR(test::alone1)    \
    TEST_ITERATOR(test::timeout1)  \
    TEST_ITERATOR(test::timeout2)  \
    TEST_ITERATOR(test::portal1)   \
    TEST_ITERATOR(test::portal2)   \
    TEST_ITERATOR(test::gc1)   \
    TEST_ITERATOR(test::tp1)   \
    TEST_ITERATOR(data::pipe1) \
    TEST_ITERATOR(data::pipe2) \
    TEST_ITERATOR(data::pipe3) \
    TEST_ITERATOR(data::pipe4) \
    TEST_ITERATOR(data::cycle1)    \

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            RLOG("Usage: <test name>");
            RLOG("  Available tests:");
#define TEST_ITERATOR(D_name) RLOG("    " #D_name);
            TESTS()
#undef TEST_ITERATOR
            return 1;
        }
        VERIFY(argc == 2, "Usage: <test name>");
        std::string name = argv[1];
        RLOG("Starting test: " << name);
#define TEST_ITERATOR(D_name) if (name == #D_name) D_name(); else
        TESTS()
#undef TEST_ITERATOR
        RAISE("Unknown test name: " + name);
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
