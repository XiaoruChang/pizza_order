#include "app.h"
#include "test_support.h"

typedef struct {
    int registrations;
    int customer_logins;
    int manager_logins;
    int customer_actions[5];
    int manager_actions[2];
    int deny_customer;
    int deny_manager;
    int zero_account;
    int omit_second_account;
    int fail_action;
    int loads;
    int releases;
    int load_error;
    int invalid_snapshot;
    int empty_snapshot;
    SalesLine line;
    SalesOrder order;
} Fake;

static void init_fake(Fake *fake)
{
    memset(fake, 0, sizeof(*fake));
    fake->line.pizza_id = 1;
    strcpy(fake->line.pizza_name, "Test pizza");
    fake->line.quantity = 1;
    fake->line.revenue_cents = 1000;
    fake->order.order_id = 1;
    fake->order.status = SALES_PAID;
    fake->order.lines = &fake->line;
    fake->order.line_count = 1;
}

static int register_customer(void *context, FILE *input, FILE *output)
{
    Fake *fake = context;
    (void)input;
    (void)output;
    ++fake->registrations;
    return fake->fail_action;
}

static int login_customer(void *context, FILE *input, FILE *output, AppSession *session)
{
    Fake *fake = context;
    (void)input;
    (void)output;
    CHECK(session->account_id == 0); /* No identity survives a previous login. */
    ++fake->customer_logins;
    if (!fake->zero_account && !(fake->omit_second_account && fake->customer_logins > 1)) {
        session->account_id = 42;
    }
    return fake->deny_customer;
}

static int login_manager(void *context, FILE *input, FILE *output)
{
    Fake *fake = context;
    (void)input;
    (void)output;
    ++fake->manager_logins;
    return fake->deny_manager;
}

static int customer_action(void *context, const AppSession *session, int index)
{
    Fake *fake = context;
    CHECK(session->account_id == 42);
    ++fake->customer_actions[index];
    if (index == 0 && !fake->fail_action) {
        fake->line.revenue_cents += 250; /* Simulate newly committed order data. */
    }
    return fake->fail_action;
}

#define CUSTOMER_HANDLER(name, index) \
static int name(void *context, const AppSession *session, FILE *input, FILE *output) \
{ \
    (void)input; (void)output; \
    return customer_action(context, session, index); \
}
CUSTOMER_HANDLER(place_order, 0)
CUSTOMER_HANDLER(show_balance, 1)
CUSTOMER_HANDLER(deposit, 2)
CUSTOMER_HANDLER(order_history, 3)
CUSTOMER_HANDLER(edit_profile, 4)

static int manage_menu(void *context, FILE *input, FILE *output)
{
    Fake *fake = context;
    (void)input;
    (void)output;
    ++fake->manager_actions[0];
    return fake->fail_action;
}

static int manage_customers(void *context, FILE *input, FILE *output)
{
    Fake *fake = context;
    (void)input;
    (void)output;
    ++fake->manager_actions[1];
    return fake->fail_action;
}

static int load_orders(void *context, const SalesOrder **orders, size_t *count)
{
    Fake *fake = context;
    ++fake->loads;
    CHECK(*orders == NULL && *count == 0);
    if (fake->load_error) {
        return 1;
    }
    *orders = fake->empty_snapshot ? NULL : &fake->order;
    *count = fake->empty_snapshot ? 0 : 1;
    if (fake->invalid_snapshot) {
        *orders = NULL;
        *count = 1;
    }
    return 0;
}

static void release_orders(void *context, const SalesOrder *orders, size_t count)
{
    Fake *fake = context;
    ++fake->releases;
    CHECK(count == (fake->empty_snapshot ? 0U : 1U));
    CHECK(orders == ((fake->invalid_snapshot || fake->empty_snapshot) ? NULL : &fake->order));
}

