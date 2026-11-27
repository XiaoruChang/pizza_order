#include "sales.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static int valid_name(const char name[SALES_NAME_CAPACITY])
{
    const char *end = memchr(name, '\0', SALES_NAME_CAPACITY);
    const char *p;
    if (end == NULL || end == name) {
        return 0;
    }
    for (p = name; p < end; ++p) {
        if ((unsigned char)*p < 32 || (unsigned char)*p == 127) {
            return 0;
        }
    }
    return 1;
}

static int compare_pizza(const void *left, const void *right)
{
    const SalesPizzaTotal *a = left;
    const SalesPizzaTotal *b = right;
    return (a->pizza_id > b->pizza_id) - (a->pizza_id < b->pizza_id);
}

void sales_report_free(SalesReport *report)
{
    if (report != NULL) {
        free(report->pizzas);
        memset(report, 0, sizeof(*report));
    }
}

SalesResult sales_report_build(const SalesOrder *orders, size_t count,
                               SalesReport *out)
{
    SalesReport result = {0};
    size_t capacity = 0;
    size_t i;
    SalesResult error = SALES_OK;
    if (out == NULL || (count != 0 && orders == NULL) ||
        out->pizzas != NULL || out->pizza_count != 0 ||
        out->paid_orders != 0 || out->quantity != 0 || out->revenue_cents != 0) {
        return SALES_INVALID_DATA;
    }
    /* Validate envelopes before allocation; non-paid lines are ignored. */
    for (i = 0; i < count; ++i) {
        size_t j;
        if (orders[i].status < SALES_PENDING || orders[i].status > SALES_REFUNDED) {
            return SALES_INVALID_DATA;
        }
        if (orders[i].status != SALES_PAID) {
            continue;
        }
        if (orders[i].order_id == 0 || orders[i].line_count == 0 ||
            orders[i].lines == NULL) {
            return SALES_INVALID_DATA;
        }
        for (j = 0; j < i; ++j) {
            if (orders[j].status == SALES_PAID &&
                orders[j].order_id == orders[i].order_id) {
                return SALES_INVALID_DATA;
            }
        }
        if (orders[i].line_count > SIZE_MAX - capacity) {
            return SALES_OVERFLOW;
        }
        capacity += orders[i].line_count;
    }
    if (capacity > SIZE_MAX / sizeof(*result.pizzas)) {
        return SALES_OVERFLOW;
    }
    if (capacity != 0) {
        result.pizzas = calloc(capacity, sizeof(*result.pizzas));
        if (result.pizzas == NULL) {
            return SALES_NO_MEMORY;
        }
    }
    for (i = 0; i < count; ++i) {
        size_t j;
        if (orders[i].status != SALES_PAID) {
            continue;
        }
        if (result.paid_orders == UINT64_MAX) {
            error = SALES_OVERFLOW;
            goto fail;
        }
        ++result.paid_orders;
        for (j = 0; j < orders[i].line_count; ++j) {
            const SalesLine *line = &orders[i].lines[j];
            size_t k;
            SalesPizzaTotal *pizza;
            if (line->pizza_id == 0 || !valid_name(line->pizza_name) ||
                line->quantity == 0 || line->revenue_cents < 0) {
                error = SALES_INVALID_DATA;
                goto fail;
            }
            for (k = 0; k < result.pizza_count; ++k) {
                if (result.pizzas[k].pizza_id == line->pizza_id) {
                    break;
                }
            }
            if (k == result.pizza_count) {
                result.pizzas[k].pizza_id = line->pizza_id;
                memcpy(result.pizzas[k].pizza_name, line->pizza_name,
                       SALES_NAME_CAPACITY);
                ++result.pizza_count;
            }
            pizza = &result.pizzas[k];
            if (line->quantity > UINT64_MAX - result.quantity ||
                line->quantity > UINT64_MAX - pizza->quantity ||
                line->revenue_cents > INT64_MAX - result.revenue_cents ||
                line->revenue_cents > INT64_MAX - pizza->revenue_cents) {
                error = SALES_OVERFLOW;
                goto fail;
            }
            result.quantity += line->quantity;
            result.revenue_cents += line->revenue_cents;
            pizza->quantity += line->quantity;
            pizza->revenue_cents += line->revenue_cents;
        }
    }
    if (result.pizza_count > 1) {
        qsort(result.pizzas, result.pizza_count, sizeof(*result.pizzas), compare_pizza);
    }
    *out = result;
    return SALES_OK;

fail:
    sales_report_free(&result);
    return error;
}

static void print_money(FILE *output, int64_t cents)
{
    fprintf(output, "%" PRId64 ".%02" PRId64, cents / 100, cents % 100);
}

int sales_report_print(FILE *output, const SalesReport *report)
{
    size_t i;
    if (output == NULL || report == NULL) {
        return -1;
    }
    fprintf(output, "\n=== Sales statistics (paid orders only) ===\n"
            "Paid orders: %" PRIu64 "\nPizzas sold: %" PRIu64 "\nSales amount: ",
            report->paid_orders, report->quantity);
    print_money(output, report->revenue_cents);
    fputs("\nID | Pizza | Quantity | Sales amount\n", output);
    for (i = 0; i < report->pizza_count; ++i) {
        const SalesPizzaTotal *pizza = &report->pizzas[i];
        fprintf(output, "%" PRIu64 " | %s | %" PRIu64 " | ",
                pizza->pizza_id, pizza->pizza_name, pizza->quantity);
        print_money(output, pizza->revenue_cents);
        fputc('\n', output);
    }
    if (report->paid_orders == 0) {
        fputs("No paid orders.\n", output);
    }
    return ferror(output) ? -1 : 0;
}

const char *sales_result_message(SalesResult result)
{
    switch (result) {
    case SALES_OK: return "OK";
    case SALES_INVALID_DATA: return "Invalid sales snapshot";
    case SALES_OVERFLOW: return "Sales totals exceed supported limits";
    case SALES_NO_MEMORY: return "Insufficient memory for sales statistics";
    default: return "Unknown sales error";
    }
}
