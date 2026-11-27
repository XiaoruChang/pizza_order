#include "account.h"

#include "account_private.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static size_t bounded_length(const char *text, size_t limit)
{
    size_t length = 0;

    if (text == NULL) {
        return limit + 1;
    }
    while (length <= limit && text[length] != '\0') {
        ++length;
    }
    return length;
}

static bool valid_text(const char *text, size_t capacity)
{
    size_t length = bounded_length(text, capacity - 1);
    size_t i;

    if (length == 0 || length >= capacity) {
        return false;
    }
    for (i = 0; i < length; ++i) {
        if (iscntrl((unsigned char)text[i]) != 0) {
            return false;
        }
    }
    return true;
}

static bool valid_password(const char *password)
{
    size_t length = bounded_length(password, PIZZA_PASSWORD_MAX_BYTES);
    size_t i;

    if (length < PIZZA_PASSWORD_MIN_BYTES ||
        length > PIZZA_PASSWORD_MAX_BYTES) {
        return false;
    }
    for (i = 0; i < length; ++i) {
        if (iscntrl((unsigned char)password[i]) != 0) {
            return false;
        }
    }
    return true;
}

static bool valid_login(const char *username)
{
    return valid_text(username, PIZZA_LOGIN_CAPACITY);
}

static bool constant_time_equal(uint64_t left, uint64_t right)
{
    uint64_t difference = left ^ right;

    difference |= difference >> 32;
    difference |= difference >> 16;
    difference |= difference >> 8;
    return (difference & UINT64_C(0xff)) == 0;
}

uint64_t account_password_digest(const char *password, uint64_t salt)
{
    uint64_t hash = UINT64_C(1469598103934665603) ^ salt;
    size_t i;
    unsigned int round;

    for (round = 0; round < 4; ++round) {
        for (i = 0; password[i] != '\0'; ++i) {
            hash ^= (unsigned char)password[i];
            hash *= UINT64_C(1099511628211);
            hash ^= hash >> 29;
        }
        hash ^= hash >> 33;
        hash *= UINT64_C(0xff51afd7ed558ccd);
        hash ^= hash >> 33;
        hash += UINT64_C(0x9e3779b97f4a7c15) + round;
    }
    return hash;
}

static AccountRecord *find_customer(AccountStore *store, AccountId account_id)
{
    size_t i;

    for (i = 0; i < store->customer_count; ++i) {
        if (store->customers[i].account_id == account_id) {
            return &store->customers[i];
        }
    }
    return NULL;
}

static const AccountRecord *find_customer_const(const AccountStore *store,
                                                AccountId account_id)
{
    size_t i;

    for (i = 0; i < store->customer_count; ++i) {
        if (store->customers[i].account_id == account_id) {
            return &store->customers[i];
        }
    }
    return NULL;
}

static Result reserve_customer(AccountStore *store)
{
    size_t new_capacity;
    AccountRecord *customers;

    if (store->customer_count < store->customer_capacity) {
        return RESULT_OK;
    }
    if (store->customer_capacity == 0) {
        new_capacity = 8;
    } else {
        if (store->customer_capacity > SIZE_MAX / 2) {
            return RESULT_NO_MEMORY;
        }
        new_capacity = store->customer_capacity * 2;
    }
    if (new_capacity > SIZE_MAX / sizeof(*customers)) {
        return RESULT_NO_MEMORY;
    }
    customers = realloc(store->customers, new_capacity * sizeof(*customers));
    if (customers == NULL) {
        return RESULT_NO_MEMORY;
    }
    store->customers = customers;
    store->customer_capacity = new_capacity;
    return RESULT_OK;
}

