#include "sales.h"
#include "test_support.h"

static void test_totals_and_historical_prices(void)
{
    const SalesLine first[] = {
        {9, "Hawaiian", 2, 6500}, {3, "Margherita", 1, 2500}
    };
    const SalesLine second[] = {
        {3, "Renamed pizza", 2, 6150}, {9, "Hawaiian", 1, 1}
    };
    const SalesOrder orders[] = {
        {10, SALES_PAID, first, 2}, {11, SALES_PAID, second, 2},
        {12, SALES_PENDING, first, 2}, {13, SALES_CANCELLED, first, 2},
        {14, SALES_REFUNDED, first, 2}
    };
    SalesReport report = {0};
    FILE *output = tmpfile();
    char text[4096];
    CHECK(output != NULL);
    CHECK(sales_report_build(orders, 5, &report) == SALES_OK);
    CHECK(report.paid_orders == 2);
    CHECK(report.quantity == 6);
    CHECK(report.revenue_cents == 15151);
    CHECK(report.pizza_count == 2);
    CHECK(report.pizzas[0].pizza_id == 3); /* Stable ascending ID order. */
    CHECK(report.pizzas[0].quantity == 3);
    CHECK(report.pizzas[0].revenue_cents == 8650);
    CHECK(strcmp(report.pizzas[0].pizza_name, "Margherita") == 0);
    CHECK(report.pizzas[1].pizza_id == 9);
    CHECK(report.pizzas[1].quantity == 3);
    CHECK(report.pizzas[1].revenue_cents == 6501);
    CHECK(sales_report_print(output, &report) == 0);
    read_output(output, text, sizeof(text));
    CHECK(strstr(text, "Paid orders: 2\nPizzas sold: 6\nSales amount: 151.51") != NULL);
    CHECK(strstr(text, "3 | Margherita | 3 | 86.50") != NULL);
    CHECK(strstr(text, "9 | Hawaiian | 3 | 65.01") != NULL);
    CHECK(sales_report_build(orders, 5, &report) == SALES_INVALID_DATA);
    CHECK(report.revenue_cents == 15151); /* Existing output is retained. */
    sales_report_free(&report);
    CHECK(report.pizzas == NULL && report.quantity == 0);
    sales_report_free(&report);
    CHECK(fclose(output) == 0);
}

static void test_empty_and_ignored_lines(void)
{
    SalesReport report = {0};
    const SalesOrder orders[] = {
        {0, SALES_PENDING, NULL, 99}, {0, SALES_CANCELLED, NULL, 1},
        {0, SALES_REFUNDED, NULL, 42}
    };
    FILE *output = tmpfile();
    char text[1024];
    CHECK(output != NULL);
    CHECK(sales_report_build(NULL, 0, &report) == SALES_OK);
    CHECK(report.pizzas == NULL && report.paid_orders == 0);
    CHECK(sales_report_print(output, &report) == 0);
    read_output(output, text, sizeof(text));
    CHECK(strstr(text, "Sales amount: 0.00") != NULL);
    CHECK(strstr(text, "No paid orders.") != NULL);
    CHECK(sales_report_build(orders, 3, &report) == SALES_OK);
    CHECK(report.paid_orders == 0 && report.revenue_cents == 0);
    sales_report_free(&report);
    CHECK(fclose(output) == 0);
}

static void expect_failure(const SalesOrder *orders, size_t count, SalesResult expected)
{
    SalesReport report = {0};
    CHECK(sales_report_build(orders, count, &report) == expected);
    CHECK(report.pizzas == NULL && report.pizza_count == 0);
    CHECK(report.paid_orders == 0 && report.quantity == 0 && report.revenue_cents == 0);
    sales_report_free(&report);
}

