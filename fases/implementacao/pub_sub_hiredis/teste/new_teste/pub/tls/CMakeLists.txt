# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.13)

project(pub_tls)

# Define the option with a default value of ON
option(NNG_ENABLE_TLS "Enable TLS support" ON)

# Define MBEDpub_tls_ROOT_DIR using environment variable
set(MBEDpub_tls_ROOT_DIR "../../mbedpub_tls/cmake")

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

add_executable(pub_tls pub_tls.c)
target_link_libraries(pub_tls nng::nng hiredis)
target_link_libraries(pub_tls ${CMAKE_THREAD_LIBS_INIT})

# Check if TLS is enabled
if(NNG_ENABLE_TLS)
    find_package(MbedTLS REQUIRED)
    target_link_libraries(pub_tls mbedtls mbedx509 mbedcrypto)
    add_definitions(-DNNG_SUPP_TLS)
endif()

if (NNG_ENABLE_SQLITE)
    add_definitions(-DNNG_SUPP_SQLITE)
    target_link_libraries(pub_tls dl)
endif (NNG_ENABLE_SQLITE)

target_compile_definitions(pub_tls PRIVATE NNG_ELIDE_DEPRECATED)
