/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT zmk_input_processor_touch_swipe
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#error "touch-swipe requires a central-side devicetree instance"
#endif

#include <limits.h>
#include <drivers/input_processor.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct touch_swipe_config {
    uint16_t hwheel_code;
    uint16_t wheel_code;
    uint16_t touch_code;
    int32_t threshold;
    int32_t axis_ratio;
    uint32_t left_keycode;
    uint32_t right_keycode;
    int32_t override_layer;
    uint32_t override_left_keycode;
    uint32_t override_right_keycode;
    const int32_t *second_override_layers;
    size_t second_override_layers_len;
    uint32_t second_override_left_keycode;
    uint32_t second_override_right_keycode;
    uint32_t second_override_hold_keycode;
    int32_t second_override_repeat_threshold;
};

struct touch_swipe_data {
    bool touching;
    bool swipe_fired;
    int32_t horizontal_accum;
    int32_t vertical_accum;
    /* Total vertical travel prevents opposite scroll samples cancelling out. */
    int32_t vertical_travel;
    /* Key held from a second override swipe until the finger lifts, or 0. */
    uint32_t holding;
};

static int64_t magnitude(int32_t value) {
    return value < 0 ? -(int64_t)value : (int64_t)value;
}

static int32_t bounded_add(int32_t accum, int64_t delta) {
    return CLAMP((int64_t)accum + delta, (int64_t)INT32_MIN, (int64_t)INT32_MAX);
}

static bool any_layer_active(const int32_t *layers, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (zmk_keymap_layer_active(layers[i])) {
            return true;
        }
    }
    return false;
}

static void tap(uint32_t keycode) {
    int64_t timestamp = k_uptime_get();
    int press = raise_zmk_keycode_state_changed_from_encoded(keycode, true, timestamp);
    int release = raise_zmk_keycode_state_changed_from_encoded(keycode, false, timestamp);
    if (press < 0 || release < 0) {
        LOG_WRN("touch swipe key output failed: %d/%d", press, release);
    }
}

static void release_hold(struct touch_swipe_data *data) {
    if (data->holding) {
        int ret = raise_zmk_keycode_state_changed_from_encoded(data->holding, false, k_uptime_get());
        if (ret < 0) {
            LOG_WRN("touch swipe hold release failed: %d", ret);
        }
        data->holding = 0;
    }
}

/* Every reset releases a held key, so a lost touch release ends at the next touch. */
static void reset(struct touch_swipe_data *data) {
    release_hold(data);
    *data = (struct touch_swipe_data){0};
}

static int observe_input(const struct device *dev, struct input_event *event,
                         uint32_t param1, uint32_t param2,
                         struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);
    struct touch_swipe_data *data = dev->data;
    const struct touch_swipe_config *cfg = dev->config;

    if (event->type == INPUT_EV_KEY && event->code == cfg->touch_code) {
        if (!event->value) {
            reset(data);
            LOG_DBG("touch swipe release/reset");
        } else if (!data->touching) {
            reset(data);
            data->touching = true;
            LOG_DBG("touch swipe start");
        }
        return ZMK_INPUT_PROC_CONTINUE;
    }
    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }
    if (event->code == cfg->wheel_code) {
        if (data->touching && !data->swipe_fired) {
            data->vertical_accum = bounded_add(data->vertical_accum, event->value);
            data->vertical_travel = bounded_add(data->vertical_travel, magnitude(event->value));
        }
        return ZMK_INPUT_PROC_CONTINUE;
    }
    if (event->code != cfg->hwheel_code) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (data->touching && data->holding) {
        /*
         * Hold mode (an app switcher): one more key per repeat threshold of
         * travel, either way, while the finger stays down.
         */
        data->horizontal_accum = bounded_add(data->horizontal_accum, event->value);
        if (magnitude(data->horizontal_accum) >= cfg->second_override_repeat_threshold) {
            bool left = data->horizontal_accum < 0;
            data->horizontal_accum = 0;
            tap(left ? cfg->second_override_left_keycode : cfg->second_override_right_keycode);
            LOG_DBG("touch swipe hold %s repeated", left ? "left" : "right");
        }
    } else if (data->touching && !data->swipe_fired) {
        data->horizontal_accum = bounded_add(data->horizontal_accum, event->value);
        int64_t horizontal = magnitude(data->horizontal_accum);
        if (horizontal >= cfg->threshold &&
            horizontal >= (int64_t)data->vertical_travel * cfg->axis_ratio) {
            /* Latch before output: neither reversal nor output failure may retrigger. */
            data->swipe_fired = true;
            bool left = data->horizontal_accum < 0;
            uint32_t keycode = left ? cfg->left_keycode : cfg->right_keycode;
            if (cfg->override_layer >= 0 && zmk_keymap_layer_active(cfg->override_layer)) {
                keycode = left ? cfg->override_left_keycode : cfg->override_right_keycode;
            } else if (any_layer_active(cfg->second_override_layers,
                                        cfg->second_override_layers_len)) {
                keycode = left ? cfg->second_override_left_keycode
                               : cfg->second_override_right_keycode;
                if (cfg->second_override_hold_keycode) {
                    /* Held until the finger lifts (reset), whatever the layers do. */
                    int ret = raise_zmk_keycode_state_changed_from_encoded(
                        cfg->second_override_hold_keycode, true, k_uptime_get());
                    if (ret < 0) {
                        LOG_WRN("touch swipe hold press failed: %d", ret);
                    } else {
                        data->holding = cfg->second_override_hold_keycode;
                        data->horizontal_accum = 0;
                    }
                }
            }
            tap(keycode);
            LOG_DBG("touch swipe %s fired", left ? "left" : "right");
        }
    }
    /* Always suppress horizontal scrolling, including outside a tracked touch. */
    return ZMK_INPUT_PROC_STOP;
}

