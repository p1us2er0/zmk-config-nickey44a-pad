/*
 * Profile layers: turns host flag layers on and off by the active Bluetooth
 * profile.
 *
 * Toggled layers are not saved, so after a restart (the power switch, or
 * waking from soft off) WIN, WIN_JIS and IPAD were off until the profile
 * key was pressed again, and a Windows host got the Mac keys. ZMK keeps the
 * active profile in settings, so this sets the layers for it once settings
 * are loaded at startup, and again whenever the active profile changes. The
 * zmk,profile-layers node in config/nickey44a.keymap lists the layers.
 *
 * ZMK also raises the profile event when the active profile connects or
 * disconnects. Those leave the layers alone, so a host chosen by hand on
 * FUNCTION (&mac_us and the others) stays until the next profile change.
 */

#define DT_DRV_COMPAT zmk_profile_layers

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "one zmk,profile-layers node only");

#define PL_NODE DT_INST(0, DT_DRV_COMPAT)

#define PL_NO_PROFILE UINT8_MAX

struct profile_entry {
    uint8_t profile;
    const uint8_t *layers;
    size_t layers_len;
};

static const uint8_t managed_layers[] = DT_PROP(PL_NODE, layers);

#define PL_LAYERS_NAME(child) UTIL_CAT(pl_layers_, DT_DEP_ORD(child))
#define PL_CHILD_LAYERS(child) static const uint8_t PL_LAYERS_NAME(child)[] = DT_PROP(child, layers);
#define PL_CHILD_ENTRY(child)                                                                      \
    {.profile = DT_PROP(child, profile),                                                           \
     .layers = PL_LAYERS_NAME(child),                                                              \
     .layers_len = DT_PROP_LEN(child, layers)},

DT_FOREACH_CHILD(PL_NODE, PL_CHILD_LAYERS)
static const struct profile_entry entries[] = {DT_FOREACH_CHILD(PL_NODE, PL_CHILD_ENTRY)};

static uint8_t applied_profile = PL_NO_PROFILE;

static bool entry_has_layer(const struct profile_entry *entry, uint8_t layer) {
    for (size_t i = 0; i < entry->layers_len; i++) {
        if (entry->layers[i] == layer) {
            return true;
        }
    }
    return false;
}

static void apply_profile(uint8_t profile) {
    const struct profile_entry *entry = NULL;

    for (size_t i = 0; i < ARRAY_SIZE(entries); i++) {
        if (entries[i].profile == profile) {
            entry = &entries[i];
            break;
        }
    }

    for (size_t i = 0; i < ARRAY_SIZE(managed_layers); i++) {
        uint8_t layer = managed_layers[i];
        bool on = entry != NULL && entry_has_layer(entry, layer);

        if (on && !zmk_keymap_layer_active(layer)) {
            zmk_keymap_layer_activate(layer);
        } else if (!on && zmk_keymap_layer_active(layer)) {
            zmk_keymap_layer_deactivate(layer);
        }
    }

    applied_profile = profile;
    LOG_DBG("profile layers set for profile %d", profile);
}

static int profile_layers_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);

    if (ev != NULL && ev->index != applied_profile) {
        apply_profile(ev->index);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(profile_layers, profile_layers_listener);
ZMK_SUBSCRIPTION(profile_layers, zmk_ble_active_profile_changed);

static void profile_layers_startup(struct k_work *work) {
    ARG_UNUSED(work);
    int profile = zmk_ble_active_profile_index();

    if (profile >= 0) {
        apply_profile((uint8_t)profile);
    }
}

static K_WORK_DEFINE(profile_layers_startup_work, profile_layers_startup);

/*
 * Commit runs once every handler has loaded its settings, the active profile
 * among them. The layers are set on the system work queue, like the
 * profile changes.
 */
static int profile_layers_commit(void) {
    k_work_submit(&profile_layers_startup_work);
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(profile_layers, "profile_layers", NULL, NULL, profile_layers_commit,
                               NULL);

#endif
