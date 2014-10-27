cmake_minimum_required(VERSION 2.8)

add_executable(client client.cpp)
target_link_libraries(client synca)

add_executable(server server.cpp)
target_link_libraries(server synca)

add_executable(data data.cpp)
target_link_libraries(data synca)
