add_library(ffmpeg_filter)

target_sources(ffmpeg_filter
    PUBLIC
        FILE_SET CXX_MODULES FILES
            filter.ixx
    PRIVATE
        filter.cpp
)

target_compile_features(ffmpeg_filter PUBLIC cxx_std_23)

target_link_libraries(ffmpeg_filter PUBLIC ffmpeg_util)
target_include_directories(ffmpeg_codec SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg_codec PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_codec PRIVATE ${FFMPEG_LIBRARIES})

target_include_directories(ffmpeg_filter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
    $<INSTALL_INTERFACE:include/ffmpeg-cpp>
)
