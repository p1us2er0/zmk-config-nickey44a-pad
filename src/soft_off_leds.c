/*
 * Turns the RGB LED off when a half goes into soft off.
 *
 * The rgbled widget turns the LED off on ZMK_ACTIVITY_SLEEP, which idle sleep
 * raises but soft off (&soft_off_hold in config/nickey44a.keymap) does not.
 * The nRF52840 keeps its GPIO outputs in System OFF, so the right half's LED
 * stayed lit in the color of the layer active at that moment (SYSTEM: red)
 * until the power switch was turned off.
 *
 * zmk_pm_soft_off() and idle sleep suspend every device right before powering
 * off. This device has no hardware of its own: suspending it turns the LEDs
 * off. Soft off runs in a cooperative thread from that point to power off, so
 * the widget's thread cannot turn an LED on again in between.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/pm/device.h>

#define LED_GPIO_NODE_ID DT_PARENT(DT_ALIAS(led_red))

static const struct device *const leds = DEVICE_DT_GET(LED_GPIO_NODE_ID);
static const uint8_t led_idx[] = {DT_NODE_CHILD_IDX(DT_ALIAS(led_red)),
                                  DT_NODE_CHILD_IDX(DT_ALIAS(led_green)),
                                  DT_NODE_CHILD_IDX(DT_ALIAS(led_blue))};

static int soft_off_leds_pm_action(const struct device *dev, enum pm_device_action action) {
    ARG_UNUSED(dev);

    if (action == PM_DEVICE_ACTION_SUSPEND && device_is_ready(leds)) {
        for (size_t i = 0; i < ARRAY_SIZE(led_idx); i++) {
            led_off(leds, led_idx[i]);
        }
    }

    return 0;
}

static int soft_off_leds_init(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

PM_DEVICE_DEFINE(soft_off_leds, soft_off_leds_pm_action);

DEVICE_DEFINE(soft_off_leds, "soft_off_leds", soft_off_leds_init, PM_DEVICE_GET(soft_off_leds),
              NULL, NULL, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY, NULL);
