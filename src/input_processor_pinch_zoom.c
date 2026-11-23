/*
 * Pinch zoom: turns the TPS43 zoom gesture into zoom keys.
 *
 * With `zoom;` on the pad, the driver reports the pinch as INPUT_REL_MISC:
 * positive while the fingers spread, negative while they close, about 1 or
 * 2 per report and 10 to 20 for a whole pinch on the right pad. This adds
 * the values up and taps in-keycode for each `threshold` of spreading and
 * out-keycode for each `threshold` of closing. A host gets keys, not a
 * pinch, because the keyboard is a mouse to it and mice have no zoom.
 *
 * When the fingers close or one lifts, the pad reports a large jump of the
 * remaining finger as cursor movement. Once a touch has pinched, its cursor
 * movement (INPUT_REL_X/Y) is dropped until the finger lifts.
 */

#define DT_DRV_COMPAT zmk_input_processor_pinch_zoom

#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/input_processor.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct pinch_zoom_config {
    uint16_t zoom_code;
    uint16_t touch_code;
    int32_t threshold;
    uint32_t in_keycode;
    uint32_t out_keycode;
};

struct pinch_zoom_data {
    bool pinched;
    int32_t accum;
};

static void tap(uint32_t keycode) {
    int64_t timestamp = k_uptime_get();
    int press = raise_zmk_keycode_state_changed_from_encoded(keycode, true, timestamp);
    int release = raise_zmk_keycode_state_changed_from_encoded(keycode, false, timestamp);

    if (press < 0 || release < 0) {
        LOG_WRN("pinch zoom key output failed: %d/%d", press, release);
    }
}

static int pinch_zoom_handle_event(const struct device *dev, struct input_event *event,
                                   uint32_t param1, uint32_t param2,
                                   struct zmk_input_processor_state *state) {
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);
    const struct pinch_zoom_config *cfg = dev->config;
    struct pinch_zoom_data *data = dev->data;

    if (event->type == INPUT_EV_KEY && event->code == cfg->touch_code) {
        /* A new touch, or the end of one: start over. */
        *data = (struct pinch_zoom_data){0};
        return ZMK_INPUT_PROC_CONTINUE;
    }
    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->code == cfg->zoom_code) {
        data->pinched = true;
        data->accum = CLAMP((int64_t)data->accum + event->value, -INT32_MAX, INT32_MAX);
        while (data->accum >= cfg->threshold) {
            data->accum -= cfg->threshold;
            tap(cfg->in_keycode);
            LOG_DBG("pinch zoom in");
        }
        while (data->accum <= -cfg->threshold) {
            data->accum += cfg->threshold;
            tap(cfg->out_keycode);
            LOG_DBG("pinch zoom out");
        }
        /* Nothing after this uses the pinch amount. */
        return ZMK_INPUT_PROC_STOP;
    }

    if (data->pinched && (event->code == INPUT_REL_X || event->code == INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_STOP;
    }
    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api pinch_zoom_api = {
    .handle_event = pinch_zoom_handle_event,
};

#define PZ_INST(n)                                                                                 \
    BUILD_ASSERT(DT_INST_PROP(n, threshold) > 0, "threshold must be positive");                    \
    static struct pinch_zoom_data pinch_zoom_data_##n;                                             \
    static const struct pinch_zoom_config pinch_zoom_config_##n = {                                \
        .zoom_code = DT_INST_PROP(n, zoom_code),                                                   \
        .touch_code = DT_INST_PROP(n, touch_code),                                                 \
        .threshold = DT_INST_PROP(n, threshold),                                                   \
        .in_keycode = DT_INST_PROP(n, in_keycode),                                                 \
        .out_keycode = DT_INST_PROP(n, out_keycode),                                               \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &pinch_zoom_data_##n, &pinch_zoom_config_##n,             \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pinch_zoom_api);

DT_INST_FOREACH_STATUS_OKAY(PZ_INST)

#endif
