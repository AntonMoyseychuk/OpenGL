cmake_minimum_required(VERSION 3.26.0 FATAL_ERROR)

project(glm)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include "${CMAKE_CURRENT_SOURCE_DIR}/")