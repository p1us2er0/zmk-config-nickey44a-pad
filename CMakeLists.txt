# Bundled until the standalone touch-inertia module is published and SHA-pinned.
add_subdirectory(modules/zmk-input-processor-touch-inertia)
add_subdirectory(modules/zmk-input-processor-touch-swipe)

# Turns the RGB LED off in soft off, which the rgbled widget leaves lit.
if(CONFIG_ZMK_PM_SOFT_OFF AND CONFIG_RGBLED_WIDGET)
  target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/soft_off_leds.c)
endif()
