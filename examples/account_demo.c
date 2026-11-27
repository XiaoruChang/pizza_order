#include "account.h"

#include <stdio.h>

int main(void)
{
    AccountStore store;
    AccountProfile profile;
    AccountId account_id = 0;

    account_store_init(&store);
    if (account_register(&store, "jiabao", "demo-password", "Jiabao Gao",
                         "+65 1234 5678", &account_id) != ACCOUNT_OK) {
        fputs("registration failed\n", stderr);
        account_store_free(&store);
        return 1;
    }
    printf("registered account: %llu\n",
           (unsigned long long)account_id);

    if (account_login(&store, "jiabao", "demo-password", &account_id) !=
        ACCOUNT_OK || account_get_profile(&store, account_id, &profile) !=
        ACCOUNT_OK) {
        fputs("login failed\n", stderr);
        account_store_free(&store);
        return 1;
    }
    printf("logged in as %s (%s), phone %s\n", profile.name,
           profile.username, profile.phone);

    account_store_free(&store);
    return 0;
}
