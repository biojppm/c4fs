c4_setup_testing(C4FS ON)

function(c4fs_add_test name)
    c4_add_executable(C4FS c4fs-test-${name}
        SOURCES ${ARGN} main.cpp
        INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
        LIBS c4fs gtest
        FOLDER c4fs-test)
    c4_add_test(C4FS c4fs-test-${name} ON)
endfunction(c4fs_add_test)

c4fs_add_test(basic test.cpp)
