#ifndef PIZZA_ACCOUNT_H
#define PIZZA_ACCOUNT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ACCOUNT_USERNAME_CAPACITY 32
#define ACCOUNT_NAME_CAPACITY 64
#define ACCOUNT_PHONE_CAPACITY 32
#define ACCOUNT_PASSWORD_CAPACITY 128

typedef uint64_t AccountId;

typedef enum {
    ACCOUNT_OK = 0,
    ACCOUNT_INVALID_ARGUMENT,
    ACCOUNT_INVALID_USERNAME,
    ACCOUNT_INVALID_NAME,
    ACCOUNT_INVALID_PHONE,
    ACCOUNT_INVALID_PASSWORD,
    ACCOUNT_DUPLICATE_USERNAME,
    ACCOUNT_AUTH_FAILED,
    ACCOUNT_NOT_FOUND,
    ACCOUNT_ALREADY_INACTIVE,
    ACCOUNT_OUT_OF_MEMORY,
    ACCOUNT_ID_EXHAUSTED
} AccountResult;

typedef struct {
    AccountId id;
    char username[ACCOUNT_USERNAME_CAPACITY];
    char name[ACCOUNT_NAME_CAPACITY];
    char phone[ACCOUNT_PHONE_CAPACITY];
    bool active;
} AccountProfile;

typedef struct AccountRecord AccountRecord;

typedef struct {
    AccountRecord *records;
    size_t count;
    size_t capacity;
    AccountId next_id;
    uint64_t next_salt;
} AccountStore;

void account_store_init(AccountStore *store);
void account_store_free(AccountStore *store);

AccountResult account_register(AccountStore *store,
                               const char *username,
                               const char *password,
                               const char *name,
                               const char *phone,
                               AccountId *account_id);

AccountResult account_login(const AccountStore *store,
                            const char *username,
                            const char *password,
                            AccountId *account_id);

AccountResult account_get_profile(const AccountStore *store,
                                  AccountId account_id,
                                  AccountProfile *profile);

AccountResult account_update_profile(AccountStore *store,
                                     AccountId account_id,
                                     const char *name,
                                     const char *phone);

AccountResult account_manager_add_customer(AccountStore *store,
                                           const char *username,
                                           const char *password,
                                           const char *name,
                                           const char *phone,
                                           AccountId *account_id);

AccountResult account_manager_update_customer(AccountStore *store,
                                              AccountId account_id,
                                              const char *name,
                                              const char *phone);

AccountResult account_manager_delete_customer(AccountStore *store,
                                              AccountId account_id);

const char *account_result_message(AccountResult result);

#endif
