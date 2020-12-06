cmake_minimum_required(VERSION 3.17)
project(RealtimeProject)

set(CMAKE_CXX_STANDARD 14)


find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
add_executable(RealtimeProject main.cpp GetBoard.cpp GetBoard.h CircularBuffer.h ConnectFour.cpp ConnectFour.cpp)
target_link_libraries(RealtimeProject ${OpenCV_LIBS} -lpthread -lrt)
