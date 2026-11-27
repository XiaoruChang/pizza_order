execute_process(COMMAND "${DEMO}"
    INPUT_FILE "${FIXTURES}/demo_input.txt"
    OUTPUT_VARIABLE demo_output ERROR_VARIABLE demo_error
    RESULT_VARIABLE demo_result TIMEOUT 10)
if(NOT "${demo_result}" STREQUAL "0")
    message(FATAL_ERROR "Demo failed (${demo_result}): ${demo_error}")
endif()
foreach(expected "DEMO ONLY" "Paid orders: 2" "Pizzas sold: 5"
        "Sales amount: 151.50" "1 | Margherita | 3 | 86.50"
        "2 | Hawaiian | 2 | 65.00" "DEMO customer session" "Module not connected.")
    string(FIND "${demo_output}" "${expected}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Demo missing '${expected}':\n${demo_output}")
    endif()
endforeach()

execute_process(COMMAND "${APP}"
    INPUT_FILE "${FIXTURES}/app_input.txt"
    OUTPUT_VARIABLE app_output ERROR_VARIABLE app_error
    RESULT_VARIABLE app_result TIMEOUT 10)
if(NOT "${app_result}" STREQUAL "0")
    message(FATAL_ERROR "App failed (${app_result}): ${app_error}")
endif()
string(REGEX MATCHALL "Module not connected[.]" missing "${app_output}")
list(LENGTH missing missing_count)
if(NOT missing_count EQUAL 3 OR app_output MATCHES "Sales amount:")
    message(FATAL_ERROR "App must report pending modules without exposing demo sales:\n${app_output}")
endif()
message(STATUS "Both executable smoke checks passed")
