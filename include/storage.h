#ifndef PIZZA_STORAGE_H
#define PIZZA_STORAGE_H

#include "models.h"
#include "result.h"

#define PIZZA_STORAGE_FORMAT_VERSION 1

typedef struct {
    const char *data_path; /* Nonempty path; parent directory must exist. */
} StorageConfig;

typedef struct {
    const char *username;
    const char *password;
} ManagerSetup;

/* Open an existing, versioned full-state file. Validate all records, references,
 * monetary totals and persistent counters before publishing the handle.
 * Missing file => NOT_FOUND; malformed/unsupported format => INVALID_DATA;
 * other read/access failures => IO_ERROR. Never erase or silently reinitialize.
 * Require *out_state == NULL; leave it NULL on failure. Copy the path internally.
 * Only one process and one live handle may access this data file at a time. */
Result storage_open(const StorageConfig *config, AppState **out_state);

/* Explicit first-run setup, never overwrite an existing path (ALREADY_EXISTS).
 * Requires explicit manager credentials: no compiled-in/default password.
 * Initialize an empty menu, zero customers/orders, menu revision=1, next IDs=1.
 * Same output ownership as storage_open; creation is already persisted. */
Result storage_create(const StorageConfig *config, const ManagerSetup *manager,
                       AppState **out_state);

/* Read-only consistency check. Internal credentials stay private and are never
 * returned through public Customer/Order objects. */
Result storage_validate(const AppState *state);

/* Free resources, not a delayed save operation. NULL is allowed. All successful
 * business mutations were committed before returning. Clear the caller's
 * pointer after close. Sessions are invalid after their owning state closes. */
void storage_close(AppState *state);

/* Atomic commit/clone helpers are PRIVATE implementation details shared by
 * business/storage implementations, not independently callable public writes. */

#endif
