add_subdirectory(util)
add_subdirectory(format)
add_subdirectory(codec)
add_subdirectory(filter)

add_library(ffmpeg)
target_sources(ffmpeg
    PUBLIC
        FILE_SET CXX_MODULES FILES
            module.ixx
)
target_compile_features(ffmpeg PUBLIC cxx_std_23)
target_link_libraries(ffmpeg PUBLIC ffmpeg_util ffmpeg_format ffmpeg_codec ffmpeg_filter)
target_include_directories(ffmpeg PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/ffmpeg-cpp>
)
