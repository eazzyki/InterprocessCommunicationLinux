cmake_minimum_required(VERSION 3.17)
project(Server_Client)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED)

add_executable(Server_Client client.cpp)

target_link_libraries(Server_Client PUBLIC ${OpenCV_LIBS})
