if(CONFIG_I2C_FAKE)
  zephyr_library()
  zephyr_library_sources(i2c_fake.c)
endif()
