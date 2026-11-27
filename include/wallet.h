#ifndef PIZZA_WALLET_H
#define PIZZA_WALLET_H

#include "models.h"
#include "result.h"

/* Active customer sessions only; operate on the session's own account. */
Result wallet_get_balance(const AppState *state, const Session *customer,
                           Money *out_balance);
/* Nonnegative amounts; read-only, not a reservation of funds. */
Result wallet_check_funds(const AppState *state, const Session *customer,
                           Money amount_cents);

/* Positive amounts only; reject overflow. Persist before reporting success. */
Result wallet_deposit(AppState *state, const Session *customer,
                       Money amount_cents, Money *out_balance);

/* Pure arithmetic helper used by order_submit; does NOT update an account or
 * save data. Balances/amounts must be >= 0; zero-price orders are permitted.
 * There is intentionally no separately persisted debit operation. */
Result wallet_calculate_debit(Money balance_cents, Money amount_cents,
                              Money *out_new_balance);

#endif
