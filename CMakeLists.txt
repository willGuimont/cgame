cmake_minimum_required(VERSION 4.1)
project(cgame LANGUAGES C OBJC)

include(CTest)
find_package(Threads REQUIRED)

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
set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
set(FLECS_STATIC ON CACHE BOOL "" FORCE)
set(FLECS_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib)
add_subdirectory(third_party/box2d)
add_subdirectory(third_party/box3d)
add_subdirectory(third_party/flecs)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE third_party/stb)

add_library(cute_headers INTERFACE)
target_include_directories(cute_headers SYSTEM INTERFACE third_party/cute_headers)

add_library(par INTERFACE)
target_include_directories(par SYSTEM INTERFACE third_party/par)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_path(S2N_INCLUDE_DIR s2n.h)
    find_library(S2N_LIBRARY s2n)
endif()

# cgame library
add_library(cgame
        src/cgame/common.h
        src/cgame/entity.h
        src/cgame/game_loop.h
        src/cgame/utils/arena.h
        src/cgame/utils/dynamic_array.h
        src/cgame/utils/string_view.h
        src/cgame/utils/vectori.h
        src/cgame/world.h
        src/cgame/entity.c
        src/cgame/game_loop.c
        src/cgame/stb_impl.c
        src/cgame/utils/arena.c
        src/cgame/world.c
        src/cgame/utils/bitset.h
        src/cgame/cute_headers_impl.c
)

target_include_directories(cgame
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cgame
)

target_link_libraries(cgame
        PUBLIC
        stb
        cute_headers
        par
        flecs::flecs_static
        raylib
        Threads::Threads
)

if (S2N_INCLUDE_DIR AND S2N_LIBRARY)
    target_include_directories(cgame SYSTEM PUBLIC ${S2N_INCLUDE_DIR})
    target_compile_definitions(cgame PUBLIC CGAME_CUTE_TLS_S2N)
    target_link_libraries(cgame PUBLIC ${S2N_LIBRARY})
endif()

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

set(CGAME_APP_TARGETS)

function(add_cgame_app target_name app_dir)
    target_include_directories(${target_name}
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/apps/${app_dir}
    )

    target_compile_definitions(${target_name}
            PRIVATE
            ROOT_DIR="${CMAKE_SOURCE_DIR}"
    )

    target_compile_options(${target_name}
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

    target_link_options(${target_name}
            PRIVATE
            $<$<AND:$<CONFIG:Debug>,$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:GNU>>>:-fsanitize=address>
    )

    target_link_libraries(${target_name}
            PRIVATE
            cgame
            raylib
    )

    if ("${PLATFORM}" STREQUAL "Web")
        set_target_properties(${target_name} PROPERTIES SUFFIX ".html")
        add_custom_command(
                TARGET ${target_name} PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/resources
        )
        target_link_options(${target_name} PRIVATE
                "-sUSE_GLFW=3"
        )
        file(GLOB_RECURSE CGAME_ASSET_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/*")
        if (CGAME_ASSET_FILES)
            target_link_options(${target_name} PRIVATE
                    "--preload-file" "${CMAKE_SOURCE_DIR}/assets@resources"
            )
        endif()
    else()
        add_custom_command(
                TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${target_name}>/resources
        )
    endif()

    if (APPLE)
        target_link_libraries(${target_name} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
    endif()

    set(CGAME_APP_TARGETS ${CGAME_APP_TARGETS} ${target_name} PARENT_SCOPE)
endfunction()

# Hexazyme application
add_executable(hexazyme
        src/apps/hexazyme/main.c
        src/apps/hexazyme/app.c
        src/apps/hexazyme/app.h
        src/cgame/hex.h
        src/apps/hexazyme/game.h
        src/apps/hexazyme/sim.h
        src/apps/hexazyme/render.h
        src/apps/hexazyme/level.h
)
add_cgame_app(hexazyme hexazyme)

add_custom_target(all_apps)
add_dependencies(all_apps ${CGAME_APP_TARGETS})

# Run target
add_custom_target(run
        COMMAND ${CMAKE_COMMAND} -E env
        "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/asan_suppressions.txt"
        $<TARGET_FILE:hexazyme>
        DEPENDS hexazyme
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
