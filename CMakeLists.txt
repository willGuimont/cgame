cmake_minimum_required(VERSION 4.1)
project(cgame LANGUAGES C OBJC)

include(CTest)
find_package(Threads REQUIRED)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Emscripten's EM_ASM macros require GNU extensions
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(CMAKE_C_EXTENSIONS ON)
else ()
    set(CMAKE_C_EXTENSIONS OFF)
endif ()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CGAME_WEB OFF)
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR "${PLATFORM}" STREQUAL "Web")
    set(CGAME_WEB ON)
endif ()

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
endif ()

# cgame library
add_library(cgame
        src/cgame/common.h
        src/cgame/ecs/entity.h
        src/cgame/game_loop.h
        src/cgame/utils/arena.h
        src/cgame/utils/dynamic_array.h
        src/cgame/utils/string_view.h
        src/cgame/utils/vectori.h
        src/cgame/utils/draw.h
        src/cgame/ecs/world.h
        src/cgame/ecs/entity.c
        src/cgame/game_loop.c
        src/cgame/stb_impl.c
        src/cgame/utils/arena.c
        src/cgame/utils/draw.c
        src/cgame/ecs/world.c
        src/cgame/utils/bitset.h
        src/cgame/coords/hex.c
)

target_include_directories(cgame
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cgame
)

