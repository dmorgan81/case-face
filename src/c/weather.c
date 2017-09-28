#include <pebble.h>
#include <enamel.h>
#include <pebble-events/pebble-events.h>
#include <pebble-generic-weather/pebble-generic-weather.h>
#include <@smallstoneapps/linked-list/linked-list.h>
#include "logging.h"
#include "geocode.h"
#include "weather.h"

static const uint32_t PERSIST_KEY_WEATHER_INFO = 2;
static const uint32_t PERSIST_KEY_WEATHER_STATUS = 3;

#ifndef WEATHER_API_KEY_1
#error Need at least one weather API key
#else
#define _WEATHER_API_KEY_1 WEATHER_API_KEY_1
#endif

#ifndef WEATHER_API_KEY_2
#define _WEATHER_API_KEY_2 NULL
#else
#define _WEATHER_API_KEY_2 WEATHER_API_KEY_2
#endif

#ifndef WEATHER_API_KEY_3
#define _WEATHER_API_KEY_3 NULL
#else
#define _WEATHER_API_KEY_3 WEATHER_API_KEY_3
#endif

#ifndef WEATHER_API_KEY_4
#define _WEATHER_API_KEY_4 NULL
#else
#define _WEATHER_API_KEY_4 WEATHER_API_KEY_4
#endif

#ifndef WEATHER_API_KEY_5
#define _WEATHER_API_KEY_5 NULL
#else
#define _WEATHER_API_KEY_5 WEATHER_API_KEY_5
#endif

#ifndef WEATHER_API_KEY_6
#define _WEATHER_API_KEY_6 NULL
#else
#define _WEATHER_API_KEY_6 WEATHER_API_KEY_6
#endif

#ifndef WEATHER_API_KEY_7
#define _WEATHER_API_KEY_7 NULL
#else
#define _WEATHER_API_KEY_7 WEATHER_API_KEY_7
#endif

#ifndef WEATHER_API_KEY_8
#define _WEATHER_API_KEY_8 NULL
#else
#define _WEATHER_API_KEY_8 WEATHER_API_KEY_8
#endif

#ifndef WEATHER_API_KEY_9
#define _WEATHER_API_KEY_9 NULL
#else
#define _WEATHER_API_KEY_9 WEATHER_API_KEY_9
#endif

#ifndef WEATHER_API_KEY_10
#define _WEATHER_API_KEY_10 NULL
#else
#define _WEATHER_API_KEY_10 WEATHER_API_KEY_10
#endif

typedef struct {
    EventWeatherHandler handler;
    void *context;
} WeatherHandlerState;

typedef struct {
    GenericWeatherInfo *info;
    GenericWeatherStatus status;
} WeatherBundle;

static GenericWeatherStatus s_status = GenericWeatherStatusNotYetFetched;

static LinkedRoot *s_handler_list;

static uint16_t s_interval;

static bool s_connected;
static bool s_ready = false;

static EventHandle s_settings_event_handle;
static EventHandle s_connection_event_handle;
static EventHandle s_app_message_event_handle;

static AppTimer *s_timer;

static const char* const s_weather_api_keys[] = {
    _WEATHER_API_KEY_1,
    _WEATHER_API_KEY_2,
    _WEATHER_API_KEY_3,
    _WEATHER_API_KEY_4,
    _WEATHER_API_KEY_5,
    _WEATHER_API_KEY_6,
    _WEATHER_API_KEY_7,
    _WEATHER_API_KEY_8,
    _WEATHER_API_KEY_9,
    _WEATHER_API_KEY_10
};

static uint8_t s_weather_api_keys_len;
static uint8_t s_weather_api_key_idx;

#ifndef PBL_PLATFORM_APLITE
static bool s_use_gps;
static EventHandle s_geocode_event_handle;
static char s_location_name[GEOCODE_MAPQUEST_MAX_LOCATION_LEN];
#endif

static void cancel_timer(void) {
    logf();
    if (s_timer) app_timer_cancel(s_timer);
    s_timer = NULL;
}

static bool each_weather_fetched(void *this, void *context) {
    logf();
    WeatherHandlerState *state = (WeatherHandlerState *) this;
    WeatherBundle *bundle = (WeatherBundle *) context;
    state->handler(bundle->info, bundle->status, state->context);
    return true;
}

