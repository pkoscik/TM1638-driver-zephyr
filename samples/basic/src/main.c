#include <drivers/tm1638.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

int main(void) {
  printf("Starting TM1638 Basic Sample...\n");

  const struct device* tm_dev = DEVICE_DT_GET_ANY(titanmicro_tm1638);

  if (!device_is_ready(tm_dev)) {
    printf("Error: TM1638 device is not ready!\n");
    return 0;
  }

  tm1638_config_display(tm_dev, 0x07, true);
  tm1638_display_string(tm_dev, " ZEPHYR ", 8);

  uint8_t current_led = 0;
  int8_t direction = 1;

  while (1) {
    for (int i = 0; i < SEGMENT_COUNT; i++) {
      tm1638_set_led(tm_dev, i, 0);
    }

    tm1638_set_led(tm_dev, current_led, 1);

    current_led += direction;

    if (current_led == (SEGMENT_COUNT - 1)) {
      direction = -1;
    } else if (current_led == 0) {
      direction = 1;
    }

    k_msleep(100);
  }

  return 0;
}
