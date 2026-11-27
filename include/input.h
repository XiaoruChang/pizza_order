#ifndef PIZZA_INPUT_H
#define PIZZA_INPUT_H

#include <stdio.h>

#include "models.h"
#include "result.h"

/* No prompts. Read one whole line, strip LF or CRLF, NUL-terminate. capacity
 * includes NUL and must be >= 2. Reject overlong/embedded-NUL lines as
 * INVALID_INPUT and consume their remainder. Clear a valid buffer on failure.
 * EOF with no bytes => END_OF_INPUT; a final line without newline is accepted.
 * Never close the stream; stream errors => IO_ERROR. */
Result input_read_line(FILE *input, char *buffer, size_t capacity);

/* Trim surrounding ASCII whitespace; accept decimal digits only (no sign,
 * exponent, decimal point or trailing text), then enforce inclusive bounds.
 * Do not change *out_value on failure. */
Result input_parse_u64(const char *text, uint64_t minimum, uint64_t maximum,
                        uint64_t *out_value);

/* Accept nonnegative decimal money with zero, one or two fractional digits.
 * Examples: "12" => 1200, "12.3" => 1230, "12.34" => 1234.
 * Reject signs, commas, exponent, "12.", extra digits and integer overflow.
 * Trim surrounding ASCII whitespace; leave *out_cents unchanged on failure. */
Result input_parse_money(const char *text, Money *out_cents);

#endif
