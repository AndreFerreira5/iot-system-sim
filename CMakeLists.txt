cmake_minimum_required(VERSION 3.24)
project(iot-system-sim C)

set(CMAKE_C_STANDARD 11)

add_executable(iot-system-sim src/home_iot.c src/log.c src/log.h src/home_iot.h src/string.c src/string.h src/config.c lib/cJSON/cJSON.c src/system_manager.c src/system_manager.h src/ring_buffer.c src/ring_buffer.h src/max_heap.c src/max_heap.h src/sensor.c src/sensor.h)

target_include_directories(iot-system-sim PRIVATE "${CMAKE_SOURCE_DIR}/lib")