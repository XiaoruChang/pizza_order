#include "result.h"

const char *result_message(Result result)
{
    switch (result) {
    case RESULT_OK: return "Success";
    case RESULT_INVALID_ARGUMENT: return "Invalid argument";
    case RESULT_INVALID_INPUT: return "Invalid input";
    case RESULT_NOT_FOUND: return "Record or data file not found";
    case RESULT_ALREADY_EXISTS: return "Record or data file already exists";
    case RESULT_AUTH_FAILED: return "Authentication failed";
    case RESULT_PERMISSION_DENIED: return "Permission denied";
    case RESULT_ACCOUNT_INACTIVE: return "Customer account is inactive";
    case RESULT_INSUFFICIENT_FUNDS: return "Insufficient balance";
    case RESULT_OPTION_UNAVAILABLE: return "Menu option is unavailable";
    case RESULT_QUOTE_CHANGED: return "Order quote changed; confirm again";
    case RESULT_CAPACITY_EXCEEDED: return "Record or quantity limit exceeded";
    case RESULT_BUFFER_TOO_SMALL: return "Output buffer is too small";
    case RESULT_OVERFLOW: return "Numeric overflow";
    case RESULT_NO_MEMORY: return "Unable to allocate memory";
    case RESULT_IO_ERROR: return "Input/output error";
    case RESULT_INVALID_DATA: return "Invalid or unsupported saved data";
    case RESULT_END_OF_INPUT: return "End of input";
    default: return "Unknown result";
    }
}