static const struct zmk_input_processor_driver_api api = {.handle_event = observe_input};

#define SECOND_LAYER_ASSERT(node_id, prop, idx)                                                    \
    BUILD_ASSERT(DT_PROP_BY_IDX(node_id, prop, idx) >= 0 &&                                        \
                     DT_PROP_BY_IDX(node_id, prop, idx) < ZMK_KEYMAP_LAYERS_LEN,                   \
                 "invalid second override layer");

#define SECOND_LAYERS(n)                                                                           \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, second_override_layers),                                  \
                (DT_INST_FOREACH_PROP_ELEM(n, second_override_layers, SECOND_LAYER_ASSERT)         \
                 BUILD_ASSERT(DT_INST_PROP(n, second_override_left_keycode) &&                     \
                                  DT_INST_PROP(n, second_override_right_keycode),                  \
                              "second override layers require both keycodes");                     \
                 static const int32_t second_layers_##n[] =                                        \
                     DT_INST_PROP(n, second_override_layers);),                                    \
                ())

#define CREATE_INSTANCE(n)                                                                         \
    SECOND_LAYERS(n)                                                                               \
    BUILD_ASSERT(DT_INST_PROP(n, override_layer) >= -1 &&                                        \
                 DT_INST_PROP(n, override_layer) < ZMK_KEYMAP_LAYERS_LEN, "invalid override layer"); \
    BUILD_ASSERT(DT_INST_PROP(n, override_layer) < 0 ||                                          \
                 (DT_INST_PROP(n, override_left_keycode) && DT_INST_PROP(n, override_right_keycode)), \
                 "override layer requires both keycodes");                                     \
    BUILD_ASSERT(DT_INST_PROP(n, threshold) > 0 &&                                                 \
                 DT_INST_PROP(n, threshold) <= INT32_MAX, "threshold must be positive int32");   \
    BUILD_ASSERT(DT_INST_PROP(n, axis_ratio) > 0 &&                                                \
                 DT_INST_PROP(n, axis_ratio) <= INT32_MAX, "axis-ratio must be positive int32"); \
    BUILD_ASSERT(DT_INST_PROP(n, hwheel_code) != DT_INST_PROP(n, wheel_code),                      \
                 "horizontal and vertical codes must differ");                                   \
    static struct touch_swipe_data data_##n;                                                       \
    static const struct touch_swipe_config config_##n = {                                          \
        .hwheel_code = DT_INST_PROP(n, hwheel_code),                                               \
        .wheel_code = DT_INST_PROP(n, wheel_code),                                                 \
        .touch_code = DT_INST_PROP(n, touch_code),                                                 \
        .threshold = DT_INST_PROP(n, threshold),                                                   \
        .axis_ratio = DT_INST_PROP(n, axis_ratio),                                                 \
        .left_keycode = DT_INST_PROP(n, left_keycode),                                             \
        .right_keycode = DT_INST_PROP(n, right_keycode),                                           \
        .override_layer = DT_INST_PROP(n, override_layer),                                       \
        .override_left_keycode = DT_INST_PROP(n, override_left_keycode),                         \
        .override_right_keycode = DT_INST_PROP(n, override_right_keycode),                       \
        .second_override_layers = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, second_override_layers),    \
                                              (second_layers_##n), (NULL)),                        \
        .second_override_layers_len = DT_INST_PROP_LEN_OR(n, second_override_layers, 0),           \
        .second_override_left_keycode = DT_INST_PROP(n, second_override_left_keycode),           \
        .second_override_right_keycode = DT_INST_PROP(n, second_override_right_keycode),         \
        .second_override_hold_keycode = DT_INST_PROP(n, second_override_hold_keycode),             \
        .second_override_repeat_threshold =                                                        \
            DT_INST_PROP(n, second_override_repeat_threshold) > 0                                  \
                ? DT_INST_PROP(n, second_override_repeat_threshold)                                \
                : DT_INST_PROP(n, threshold),                                                      \
    };                                                                                            \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &data_##n, &config_##n, POST_KERNEL,                       \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_INSTANCE)
#endif
