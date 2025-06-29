#define DT_DRV_COMPAT titanmicro_tm1638

#include <drivers/tm1638.h>
#include "font.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define DISPLAY_COMMAND                 0x80
#define DISPLAY_COMMAND_BRIGHTNESS_MASK 0b111
#define DISPLAY_COMMAND_ENABLE_SHIFT    3
#define ADDRESS_COMMAND                 0xC0
#define ADDRESS_COMMAND_LED_BIT         0b1
#define DATA_COMMAND                    0x40
#define DATA_COMMAND_WRITE              0b00
#define DATA_COMMAND_READ               0b10
#define DATA_COMMAND_AUTO_INCREMENT     0b000
#define DATA_COMMAND_FIXED_ADDRESS      0b100
#define DATA_COMMAND_NORMAL_MODE        0b0000

struct tm1638_config {
	struct gpio_dt_spec stb;
	struct gpio_dt_spec clk;
	struct gpio_dt_spec dio;
};

static inline void start_comm(const struct tm1638_config *cfg)
{
	gpio_pin_set_dt(&cfg->stb, 0);
}

static inline void stop_comm(const struct tm1638_config *cfg)
{
	gpio_pin_set_dt(&cfg->stb, 1);
}

static void write_bytes(const struct tm1638_config *cfg, const uint8_t *data, uint8_t count)
{
	gpio_pin_configure_dt(&cfg->dio, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);

	for (uint8_t j = 0; j < count; j++) {
		uint8_t b = data[j];
		for (uint8_t i = 0; i < 8; i++, b >>= 1) {
			gpio_pin_set_dt(&cfg->clk, 0);
			k_busy_wait(1);
			gpio_pin_set_dt(&cfg->dio, b & 0x01);
			gpio_pin_set_dt(&cfg->clk, 1);
			k_busy_wait(1);
		}
	}
}

static uint8_t api_read_keys(const struct device *dev)
{
	const struct tm1638_config *cfg = dev->config;
	uint8_t keys = 0;
	uint8_t cmd = DATA_COMMAND | DATA_COMMAND_READ | DATA_COMMAND_NORMAL_MODE;

	start_comm(cfg);
	write_bytes(cfg, &cmd, 1);

	gpio_pin_configure_dt(&cfg->dio, GPIO_INPUT);

	for (uint8_t i = 0; i < 4; i++) {
		uint8_t b = 0;
		for (uint8_t j = 0; j < 8; j++) {
			gpio_pin_set_dt(&cfg->clk, 0);
			k_busy_wait(1);
			if (gpio_pin_get_dt(&cfg->dio)) {
				b |= (1 << j);
			}
			gpio_pin_set_dt(&cfg->clk, 1);
			k_busy_wait(1);
		}
		if (b & 0x07) {
			keys |= (1 << i);
		}
		if (b & 0x70) {
			keys |= (1 << (i + 4));
		}
	}

	stop_comm(cfg);
	return keys;
}

static void api_reset(const struct device *dev)
{
	const struct tm1638_config *cfg = dev->config;
	uint8_t zero = 0;
	uint8_t cmd = DATA_COMMAND | DATA_COMMAND_WRITE | DATA_COMMAND_AUTO_INCREMENT |
		      DATA_COMMAND_NORMAL_MODE;

	start_comm(cfg);
	write_bytes(cfg, &cmd, 1);
	stop_comm(cfg);

	uint8_t adr = ADDRESS_COMMAND;
	start_comm(cfg);
	write_bytes(cfg, &adr, 1);
	for (uint8_t i = 0; i < SEGMENT_COUNT * 2; i++) {
		write_bytes(cfg, &zero, 1);
	}
	stop_comm(cfg);
}

static void api_config_display(const struct device *dev, uint8_t brightness, bool enable_display)
{
	const struct tm1638_config *cfg = dev->config;
	uint8_t cmd = DISPLAY_COMMAND | (brightness & DISPLAY_COMMAND_BRIGHTNESS_MASK) |
		      (enable_display << DISPLAY_COMMAND_ENABLE_SHIFT);

	start_comm(cfg);
	write_bytes(cfg, &cmd, 1);
	stop_comm(cfg);
}

