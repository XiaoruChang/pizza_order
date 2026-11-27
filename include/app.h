#ifndef PIZZA_APP_H
#define PIZZA_APP_H

#include "sales.h"

typedef struct {
    uint64_t account_id;
} AppSession;

/* Callbacks return 0 on success, nonzero on failure. They own business input
 * and persistence, and must consume complete input lines. context and streams
 * are borrowed for the duration of app_run. */
typedef int (*AppAction)(void *context, FILE *input, FILE *output);
typedef int (*AppCustomerAction)(void *context, const AppSession *session,
                                 FILE *input, FILE *output);

typedef struct {
    void *context;
    AppAction register_customer;
    int (*login_customer)(void *context, FILE *input, FILE *output,
                          AppSession *session);
    AppAction login_manager;
    AppCustomerAction place_order;
    AppCustomerAction show_balance;
    AppCustomerAction deposit;
    AppCustomerAction order_history;
    AppCustomerAction edit_profile;
    AppAction manage_menu;
    AppAction manage_customers;
    /* A successful load is paired with exactly one release, including empty or
     * invalid snapshots. On failure the provider cleans up. Bind both hooks. */
    int (*load_orders)(void *context, const SalesOrder **orders, size_t *count);
    void (*release_orders)(void *context, const SalesOrder *orders, size_t count);
} AppServices;

/* Missing authentication denies entry. Customer IDs must be nonzero.
 * EOF exits cleanly. Returns 0 normally, 1 on stream error/invalid arguments. */
int app_run(FILE *input, FILE *output, const AppServices *services);

#endif
