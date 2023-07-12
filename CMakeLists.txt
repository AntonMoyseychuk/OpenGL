cmake_minimum_required(VERSION 3.26.0 FATAL_ERROR)

project(sandbox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/glad)
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/glm)
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/imgui)
add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty/spdlog)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/glew/glew-2.2.0/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/glm/glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/imgui)

if (MSVC)
    set_target_properties("${CMAKE_PROJECT_NAME}" PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm imgui spdlog)