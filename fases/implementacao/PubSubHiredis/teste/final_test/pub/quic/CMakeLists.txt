#
# Copyright 2018 Capitar IT Group BV <info@capitar.com>
# Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.13)

project(pub_quic)

# Call this from your own project's makefile.
find_package(nng CONFIG REQUIRED)

find_package(Threads)
find_package(msquic)
find_package(OpenSSL)



if (DEBUG)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
  if (ASAN)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  endif (ASAN)
  if (TSAN)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
  endif (TSAN)
endif (DEBUG)
add_executable(pub_quic  pub_quic.c )
#add_executable(sub quic_client.c)


if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
    target_link_libraries(pub_quic
 OpenSSL::Crypto OpenSSL::SSL)
endif()

target_link_libraries(pub_quic nng::nng msquic pthread hiredis) #adicionei o hiredis no target_link_libraries pois ela já foi instalada no sistema e não precisa ser compilada
target_compile_definitions(pub_quic PRIVATE NNG_ELIDE_DEPRECATED)

if (NNG_ENABLE_SQLITE)
    add_definitions(-DNNG_SUPP_SQLITE)
    target_link_libraries(pub_quic
 dl)
endif (NNG_ENABLE_SQLITE)