static void test_invalid_snapshots(void)
{
    SalesLine line = {1, "Pizza", 1, 2000};
    SalesOrder order = {1, SALES_PAID, &line, 1};
    SalesOrder duplicates[2] = {order, order};
    expect_failure(NULL, 1, SALES_INVALID_DATA);
    CHECK(sales_report_build(&order, 1, NULL) == SALES_INVALID_DATA);
    expect_failure(duplicates, 2, SALES_INVALID_DATA);
    order.order_id = 0;
    expect_failure(&order, 1, SALES_INVALID_DATA);
    order.order_id = 1;
    order.status = (SalesOrderStatus)99;
    expect_failure(&order, 1, SALES_INVALID_DATA);
    order.status = SALES_PAID;
    order.lines = NULL;
    expect_failure(&order, 1, SALES_INVALID_DATA);
    order.lines = &line;
    order.line_count = 0;
    expect_failure(&order, 1, SALES_INVALID_DATA);
    order.line_count = 1;
    line.pizza_id = 0;
    expect_failure(&order, 1, SALES_INVALID_DATA);
    line.pizza_id = 1;
    line.quantity = 0;
    expect_failure(&order, 1, SALES_INVALID_DATA);
    line.quantity = 1;
    line.revenue_cents = -1;
    expect_failure(&order, 1, SALES_INVALID_DATA);
    line.revenue_cents = 1;
    line.pizza_name[0] = '\0';
    expect_failure(&order, 1, SALES_INVALID_DATA);
    strcpy(line.pizza_name, "Bad\nname");
    expect_failure(&order, 1, SALES_INVALID_DATA);
    memset(line.pizza_name, 'x', sizeof(line.pizza_name));
    expect_failure(&order, 1, SALES_INVALID_DATA);
    CHECK(sales_report_print(NULL, NULL) != 0);
    sales_report_free(NULL);
}

static void test_limits(void)
{
    SalesLine lines[] = {{1, "Pizza", UINT64_MAX, INT64_MAX}, {1, "Pizza", 1, 1}};
    SalesOrder orders[] = {{1, SALES_PAID, lines, 1}, {2, SALES_PAID, lines + 1, 1}};
    SalesReport report = {0};
    FILE *output = tmpfile();
    char text[2048];
    CHECK(output != NULL);
    CHECK(sales_report_build(orders, 1, &report) == SALES_OK);
    CHECK(report.quantity == UINT64_MAX && report.revenue_cents == INT64_MAX);
    CHECK(sales_report_print(output, &report) == 0);
    read_output(output, text, sizeof(text));
    CHECK(strstr(text, "Sales amount: 92233720368547758.07") != NULL);
    sales_report_free(&report);
    expect_failure(orders, 2, SALES_OVERFLOW);
    lines[0].quantity = 1; /* Isolate the money overflow check. */
    expect_failure(orders, 2, SALES_OVERFLOW);
    lines[0].revenue_cents = 0;
    lines[0].quantity = UINT64_MAX; /* Isolate the quantity overflow check. */
    expect_failure(orders, 2, SALES_OVERFLOW);
    lines[1].pizza_id = 2; /* Overflow across distinct pizza types. */
    expect_failure(orders, 2, SALES_OVERFLOW);
    orders[0].line_count = SIZE_MAX;
    expect_failure(orders, 1, SALES_OVERFLOW);
    expect_failure(orders, 2, SALES_OVERFLOW);
    CHECK(fclose(output) == 0);
}

static void test_owned_names_and_free_items(void)
{
    SalesLine line = {1, "Gift", 1, 0};
    SalesOrder order = {1, SALES_PAID, &line, 1};
    SalesReport report = {0};
    CHECK(sales_report_build(&order, 1, &report) == SALES_OK);
    strcpy(line.pizza_name, "Changed");
    CHECK(strcmp(report.pizzas[0].pizza_name, "Gift") == 0);
    CHECK(report.quantity == 1 && report.revenue_cents == 0);
    sales_report_free(&report);
    CHECK(strcmp(sales_result_message(SALES_OVERFLOW),
                 "Sales totals exceed supported limits") == 0);
    CHECK(strcmp(sales_result_message(SALES_NO_MEMORY),
                 "Insufficient memory for sales statistics") == 0);
}

int main(void)
{
    test_totals_and_historical_prices();
    test_empty_and_ignored_lines();
    test_invalid_snapshots();
    test_limits();
    test_owned_names_and_free_items();
    puts("sales: 5 scenario groups passed");
    return 0;
}
