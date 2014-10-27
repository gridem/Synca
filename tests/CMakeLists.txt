cmake_minimum_required(VERSION 2.8)

file(GLOB TESTS_SRC *.cpp)

add_executable(tests ${TESTS_SRC})
target_link_libraries(tests synca)
