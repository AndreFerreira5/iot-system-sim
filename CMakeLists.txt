cmake_minimum_required(VERSION 3.24)
project(iot-system-sim C)

set(CMAKE_C_STANDARD 11)

add_executable(iot-system-sim src/home_iot.c src/log.c src/log.h src/home_iot.h src/string.c src/string.h)