static AppServices services_for(Fake *fake)
{
    AppServices services = {0};
    services.context = fake;
    services.register_customer = register_customer;
    services.login_customer = login_customer;
    services.login_manager = login_manager;
    services.place_order = place_order;
    services.show_balance = show_balance;
    services.deposit = deposit;
    services.order_history = order_history;
    services.edit_profile = edit_profile;
    services.manage_menu = manage_menu;
    services.manage_customers = manage_customers;
    services.load_orders = load_orders;
    services.release_orders = release_orders;
    return services;
}

static void run_script(const AppServices *services, const char *script,
                        char *text, size_t capacity)
{
    FILE *input = tmpfile();
    FILE *output = tmpfile();
    CHECK(input != NULL && output != NULL);
    CHECK(fputs(script, input) >= 0);
    rewind(input);
    CHECK(app_run(input, output, services) == 0);
    read_output(output, text, capacity);
    CHECK(fclose(input) == 0);
    CHECK(fclose(output) == 0);
}

static size_t occurrences(const char *text, const char *needle)
{
    size_t count = 0;
    while ((text = strstr(text, needle)) != NULL) {
        ++count;
        text += strlen(needle);
    }
    return count;
}

static void test_routing(void)
{
    Fake fake;
    AppServices services;
    char text[32768];
    size_t i;
    init_fake(&fake);
    services = services_for(&fake);
    run_script(&services, "3\n1\n1\n2\n3\n4\n5\n0\n2\n1\n2\n3\n0\n0\n", text, sizeof(text));
    CHECK(fake.registrations == 1 && fake.customer_logins == 1 && fake.manager_logins == 1);
    for (i = 0; i < 5; ++i) {
        CHECK(fake.customer_actions[i] == 1);
    }
    CHECK(fake.manager_actions[0] == 1 && fake.manager_actions[1] == 1);
    CHECK(fake.loads == 1 && fake.releases == 1);
    CHECK(strstr(text, "Sales amount: 12.50") != NULL);
    CHECK(strstr(text, "Operation failed") == NULL);
}

static void test_authentication_and_role_isolation(void)
{
    Fake fake;
    AppServices services;
    char text[32768];
    init_fake(&fake);
    services = services_for(&fake);
    fake.deny_customer = 1;
    fake.deny_manager = 1;
    run_script(&services, "1\n2\n0\n", text, sizeof(text));
    CHECK(strstr(text, "Customer login failed.") != NULL);
    CHECK(strstr(text, "Manager login failed.") != NULL);
    CHECK(strstr(text, "=== Customer menu") == NULL);
    CHECK(strstr(text, "=== Manager menu") == NULL);
    CHECK(fake.loads == 0 && fake.customer_actions[0] == 0);
    fake.deny_customer = 0;
    fake.zero_account = 1;
    run_script(&services, "1\n0\n", text, sizeof(text));
    CHECK(strstr(text, "Customer login failed.") != NULL);
    fake.zero_account = 0;
    run_script(&services, "1\n3\n0\n0\n", text, sizeof(text));
    CHECK(fake.customer_actions[2] == 1); /* Customer 3 is deposit, never sales. */
    CHECK(fake.loads == 0 && fake.manager_actions[0] == 0);
    CHECK(strstr(text, "Sales statistics") == NULL);
    init_fake(&fake);
    fake.omit_second_account = 1;
    run_script(&services, "1\n0\n1\n0\n", text, sizeof(text));
    CHECK(fake.customer_logins == 2);
    CHECK(occurrences(text, "=== Customer menu") == 1);
    CHECK(strstr(text, "Customer login failed.") != NULL);
}

static void test_missing_modules(void)
{
    Fake fake;
    AppServices services = {0};
    char text[32768];
    run_script(&services, "1\n2\n3\n0\n", text, sizeof(text));
    CHECK(occurrences(text, "Module not connected.") == 3);
    CHECK(strstr(text, "=== Manager menu") == NULL);
    init_fake(&fake);
    services.context = &fake;
    services.login_customer = login_customer;
    services.login_manager = login_manager;
    run_script(&services, "1\n1\n2\n3\n4\n5\n0\n2\n1\n2\n3\n0\n0\n", text, sizeof(text));
    CHECK(occurrences(text, "Module not connected.") == 8);
    services.load_orders = load_orders;
    run_script(&services, "2\n3\n0\n0\n", text, sizeof(text));
    CHECK(fake.loads == 0); /* Never acquire a snapshot without a release hook. */
    CHECK(strstr(text, "Module not connected.") != NULL);
}

