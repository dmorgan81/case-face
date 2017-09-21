#pragma once
#include <pebble-geocode-mapquest/pebble-geocode-mapquest.h>

typedef void* EventHandle;

typedef void(*EventGeocodeHandler)(GeocodeMapquestCoordinates *coordinates, GeocodeMapquestStatus status, void *context);

void geocode_init(void);
void geocode_deinit(void);
void geocode_fetch(const char *location);
GeocodeMapquestCoordinates *geocode_peek(void);

EventHandle events_geocode_subscribe(EventGeocodeHandler handler, void *context);
void events_geocode_unsubscribe(EventHandle handle);
