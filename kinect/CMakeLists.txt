cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(sentiers-aveugles)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies

# using brew installed libfreenect
set(LIBFREENECT_DIR /opt/homebrew/Cellar/libfreenect/0.7.0)
list(APPEND CMAKE_MODULE_PATH ${LIBFREENECT_DIR}/share/libfreenect)
find_package(libfreenect REQUIRED)
include_directories(${LIBFREENECT_DIR}/include/libfreenect)
include_directories(./lib/TinyOsc)

# Our Project

add_executable(${PROJECT_NAME} main.c ./lib/TinyOsc/tinyosc.c ./lib/TinyOsc/tinyosc.h)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
    target_link_libraries(${PROJECT_NAME} "-framework GLUT")
endif ()

find_package(OpenAL REQUIRED)
target_link_libraries(${PROJECT_NAME} ${LIBFREENECT_DIR}/lib/libfreenect.0.7.0.dylib)
target_link_libraries(${PROJECT_NAME} ${LIBFREENECT_DIR}/lib/libfreenect_sync.dylib)
target_link_libraries(${PROJECT_NAME} OpenAL::OpenAL)