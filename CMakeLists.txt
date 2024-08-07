cmake_minimum_required(VERSION 3.24)
project(iot-system-sim C)

set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(iot-system-sim
        src/home_iot.c
        src/log.c
        src/log.h
        src/home_iot.h
        src/string.c
        src/string.h
        src/config.c
        lib/cJSON/cJSON.c
        src/system_manager.c
        src/system_manager.h
        src/ring_buffer.c
        src/ring_buffer.h
        src/max_heap.c
        src/max_heap.h
        src/worker.c
        src/worker.h
        src/sensor_reader.c
        src/sensor_reader.h
        src/sensors_alerts.h
        )

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(iot-system-sim PRIVATE DEBUG=1)
endif()
target_include_directories(iot-system-sim PRIVATE "${CMAKE_SOURCE_DIR}/lib")

add_executable(sensor
        src/sensor.c
        src/sensor.h
        src/config.c
        src/config.h
        lib/cJSON/cJSON.c
        )
target_include_directories(sensor PRIVATE "${CMAKE_SOURCE_DIR}/lib")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(iot-system-sim PRIVATE Threads::Threads)