static void generic_weather_fetch_callback(GenericWeatherInfo *info, GenericWeatherStatus status) {
    logf();
    s_status = status;

    WeatherBundle bundle = {
        .info = info,
        .status = status
    };
    linked_list_foreach(s_handler_list, each_weather_fetched, &bundle);
}

static void do_fetch_weather(void) {
    logf();

    const char *key = s_weather_api_keys[s_weather_api_key_idx++];
    if (s_weather_api_key_idx > s_weather_api_keys_len - 1) s_weather_api_key_idx = 0;
    logd("weather api key: %s", key);
    generic_weather_set_api_key(key);

    generic_weather_fetch(generic_weather_fetch_callback);
}

static void app_timer_callback(void *context) {
    logf();
    do_fetch_weather();
    s_timer = app_timer_register(s_interval * 1000, app_timer_callback, NULL);
}

static void fetch_or_setup_timer(void) {
    logf();
    GenericWeatherInfo *info = generic_weather_peek();
    time_t now = time(NULL);
    logd("%ld - %ld", now, info->timestamp);
    if (now - info->timestamp > s_interval) {
        do_fetch_weather();
        s_timer = app_timer_register(s_interval * 1000, app_timer_callback, NULL);
    } else {
        logd("%ld", s_interval - (now - info->timestamp));
        s_timer = app_timer_register((s_interval - (now - info->timestamp)) * 1000, app_timer_callback, NULL);
    }
}

static void pebble_app_connection_handler(bool connected) {
    logf();
    if (!connected && s_timer) cancel_timer();
    else if (s_connected != connected) fetch_or_setup_timer();
    s_connected = connected;
}

#ifndef PBL_PLATFORM_APLITE
static void geocode_handler(GeocodeMapquestCoordinates *coordinates, GeocodeMapquestStatus status, void *context) {
    logf();
    if (status == GeocodeMapquestStatusAvailable) {
        generic_weather_set_location((GenericWeatherCoordinates) {
            .latitude = coordinates->latitude,
            .longitude = coordinates->longitude
        });
    } else if (status != GeocodeMapquestStatusPending) {
        generic_weather_set_location(GENERIC_WEATHER_GPS_LOCATION);
    }
    if (status != GeocodeMapquestStatusPending) {
        cancel_timer();
        if (s_ready && s_connected && linked_list_count(s_handler_list) > 0) {
            do_fetch_weather();
            s_timer = app_timer_register(s_interval * 1000, app_timer_callback, NULL);
        }
    }
}
#endif

static void settings_handler(void *context) {
    logf();
    uint32_t interval = atoi(enamel_get_WEATHER_INTERVAL()) * SECONDS_PER_MINUTE;
    interval = interval < (30 * SECONDS_PER_MINUTE) ? (30 * SECONDS_PER_MINUTE) : interval;
#ifndef PBL_PLATFORM_APLITE
    bool use_gps = enamel_get_WEATHER_USE_GPS();
    const char *location_name = enamel_get_WEATHER_LOCATION_NAME();
#endif
    bool fetch_weather = false;

    if (interval != s_interval) {
        s_interval = interval;
        fetch_weather = true;
    }

#ifndef PBL_PLATFORM_APLITE
    if (use_gps != s_use_gps || strcmp(location_name, s_location_name) != 0) {
        s_use_gps = use_gps;
        strncpy(s_location_name, location_name, sizeof(s_location_name));
        if (use_gps) {
            generic_weather_set_location(GENERIC_WEATHER_GPS_LOCATION);
            fetch_weather = true;
        } else {
            geocode_fetch(s_location_name);
            fetch_weather = false;
        }
    }
#endif

    if (fetch_weather) {
        cancel_timer();
        if (s_ready && s_connected && linked_list_count(s_handler_list) > 0) {
            do_fetch_weather();
            s_timer = app_timer_register(s_interval * 1000, app_timer_callback, NULL);
        }
    }
}

static void inbox_received(DictionaryIterator *iterator, void *context) {
    logf();
    Tuple *tuple = dict_find(iterator, MESSAGE_KEY_APP_READY);
    if (tuple && !s_ready) {
        s_ready = true;
        if (s_connected && linked_list_count(s_handler_list) > 0) fetch_or_setup_timer();
    }
}

