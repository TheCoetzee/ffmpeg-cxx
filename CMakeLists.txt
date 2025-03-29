cmake_minimum_required(VERSION 3.31)

project(ffmpeg_cxx VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(FFMPEG REQUIRED COMPONENTS avformat avcodec avutil)

add_subdirectory(src)

install(TARGETS ffmpeg ffmpeg_format ffmpeg_codec ffmpeg_util
  EXPORT ffmpeg-cpp-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  FILE_SET CXX_MODULES DESTINATION include/ffmpeg-cpp
)

install(EXPORT ffmpeg-cpp-targets
  FILE ffmpeg-cpp-targets.cmake
  NAMESPACE ffmpeg::
  DESTINATION lib/cmake/ffmpeg-cpp
)

# Generate the ffmpeg-cpp-config.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-cpp-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ffmpeg-cpp-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-cpp-config.cmake"
    INSTALL_DESTINATION lib/cmake/ffmpeg-cpp
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-cpp-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-cpp-version.cmake" 
    DESTINATION lib/cmake/ffmpeg-cpp
)
