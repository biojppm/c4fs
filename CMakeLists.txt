cmake_minimum_required(VERSION 3.12)
include(./ext/c4core/cmake/c4Project.cmake)
project(c4fs
    DESCRIPTION "C++ filesystem utilities"
    HOMEPAGE_URL "https://github.com/biojppm/c4fs/")
c4_project(VERSION 0.0.1 STANDALONE
    AUTHOR "Joao Paulo Magalhaes <dev@jpmag.me>")

set(C4FS_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(C4FS_EXT_DIR ${CMAKE_CURRENT_LIST_DIR}/ext)

c4_require_subproject(c4core
    SUBDIRECTORY ${C4FS_EXT_DIR}/c4core)

c4_add_library(c4fs
    SOURCES c4/fs/export.hpp c4/fs/fs.hpp c4/fs/fs.cpp
    SOURCE_ROOT ${C4FS_SRC_DIR}
    LIBS c4core
    INC_DIRS
       $<BUILD_INTERFACE:${C4FS_SRC_DIR}> $<INSTALL_INTERFACE:include>
)

c4_install_target(c4fs)
c4_install_exports()

c4_add_dev_targets()
