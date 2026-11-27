#ifndef PIZZA_TEST_SUPPORT_H
#define PIZZA_TEST_SUPPORT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unlike assert, checks also execute in Release builds with NDEBUG defined. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void read_output(FILE *stream, char *buffer, size_t capacity)
{
    size_t count;
    CHECK(fflush(stream) == 0);
    rewind(stream);
    count = fread(buffer, 1, capacity - 1, stream);
    CHECK(!ferror(stream));
    CHECK(count < capacity - 1); /* Do not silently truncate an assertion. */
    buffer[count] = '\0';
}

#endif
