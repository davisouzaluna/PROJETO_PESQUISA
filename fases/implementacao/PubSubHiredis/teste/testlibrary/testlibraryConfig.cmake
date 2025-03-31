# Define where to find the installed library
include(CMakeFindDependencyMacro)
find_dependency(hiredis REQUIRED)

# Define the targets and include directories
include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/testlibraryConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/testlibraryConfig.cmake"
  INSTALL_DESTINATION lib/cmake/testlibrary
)

# Define the targets and include directories
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/testlibraryConfig.cmake"
  DESTINATION lib/cmake/testlibrary
)
