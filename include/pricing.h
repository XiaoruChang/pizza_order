#ifndef PIZZA_PRICING_H
#define PIZZA_PRICING_H

#include "models.h"
#include "result.h"

/* Pure read. Verify nonzero existing IDs, correct categories, availability and
 * quantity in 1..PIZZA_MAX_LINE_QUANTITY. Unit = crust + size + topping;
 * subtotal = unit * quantity. Check every sum/product for int64_t overflow.
 * No balance change, file writes or console I/O. */
Result pricing_quote(const AppState *state, const PizzaConfig *config,
                      uint32_t quantity, PriceQuote *out_quote);

#endif
