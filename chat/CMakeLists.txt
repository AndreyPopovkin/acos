cmake_minimum_required(VERSION 3.0)
project(chat)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
set(SOURCE_FILES chat.cpp)
add_executable(chat ${SOURCE_FILES})
target_link_libraries(chat ${CMAKE_DL_LIBS})

