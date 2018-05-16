cmake_minimum_required(VERSION 3.2)
project(c4fs)

include(./cmake/c4Project.cmake)

c4_declare_project(C4FS)

set(C4FS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(C4FS_EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)

add_subdirectory(src)

if(C4FS_BUILD_TESTS)
    enable_testing() # this must be done here (and not inside the test dir)
                     # so that the test targets are available at the top level
    add_subdirectory(test)
endif()

if(C4FS_BUILD_BENCHMARKS)
    add_subdirectory(bm)
endif()
