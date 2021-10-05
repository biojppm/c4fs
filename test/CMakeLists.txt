c4_setup_testing(DOCTEST)

function(c4fs_add_test name)
    c4_add_executable(c4fs-test-${name}
        SOURCES ${ARGN}
        INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
        LIBS c4fs doctest
        FOLDER test)
    c4_add_test(c4fs-test-${name} ON)
endfunction(c4fs_add_test)

c4fs_add_test(basic test_basic.cpp)
