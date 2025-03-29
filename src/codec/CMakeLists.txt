add_library(ffmpeg_codec)
target_sources(ffmpeg_codec
	PUBLIC
		FILE_SET CXX_MODULES FILES
			module.ixx
			decoder.ixx
	PRIVATE
		decoder.cpp
)
target_compile_features(ffmpeg_codec PUBLIC cxx_std_23)
target_link_libraries(ffmpeg_codec PUBLIC ffmpeg_util ffmpeg_format)
target_include_directories(ffmpeg_codec PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
    $<INSTALL_INTERFACE:include/ffmpeg-cpp>
)
target_include_directories(ffmpeg_codec PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(ffmpeg_codec PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(ffmpeg_codec PRIVATE ${FFMPEG_LIBRARIES})