static void test_snapshot_lifecycle_and_refresh(void)
{
    Fake fake;
    AppServices services;
    char text[32768];
    init_fake(&fake);
    services = services_for(&fake);
    run_script(&services, "2\n3\n0\n1\n1\n0\n2\n3\n0\n0\n", text, sizeof(text));
    CHECK(fake.loads == 2 && fake.releases == 2);
    CHECK(strstr(text, "Sales amount: 10.00") != NULL);
    CHECK(strstr(text, "Sales amount: 12.50") != NULL);
    init_fake(&fake);
    fake.invalid_snapshot = 1;
    run_script(&services, "2\n3\n0\n0\n", text, sizeof(text));
    CHECK(fake.loads == 1 && fake.releases == 1);
    CHECK(strstr(text, "Invalid sales snapshot.") != NULL);
    CHECK(strstr(text, "Sales amount:") == NULL);
    init_fake(&fake);
    fake.load_error = 1;
    run_script(&services, "2\n3\n0\n0\n", text, sizeof(text));
    CHECK(fake.loads == 1 && fake.releases == 0);
    CHECK(strstr(text, "Unable to load sales snapshot.") != NULL);
    init_fake(&fake);
    fake.empty_snapshot = 1;
    run_script(&services, "2\n3\n0\n0\n", text, sizeof(text));
    CHECK(fake.loads == 1 && fake.releases == 1);
    CHECK(strstr(text, "No paid orders.") != NULL);
}

static void test_input_and_callback_errors(void)
{
    Fake fake;
    AppServices services;
    char text[32768];
    char long_line[256];
    init_fake(&fake);
    services = services_for(&fake);
    run_script(&services, "hello\n1abc\n-1\n4\n\n99999999999999999999999999\n 0 \n", text, sizeof(text));
    CHECK(occurrences(text, "Invalid choice.") == 6);
    CHECK(fake.customer_logins == 0 && fake.manager_logins == 0);
    memset(long_line, '9', 180);
    strcpy(long_line + 180, "\n2\n0\n0\n");
    run_script(&services, long_line, text, sizeof(text));
    CHECK(occurrences(text, "Invalid choice.") == 1);
    CHECK(fake.manager_logins == 1);
    run_script(&services, "1\n6\n0\n2\n4\n0\n0\n", text, sizeof(text));
    CHECK(occurrences(text, "Invalid choice.") == 2);
    run_script(&services, "", text, sizeof(text));
    run_script(&services, "1\n", text, sizeof(text));
    run_script(&services, "2", text, sizeof(text)); /* EOF without final newline. */
    fake.fail_action = 1;
    run_script(&services, "3\n1\n1\n0\n2\n1\n0\n0\n", text, sizeof(text));
    CHECK(occurrences(text, "Operation failed.") == 3);
    CHECK(app_run(NULL, NULL, NULL) == 1);
}

static void test_output_failure(void)
{
    AppServices services = {0};
    FILE *input = tmpfile();
    FILE *output = fopen(__FILE__, "r");
    CHECK(input != NULL && output != NULL);
    CHECK(app_run(input, output, &services) == 1);
    CHECK(fclose(input) == 0);
    CHECK(fclose(output) == 0);
}

int main(void)
{
    test_routing();
    test_authentication_and_role_isolation();
    test_missing_modules();
    test_snapshot_lifecycle_and_refresh();
    test_input_and_callback_errors();
    test_output_failure();
    puts("app: 6 scenario groups passed");
    return 0;
}
