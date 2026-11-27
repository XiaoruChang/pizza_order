#ifndef PIZZA_SALES_H
#define PIZZA_SALES_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SALES_NAME_CAPACITY 64

typedef enum {
    SALES_PENDING = 0,
    SALES_PAID,
    SALES_CANCELLED,
    SALES_REFUNDED
} SalesOrderStatus;

/* Historical read-only projection, not the order module's storage model.
 * revenue_cents is the actual charged line subtotal, including its quantity.
 * All lines must use the same currency with two decimal places. */
typedef struct {
    uint64_t pizza_id;
    char pizza_name[SALES_NAME_CAPACITY];
    uint64_t quantity;
    int64_t revenue_cents;
} SalesLine;

typedef struct {
    uint64_t order_id;
    SalesOrderStatus status;
    const SalesLine *lines;
    size_t line_count;
} SalesOrder;

typedef struct {
    uint64_t pizza_id;
    char pizza_name[SALES_NAME_CAPACITY];
    uint64_t quantity;
    int64_t revenue_cents;
} SalesPizzaTotal;

typedef struct {
    uint64_t paid_orders;
    uint64_t quantity;
    int64_t revenue_cents;
    SalesPizzaTotal *pizzas;
    size_t pizza_count;
} SalesReport;

typedef enum {
    SALES_OK = 0,
    SALES_INVALID_DATA,
    SALES_OVERFLOW,
    SALES_NO_MEMORY
} SalesResult;

/* Initialize out to {0}; free it before rebuilding. Failure leaves out unchanged.
 * Only PAID orders contribute. Repeated paid order IDs are rejected.
 * Repeated pizza IDs aggregate using the first historical name encountered. */
SalesResult sales_report_build(const SalesOrder *orders, size_t count,
                               SalesReport *out);
void sales_report_free(SalesReport *report);
/* Returns zero on success; report must have been built successfully. */
int sales_report_print(FILE *output, const SalesReport *report);
const char *sales_result_message(SalesResult result);

#endif
