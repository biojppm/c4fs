set(src
    c4/fs/fs.hpp
    c4/fs/fs.cpp
)

c4_require_module(c4fs
    c4core
    REMOTE GIT_REPOSITORY https://github.com/biojppm/c4core GIT_TAG master
    )

c4_add_library(C4FS c4fs
    SOURCES ${src}
    LIBS c4core
    INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
)
