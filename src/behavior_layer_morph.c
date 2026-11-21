/*
 * Layer morph: like &mod_morph, but chosen by layers instead of modifiers.
 *
 * Invokes the second binding while any of `layers` is active, and the first
 * one otherwise. config/nickey44a.keymap uses it for MOUSE keys whose
 * shortcut differs between the Mac and Windows: WIN and WIN_JIS are on while
 * a Windows profile is selected. The binding chosen on press is the one
 * released, even if the layers change in between.
 */

#define DT_DRV_COMPAT zmk_behavior_layer_morph

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_layer_morph_config {
    struct zmk_behavior_binding normal_binding;
    struct zmk_behavior_binding morph_binding;
    const uint8_t *layers;
    size_t layers_len;
};

struct behavior_layer_morph_data {
    struct zmk_behavior_binding *pressed_binding;
};

static bool any_layer_active(const struct behavior_layer_morph_config *cfg) {
    for (size_t i = 0; i < cfg->layers_len; i++) {
        if (zmk_keymap_layer_active(cfg->layers[i])) {
            return true;
        }
    }
    return false;
}

static int on_layer_morph_binding_pressed(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_layer_morph_config *cfg = dev->config;
    struct behavior_layer_morph_data *data = dev->data;

    if (data->pressed_binding != NULL) {
        LOG_ERR("Can't press the same layer-morph twice");
        return -ENOTSUP;
    }

    data->pressed_binding = (struct zmk_behavior_binding *)(any_layer_active(cfg)
                                                                ? &cfg->morph_binding
                                                                : &cfg->normal_binding);
    return zmk_behavior_invoke_binding(data->pressed_binding, event, true);
}

static int on_layer_morph_binding_released(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_layer_morph_data *data = dev->data;

    if (data->pressed_binding == NULL) {
        LOG_ERR("Layer-morph already released");
        return -ENOTSUP;
    }

    struct zmk_behavior_binding *pressed_binding = data->pressed_binding;
    data->pressed_binding = NULL;
    return zmk_behavior_invoke_binding(pressed_binding, event, false);
}

static const struct behavior_driver_api behavior_layer_morph_driver_api = {
    .binding_pressed = on_layer_morph_binding_pressed,
    .binding_released = on_layer_morph_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define _TRANSFORM_ENTRY(idx, node)                                                                \
    {                                                                                              \
        .behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(node, bindings, idx)),               \
        .param1 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param1), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param1))),                  \
        .param2 = COND_CODE_0(DT_INST_PHA_HAS_CELL_AT_IDX(node, bindings, idx, param2), (0),       \
                              (DT_INST_PHA_BY_IDX(node, bindings, idx, param2))),                  \
    }

#define LM_INST(n)                                                                                 \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, bindings) == 2, "layer-morph takes exactly two bindings");    \
    static const uint8_t behavior_layer_morph_layers_##n[] = DT_INST_PROP(n, layers);             \
    static const struct behavior_layer_morph_config behavior_layer_morph_config_##n = {           \
        .normal_binding = _TRANSFORM_ENTRY(0, n),                                                  \
        .morph_binding = _TRANSFORM_ENTRY(1, n),                                                   \
        .layers = behavior_layer_morph_layers_##n,                                                 \
        .layers_len = DT_INST_PROP_LEN(n, layers),                                                 \
    };                                                                                             \
    static struct behavior_layer_morph_data behavior_layer_morph_data_##n = {};                    \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &behavior_layer_morph_data_##n,                         \
                            &behavior_layer_morph_config_##n, POST_KERNEL,                         \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_layer_morph_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LM_INST)

#endif