static Result read_store(const AppState *state, const AccountStore **out_store)
{
    Result result;

    if (state == NULL || out_store == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    result = account_backend_read(state, out_store);
    if (result == RESULT_OK && *out_store == NULL) {
        return RESULT_INVALID_DATA;
    }
    return result;
}

static Result clone_store(const AppState *state, AccountStore *out_store)
{
    Result result;

    if (state == NULL || out_store == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    memset(out_store, 0, sizeof(*out_store));
    result = account_backend_clone(state, out_store);
    if (result != RESULT_OK) {
        account_backend_dispose(out_store);
    }
    return result;
}

static Result require_manager(const Session *session)
{
    if (session == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (session->role != SESSION_MANAGER || session->account_id != 0) {
        return RESULT_PERMISSION_DENIED;
    }
    return RESULT_OK;
}

static Result require_customer_owner(const Session *session,
                                     AccountId account_id)
{
    if (session == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (session->role != SESSION_CUSTOMER || session->account_id == 0) {
        return RESULT_PERMISSION_DENIED;
    }
    return session->account_id == account_id ? RESULT_OK
                                              : RESULT_PERMISSION_DENIED;
}

static Result validate_registration(const CustomerRegistration *registration)
{
    if (registration == NULL || registration->password == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (!valid_text(registration->profile.name, PIZZA_NAME_CAPACITY) ||
        !valid_text(registration->profile.telephone,
                    PIZZA_PHONE_CAPACITY) ||
        !valid_password(registration->password)) {
        return RESULT_INVALID_INPUT;
    }
    return RESULT_OK;
}

static Result add_customer_to_store(AccountStore *store,
                                    const CustomerRegistration *registration,
                                    AccountId *out_account_id)
{
    AccountRecord *customer;
    Result result;

    if (store->next_account_id == 0 ||
        store->next_account_id == UINT64_MAX || store->next_salt == 0 ||
        store->next_salt == UINT64_MAX) {
        return RESULT_OVERFLOW;
    }
    result = reserve_customer(store);
    if (result != RESULT_OK) {
        return result;
    }
    customer = &store->customers[store->customer_count];
    memset(customer, 0, sizeof(*customer));
    customer->account_id = store->next_account_id++;
    customer->profile = registration->profile;
    customer->balance_cents = 0;
    customer->password_salt = store->next_salt++;
    customer->password_digest =
        account_password_digest(registration->password, customer->password_salt);
    customer->active = true;
    ++store->customer_count;
    *out_account_id = customer->account_id;
    return RESULT_OK;
}

Result account_register(AppState *state,
                        const CustomerRegistration *registration,
                        AccountId *out_account_id)
{
    AccountStore candidate;
    AccountId account_id;
    Result result;

    if (state == NULL || registration == NULL || out_account_id == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    result = validate_registration(registration);
    if (result != RESULT_OK) {
        return result;
    }
    result = clone_store(state, &candidate);
    if (result != RESULT_OK) {
        return result;
    }
    result = add_customer_to_store(&candidate, registration, &account_id);
    if (result == RESULT_OK) {
        result = account_backend_commit(state, &candidate);
    }
    if (result != RESULT_OK) {
        account_backend_dispose(&candidate);
    } else {
        *out_account_id = account_id;
    }
    return result;
}

Result account_login_customer(const AppState *state, AccountId account_id,
                              const char *password, Session *out_session)
{
    const AccountStore *store;
    const AccountRecord *customer;
    Result result;

    if (out_session != NULL) {
        memset(out_session, 0, sizeof(*out_session));
    }
    if (out_session == NULL || state == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (password == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (account_id == 0 || !valid_password(password)) {
        return RESULT_INVALID_INPUT;
    }
    result = read_store(state, &store);
    if (result != RESULT_OK) {
        return result;
    }
    customer = find_customer_const(store, account_id);
    if (customer == NULL ||
        !constant_time_equal(customer->password_digest,
                             account_password_digest(password,
                                                    customer->password_salt))) {
        return RESULT_AUTH_FAILED;
    }
    if (!customer->active) {
        return RESULT_ACCOUNT_INACTIVE;
    }
    out_session->account_id = customer->account_id;
    out_session->role = SESSION_CUSTOMER;
    return RESULT_OK;
}

Result account_login_manager(const AppState *state, const char *username,
                             const char *password, Session *out_session)
{
    const AccountStore *store;
    Result result;

    if (out_session != NULL) {
        memset(out_session, 0, sizeof(*out_session));
    }
    if (out_session == NULL || state == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (username == NULL || password == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (!valid_login(username) || !valid_password(password)) {
        return RESULT_INVALID_INPUT;
    }
    result = read_store(state, &store);
    if (result != RESULT_OK) {
        return result;
    }
    if (!store->manager_configured ||
        strcmp(store->manager_username, username) != 0 ||
        !constant_time_equal(store->manager_digest,
                             account_password_digest(password,
                                                    store->manager_salt))) {
        return RESULT_AUTH_FAILED;
    }
    out_session->account_id = 0;
    out_session->role = SESSION_MANAGER;
    return RESULT_OK;
}

void account_logout(Session *session)
{
    if (session != NULL) {
        memset(session, 0, sizeof(*session));
    }
}

static void copy_customer(Customer *out_customer,
                          const AccountRecord *customer)
{
    out_customer->account_id = customer->account_id;
    out_customer->profile = customer->profile;
    out_customer->balance_cents = customer->balance_cents;
    out_customer->active = customer->active;
}

Result account_get_customer(const AppState *state, const Session *session,
                            AccountId account_id, Customer *out_customer)
{
    const AccountStore *store;
    const AccountRecord *customer;
    Result result;

    if (state == NULL || session == NULL || out_customer == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (account_id == 0) {
        return RESULT_INVALID_INPUT;
    }
    if (session->role == SESSION_CUSTOMER) {
        result = require_customer_owner(session, account_id);
        if (result != RESULT_OK) {
            return result;
        }
    } else {
        result = require_manager(session);
        if (result != RESULT_OK) {
            return result;
        }
    }
    result = read_store(state, &store);
    if (result != RESULT_OK) {
        return result;
    }
    customer = find_customer_const(store, account_id);
    if (customer == NULL) {
        return RESULT_NOT_FOUND;
    }
    if (session->role == SESSION_CUSTOMER && !customer->active) {
        return RESULT_ACCOUNT_INACTIVE;
    }
    copy_customer(out_customer, customer);
    return RESULT_OK;
}

Result account_update_profile(AppState *state, const Session *session,
                              AccountId account_id,
                              const CustomerProfile *profile)
{
    AccountStore candidate;
    AccountRecord *customer;
    Result result;

    if (state == NULL || session == NULL || profile == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (account_id == 0) {
        return RESULT_INVALID_INPUT;
    }
    if (!valid_text(profile->name, PIZZA_NAME_CAPACITY) ||
        !valid_text(profile->telephone, PIZZA_PHONE_CAPACITY)) {
        return RESULT_INVALID_INPUT;
    }
    if (session->role == SESSION_CUSTOMER) {
        result = require_customer_owner(session, account_id);
    } else {
        result = require_manager(session);
    }
    if (result != RESULT_OK) {
        return result;
    }
    result = clone_store(state, &candidate);
    if (result != RESULT_OK) {
        return result;
    }
    customer = find_customer(&candidate, account_id);
    if (customer == NULL) {
        result = RESULT_NOT_FOUND;
    } else if (session->role == SESSION_CUSTOMER && !customer->active) {
        result = RESULT_ACCOUNT_INACTIVE;
    } else {
        customer->profile = *profile;
        result = account_backend_commit(state, &candidate);
    }
    if (result != RESULT_OK) {
        account_backend_dispose(&candidate);
    }
    return result;
}

Result account_add_customer(AppState *state, const Session *manager,
                            const CustomerRegistration *registration,
                            AccountId *out_account_id)
{
    AccountStore candidate;
    AccountId account_id;
    Result result;

    if (state == NULL || manager == NULL || registration == NULL ||
        out_account_id == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    result = require_manager(manager);
    if (result != RESULT_OK) {
        return result;
    }
    result = validate_registration(registration);
    if (result != RESULT_OK) {
        return result;
    }
    result = clone_store(state, &candidate);
    if (result != RESULT_OK) {
        return result;
    }
    result = add_customer_to_store(&candidate, registration, &account_id);
    if (result == RESULT_OK) {
        result = account_backend_commit(state, &candidate);
    }
    if (result != RESULT_OK) {
        account_backend_dispose(&candidate);
    } else {
        *out_account_id = account_id;
    }
    return result;
}

Result account_set_active(AppState *state, const Session *manager,
                          AccountId account_id, bool active)
{
    AccountStore candidate;
    AccountRecord *customer;
    Result result;

    if (state == NULL || manager == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    if (account_id == 0) {
        return RESULT_INVALID_INPUT;
    }
    result = require_manager(manager);
    if (result != RESULT_OK) {
        return result;
    }
    result = clone_store(state, &candidate);
    if (result != RESULT_OK) {
        return result;
    }
    customer = find_customer(&candidate, account_id);
    if (customer == NULL) {
        result = RESULT_NOT_FOUND;
    } else {
        customer->active = active;
        result = account_backend_commit(state, &candidate);
    }
    if (result != RESULT_OK) {
        account_backend_dispose(&candidate);
    }
    return result;
}

Result account_list_customers(const AppState *state, const Session *manager,
                              bool include_inactive, Customer *out_customers,
                              size_t capacity, size_t *out_count)
{
    const AccountStore *store;
    size_t required = 0;
    size_t i;
    size_t written = 0;
    Result result;

    if (state == NULL || manager == NULL || out_count == NULL) {
        return RESULT_INVALID_ARGUMENT;
    }
    result = require_manager(manager);
    if (result != RESULT_OK) {
        return result;
    }
    if (out_customers == NULL && capacity != 0) {
        return RESULT_INVALID_ARGUMENT;
    }
    result = read_store(state, &store);
    if (result != RESULT_OK) {
        return result;
    }
    for (i = 0; i < store->customer_count; ++i) {
        if (include_inactive || store->customers[i].active) {
            ++required;
        }
    }
    if (out_customers == NULL) {
        *out_count = required;
        return RESULT_OK;
    }
    if (capacity < required) {
        *out_count = required;
        return RESULT_BUFFER_TOO_SMALL;
    }
    for (i = 0; i < store->customer_count; ++i) {
        size_t position;

        if (!include_inactive && !store->customers[i].active) {
            continue;
        }
        position = written;
        while (position > 0 &&
               out_customers[position - 1].account_id >
                   store->customers[i].account_id) {
            out_customers[position] = out_customers[position - 1];
            --position;
        }
        copy_customer(&out_customers[position], &store->customers[i]);
        ++written;
    }
    *out_count = written;
    return RESULT_OK;
}
