#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>

#define SEGMENT_COUNT 8

__subsystem struct tm1638_driver_api {
	void (*reset)(const struct device *dev);
	void (*config_display)(const struct device *dev, uint8_t brightness, bool enable);
	void (*set_led)(const struct device *dev, uint8_t pos, uint8_t value);
	void (*set_segment_single)(const struct device *dev, uint8_t pos, uint8_t value);
	void (*display_ascii)(const struct device *dev, uint8_t pos, uint8_t ascii, bool dp);
	void (*display_string)(const struct device *dev, const char *str, uint8_t count);
	void (*set_state)(const struct device *dev, const char *str, const uint8_t *leds);
	uint8_t (*read_keys)(const struct device *dev);
};

static inline void tm1638_reset(const struct device *dev)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	api->reset(dev);
}

static inline void tm1638_config_display(const struct device *dev, uint8_t brightness, bool enable)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	api->config_display(dev, brightness, enable);
}

static inline void tm1638_set_led(const struct device *dev, uint8_t pos, uint8_t value)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	api->set_led(dev, pos, value);
}

static inline void tm1638_set_segment_single(const struct device *dev, uint8_t pos, uint8_t value)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	api->set_segment_single(dev, pos, value);
}

static inline void tm1638_display_ascii(const struct device *dev, uint8_t pos, uint8_t ascii,
					bool dp)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	api->display_ascii(dev, pos, ascii, dp);
}

static inline void tm1638_display_string(const struct device *dev, const char *str, uint8_t count)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	api->display_string(dev, str, count);
}

static inline void tm1638_set_state(const struct device *dev, const char *str, const uint8_t *leds)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	api->set_state(dev, str, leds);
}

static inline uint8_t tm1638_read_keys(const struct device *dev)
{
	const struct tm1638_driver_api *api = (const struct tm1638_driver_api *)dev->api;
	return api->read_keys(dev);
}
