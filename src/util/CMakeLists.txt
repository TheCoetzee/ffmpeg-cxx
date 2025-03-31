add_library(ffmpeg_util)
target_sources(ffmpeg_util 
        PUBLIC 
            FILE_SET CXX_MODULES FILES 
                module.ixx
                error.ixx
                packet.ixx
                frame.ixx
)
target_compile_features(ffmpeg_util PUBLIC cxx_std_23)
target_include_directories(ffmpeg_util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
    $<INSTALL_INTERFACE:include/ffmpeg-cpp>
)
target_include_directories(ffmpeg_util SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg_util PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_util PRIVATE ${FFMPEG_LIBRARIES})
