#include "app.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* -1 = EOF, -2 = malformed choice; overflow lines are fully consumed. */
static int read_choice(FILE *input, FILE *output, int maximum)
{
    char line[64];
    char *end;
    long value;
    fputs("> ", output);
    if (fflush(output) != 0 || fgets(line, sizeof(line), input) == NULL) {
        return -1;
    }
    if (strchr(line, '\n') == NULL && !feof(input)) {
        int ch;
        while ((ch = fgetc(input)) != '\n' && ch != EOF) {
            /* Drain the entire line rather than treating its tail as input. */
        }
        return -2;
    }
    errno = 0;
    value = strtol(line, &end, 10);
    if (end == line || errno == ERANGE) {
        return -2;
    }
    while (isspace((unsigned char)*end)) {
        ++end;
    }
    if (*end != '\0' || value < 0 || value > maximum) {
        return -2;
    }
    return (int)value;
}

static void unavailable(FILE *output)
{
    fputs("Module not connected. Ask the module owner to bind its adapter.\n", output);
}

static void run_action(AppAction action, const AppServices *services,
                       FILE *input, FILE *output)
{
    if (action == NULL) {
        unavailable(output);
    } else if (action(services->context, input, output) != 0) {
        fputs("Operation failed.\n", output);
    }
}

static void show_sales(const AppServices *services, FILE *output)
{
    const SalesOrder *orders = NULL;
    size_t count = 0;
    SalesReport report = {0};
    SalesResult result;
    if (services->load_orders == NULL || services->release_orders == NULL) {
        unavailable(output);
        return;
    }
    if (services->load_orders(services->context, &orders, &count) != 0) {
        fputs("Unable to load sales snapshot.\n", output);
        return;
    }
    result = sales_report_build(orders, count, &report);
    /* Report owns its names/totals; all raw lines can now be released. */
    services->release_orders(services->context, orders, count);
    if (result != SALES_OK) {
        fprintf(output, "%s.\n", sales_result_message(result));
    } else {
        (void)sales_report_print(output, &report);
    }
    sales_report_free(&report);
}

static void customer_menu(FILE *input, FILE *output,
                           const AppServices *services, const AppSession *session)
{
    for (;;) {
        int choice;
        AppCustomerAction action = NULL;
        fputs("\n=== Customer menu ===\n1. Place order\n2. View balance\n"
              "3. Deposit\n4. Order history\n5. Edit profile\n0. Log out\n", output);
        choice = read_choice(input, output, 5);
        if (choice == 0 || choice == -1) {
            return;
        }
        switch (choice) {
        case 1: action = services->place_order; break;
        case 2: action = services->show_balance; break;
        case 3: action = services->deposit; break;
        case 4: action = services->order_history; break;
        case 5: action = services->edit_profile; break;
        default: fputs("Invalid choice.\n", output); continue;
        }
        if (action == NULL) {
            unavailable(output);
        } else if (action(services->context, session, input, output) != 0) {
            fputs("Operation failed.\n", output);
        }
    }
}

static void manager_menu(FILE *input, FILE *output, const AppServices *services)
{
    for (;;) {
        int choice;
        fputs("\n=== Manager menu ===\n1. Edit pizza menu and prices\n"
              "2. Manage customers\n3. Sales statistics\n0. Log out\n", output);
        choice = read_choice(input, output, 3);
        switch (choice) {
        case -1:
        case 0: return;
        case 1: run_action(services->manage_menu, services, input, output); break;
        case 2: run_action(services->manage_customers, services, input, output); break;
        case 3: show_sales(services, output); break;
        default: fputs("Invalid choice.\n", output); break;
        }
    }
}

int app_run(FILE *input, FILE *output, const AppServices *services)
{
    int choice;
    if (input == NULL || output == NULL || services == NULL) {
        return 1;
    }
    for (;;) {
        fputs("\n=== Pizza ordering system ===\n1. Customer login\n"
              "2. Manager login\n3. Register customer\n0. Exit\n", output);
        choice = read_choice(input, output, 3);
        if (choice == -1 || choice == 0) {
            break;
        }
        if (choice == 1) {
            AppSession session = {0};
            if (services->login_customer == NULL) {
                unavailable(output);
            } else if (services->login_customer(services->context, input, output,
                                                &session) != 0 ||
                       session.account_id == 0) {
                fputs("Customer login failed.\n", output);
            } else {
                customer_menu(input, output, services, &session);
            }
        } else if (choice == 2) {
            if (services->login_manager == NULL) {
                unavailable(output);
            } else if (services->login_manager(services->context, input, output) != 0) {
                fputs("Manager login failed.\n", output);
            } else {
                manager_menu(input, output, services);
            }
        } else if (choice == 3) {
            run_action(services->register_customer, services, input, output);
        } else {
            fputs("Invalid choice.\n", output);
        }
        if (ferror(input) || ferror(output) || feof(input)) {
            break;
        }
    }
    if (fflush(output) != 0) {
        return 1;
    }
    return (ferror(input) || ferror(output)) ? 1 : 0;
}
