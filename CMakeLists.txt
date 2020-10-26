cmake_minimum_required(VERSION 3.6)
project(Lab4)
add_executable(app Lab4.cpp)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT app)