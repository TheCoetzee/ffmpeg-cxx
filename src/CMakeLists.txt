add_library(ffmpeg_frame)
target_sources(ffmpeg_frame
    PUBLIC
        FILE_SET CXX_MODULES FILES
            frame.ixx
    PRIVATE
        frame.cpp
)
target_compile_features(ffmpeg_frame PUBLIC cxx_std_23)
target_link_libraries(ffmpeg_frame PUBLIC ffmpeg_util)
target_include_directories(ffmpeg_frame PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/ffmpeg-cpp>
)
target_include_directories(ffmpeg_frame PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg_frame PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_frame PRIVATE ${FFMPEG_LIBRARIES})

add_subdirectory(util)
add_subdirectory(format)
add_subdirectory(codec)

add_library(ffmpeg)
target_sources(ffmpeg
    PUBLIC
        FILE_SET CXX_MODULES FILES
            module.ixx
)
target_compile_features(ffmpeg PUBLIC cxx_std_23)
target_link_libraries(ffmpeg PUBLIC ffmpeg_frame ffmpeg_util ffmpeg_format ffmpeg_codec)
target_include_directories(ffmpeg PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/ffmpeg-cpp>
)