void weather_init(void) {
    logf();
#ifdef DEBUG
    for (uint i = 0; i < ARRAY_LENGTH(s_weather_api_keys); i++) {
        logd("%s", s_weather_api_keys[i]);
    }
#endif

    for (uint i = 0; i < ARRAY_LENGTH(s_weather_api_keys); i++) {
        if (s_weather_api_keys[i]) s_weather_api_keys_len++;
    }
    s_weather_api_key_idx = rand() / (RAND_MAX / s_weather_api_keys_len + 1);

    s_status = persist_exists(PERSIST_KEY_WEATHER_STATUS) ? persist_read_int(PERSIST_KEY_WEATHER_STATUS) : GenericWeatherStatusNotYetFetched;

    s_handler_list = linked_list_create_root();

    s_interval = atoi(enamel_get_WEATHER_INTERVAL()) * SECONDS_PER_MINUTE;
    s_interval = s_interval < (30 * SECONDS_PER_MINUTE) ? (30 * SECONDS_PER_MINUTE) : s_interval;
#ifndef PBL_PLATFORM_APLITE
    s_use_gps = enamel_get_WEATHER_USE_GPS();
#endif

    generic_weather_init();
#ifndef PBL_PLATFORM_APLITE
    geocode_init();
#endif

    generic_weather_set_provider(GenericWeatherProviderWeatherUnderground);
    generic_weather_load(PERSIST_KEY_WEATHER_INFO);

#ifndef PBL_PLATFORM_APLITE
    strncpy(s_location_name, enamel_get_WEATHER_LOCATION_NAME(), sizeof(s_location_name));
    GeocodeMapquestCoordinates *coordinates = geocode_peek();
    if (s_use_gps || coordinates == NULL || strlen(s_location_name) == 0) {
        generic_weather_set_location(GENERIC_WEATHER_GPS_LOCATION);
    } else {
        generic_weather_set_location((GenericWeatherCoordinates) {
            .latitude = coordinates->latitude,
            .longitude = coordinates->longitude
        });
    }
    s_geocode_event_handle = events_geocode_subscribe(geocode_handler, NULL);
#else
    generic_weather_set_location(GENERIC_WEATHER_GPS_LOCATION);
#endif

    s_settings_event_handle = enamel_settings_received_subscribe(settings_handler, NULL);

    s_connected = connection_service_peek_pebble_app_connection();
    s_connection_event_handle = events_connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = pebble_app_connection_handler
    });

    s_app_message_event_handle = events_app_message_subscribe_handlers((EventAppMessageHandlers) {
        .received = inbox_received
    }, NULL);
}

void weather_deinit(void) {
    logf();
    cancel_timer();

    events_app_message_unsubscribe(s_app_message_event_handle);
    events_connection_service_unsubscribe(s_connection_event_handle);
    enamel_settings_received_unsubscribe(s_settings_event_handle);
#ifndef PBL_PLATFORM_APLITE
    events_geocode_unsubscribe(s_geocode_event_handle);
#endif
    generic_weather_save(PERSIST_KEY_WEATHER_INFO);
    generic_weather_deinit();

    persist_write_int(PERSIST_KEY_WEATHER_STATUS, s_status);

#ifndef PBL_PLATFORM_APLITE
    geocode_deinit();
#endif

    free(s_handler_list);
}

EventHandle events_weather_subscribe(EventWeatherHandler handler, void *context) {
    logf();
    WeatherHandlerState *this = malloc(sizeof(WeatherHandlerState));
    this->handler = handler;
    this->context = context;

    uint16_t count = linked_list_count(s_handler_list);
    linked_list_append(s_handler_list, this);

    if (count == 0 && s_ready && s_connected) fetch_or_setup_timer();
    return this;
}

void events_weather_unsubscribe(EventHandle handle) {
    logf();

    int16_t index = linked_list_find(s_handler_list, handle);
    if (index == -1) return;

    free(linked_list_get(s_handler_list, index));
    linked_list_remove(s_handler_list, index);

    if (linked_list_count(s_handler_list) == 0) cancel_timer();
}

GenericWeatherStatus weather_status_peek(void) {
    logf();
    return s_status;
}
