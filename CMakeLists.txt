cmake_minimum_required(VERSION 3.20)
project(pizza_order VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(pizza_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_library(pizza_core src/sales.c src/app.c)
target_include_directories(pizza_core PUBLIC include)
if(MINGW)
    # Older MinGW headers otherwise select non-ISO I64 printf specifiers.
    target_compile_definitions(pizza_core PUBLIC __USE_MINGW_ANSI_STDIO=1)
endif()
pizza_warnings(pizza_core)

add_executable(pizza_order src/main.c)
target_link_libraries(pizza_order PRIVATE pizza_core)
pizza_warnings(pizza_order)

add_executable(pizza_demo examples/demo.c)
target_link_libraries(pizza_demo PRIVATE pizza_core)
pizza_warnings(pizza_demo)

include(CTest)
if(BUILD_TESTING)
    foreach(module sales app)
        add_executable(test_${module} tests/test_${module}.c)
        target_link_libraries(test_${module} PRIVATE pizza_core)
        pizza_warnings(test_${module})
        add_test(NAME ${module} COMMAND test_${module})
        set_tests_properties(${module} PROPERTIES TIMEOUT 10)
    endforeach()
    add_test(NAME cli_smoke COMMAND ${CMAKE_COMMAND}
        -DDEMO=$<TARGET_FILE:pizza_demo>
        -DAPP=$<TARGET_FILE:pizza_order>
        -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_smoke.cmake)
endif()