static void api_set_led(const struct device *dev, uint8_t pos, uint8_t value)
{
	const struct tm1638_config *cfg = dev->config;
	uint8_t cmd = DATA_COMMAND | DATA_COMMAND_WRITE | DATA_COMMAND_FIXED_ADDRESS |
		      DATA_COMMAND_NORMAL_MODE;

	start_comm(cfg);
	write_bytes(cfg, &cmd, 1);
	stop_comm(cfg);

	uint8_t adr = (ADDRESS_COMMAND | ADDRESS_COMMAND_LED_BIT) + (pos << 1);
	start_comm(cfg);
	write_bytes(cfg, &adr, 1);
	write_bytes(cfg, &value, 1);
	stop_comm(cfg);
}

static void api_set_segment_single(const struct device *dev, uint8_t pos, uint8_t value)
{
	const struct tm1638_config *cfg = dev->config;
	uint8_t cmd = DATA_COMMAND | DATA_COMMAND_WRITE | DATA_COMMAND_FIXED_ADDRESS |
		      DATA_COMMAND_NORMAL_MODE;

	start_comm(cfg);
	write_bytes(cfg, &cmd, 1);
	stop_comm(cfg);

	uint8_t adr = ADDRESS_COMMAND + (pos << 1);
	start_comm(cfg);
	write_bytes(cfg, &adr, 1);
	write_bytes(cfg, &value, 1);
	stop_comm(cfg);
}

static inline uint8_t map_ascii(uint8_t ascii, bool dp)
{
	if (ascii < ASCII_FONT_OFFSET || ascii >= ASCII_FONT_END) {
		ascii = ' ';
	}
	uint8_t seg = fontData[ascii - ASCII_FONT_OFFSET];
	if (dp) {
		seg |= DEC_POINT_MASK;
	}
	return seg;
}

static void api_display_ascii(const struct device *dev, uint8_t pos, uint8_t ascii, bool dp)
{
	api_set_segment_single(dev, pos, map_ascii(ascii, dp));
}

static void api_display_string(const struct device *dev, const char *str, uint8_t count)
{
	if (!str || count == 0 || count > 8) {
		return;
	}
	for (uint8_t i = 0; i < count; i++) {
		api_set_segment_single(dev, i, map_ascii((uint8_t)str[i], false));
	}
}

static void api_set_state(const struct device *dev, const char *str, const uint8_t *leds)
{
	const struct tm1638_config *cfg = dev->config;
	if (!str || !leds) {
		return;
	}

	uint8_t buffer[SEGMENT_COUNT * 2];
	for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
		buffer[i * 2] = map_ascii((uint8_t)str[i], false);
		buffer[i * 2 + 1] = leds[i];
	}

	uint8_t cmd = DATA_COMMAND | DATA_COMMAND_WRITE | DATA_COMMAND_AUTO_INCREMENT |
		      DATA_COMMAND_NORMAL_MODE;
	start_comm(cfg);
	write_bytes(cfg, &cmd, 1);
	stop_comm(cfg);

	uint8_t adr = ADDRESS_COMMAND;
	start_comm(cfg);
	write_bytes(cfg, &adr, 1);
	write_bytes(cfg, buffer, SEGMENT_COUNT * 2);
	stop_comm(cfg);
}

static const struct tm1638_driver_api tm1638_api_funcs = {
	.reset = api_reset,
	.config_display = api_config_display,
	.set_led = api_set_led,
	.set_segment_single = api_set_segment_single,
	.display_ascii = api_display_ascii,
	.display_string = api_display_string,
	.set_state = api_set_state,
	.read_keys = api_read_keys,
};

static int tm1638_init(const struct device *dev)
{
	const struct tm1638_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->stb) || !gpio_is_ready_dt(&cfg->clk) ||
	    !gpio_is_ready_dt(&cfg->dio)) {
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->stb, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&cfg->clk, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&cfg->dio, GPIO_OUTPUT_INACTIVE);

	api_reset(dev);
	return 0;
}

#define TM1638_INIT(inst)                                                                          \
	static const struct tm1638_config tm1638_cfg_##inst = {                                    \
		.stb = GPIO_DT_SPEC_INST_GET(inst, stb_gpios),                                     \
		.clk = GPIO_DT_SPEC_INST_GET(inst, clk_gpios),                                     \
		.dio = GPIO_DT_SPEC_INST_GET(inst, dio_gpios),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tm1638_init, NULL, NULL, &tm1638_cfg_##inst, POST_KERNEL,      \
			      CONFIG_APPLICATION_INIT_PRIORITY, &tm1638_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(TM1638_INIT)
