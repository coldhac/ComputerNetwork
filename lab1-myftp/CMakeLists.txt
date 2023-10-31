cmake_minimum_required(VERSION 3.22)
project(lab1-myFTP)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


add_executable(ftp_server src/ftp_server.cpp)
add_executable(ftp_client src/ftp_client.cpp)

target_include_directories(ftp_server PUBLIC include)
target_include_directories(ftp_client PUBLIC include)