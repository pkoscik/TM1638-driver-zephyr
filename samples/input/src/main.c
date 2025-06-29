#include <drivers/tm1638.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

int main(void) {
  const struct device* tm_dev = DEVICE_DT_GET_ANY(titanmicro_tm1638);

  if (!device_is_ready(tm_dev)) {
    printf("TM1638 device is not ready!\n");
    return 0;
  }

  tm1638_config_display(tm_dev, 0x0, true);

  char text[SEGMENT_COUNT] = "        ";
  uint8_t leds[SEGMENT_COUNT] = {0};

  while (1) {
    uint8_t keys = tm1638_read_keys(tm_dev);

    for (int i = 0; i < SEGMENT_COUNT; i++) {
      if (keys & (1 << i)) {
        leds[i] = 1;
        text[i] = '1';
      } else {
        leds[i] = 0;
        text[i] = '0';
      }
    }

    tm1638_set_state(tm_dev, text, leds);
    k_msleep(50);
  }

  return 0;
}
