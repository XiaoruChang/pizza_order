#include "app.h"

int main(void)
{
    /* Bind real member adapters after agreeing on their public interfaces. */
    const AppServices services = {0};
    puts("Integration preview: account, menu, order and balance modules are pending.");
    return app_run(stdin, stdout, &services);
}
