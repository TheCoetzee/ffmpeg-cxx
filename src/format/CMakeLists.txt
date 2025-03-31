add_library(ffmpeg_format)
target_sources(
  ffmpeg_format
    PUBLIC 
      FILE_SET CXX_MODULES FILES 
        format.ixx
    PRIVATE 
      format.cpp
)
target_compile_features(ffmpeg_format PUBLIC cxx_std_23)
target_link_libraries(ffmpeg_format PUBLIC ffmpeg_util)
target_include_directories(ffmpeg_format PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
    $<INSTALL_INTERFACE:include/ffmpeg-cpp>
)
target_include_directories(ffmpeg_format SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg_format PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_format PRIVATE ${FFMPEG_LIBRARIES})
