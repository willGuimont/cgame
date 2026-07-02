enable_testing()

set(TEST_COMPILE_OPTIONS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Werror
        -Wno-unused-function
)

set(TEST_TARGETS)

foreach (test_src IN ITEMS
        arena_tests.c
        bitset_tests.c
        dynamic_array_tests.c
        string_view_tests.c
        entity_tests.c
        world_tests.c
)
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE cgame m)
    target_include_directories(${test_name} PRIVATE
            ${PROJECT_SOURCE_DIR}/src
            ${PROJECT_SOURCE_DIR}/tests
    )
    target_compile_options(${test_name} PRIVATE ${TEST_COMPILE_OPTIONS})
    add_test(NAME ${test_name} COMMAND ${test_name})
    list(APPEND TEST_TARGETS ${test_name})
endforeach ()

add_custom_target(tests DEPENDS ${TEST_TARGETS})
