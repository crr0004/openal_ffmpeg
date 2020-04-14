cmake_minimum_required(VERSION 3.1)
project(OpenALFFMpeg)

find_package(PkgConfig)

set(SOURCE_CXX
    main.cpp
    common.cpp
)
set(CMAKE_BUILD_TYPE Debug)

pkg_check_modules(OpenAL REQUIRED openal)
pkg_check_modules(avcodec REQUIRED libavcodec)
pkg_check_modules(avformat REQUIRED libavformat)
pkg_check_modules(avutil REQUIRED libavutil)
pkg_check_modules(swresample REQUIRED libswresample)

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
