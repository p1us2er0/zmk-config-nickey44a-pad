# Bundled until the standalone touch-inertia module is published and SHA-pinned.
add_subdirectory(modules/zmk-input-processor-touch-inertia)
add_subdirectory(modules/zmk-input-processor-touch-swipe)

# Keys that choose their binding by layer (&doc_top and others in the keymap).
# The keymap runs on the central only.
if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/behavior_layer_morph.c)
endif()

# Turns WIN, WIN_JIS and IPAD on for the active Bluetooth profile, also after
# a restart (the zmk,profile-layers node in the keymap).
if(CONFIG_ZMK_BLE AND CONFIG_SETTINGS AND ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
  target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/profile_layers.c)
endif()

# Turns the RGB LED off in soft off, which the rgbled widget leaves lit.
if(CONFIG_ZMK_PM_SOFT_OFF AND CONFIG_RGBLED_WIDGET)
  target_sources(app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/soft_off_leds.c)
endif()
