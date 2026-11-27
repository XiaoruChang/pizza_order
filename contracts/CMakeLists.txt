cmake_minimum_required(VERSION 3.20)
project(pizza_public_contracts VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(contract_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

set(PIZZA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")

# Deliberately independent of the full application's later CMake project.
add_library(pizza_contracts STATIC ../src/result.c)
target_include_directories(pizza_contracts PUBLIC "${PIZZA_INCLUDE_DIR}")
contract_warnings(pizza_contracts)

set(PIZZA_PUBLIC_HEADERS
    account.h app.h history.h input.h menu.h models.h order.h
    pricing.h result.h sales.h storage.h wallet.h)

set(header_sources "")
set(all_includes "")
set(reverse_includes "")
foreach(header IN LISTS PIZZA_PUBLIC_HEADERS)
    set(source "${CMAKE_CURRENT_BINARY_DIR}/headers/${header}.c")
    file(GENERATE OUTPUT "${source}"
        CONTENT "#include \"${header}\"\n#include \"${header}\"\n")
    list(APPEND header_sources "${source}")
    string(APPEND all_includes "#include \"${header}\"\n")
    string(PREPEND reverse_includes "#include \"${header}\"\n")
endforeach()

foreach(order IN ITEMS all reverse)
    set(source "${CMAKE_CURRENT_BINARY_DIR}/headers/${order}.c")
    file(GENERATE OUTPUT "${source}" CONTENT "${${order}_includes}")
    list(APPEND header_sources "${source}")
endforeach()

# Compile only: declared business functions do not need placeholder bodies.
add_library(pizza_public_headers OBJECT ${header_sources})
target_include_directories(pizza_public_headers PRIVATE "${PIZZA_INCLUDE_DIR}")
contract_warnings(pizza_public_headers)
