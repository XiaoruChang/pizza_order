#ifndef PIZZA_ACCOUNT_PRIVATE_H
#define PIZZA_ACCOUNT_PRIVATE_H

#include "models.h"
#include "result.h"

/* Shared only by account.c and the storage implementation. AppState remains
 * opaque; storage owns this data and supplies the four backend operations. */
typedef struct {
    AccountId account_id;
    CustomerProfile profile;
    Money balance_cents;
    uint64_t password_salt;
    uint64_t password_digest;
    bool active;
} AccountRecord;

typedef struct {
    AccountRecord *customers;
    size_t customer_count;
    size_t customer_capacity;
    AccountId next_account_id;
    uint64_t next_salt;
    char manager_username[PIZZA_LOGIN_CAPACITY];
    uint64_t manager_salt;
    uint64_t manager_digest;
    bool manager_configured;
} AccountStore;

/* The returned store is borrowed and must not be modified or retained. */
Result account_backend_read(const AppState *state,
                            const AccountStore **out_store);

/* Clone the account portion of state. The caller owns the clone and must
 * release it with account_backend_dispose. On failure, out_store must be
 * empty or safe to dispose. */
Result account_backend_clone(const AppState *state, AccountStore *out_store);

/* Atomically publish candidate. On success storage takes ownership and clears
 * candidate. On failure candidate remains caller-owned and unchanged. */
Result account_backend_commit(AppState *state, AccountStore *candidate);

/* Dispose a candidate and wipe credential material before releasing memory. */
void account_backend_dispose(AccountStore *store);

/* Storage setup and tests use the same verifier implementation. */
uint64_t account_password_digest(const char *password, uint64_t salt);

#endif
