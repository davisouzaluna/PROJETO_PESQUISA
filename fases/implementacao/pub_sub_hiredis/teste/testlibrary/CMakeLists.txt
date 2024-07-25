cmake_minimum_required(VERSION 3.10)
project(testlibrary VERSION 1.0.0)

# Lista de fontes da biblioteca(testlibrary)
set(SRC_FILES
    src/latency.c
    src/save_data.c
)

# Criação da biblioteca
add_library(testlibrary STATIC ${SRC_FILES})

# Diretórios de cabeçalho
target_include_directories(testlibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Localize a biblioteca hiredis
find_library(HIREDIS_LIB hiredis PATHS /usr/local/lib /usr/lib /opt/homebrew/lib)
find_path(HIREDIS_INCLUDE hiredis/hiredis.h PATHS /usr/local/include /usr/include /opt/homebrew/include)

if (HIREDIS_LIB AND HIREDIS_INCLUDE)
    target_include_directories(testlibrary PUBLIC ${HIREDIS_INCLUDE})
    target_link_libraries(testlibrary PUBLIC ${HIREDIS_LIB})
else()
    message(FATAL_ERROR "hiredis library not found")
endif()

# Instala a biblioteca
install(TARGETS testlibrary
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

# Instala os arquivos de cabeçalho
install(DIRECTORY include/ DESTINATION include/testlibrary)

# Instala o arquivo de configuração CMake
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/testlibraryConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/testlibraryConfigVersion.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/testlibraryConfig.cmake"
  DESTINATION lib/cmake/testlibrary
)


