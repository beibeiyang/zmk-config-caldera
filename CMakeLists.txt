if(CONFIG_ZMK_MIN_SPLIT_BATTERY)
  zephyr_library()
  zephyr_library_sources(src/min_split_battery.c)
endif()
