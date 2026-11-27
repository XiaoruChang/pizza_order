#include "app.h"

static const SalesLine first_lines[] = {
    {2, "Hawaiian", 2, 6500},
    {1, "Margherita", 1, 2500}
};
static const SalesLine second_lines[] = {
    {1, "Margherita", 2, 6150}
};
static const SalesOrder orders[] = {
    {101, SALES_PAID, first_lines, 2},
    {102, SALES_PAID, second_lines, 1},
    {103, SALES_PENDING, NULL, 0},
    {104, SALES_CANCELLED, NULL, 0},
    {105, SALES_REFUNDED, second_lines, 1}
};

static int demo_customer(void *context, FILE *input, FILE *output,
                          AppSession *session)
{
    (void)context;
    (void)input;
    fputs("DEMO customer session: account 1001, no real authentication.\n", output);
    session->account_id = 1001;
    return 0;
}

static int demo_manager(void *context, FILE *input, FILE *output)
{
    (void)context;
    (void)input;
    fputs("DEMO manager session: no real authentication.\n", output);
    return 0;
}

static int demo_load(void *context, const SalesOrder **snapshot, size_t *count)
{
    (void)context;
    *snapshot = orders;
    *count = sizeof(orders) / sizeof(orders[0]);
    return 0;
}

static void demo_release(void *context, const SalesOrder *snapshot, size_t count)
{
    (void)context;
    (void)snapshot;
    (void)count; /* Static demo storage: nothing to free. */
}

int main(void)
{
    AppServices services = {0};
    services.login_customer = demo_customer;
    services.login_manager = demo_manager;
    services.load_orders = demo_load;
    services.release_orders = demo_release;
    puts("DEMO ONLY: fixed in-memory sales; no real accounts or persistence.");
    return app_run(stdin, stdout, &services);
}
