#ifndef PIZZA_RESULT_H
#define PIZZA_RESULT_H

/* Shared business results. Existing AppAction and SalesResult remain unchanged. */
typedef enum {
    RESULT_OK = 0,
    RESULT_INVALID_ARGUMENT,
    RESULT_INVALID_INPUT,
    RESULT_NOT_FOUND,
    RESULT_ALREADY_EXISTS,
    RESULT_AUTH_FAILED,
    RESULT_PERMISSION_DENIED,
    RESULT_ACCOUNT_INACTIVE,
    RESULT_INSUFFICIENT_FUNDS,
    RESULT_OPTION_UNAVAILABLE,
    RESULT_QUOTE_CHANGED,
    RESULT_CAPACITY_EXCEEDED,
    RESULT_BUFFER_TOO_SMALL,
    RESULT_OVERFLOW,
    RESULT_NO_MEMORY,
    RESULT_IO_ERROR,
    RESULT_INVALID_DATA,
    RESULT_END_OF_INPUT
} Result;

/* Borrowed static text; never NULL, including for an unknown result value. */
const char *result_message(Result result);

#endif
