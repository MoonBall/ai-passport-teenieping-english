idf_component_register(
    SRCS "main.c" "ui_pixel.c" "adpcm_ima.c"
         "spoken_core.c" "spoken_app.c" "spoken_audio.c" "spoken_data.c"
         "ui_font_spoken_14.c" "ui_font_spoken_16.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_driver_usb_serial_jtag esp_partition
)
