#ifndef PIZZA_ACCOUNT_H
#define PIZZA_ACCOUNT_H

#include "models.h"
#include "result.h"

typedef struct {
    CustomerProfile profile;
    const char *password; /* Borrowed only until the function returns. */
} CustomerRegistration;

/* Public self-registration. Assign a fresh persistent ID, active=true and a
 * zero balance. A successful mutation is already saved to disk. */
Result account_register(AppState *state,
                        const CustomerRegistration *registration,
                        AccountId *out_account_id);

/* Both logins reset a non-NULL out_session to {0} BEFORE any validation.
 * Unknown IDs/usernames and wrong passwords return AUTH_FAILED; a correctly
 * authenticated disabled customer returns ACCOUNT_INACTIVE. */
Result account_login_customer(const AppState *state, AccountId account_id,
                              const char *password, Session *out_session);
Result account_login_manager(const AppState *state, const char *username,
                             const char *password, Session *out_session);
void account_logout(Session *session); /* NULL is allowed. */

/* Customer: own ID only. Manager: any customer. Copies public data to out. */
Result account_get_customer(const AppState *state, const Session *session,
                            AccountId account_id, Customer *out_customer);
Result account_update_profile(AppState *state, const Session *session,
                              AccountId account_id,
                              const CustomerProfile *profile);

/* Manager-only operations. Disabling is a logical deletion: retain balance,
 * identity and all historical orders; re-enabling is supported. */
Result account_add_customer(AppState *state, const Session *manager,
                            const CustomerRegistration *registration,
                            AccountId *out_account_id);
Result account_set_active(AppState *state, const Session *manager,
                          AccountId account_id, bool active);

/* Manager only, sorted by account ID. For the shared list/buffer contract see
 * docs/public-api.md. include_inactive=false lists active customers only. */
Result account_list_customers(const AppState *state, const Session *manager,
                              bool include_inactive, Customer *out_customers,
                              size_t capacity, size_t *out_count);

#endif
