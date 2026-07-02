cmake_minimum_required(VERSION 4.1)
project(cgame LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Emscripten's EM_ASM macros require GNU extensions
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(CMAKE_C_EXTENSIONS ON)
else()
    set(CMAKE_C_EXTENSIONS OFF)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib)
add_subdirectory(third_party/box2d)
add_subdirectory(third_party/box3d)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE third_party/stb)

# cgame library
add_library(cgame
        src/common.h
        src/entity.h
        src/utils/arena.h
        src/utils/dynamic_array.h
        src/utils/string_view.h
        src/utils/vectori.h
        src/world.h
        src/entity.c
        src/stb_impl.c
        src/utils/arena.c
        src/world.c
        src/utils/bitset.h
)

target_include_directories(cgame
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cgame
        PUBLIC
        stb
)

target_compile_options(cgame
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Werror
        -Wno-unused-function
)

# cgame app executable
add_executable(cgame_app
        src/main.c
        src/app.c
        src/app.h
)

target_include_directories(cgame_app
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(cgame_app
        PRIVATE
        ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

target_compile_options(cgame_app
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Werror
        -Wno-unused-function
        -O2
        $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,Clang,GNU>>:-fsanitize=address>
)

target_link_options(cgame_app
        PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:GNU>>>:-fsanitize=address>
)

target_link_libraries(cgame_app
        PRIVATE
        cgame
        raylib
        box2d
        box3d
)

# Resource copying
if ("${PLATFORM}" STREQUAL "Web")
    set_target_properties(cgame_app PROPERTIES SUFFIX ".html")
    add_custom_command(
        TARGET cgame_app PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/resources
    )
    target_link_options(cgame_app PRIVATE
        "-sUSE_GLFW=3"
    )
    file(GLOB_RECURSE CGAME_ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/*")
    if (CGAME_ASSET_FILES)
        target_link_options(cgame_app PRIVATE
            "--preload-file" "${CMAKE_SOURCE_DIR}/assets@resources"
        )
    endif()
else()
    add_custom_command(
        TARGET cgame_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:cgame_app>/resources
    )
endif()

# macOS Frameworks
if (APPLE)
    target_link_libraries(cgame_app PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

# Run target
add_custom_target(run
        COMMAND ${CMAKE_COMMAND} -E env
        "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/asan_suppressions.txt"
        $<TARGET_FILE:cgame_app>
        DEPENDS cgame_app
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL
)

# Testing
if (BUILD_TESTING)
    add_subdirectory(tests)

    add_custom_target(check
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS tests
            USES_TERMINAL
    )
else ()
    add_custom_target(check
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            USES_TERMINAL
    )
endif ()
