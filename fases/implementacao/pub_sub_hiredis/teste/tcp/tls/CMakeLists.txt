#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.13)

project(mqtt_async)

if (BUILD_DEMO)
else ()
# Call this from your own project's makefile.
  find_package(nng CONFIG REQUIRED)
endif (BUILD_DEMO)

find_package(Threads)

if (DEBUG)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
  if (ASAN)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  endif (ASAN)
  if (TSAN)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
  endif (TSAN)
endif (DEBUG)

add_executable(mqtt_async mqtt_async.c)
target_link_libraries(mqtt_async nng::nng)
target_link_libraries(mqtt_async ${CMAKE_THREAD_LIBS_INIT})

if(NNG_ENABLE_TLS)
    add_definitions(-DNNG_SUPP_TLS)
endif()

if (NNG_ENABLE_SQLITE)
    add_definitions(-DNNG_SUPP_SQLITE)
    target_link_libraries(mqtt_async dl)
endif (NNG_ENABLE_SQLITE)

target_compile_definitions(mqtt_async PRIVATE NNG_ELIDE_DEPRECATED)