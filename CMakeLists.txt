cmake_minimum_required(VERSION 2.8)

option(STATIC_ALL "Use static libraries" ON)
option(LOG_MUTEX "Use log output under mutex" ON)
option(LOG_DEBUG "Use debug output" OFF)

if(LOG_MUTEX)
    add_definitions(-DflagLOG_MUTEX)
endif()

if(LOG_DEBUG)
    add_definitions(-DflagLOG_DEBUG)
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(GCC_LIKE_COMPILER ON)
endif()

if(NOT MSVC AND NOT GCC_LIKE_COMPILER)
    message(FATAL_ERROR "Supports only gcc, clang or msvc compilers")
endif()

if(STATIC_ALL)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
    if(MSVC)
        set(MSVC_RUNTIME static)
    endif()
    #if(GCC_LIKE_COMPILER)
    #    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    #endif()
else()
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_STATIC_RUNTIME OFF)
    if(MSVC)
        set(MSVC_RUNTIME dynamic)
    endif()
endif()

if(MSVC)
    add_definitions(-DflagMSC)
    add_definitions(-D_WIN32_WINNT=0x0501)
    add_definitions(-DBOOST_ASIO_HAS_MOVE)
    include(MSVCRuntime.cmake)
    configure_msvc_runtime()
endif()

if(GCC_LIKE_COMPILER)
    add_definitions(-std=c++11)
    #if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    #    add_definitions(-stdlib=libc++)
    #endif()
    find_package(Threads)
endif()

set(Boost_USE_MULTITHREADED ON)

find_package(Boost 1.56 REQUIRED COMPONENTS context system date_time regex)

file(GLOB SYNCA_SRC src/*)
file(GLOB SYNCA_HDR include/*)

file(GLOB TESTS_SRC tests/*.cpp)
file(GLOB TESTS_HDR tests/*.h)

include_directories(include ${Boost_INCLUDE_DIR})

add_library(synca ${SYNCA_SRC} ${SYNCA_HDR})
set(SYNCA_LIBS synca ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(tests ${TESTS_SRC} ${TESTS_HDR})
target_link_libraries(tests ${SYNCA_LIBS})

add_executable(client examples/client.cpp)
target_link_libraries(client ${SYNCA_LIBS})

add_executable(server examples/server.cpp)
target_link_libraries(server ${SYNCA_LIBS})

add_executable(data examples/data.cpp)
target_link_libraries(data ${SYNCA_LIBS})