target_compile_definitions(cgame
        PUBLIC
        $<$<CONFIG:Debug>:DEV_MODE>
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
endif ()

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
    set(options EXCLUDE_FROM_INDEX)
    set(one_value_args TITLE DESCRIPTION WEB_DIR ASSET_DIR SHELL_FILE)
    set(multi_value_args)
    cmake_parse_arguments(APP "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (NOT APP_TITLE)
        set(APP_TITLE "${target_name}")
    endif ()

    if (NOT APP_DESCRIPTION)
        set(APP_DESCRIPTION "")
    endif ()

    if (NOT APP_WEB_DIR)
        set(APP_WEB_DIR "${CMAKE_SOURCE_DIR}/web/${target_name}")
    elseif (NOT IS_ABSOLUTE "${APP_WEB_DIR}")
        set(APP_WEB_DIR "${CMAKE_SOURCE_DIR}/${APP_WEB_DIR}")
    endif ()

    if (NOT APP_ASSET_DIR)
        set(APP_ASSET_DIR "${CMAKE_SOURCE_DIR}/assets/${target_name}")
    elseif (NOT IS_ABSOLUTE "${APP_ASSET_DIR}")
        set(APP_ASSET_DIR "${CMAKE_SOURCE_DIR}/${APP_ASSET_DIR}")
    endif ()

    set_property(GLOBAL APPEND PROPERTY CGAME_APP_TARGETS "${target_name}")
    set_property(GLOBAL PROPERTY "CGAME_APP_${target_name}_TITLE" "${APP_TITLE}")
    set_property(GLOBAL PROPERTY "CGAME_APP_${target_name}_DESCRIPTION" "${APP_DESCRIPTION}")
    set_property(GLOBAL PROPERTY "CGAME_APP_${target_name}_WEB_DIR" "${APP_WEB_DIR}")
    if (APP_EXCLUDE_FROM_INDEX)
        set_property(GLOBAL PROPERTY "CGAME_APP_${target_name}_EXCLUDE_FROM_INDEX" TRUE)
    endif ()

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

    if (CGAME_WEB)
        set(app_web_output_dir "${CMAKE_BINARY_DIR}/${target_name}")
        if (APP_SHELL_FILE)
            set(app_shell_file "${APP_WEB_DIR}/${APP_SHELL_FILE}")
        else ()
            set(app_shell_file "${APP_WEB_DIR}/minshell.html")
        endif ()

        if (NOT EXISTS "${app_shell_file}")
            message(FATAL_ERROR "Missing web shell for ${target_name}: ${app_shell_file}")
        endif ()

        set_target_properties(${target_name} PROPERTIES
                OUTPUT_NAME "index"
                RUNTIME_OUTPUT_DIRECTORY "${app_web_output_dir}"
                SUFFIX ".html"
        )

        target_link_options(${target_name} PRIVATE
                "-sUSE_GLFW=3"
                "-sWASM=1"
                "-sTOTAL_MEMORY=67108864"
                "--shell-file" "${app_shell_file}"
        )

        set_target_properties(${target_name} PROPERTIES
                LINK_DEPENDS "${app_shell_file}"
        )

        file(COPY "${APP_WEB_DIR}/"
                DESTINATION "${app_web_output_dir}"
                PATTERN "minshell.html" EXCLUDE
                PATTERN "itch.html" EXCLUDE
                PATTERN "index.html.in" EXCLUDE
        )

        file(GLOB_RECURSE CGAME_ASSET_FILES CONFIGURE_DEPENDS "${APP_ASSET_DIR}/*")
        if (CGAME_ASSET_FILES)
            target_link_options(${target_name} PRIVATE
                    "--preload-file" "${APP_ASSET_DIR}@resources"
            )
        endif ()
    else ()
        add_custom_target(copy_assets_${target_name} ALL
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${APP_ASSET_DIR}
                $<TARGET_FILE_DIR:${target_name}>/resources
        )
        add_dependencies(${target_name} copy_assets_${target_name})
    endif ()

    if (APPLE)
        target_link_libraries(${target_name} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
    endif ()
endfunction()

function(cgame_generate_web_index)
    get_property(apps GLOBAL PROPERTY CGAME_APP_TARGETS)

    set(CGAME_WEB_APP_CARDS "")
    set(CGAME_WEB_APP_MANIFEST "")
    set(CGAME_WEB_INDEX_TEMPLATE "${CMAKE_SOURCE_DIR}/web/index.html.in")
    set(CGAME_WEB_INDEX_FAVICON "")

    foreach (app IN LISTS apps)
        get_property(web_dir GLOBAL PROPERTY "CGAME_APP_${app}_WEB_DIR")
        if (EXISTS "${web_dir}/favicon.svg")
            set(CGAME_WEB_INDEX_FAVICON "${web_dir}/favicon.svg")
            break()
        endif ()
    endforeach ()

    if (CGAME_WEB_INDEX_FAVICON)
        configure_file(
                "${CGAME_WEB_INDEX_FAVICON}"
                "${CMAKE_BINARY_DIR}/favicon.svg"
                COPYONLY
        )
    endif ()

    foreach (app IN LISTS apps)
        string(APPEND CGAME_WEB_APP_MANIFEST "${app}\n")

        get_property(exclude GLOBAL PROPERTY "CGAME_APP_${app}_EXCLUDE_FROM_INDEX")
        if (exclude)
            continue()
        endif ()

        get_property(title GLOBAL PROPERTY "CGAME_APP_${app}_TITLE")
        get_property(description GLOBAL PROPERTY "CGAME_APP_${app}_DESCRIPTION")

        if (NOT title)
            set(title "${app}")
        endif ()

        string(APPEND CGAME_WEB_APP_CARDS
                "        <a class=\"card\" href=\"${app}/\">\n"
                "            <h2>${title}</h2>\n"
        )

        if (description)
            string(APPEND CGAME_WEB_APP_CARDS
                    "            <p>${description}</p>\n"
            )
        endif ()

        string(APPEND CGAME_WEB_APP_CARDS
                "            <code>${app}/</code>\n"
                "        </a>\n"
        )
    endforeach ()

    if (NOT EXISTS "${CGAME_WEB_INDEX_TEMPLATE}")
        message(FATAL_ERROR "Missing web index template: ${CGAME_WEB_INDEX_TEMPLATE}")
    endif ()

    configure_file(
            "${CGAME_WEB_INDEX_TEMPLATE}"
            ${CMAKE_BINARY_DIR}/index.html
            @ONLY
    )

    file(GENERATE
            OUTPUT "${CMAKE_BINARY_DIR}/cgame_web_apps.txt"
            CONTENT "${CGAME_WEB_APP_MANIFEST}"
    )

    add_custom_target(web_index ALL
            DEPENDS ${CMAKE_BINARY_DIR}/index.html ${CMAKE_BINARY_DIR}/cgame_web_apps.txt
    )
endfunction()

set(HEXATAK_CLASSIC_SOURCES
        src/apps/hexatak_classic/main.c
        src/apps/hexatak_classic/app.c
        src/apps/hexatak_classic/app.h
        src/apps/hexatak_classic/app_types.h
        src/apps/hexatak_classic/app_state.h
        src/apps/hexatak_classic/app_state.c
        src/apps/hexatak_classic/app_render.h
        src/apps/hexatak_classic/app_render.c
        src/apps/hexatak_classic/game.h
        src/apps/hexatak_classic/game.c
        src/apps/hexatak_classic/solver.h
        src/apps/hexatak_classic/solver.c
        src/cgame/coords/hex.h
)

# Hexatak Classic application
add_executable(hexatak_classic
        ${HEXATAK_CLASSIC_SOURCES}
        web/hexatak_classic/minshell.html
)
add_cgame_app(hexatak_classic hexatak_classic
        TITLE "Hexatak Classic"
        DESCRIPTION "Hex-grid puzzle about merging and stacking stones to build a road across the board."
        WEB_DIR "web/hexatak_classic"
)

# Hexatak Classic itch.io application
add_executable(hexatak_classic-itch
        ${HEXATAK_CLASSIC_SOURCES}
        web/hexatak_classic/itch.html
)
add_cgame_app(hexatak_classic-itch hexatak_classic
        TITLE "Hexatak Classic Itch"
        DESCRIPTION "Hex-grid puzzle about merging and stacking stones to build a road across the board (itch.io release target)."
        WEB_DIR "web/hexatak_classic"
        ASSET_DIR "assets/hexatak_classic"
        SHELL_FILE "itch.html"
        EXCLUDE_FROM_INDEX
)

# Hexatak application
add_executable(hexatak
        src/apps/hexatak/main.c
        src/apps/hexatak/app.c
        src/apps/hexatak/app.h
        web/hexatak/minshell.html
)
add_cgame_app(hexatak hexatak
        TITLE "Hexatak"
        DESCRIPTION "New Hexatak target scaffold."
        WEB_DIR "web/hexatak"
)

# Sokoban application
add_executable(sokoban
        src/apps/sokoban/main.c
        src/apps/sokoban/app.c
        src/apps/sokoban/app.h
        web/sokoban/minshell.html
)
add_cgame_app(sokoban sokoban
        TITLE "Sokoban 3D"
        DESCRIPTION "Sokoban, but in 3D and a bit of momentum"
        WEB_DIR "web/sokoban"
)

get_property(CGAME_APP_TARGETS GLOBAL PROPERTY CGAME_APP_TARGETS)

add_custom_target(all_apps)

if (CGAME_APP_TARGETS)
    add_dependencies(all_apps ${CGAME_APP_TARGETS})
endif ()

if (CGAME_WEB)
    cgame_generate_web_index()
    add_dependencies(all_apps web_index)
endif ()

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
