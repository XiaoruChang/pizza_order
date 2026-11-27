#include "account.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int test_registration_login_and_profile(void)
{
    AccountStore store;
    AccountProfile profile;
    AccountId id = 0;
    AccountId second_id = 0;

    account_store_init(&store);
    CHECK(account_register(&store, "jiabao", "secret-1", "Jiabao Gao",
                           "+65 1234 5678", &id) == ACCOUNT_OK);
    CHECK(id != 0);
    CHECK(account_register(&store, "jiabao", "secret-2", "Another",
                           "123", &second_id) == ACCOUNT_DUPLICATE_USERNAME);
    CHECK(account_login(&store, "jiabao", "wrong", &second_id) ==
          ACCOUNT_AUTH_FAILED);
    CHECK(second_id == 0);
    CHECK(account_login(&store, "jiabao", "secret-1", &second_id) ==
          ACCOUNT_OK);
    CHECK(second_id == id);
    CHECK(account_get_profile(&store, id, &profile) == ACCOUNT_OK);
    CHECK(strcmp(profile.name, "Jiabao Gao") == 0);
    CHECK(strcmp(profile.phone, "+65 1234 5678") == 0);
    CHECK(account_update_profile(&store, id, "Jiabao G.", "12345678") ==
          ACCOUNT_OK);
    CHECK(account_get_profile(&store, id, &profile) == ACCOUNT_OK);
    CHECK(strcmp(profile.name, "Jiabao G.") == 0);
    CHECK(strcmp(profile.phone, "12345678") == 0);
    account_store_free(&store);
    return 0;
}

static int test_manager_lifecycle(void)
{
    AccountStore store;
    AccountProfile profile;
    AccountId id = 0;

    account_store_init(&store);
    CHECK(account_manager_add_customer(&store, "customer2", "password2",
                                       "Customer Two", "555-0100", &id) ==
          ACCOUNT_OK);
    CHECK(account_manager_update_customer(&store, id, "Updated Customer",
                                          "555-0101") == ACCOUNT_OK);
    CHECK(account_get_profile(&store, id, &profile) == ACCOUNT_OK);
    CHECK(strcmp(profile.name, "Updated Customer") == 0);
    CHECK(account_manager_delete_customer(&store, id) == ACCOUNT_OK);
    CHECK(account_login(&store, "customer2", "password2", &id) ==
          ACCOUNT_AUTH_FAILED);
    CHECK(account_get_profile(&store, profile.id, &profile) ==
          ACCOUNT_NOT_FOUND);
    CHECK(account_manager_delete_customer(&store, profile.id) ==
          ACCOUNT_ALREADY_INACTIVE);
    account_store_free(&store);
    return 0;
}

static int test_validation_and_cleanup(void)
{
    AccountStore store;
    AccountId id = 0;
    char long_name[ACCOUNT_NAME_CAPACITY + 1];

    memset(long_name, 'x', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    account_store_init(&store);
    CHECK(account_register(&store, "bad name", "password", "Name", "123",
                           &id) == ACCOUNT_INVALID_USERNAME);
    CHECK(account_register(&store, "valid", "password", long_name, "123",
                           &id) == ACCOUNT_INVALID_NAME);
    CHECK(account_register(&store, "valid", "password", "Name", "abc",
                           &id) == ACCOUNT_INVALID_PHONE);
    CHECK(account_register(&store, "valid", "password", "Name", "123",
                           &id) == ACCOUNT_OK);
    CHECK(strcmp(account_result_message(ACCOUNT_AUTH_FAILED),
                 "Username or password is incorrect") == 0);
    account_store_free(&store);
    CHECK(store.records == NULL && store.count == 0);
    return 0;
}

int main(void)
{
    CHECK(test_registration_login_and_profile() == 0);
    CHECK(test_manager_lifecycle() == 0);
    CHECK(test_validation_and_cleanup() == 0);
    puts("account: 3 scenario groups passed");
    return 0;
}
