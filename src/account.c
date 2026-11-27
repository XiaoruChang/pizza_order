#include "account.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct AccountRecord {
    AccountId id;
    char username[ACCOUNT_USERNAME_CAPACITY];
    char name[ACCOUNT_NAME_CAPACITY];
    char phone[ACCOUNT_PHONE_CAPACITY];
    uint64_t salt;
    uint64_t password_hash;
    bool active;
};

static size_t text_length(const char *text, size_t capacity)
{
    size_t length = 0;

    if (text == NULL) {
        return capacity;
    }
    while (length < capacity && text[length] != '\0') {
        ++length;
    }
    return length;
}

static bool valid_text(const char *text, size_t capacity)
{
    size_t i;
    size_t length = text_length(text, capacity);

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

static bool valid_username(const char *username)
{
    size_t i;

    if (!valid_text(username, ACCOUNT_USERNAME_CAPACITY)) {
        return false;
    }
    for (i = 0; username[i] != '\0'; ++i) {
        if (isalnum((unsigned char)username[i]) == 0 && username[i] != '_' &&
            username[i] != '-' && username[i] != '.') {
            return false;
        }
    }
    return true;
}

static bool valid_phone(const char *phone)
{
    size_t i;

    if (!valid_text(phone, ACCOUNT_PHONE_CAPACITY)) {
        return false;
    }
    for (i = 0; phone[i] != '\0'; ++i) {
        if (isdigit((unsigned char)phone[i]) == 0 && phone[i] != '+' &&
            phone[i] != '-' && phone[i] != '(' && phone[i] != ')' &&
            phone[i] != ' ') {
            return false;
        }
    }
    return true;
}

static bool valid_password(const char *password)
{
    return valid_text(password, ACCOUNT_PASSWORD_CAPACITY);
}

static void copy_text(char *destination, const char *source)
{
    size_t length = strlen(source);
    memcpy(destination, source, length + 1);
}

/* A compact salted digest keeps plaintext passwords out of AccountRecord.
 * A deployed service should replace this educational digest with Argon2id,
 * scrypt, or another password-specific KDF supplied by its platform. */
static uint64_t password_digest(const char *password, uint64_t salt)
{
    uint64_t hash = UINT64_C(1469598103934665603) ^ salt;
    size_t i;

    for (i = 0; password[i] != '\0'; ++i) {
        hash ^= (unsigned char)password[i];
        hash *= UINT64_C(1099511628211);
        hash ^= hash >> 29;
    }
    hash ^= hash >> 33;
    hash *= UINT64_C(0xff51afd7ed558ccd);
    hash ^= hash >> 33;
    return hash;
}

static AccountRecord *find_record(AccountStore *store, AccountId id)
{
    size_t i;

    for (i = 0; i < store->count; ++i) {
        if (store->records[i].id == id) {
            return &store->records[i];
        }
    }
    return NULL;
}

static const AccountRecord *find_record_const(const AccountStore *store,
                                              AccountId id)
{
    size_t i;

    for (i = 0; i < store->count; ++i) {
        if (store->records[i].id == id) {
            return &store->records[i];
        }
    }
    return NULL;
}

static const AccountRecord *find_username(const AccountStore *store,
                                          const char *username)
{
    size_t i;

    for (i = 0; i < store->count; ++i) {
        if (strcmp(store->records[i].username, username) == 0) {
            return &store->records[i];
        }
    }
    return NULL;
}

static AccountResult reserve_record(AccountStore *store)
{
    size_t new_capacity;
    AccountRecord *records;

    if (store->count < store->capacity) {
        return ACCOUNT_OK;
    }
    if (store->capacity == 0) {
        new_capacity = 8;
    } else {
        if (store->capacity > SIZE_MAX / 2) {
            return ACCOUNT_OUT_OF_MEMORY;
        }
        new_capacity = store->capacity * 2;
    }
    records = realloc(store->records, new_capacity * sizeof(*records));
    if (records == NULL) {
        return ACCOUNT_OUT_OF_MEMORY;
    }
    store->records = records;
    store->capacity = new_capacity;
    return ACCOUNT_OK;
}

void account_store_init(AccountStore *store)
{
    if (store == NULL) {
        return;
    }
    store->records = NULL;
    store->count = 0;
    store->capacity = 0;
    store->next_id = UINT64_C(1000001);
    store->next_salt = UINT64_C(0x9e3779b97f4a7c15);
}

void account_store_free(AccountStore *store)
{
    size_t i;

    if (store == NULL) {
        return;
    }
    for (i = 0; i < store->count; ++i) {
        memset(&store->records[i], 0, sizeof(store->records[i]));
    }
    free(store->records);
    account_store_init(store);
}

static AccountResult validate_registration(const char *username,
                                           const char *password,
                                           const char *name,
                                           const char *phone)
{
    if (!valid_username(username)) {
        return ACCOUNT_INVALID_USERNAME;
    }
    if (!valid_password(password)) {
        return ACCOUNT_INVALID_PASSWORD;
    }
    if (!valid_text(name, ACCOUNT_NAME_CAPACITY)) {
        return ACCOUNT_INVALID_NAME;
    }
    if (!valid_phone(phone)) {
        return ACCOUNT_INVALID_PHONE;
    }
    return ACCOUNT_OK;
}

AccountResult account_register(AccountStore *store,
                               const char *username,
                               const char *password,
                               const char *name,
                               const char *phone,
                               AccountId *account_id)
{
    AccountRecord *record;
    AccountResult result;

    if (store == NULL || account_id == NULL) {
        return ACCOUNT_INVALID_ARGUMENT;
    }
    result = validate_registration(username, password, name, phone);
    if (result != ACCOUNT_OK) {
        return result;
    }
    if (find_username(store, username) != NULL) {
        return ACCOUNT_DUPLICATE_USERNAME;
    }
    if (store->next_id == 0 || store->next_salt == 0) {
        return ACCOUNT_ID_EXHAUSTED;
    }
    result = reserve_record(store);
    if (result != ACCOUNT_OK) {
        return result;
    }
    record = &store->records[store->count];
    record->id = store->next_id++;
    record->salt = store->next_salt++;
    record->password_hash = password_digest(password, record->salt);
    copy_text(record->username, username);
    copy_text(record->name, name);
    copy_text(record->phone, phone);
    record->active = true;
    ++store->count;
    *account_id = record->id;
    return ACCOUNT_OK;
}

AccountResult account_login(const AccountStore *store,
                            const char *username,
                            const char *password,
                            AccountId *account_id)
{
    const AccountRecord *record;

    if (store == NULL || account_id == NULL || !valid_username(username) ||
        !valid_password(password)) {
        return ACCOUNT_INVALID_ARGUMENT;
    }
    record = find_username(store, username);
    if (record == NULL || !record->active ||
        record->password_hash != password_digest(password, record->salt)) {
        *account_id = 0;
        return ACCOUNT_AUTH_FAILED;
    }
    *account_id = record->id;
    return ACCOUNT_OK;
}

AccountResult account_get_profile(const AccountStore *store,
                                  AccountId account_id,
                                  AccountProfile *profile)
{
    const AccountRecord *record;

    if (store == NULL || profile == NULL || account_id == 0) {
        return ACCOUNT_INVALID_ARGUMENT;
    }
    record = find_record_const(store, account_id);
    if (record == NULL || !record->active) {
        return ACCOUNT_NOT_FOUND;
    }
    profile->id = record->id;
    copy_text(profile->username, record->username);
    copy_text(profile->name, record->name);
    copy_text(profile->phone, record->phone);
    profile->active = record->active;
    return ACCOUNT_OK;
}

AccountResult account_update_profile(AccountStore *store,
                                     AccountId account_id,
                                     const char *name,
                                     const char *phone)
{
    AccountRecord *record;

    if (store == NULL || account_id == 0) {
        return ACCOUNT_INVALID_ARGUMENT;
    }
    if (!valid_text(name, ACCOUNT_NAME_CAPACITY)) {
        return ACCOUNT_INVALID_NAME;
    }
    if (!valid_phone(phone)) {
        return ACCOUNT_INVALID_PHONE;
    }
    record = find_record(store, account_id);
    if (record == NULL || !record->active) {
        return ACCOUNT_NOT_FOUND;
    }
    copy_text(record->name, name);
    copy_text(record->phone, phone);
    return ACCOUNT_OK;
}

AccountResult account_manager_add_customer(AccountStore *store,
                                           const char *username,
                                           const char *password,
                                           const char *name,
                                           const char *phone,
                                           AccountId *account_id)
{
    return account_register(store, username, password, name, phone, account_id);
}

AccountResult account_manager_update_customer(AccountStore *store,
                                              AccountId account_id,
                                              const char *name,
                                              const char *phone)
{
    return account_update_profile(store, account_id, name, phone);
}

AccountResult account_manager_delete_customer(AccountStore *store,
                                              AccountId account_id)
{
    AccountRecord *record;

    if (store == NULL || account_id == 0) {
        return ACCOUNT_INVALID_ARGUMENT;
    }
    record = find_record(store, account_id);
    if (record == NULL) {
        return ACCOUNT_NOT_FOUND;
    }
    if (!record->active) {
        return ACCOUNT_ALREADY_INACTIVE;
    }
    record->active = false;
    record->password_hash = 0;
    record->salt = 0;
    return ACCOUNT_OK;
}

const char *account_result_message(AccountResult result)
{
    switch (result) {
    case ACCOUNT_OK:
        return "Operation completed";
    case ACCOUNT_INVALID_ARGUMENT:
        return "Invalid argument";
    case ACCOUNT_INVALID_USERNAME:
        return "Username must use letters, digits, '.', '-' or '_'";
    case ACCOUNT_INVALID_NAME:
        return "Name is empty, too long, or contains a control character";
    case ACCOUNT_INVALID_PHONE:
        return "Phone contains unsupported characters";
    case ACCOUNT_INVALID_PASSWORD:
        return "Password is empty, too long, or contains a control character";
    case ACCOUNT_DUPLICATE_USERNAME:
        return "Username already exists";
    case ACCOUNT_AUTH_FAILED:
        return "Username or password is incorrect";
    case ACCOUNT_NOT_FOUND:
        return "Customer account was not found";
    case ACCOUNT_ALREADY_INACTIVE:
        return "Customer account is already inactive";
    case ACCOUNT_OUT_OF_MEMORY:
        return "Unable to allocate account storage";
    case ACCOUNT_ID_EXHAUSTED:
        return "No more account identifiers are available";
    default:
        return "Unknown account error";
    }
}
