cmake_minimum_required(VERSION 3.1)
project(OpenALFFMpeg)



set(SOURCE_CXX
    main.cpp
    common.cpp
)
set(CMAKE_BUILD_TYPE Debug)

if(MSCV)

	# Just a quick helper function for finding FFMPEG libraries and headers
	function(add_ffmpeg_library library_name)
		find_library(${library_name}_LINK_LIBRARIES	NAMES ${library_name}.lib PATH_SUFFIXES lib	DOC "Paths for " ${library_name})
		find_path(${library_name}_INCLUDE_DIRS NAMES lib${library_name}/version.h PATH_SUFFIXES include DOC "Header path for " ${library_name})
	endfunction()

	find_library(
		OpenAL_LINK_LIBRARIES
		NAMES OpenAL32
		PATH_SUFFIXES libs/Win64
		DOC "Paths for OpenAL-soft"
	)
	find_path(
		OpenAL_INCLUDE_DIRS
		NAMES AL/al.h
		PATH_SUFFIXES include
		DOC "Path for OpenAL include directory"
	)

	add_ffmpeg_library(avcodec)
	add_ffmpeg_library(avformat)
	add_ffmpeg_library(avutil)
	add_ffmpeg_library(swresample)

	
else()
	find_package(PkgConfig)
	pkg_check_modules(OpenAL REQUIRED openal)
	pkg_check_modules(avcodec REQUIRED libavcodec)
	pkg_check_modules(avformat REQUIRED libavformat)
	pkg_check_modules(avutil REQUIRED libavutil)
	pkg_check_modules(swresample REQUIRED libswresample)
endif()


add_executable(${PROJECT_NAME} ${SOURCE_CXX})
target_link_libraries(${PROJECT_NAME} 
    ${OpenAL_LINK_LIBRARIES}
    ${avcodec_LINK_LIBRARIES}
    ${avformat_LINK_LIBRARIES}
    ${avutil_LINK_LIBRARIES}
    ${swresample_LINK_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
    ${OpenAL_INCLUDE_DIRS}
    ${avcodec_INCLUDE_DIRS}
    ${avformat_INCLUDE_DIRS}
    ${avutil_INCLUDE_DIRS}
    ${swresample_INCLUDE_DIRS}
)
if(
    ${CMAKE_BUILD_TYPE} STREQUAL Debug
    AND
    ${CMAKE_CXX_COMPILER_ID} STREQUAL Clang
    )
    message("Setting extra clang compiler options")
    target_compile_options(
        ${PROJECT_NAME}
        PUBLIC
        -fno-inline-functions
        -ggdb
        )
endif()
