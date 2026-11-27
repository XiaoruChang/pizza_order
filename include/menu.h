#ifndef PIZZA_MENU_H
#define PIZZA_MENU_H

#include "models.h"
#include "result.h"

typedef struct {
    MenuCategory category;
    const char *name;
    Money price_cents;
} MenuOptionCreate;

typedef struct {
    const char *name;
    Money price_cents;
} MenuOptionUpdate;

/* Public reads; category must be CRUST, SIZE or TOPPING. Lists sort by ID.
 * menu_get can return an unavailable item for inspection. */
Result menu_get(const AppState *state, MenuOptionId option_id,
                MenuOption *out_option);
Result menu_list(const AppState *state, MenuCategory category,
                 bool include_unavailable, MenuOption *out_options,
                 size_t capacity, size_t *out_count);
Result menu_get_revision(const AppState *state, uint64_t *out_revision);

/* Manager only. Add assigns a nonzero global ID and starts as available.
 * Update changes name/price only; an item's ID and category never change.
 * Each successful write increases the persistent menu revision; overflow fails
 * without changing anything. Historical orders are never edited. */
Result menu_add(AppState *state, const Session *manager,
                const MenuOptionCreate *option, MenuOptionId *out_option_id);
Result menu_update(AppState *state, const Session *manager,
                   MenuOptionId option_id, const MenuOptionUpdate *update);
Result menu_set_available(AppState *state, const Session *manager,
                          MenuOptionId option_id, bool available);

#endif
