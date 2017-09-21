#pragma once
#include <pebble-generic-weather/pebble-generic-weather.h>

typedef void* EventHandle;

typedef void(*EventWeatherHandler)(GenericWeatherInfo *info, GenericWeatherStatus status, void *context);

void weather_init(void);
void weather_deinit(void);

EventHandle events_weather_subscribe(EventWeatherHandler handler, void *context);
void events_weather_unsubscribe(EventHandle handle);
#define weather_peek generic_weather_peek
GenericWeatherStatus weather_status_peek(void);
