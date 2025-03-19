cmake_minimum_required(VERSION 3.21)
project(PlExA VERSION 0.2.0 LANGUAGES CXX)

include(CTest)
enable_testing()

add_subdirectory(extern/solver)

add_library(PlExA src/executor.cpp)
target_compile_features(PlExA PUBLIC cxx_std_17)
target_include_directories(PlExA PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
add_dependencies(PlExA oRatioLib)
target_link_libraries(PlExA PUBLIC oRatioLib)

if(${CONSTRAINT_REASONER_TYPE} STREQUAL "Z3")
    target_include_directories(PlExA PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/z3> ${z3_SOURCE_DIR}/src/api)
    target_sources(PlExA PRIVATE src/z3/z3executor.cpp)
elseif(${CONSTRAINT_REASONER_TYPE} STREQUAL "MathSAT")
    target_include_directories(PlExA PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/msat>)
elseif(${CONSTRAINT_REASONER_TYPE} STREQUAL "SeMiTONE")
    target_include_directories(PlExA PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/st>)
    target_sources(PlExA PRIVATE src/st/stexecutor.cpp)
endif()

setup_sanitizers(PlExA)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PROJECT_NAME SeMiTONE)
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)