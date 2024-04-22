cmake_minimum_required(VERSION 3.5.0)
project(PlExA VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")

include(CTest)
enable_testing()

add_subdirectory(extern/solver)

add_library(PlExA src/executor.cpp src/executor_theory.cpp)
add_dependencies(PlExA oRatioLib)
target_link_libraries(PlExA PUBLIC oRatioLib)
target_include_directories(PlExA PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(MSVC)
    target_compile_options(PlExA PRIVATE /W4)
else()
    target_compile_options(PlExA PRIVATE -Wall -Wextra -Wpedantic)
    if (ENABLE_COVERAGE)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(PlExA PRIVATE --coverage)
            target_link_libraries(PlExA PUBLIC gcov)
        endif()
    endif()
endif()

set(CPACK_PROJECT_NAME PlExA)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
