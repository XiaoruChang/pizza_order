#ifndef PIZZA_HISTORY_H
#define PIZZA_HISTORY_H

#include "models.h"
#include "result.h"

/* Active customer only; derive customer ID from the session, not user input.
 * Copy their orders, newest order ID first. An empty history is a valid list.
 * Use order_get for a selected order's complete details. */
Result history_list_orders(const AppState *state, const Session *customer,
                            Order *out_orders, size_t capacity,
                            size_t *out_count);

#endif
