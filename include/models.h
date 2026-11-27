#ifndef PIZZA_MODELS_H
#define PIZZA_MODELS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PIZZA_NAME_CAPACITY 64
#define PIZZA_PHONE_CAPACITY 32
#define PIZZA_LOGIN_CAPACITY 64
#define PIZZA_PASSWORD_MIN_BYTES 8
#define PIZZA_PASSWORD_MAX_BYTES 64
#define PIZZA_MAX_ORDER_LINES 20
#define PIZZA_MAX_LINE_QUANTITY 99

typedef uint64_t AccountId;
typedef uint64_t OrderId;
typedef uint64_t MenuOptionId;
typedef int64_t Money;     /* Integer cents in one agreed currency. */
typedef int64_t Timestamp; /* UTC seconds since 1970-01-01. */

/* Created by storage_open/create, freed by storage_close. Internal collections,
 * credential records, ID counters and the storage path are private. */
typedef struct AppState AppState;

typedef enum {
    SESSION_NONE = 0,
    SESSION_CUSTOMER,
    SESSION_MANAGER
} SessionRole;

/* Obtain from account_login_*; do not manufacture a role in an adapter.
 * Customers have a nonzero account_id; managers use account_id == 0. */
typedef struct {
    AccountId account_id;
    SessionRole role;
} Session;

typedef struct {
    char name[PIZZA_NAME_CAPACITY];
    char telephone[PIZZA_PHONE_CAPACITY];
} CustomerProfile;

/* Public customer data deliberately excludes passwords and password verifiers. */
typedef struct {
    AccountId account_id;
    CustomerProfile profile;
    Money balance_cents;
    bool active;
} Customer;

typedef enum {
    MENU_CRUST = 1,
    MENU_SIZE,
    MENU_TOPPING
} MenuCategory;

/* IDs are globally unique across menu categories and are never reused.
 * price_cents is a crust base price, size surcharge or topping surcharge. */
typedef struct {
    MenuOptionId option_id;
    MenuCategory category;
    char name[PIZZA_NAME_CAPACITY];
    Money price_cents;
    bool available;
} MenuOption;

/* Version 1 chooses exactly one crust, size and topping/flavour per pizza. */
typedef struct {
    MenuOptionId crust_id;
    MenuOptionId size_id;
    MenuOptionId topping_id;
} PizzaConfig;

typedef struct {
    PizzaConfig config;
    uint32_t quantity;
} DraftLine;

/* 1..PIZZA_MAX_ORDER_LINES entries; unused array slots are ignored. */
typedef struct {
    size_t line_count;
    DraftLine lines[PIZZA_MAX_ORDER_LINES];
} OrderDraft;

typedef struct {
    uint64_t menu_revision;
    Money unit_price_cents;
    Money subtotal_cents;
} PriceQuote;

/* Historical snapshots, including names: never recalculate from today's menu.
 * subtotal_cents already includes quantity. */
typedef struct {
    PizzaConfig config;
    char crust_name[PIZZA_NAME_CAPACITY];
    char size_name[PIZZA_NAME_CAPACITY];
    char topping_name[PIZZA_NAME_CAPACITY];
    uint32_t quantity;
    Money unit_price_cents;
    Money subtotal_cents;
} OrderLine;

typedef struct {
    uint64_t menu_revision;
    size_t line_count;
    OrderLine lines[PIZZA_MAX_ORDER_LINES];
    Money total_cents;
} OrderQuote;

typedef enum {
    ORDER_PENDING = 0,
    ORDER_PAID,
    ORDER_CANCELLED,
    ORDER_REFUNDED
} OrderStatus;

/* order_submit creates PAID orders only. Other states are reserved; no cancel
 * or refund operation is provided in version 1. Abandoned drafts are not saved. */
typedef struct {
    OrderId order_id;
    AccountId customer_id;
    Timestamp created_at_utc;
    OrderStatus status;
    size_t line_count;
    OrderLine lines[PIZZA_MAX_ORDER_LINES];
    Money total_cents;
} Order;

#endif
