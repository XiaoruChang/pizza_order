#include "account.h"
#include "account_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct AppState {
    AccountStore accounts;
};

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void dispose_store(AccountStore *store)
{
    size_t i;

    if (store == NULL) {
        return;
    }
    for (i = 0; i < store->customer_count; ++i) {
        memset(&store->customers[i], 0, sizeof(store->customers[i]));
    }
    free(store->customers);
    memset(store, 0, sizeof(*store));
}

void account_backend_dispose(AccountStore *store)
{
    dispose_store(store);
}

Result account_backend_read(const AppState *state,
                            const AccountStore **out_store)
{
    if (state == NULL || out_store == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    *out_store = &state->accounts;
    return RESULT_OK;
}

Result account_backend_clone(const AppState *state, AccountStore *out_store)
{
    if (state == NULL || out_store == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    *out_store = state->accounts;
    out_store->customers = NULL;
    out_store->customer_capacity = state->accounts.customer_count;
    if (state->accounts.customer_count != 0) {
        out_store->customers = malloc(state->accounts.customer_count *
                                      sizeof(*out_store->customers));
        if (out_store->customers == NULL) {
            memset(out_store, 0, sizeof(*out_store));
            return RESULT_NO_MEMORY;
        }
        memcpy(out_store->customers, state->accounts.customers,
               state->accounts.customer_count * sizeof(*out_store->customers));
    }
    return RESULT_OK;
}

Result account_backend_commit(AppState *state, AccountStore *candidate)
{
    AccountStore old;

    if (state == NULL || candidate == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    old = state->accounts;
    state->accounts = *candidate;
    memset(candidate, 0, sizeof(*candidate));
    dispose_store(&old);
    return RESULT_OK;
}

static void init_state(AppState *state)
{
    memset(state, 0, sizeof(*state));
    state->accounts.next_account_id = 1;
    state->accounts.next_salt = 1;
    strcpy(state->accounts.manager_username, "manager");
    state->accounts.manager_salt = 99;
    state->accounts.manager_digest =
        account_password_digest("manager-pass", state->accounts.manager_salt);
    state->accounts.manager_configured = true;
}

static CustomerRegistration registration(const char *name,
                                         const char *telephone,
                                         const char *password)
{
    CustomerRegistration value;

    memset(&value, 0, sizeof(value));
    strcpy(value.profile.name, name);
    strcpy(value.profile.telephone, telephone);
    value.password = password;
    return value;
}

static int test_registration_and_login(void)
{
    AppState state;
    CustomerRegistration first = registration("Jiabao Gao", "12345678",
                                              "customer-pass");
    CustomerRegistration second = registration("Second Customer", "87654321",
                                               "another-pass");
    Session session = {99, SESSION_MANAGER};
    Session stale = {77, SESSION_CUSTOMER};
    AccountId first_id = 0;
    AccountId second_id = 0;
    Customer customer;

    init_state(&state);
    CHECK(account_register(&state, &first, &first_id) == RESULT_OK);
    CHECK(account_register(&state, &second, &second_id) == RESULT_OK);
    CHECK(second_id == first_id + 1);
    CHECK(account_login_customer(&state, first_id, "wrong-pass", &stale) ==
          RESULT_AUTH_FAILED);
    CHECK(stale.account_id == 0 && stale.role == SESSION_NONE);
    stale.account_id = 42;
    stale.role = SESSION_CUSTOMER;
    CHECK(account_login_customer(&state, first_id, "short", &stale) ==
          RESULT_INVALID_INPUT);
    CHECK(stale.account_id == 0 && stale.role == SESSION_NONE);
    CHECK(account_login_customer(&state, first_id, "customer-pass", &stale) ==
          RESULT_OK);
    CHECK(stale.account_id == first_id && stale.role == SESSION_CUSTOMER);
    session.account_id = 8;
    session.role = SESSION_CUSTOMER;
    CHECK(account_login_manager(&state, "manager", "wrong-pass", &session) ==
          RESULT_AUTH_FAILED);
    CHECK(session.account_id == 0 && session.role == SESSION_NONE);
    CHECK(account_login_manager(&state, "manager", "manager-pass", &session) ==
          RESULT_OK);
    CHECK(session.account_id == 0 && session.role == SESSION_MANAGER);
    CHECK(account_get_customer(&state, &stale, first_id, &customer) ==
          RESULT_OK);
    CHECK(customer.balance_cents == 0 && customer.active);
    customer.profile.name[0] = 'X';
    CHECK(account_get_customer(&state, &stale, first_id, &customer) ==
          RESULT_OK);
    CHECK(strcmp(customer.profile.name, "Jiabao Gao") == 0);
    CHECK(account_backend_read(&state, NULL) == RESULT_INVALID_ARGUMENT);
    CHECK(state.accounts.customers[0].password_digest != 0);
    account_logout(&stale);
    CHECK(stale.account_id == 0 && stale.role == SESSION_NONE);
    dispose_store(&state.accounts);
    return 0;
}

static int test_permissions_and_manager_lifecycle(void)
{
    AppState state;
    CustomerRegistration first = registration("First", "111", "password1");
    CustomerRegistration second = registration("Second", "222", "password2");
    CustomerRegistration third = registration("Third", "333", "password3");
    Session manager = {0, SESSION_MANAGER};
    Session customer_session = {0, SESSION_NONE};
    CustomerProfile profile = {"Updated", "333"};
    Customer customers[3];
    AccountId first_id = 0;
    AccountId second_id = 0;
    size_t count = 0;

    init_state(&state);
    CHECK(account_register(&state, &first, &first_id) == RESULT_OK);
    CHECK(account_login_customer(&state, first_id, "password1",
                                 &customer_session) == RESULT_OK);
    CHECK(account_register(&state, &second, &second_id) == RESULT_OK);
    CHECK(account_get_customer(&state, &customer_session, second_id,
                               &customers[0]) == RESULT_PERMISSION_DENIED);
    CHECK(account_update_profile(&state, &customer_session, second_id,
                                 &profile) == RESULT_PERMISSION_DENIED);
    profile.name[0] = 'O';
    strcpy(profile.name, "Own Update");
    CHECK(account_update_profile(&state, &customer_session, first_id,
                                 &profile) == RESULT_OK);
    CHECK(account_get_customer(&state, &customer_session, first_id,
                               &customers[0]) == RESULT_OK);
    CHECK(strcmp(customers[0].profile.name, "Own Update") == 0);
    CHECK(account_add_customer(&state, &customer_session, &second,
                               &second_id) == RESULT_PERMISSION_DENIED);
    CHECK(account_add_customer(&state, &manager, &third, &second_id) ==
          RESULT_OK);
    CHECK(account_update_profile(&state, &manager, second_id, &profile) ==
          RESULT_OK);
    CHECK(account_set_active(&state, &manager, first_id, false) == RESULT_OK);
    CHECK(account_get_customer(&state, &manager, first_id, &customers[0]) ==
          RESULT_OK);
    CHECK(!customers[0].active);
    CHECK(account_login_customer(&state, first_id, "password1",
                                 &customer_session) == RESULT_ACCOUNT_INACTIVE);
    CHECK(account_set_active(&state, &manager, first_id, true) == RESULT_OK);
    CHECK(account_login_customer(&state, first_id, "password1",
                                 &customer_session) == RESULT_OK);
    CHECK(account_list_customers(&state, &manager, false, NULL, 0, &count) ==
          RESULT_OK);
    CHECK(count == 3);
    CHECK(account_list_customers(&state, &manager, true, NULL, 0, &count) ==
          RESULT_OK);
    CHECK(count == 3);
    CHECK(account_list_customers(&state, &manager, false, customers, 1,
                                 &count) == RESULT_BUFFER_TOO_SMALL);
    CHECK(count == 3);
    CHECK(account_list_customers(&state, &manager, false, customers, 3,
                                 &count) == RESULT_OK);
    CHECK(count == 3);
    CHECK(customers[0].account_id < customers[1].account_id &&
          customers[1].account_id < customers[2].account_id);
    CHECK(account_list_customers(&state, &customer_session, false, NULL, 0,
                                 &count) == RESULT_PERMISSION_DENIED);
    dispose_store(&state.accounts);
    return 0;
}

static int test_validation(void)
{
    AppState state;
    CustomerRegistration value = registration("Name", "Phone", "short");
    Session session = {123, SESSION_CUSTOMER};
    AccountId id = 55;
    char long_name[PIZZA_NAME_CAPACITY];

    memset(long_name, 'x', sizeof(long_name));
    init_state(&state);
    CHECK(account_register(&state, &value, &id) == RESULT_INVALID_INPUT);
    value.password = "valid-pass";
    value.profile.name[0] = '\0';
    CHECK(account_register(&state, &value, &id) == RESULT_INVALID_INPUT);
    strcpy(value.profile.name, "Name");
    value.profile.name[1] = '\n';
    CHECK(account_register(&state, &value, &id) == RESULT_INVALID_INPUT);
    memcpy(value.profile.name, long_name, sizeof(long_name));
    CHECK(account_register(&state, &value, &id) == RESULT_INVALID_INPUT);
    CHECK(id == 55);
    CHECK(account_list_customers(&state, &session, false, NULL, 0, NULL) ==
          RESULT_INVALID_ARGUMENT);
    dispose_store(&state.accounts);
    return 0;
}

int main(void)
{
    CHECK(test_registration_and_login() == 0);
    CHECK(test_permissions_and_manager_lifecycle() == 0);
    CHECK(test_validation() == 0);
    puts("account: all scenarios passed");
    return 0;
}
