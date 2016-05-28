cmake_minimum_required(VERSION 3.0)
project(chat)

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -Wall -Wextra -pthread")
set(SOURCE_FILES chat2.cpp)
add_executable(chat2 ${SOURCE_FILES})
target_link_libraries(chat2 ${CMAKE_DL_LIBS})

