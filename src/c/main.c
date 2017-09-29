#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include <pebble-connection-vibes/connection-vibes.h>
#include <pebble-hourly-vibes/hourly-vibes.h>
#include <enamel.h>
#include <pebble-fctx/fpath.h>
#include "fctx-layer.h"
#include "fctx-text-layer.h"
#include "weather.h"
#include "logging.h"

#ifdef PBL_PLATFORM_APLITE
#define RESOURCE_ID_TEXT_FFONT 0
#define PBL_IF_APLITE_ELSE(A, B) (A)
#else
#define PBL_IF_APLITE_ELSE(A, B) (B)
#endif

#define WIDGET_BUF_LEN 16
#define WIDGET_BUF_SIZEOF(b) sizeof(b) * WIDGET_BUF_LEN

typedef enum {
    WidgetTypeNone = 0,
    WidgetTypeHumidity,
    WidgetTypeFeelsLike,
    WidgetTypeLowTemperature,
    WidgetTypeHighTemperature,
    WidgetTypeBattery,
    WidgetTypeSteps,
    WidgetTypeConnection,
    WidgetTypeHeartRate,
    WidgetTypeDistance,
    WidgetTypeActiveSeconds,
    WidgetTypeSeconds,
    WidgetTypeSunrise,
    WidgetTypeSunset,
    WidgetTypeEnd
} WidgetType;

static Window *s_window;
static FctxLayer *s_root_layer;
static FctxLayer *s_grid_layer;
static FctxLayer *s_weather_icon_layer;
static FctxLayer *s_widget_container_layer;
static FctxTextLayer *s_time_layer;
static FctxTextLayer *s_date_layer;
static FctxTextLayer *s_temperature_layer;
static FctxTextLayer *s_widget_layers[8];

static FctxTextLayer** s_text_layers[] = {
    &s_time_layer,
    &s_date_layer,
    &s_temperature_layer,
    &s_widget_layers[0],
    &s_widget_layers[1],
    &s_widget_layers[2],
    &s_widget_layers[3],
    &s_widget_layers[4],
    &s_widget_layers[5],
    &s_widget_layers[6],
    &s_widget_layers[7],
};
static char s_widget_buffers[WidgetTypeEnd][WIDGET_BUF_LEN];

typedef struct {
    uint32_t resource_id;
    uint8_t scale_factor;
    GPoint advance;
} WeatherIcon;

static WeatherIcon s_weather_icon;

static const WeatherIcon s_weather_icon_na = { RESOURCE_ID_WEATHER_NA, 7, { 0, -9 }};

static const WeatherIcon const s_weather_icons_day[] = {
    { RESOURCE_ID_WEATHER_CLEAR_DAY, 10, { 10, 0 }},
    { RESOURCE_ID_WEATHER_PARTLY_CLOUDY_DAY, 12, { 14, 4 }},
    { RESOURCE_ID_WEATHER_CLOUDY, 10, { 10, 0 }},
    { RESOURCE_ID_WEATHER_CLOUDY, 10, { 10, 0 }},
    { RESOURCE_ID_WEATHER_RAIN, 12, { 14, 4 }},
    { RESOURCE_ID_WEATHER_RAIN, 12, { 14, 4 }},
    { RESOURCE_ID_WEATHER_THUNDER, 12, { 14, 6 }},
    { RESOURCE_ID_WEATHER_SNOW, 12, { 14, 4 }},
    { RESOURCE_ID_WEATHER_MIST, 8, { 4, -4 }},
};

static const WeatherIcon const s_weather_icons_night[] = {
    { RESOURCE_ID_WEATHER_CLEAR_NIGHT, 7, { -4, -11 }},
    { RESOURCE_ID_WEATHER_PARTLY_CLOUDY_NIGHT, 12, { 14, 2 }},
    { RESOURCE_ID_WEATHER_CLOUDY, 10, { 10, 0 }},
    { RESOURCE_ID_WEATHER_CLOUDY, 10, { 10, 0 }},
    { RESOURCE_ID_WEATHER_RAIN, 12, { 14, 4 }},
    { RESOURCE_ID_WEATHER_RAIN, 12, { 14, 4 }},
    { RESOURCE_ID_WEATHER_THUNDER, 12, { 14, 6 }},
    { RESOURCE_ID_WEATHER_SNOW, 12, { 14, 4 }},
    { RESOURCE_ID_WEATHER_MIST, 8, { 4, -4 }},
};

static EventHandle s_tick_timer_event_handle;
static EventHandle s_weather_event_handle;
static EventHandle s_settings_event_handle;
static EventHandle s_battery_state_event_handle;
#ifdef PBL_HEALTH
static EventHandle s_health_event_handle;
#endif
static EventHandle s_connection_event_handle;
static EventHandle s_tap_event_handle;

static bool s_tap_animated;

static void prv_fctx_draw_rect(FContext *fctx, GRect rect) {
    logf();
    fctx_begin_fill(fctx);
    fctx_move_to(fctx, FPoint(rect.origin.x, rect.origin.y));
    fctx_line_to(fctx, FPoint(rect.origin.x + rect.size.w, rect.origin.y));
    fctx_line_to(fctx, FPoint(rect.origin.x + rect.size.w, rect.origin.y + rect.size.h));
    fctx_line_to(fctx, FPoint(rect.origin.x, rect.origin.y + rect.size.h));
    fctx_close_path(fctx);
    fctx_end_fill(fctx);
}

static void prv_grid_layer_update_proc(FctxLayer *this, FContext *fctx) {
    logf();
    fctx_set_fill_color(fctx, enamel_get_COLOR_PINLINE());

    GRect line_rect = GRect(0, 0, 1, 1);
    fctx_set_scale(fctx, FPointOne, FPointI(PBL_DISPLAY_WIDTH, 2));
    fctx_set_offset(fctx, FPointI(0, 72));
    prv_fctx_draw_rect(fctx, line_rect);
}

static void prv_widget_container_layer_update_proc(FctxLayer *this, FContext *fctx) {
    logf();
    GRect frame = fctx_layer_get_frame(this);
    GRect line_rect = GRect(0, 0, 1, 1);

    fctx_set_scale(fctx, FPointOne, FPointI(frame.size.w, frame.size.h));
    fctx_set_fill_color(fctx, enamel_get_COLOR_BACKGROUND());
    prv_fctx_draw_rect(fctx, line_rect);

    fctx_set_fill_color(fctx, enamel_get_COLOR_PINLINE());
    fctx_set_scale(fctx, FPointOne, FPointI(PBL_DISPLAY_WIDTH, 2));

    for (int i = 0; i < 4; i++) {
        fctx_set_offset(fctx, FPointI(0, frame.origin.y + (22 * i)));
        prv_fctx_draw_rect(fctx, line_rect);
    }

    fctx_set_scale(fctx, FPointOne, FPointI(2, PBL_DISPLAY_HEIGHT));
    fctx_set_offset(fctx, FPointI(PBL_DISPLAY_WIDTH / 2 - 1, frame.origin.y));
    prv_fctx_draw_rect(fctx, line_rect);
}

static void prv_weather_icon_layer_update_proc(FctxLayer *this, FContext *fctx) {
    logf();
    FPath *path = fpath_create_from_resource(s_weather_icon.resource_id);
    uint8_t scale_factor = s_weather_icon.scale_factor;
    GPoint advance = s_weather_icon.advance;
    advance.x *= scale_factor;
    advance.y *= scale_factor;

    fctx_set_scale(fctx, FPoint(scale_factor, scale_factor), FPointOne);

    fctx_begin_fill(fctx);
    fctx_set_fill_color(fctx, enamel_get_COLOR_TEXT());
    fctx_draw_commands(fctx, FPointI(advance.x, advance.y), path->data, path->size);
    fctx_end_fill(fctx);

    fpath_destroy(path);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    logf();
    static char buf_time[8];
    strftime(buf_time, sizeof(buf_time), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    if (enamel_get_LEADING_ZERO()) fctx_text_layer_set_text(s_time_layer, buf_time);
    else fctx_text_layer_set_text(s_time_layer, buf_time + ((buf_time[0] == '0') ? 1 : 0));

#ifdef DEMO
    fctx_text_layer_set_text(s_time_layer, "12:34");
#endif

    if (units_changed & SECOND_UNIT) {
        char *s = s_widget_buffers[WidgetTypeSeconds];
        strftime(s, WIDGET_BUF_SIZEOF(s), "SE: %S", tick_time);
    }

    if (units_changed & DAY_UNIT) {
        static char buf_date[16];
        strftime(buf_date, sizeof(buf_date), "%a, %b %d", tick_time);
        fctx_text_layer_set_text(s_date_layer, buf_date);
    }
}

static void prv_weather_handler(GenericWeatherInfo *info, GenericWeatherStatus status, void *context) {
    logf();
    if (info->condition == GenericWeatherConditionUnknown)
        s_weather_icon = s_weather_icon_na;
    else
        s_weather_icon = info->day ? s_weather_icons_day[info->condition] : s_weather_icons_night[info->condition];

#ifdef DEMO
    s_weather_icon = s_weather_icons_day[0];
#endif

    static char buf_temperature[8];
    int unit = atoi(enamel_get_WEATHER_UNIT());
    snprintf(buf_temperature, sizeof(buf_temperature), "%d°", unit == 1 ? info->temp_f : info->temp_c);
    fctx_text_layer_set_text(s_temperature_layer, buf_temperature);

    char *buf_humidity = s_widget_buffers[WidgetTypeHumidity];
    snprintf(buf_humidity, WIDGET_BUF_SIZEOF(buf_humidity), "HU: %d%%", info->humidity);

    char *buf_feels_like = s_widget_buffers[WidgetTypeFeelsLike];
    snprintf(buf_feels_like, WIDGET_BUF_SIZEOF(buf_feels_like), "FL: %d°", unit == 1 ? info->temp_feels_like_f : info->temp_feels_like_c);

    char *buf_temp_low = s_widget_buffers[WidgetTypeLowTemperature];
    snprintf(buf_temp_low, WIDGET_BUF_SIZEOF(buf_temp_low), "LO: %d°", unit == 1 ? info->temp_low_f: info->temp_low_c);

    char *buf_temp_high = s_widget_buffers[WidgetTypeHighTemperature];
    snprintf(buf_temp_high, WIDGET_BUF_SIZEOF(buf_temp_high), "HI: %d°", unit == 1 ? info->temp_high_f: info->temp_high_c);

    struct tm *tick_time = localtime(&info->timesunrise);
    char *buf_sunrise = s_widget_buffers[WidgetTypeSunrise];
    strftime(buf_sunrise, WIDGET_BUF_SIZEOF(buf_sunrise), clock_is_24h_style() ? "SR: %H:%M" : "SR: %I:%M", tick_time);

    tick_time = localtime(&info->timesunset);
    char *buf_sunset = s_widget_buffers[WidgetTypeSunset];
    strftime(buf_sunset, WIDGET_BUF_SIZEOF(buf_sunset), clock_is_24h_style() ? "SS: %H:%M" : "SS: %I:%M", tick_time);

#ifdef DEMO
    fctx_text_layer_set_text(s_temperature_layer, "19°");
    snprintf(buf_humidity, WIDGET_BUF_SIZEOF(buf_humidity), "HU: 80%%");
    snprintf(buf_feels_like, WIDGET_BUF_SIZEOF(buf_feels_like), "FL: 20°");
    snprintf(buf_temp_low, WIDGET_BUF_SIZEOF(buf_temp_low), "LO: 15°");
    snprintf(buf_temp_high, WIDGET_BUF_SIZEOF(buf_temp_high), "HI: 20°");
#endif
}

static void prv_battery_state_handler(BatteryChargeState charge_state) {
    logf();
    char *s = s_widget_buffers[WidgetTypeBattery];
    snprintf(s, WIDGET_BUF_SIZEOF(s), "BT: %d%%", charge_state.charge_percent);
    fctx_layer_mark_dirty(s_root_layer);
}

#ifdef PBL_HEALTH
static void prv_health_handler(HealthEventType event, void *context) {
    logf();
    if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
        time_t start = time_start_of_today();
        time_t end = time(NULL);
        HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricStepCount, start, end);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            HealthValue steps = health_service_sum_today(HealthMetricStepCount);
            char *s = s_widget_buffers[WidgetTypeSteps];
            if (steps < 1000) snprintf(s, WIDGET_BUF_SIZEOF(s), "ST: %ld", steps);
            else snprintf(s, WIDGET_BUF_SIZEOF(s), "ST: %ld.%ldK", steps / 1000, steps / 1000 % 10);
        }

        mask = health_service_metric_accessible(HealthMetricWalkedDistanceMeters, start, end);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            HealthValue distance = health_service_sum_today(HealthMetricWalkedDistanceMeters);
            MeasurementSystem system = health_service_get_measurement_system_for_display(HealthMetricWalkedDistanceMeters);
            char *s = s_widget_buffers[WidgetTypeDistance];
            if (system == MeasurementSystemMetric) {
                if (distance < 100) snprintf(s, WIDGET_BUF_SIZEOF(s), "DI: %ldm", distance);
                else if (distance < 1000) snprintf(s, WIDGET_BUF_SIZEOF(s), "DI: .%ldkm", distance / 100);
                else snprintf(s, WIDGET_BUF_SIZEOF(s), "DI: %ldkm", distance / 1000);
            } else {
                uint tenths = distance * 10 / 1609 % 10;
                uint whole = distance / 1609;
                if (whole < 10) snprintf(s, WIDGET_BUF_SIZEOF(s), "DI: %d.%dmi", whole, tenths);
                else snprintf(s, WIDGET_BUF_SIZEOF(s), "DI: %dmi", whole);
            }

            mask = health_service_metric_accessible(HealthMetricActiveSeconds, start, end);
            if (mask & HealthServiceAccessibilityMaskAvailable) {
                HealthValue active_seconds = health_service_sum_today(HealthMetricActiveSeconds);
                uint minutes = active_seconds / 60;
                uint hours = minutes / 60;
                minutes %= 60;

                struct tm t = {
                    .tm_hour = hours,
                    .tm_min = minutes
                };
                char *s = s_widget_buffers[WidgetTypeActiveSeconds];
                strftime(s, WIDGET_BUF_SIZEOF(s), "AT:%k:%M", &t);
            }
        }

        fctx_layer_mark_dirty(s_root_layer);
    }

#ifdef PBL_PLATFORM_DIORITE
    if (event == HealthEventSignificantUpdate || event == HealthEventHeartRateUpdate) {
        time_t now = time(NULL);
        HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricHeartRateBPM, now, now);
        if (mask & HealthServiceAccessibilityMaskAvailable) {
            HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
            char *s = s_widget_buffers[WidgetTypeHeartRate];
            snprintf(s, WIDGET_BUF_SIZEOF(s), "HR: %ld", hr);
        }

        fctx_layer_mark_dirty(s_root_layer);
    }
#endif // PBL_PLATFORM_DIORITE
}
#endif // PBL_HEALTH

static void prv_connection_handler(bool connected) {
    logf();
    char *s = s_widget_buffers[WidgetTypeConnection];
    snprintf(s, WIDGET_BUF_SIZEOF(s), "CN: %s", connected ? "ON" : "OFF");
    fctx_layer_mark_dirty(s_root_layer);
}

static void prv_tap_animation_stopped(Animation *animation, bool finished, void *context) {
    logf();
    s_tap_animated = false;
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
    logf();
    if (s_tap_animated) return;
    s_tap_animated = true;

    GRect from = fctx_layer_get_frame(s_widget_container_layer);
    GRect to = GRect(from.origin.x, from.origin.y - 44, from.size.w, from.size.h);
    PropertyAnimation *animation = property_animation_create_layer_frame(fctx_layer_get_layer(s_widget_container_layer), &from, &to);

    Animation *clone = animation_clone(property_animation_get_animation(animation));
    animation_set_handlers(clone, (AnimationHandlers) {
        .stopped = prv_tap_animation_stopped
    }, NULL);
    animation_set_reverse(clone, true);
    animation_set_delay(clone, 5000);

    Animation *sequence = animation_sequence_create(property_animation_get_animation(animation), clone, NULL);
    s_tap_animated = animation_schedule(sequence);
}

static bool prv_has_widget_type(WidgetType type) {
    WidgetType widget_nw = atoi(enamel_get_WIDGET_NW());
    WidgetType widget_ne = atoi(enamel_get_WIDGET_NE());
    WidgetType widget_sw = atoi(enamel_get_WIDGET_SW());
    WidgetType widget_se = atoi(enamel_get_WIDGET_SE());
    bool widgets = widget_nw == type || widget_ne == type || widget_sw == type || widget_se == type;

    if (!enamel_get_EXTRA_WIDGETS_ENABLED()) return widgets;

    widget_nw = atoi(enamel_get_EXTRA_WIDGET_NW());
    widget_ne = atoi(enamel_get_EXTRA_WIDGET_NE());
    widget_sw = atoi(enamel_get_WIDGET_SW());
    widget_se = atoi(enamel_get_WIDGET_SE());
    bool extra_widgets = widget_nw == type || widget_ne == type || widget_sw == type || widget_se == type;
    return widgets || extra_widgets;
}

static void prv_settings_handler(void *context) {
    logf();
    prv_weather_handler(weather_peek(), weather_status_peek(), NULL);

    connection_vibes_set_state(atoi(enamel_get_CONNECTION_VIBE()));
    hourly_vibes_set_enabled(enamel_get_HOURLY_VIBE());

    for (uint i = 0; i < ARRAY_LENGTH(s_text_layers); i++) {
        fctx_text_layer_set_color(*s_text_layers[i], enamel_get_COLOR_TEXT());
    }

    bool needs_seconds = prv_has_widget_type(WidgetTypeSeconds);
    if (s_tick_timer_event_handle) events_tick_timer_service_unsubscribe(s_tick_timer_event_handle);
    time_t now = time(NULL);
    prv_tick_handler(localtime(&now), DAY_UNIT | SECOND_UNIT);
    s_tick_timer_event_handle = events_tick_timer_service_subscribe(needs_seconds ? SECOND_UNIT : MINUTE_UNIT, prv_tick_handler);

    bool needs_battery = prv_has_widget_type(WidgetTypeBattery);
    if (needs_battery && !s_battery_state_event_handle) {
        prv_battery_state_handler(battery_state_service_peek());
        s_battery_state_event_handle = events_battery_state_service_subscribe(prv_battery_state_handler);
    } else if (!needs_battery && s_battery_state_event_handle) {
        events_battery_state_service_unsubscribe(s_battery_state_event_handle);
        s_battery_state_event_handle = NULL;
    }

#ifdef PBL_HEALTH
    bool needs_health = prv_has_widget_type(WidgetTypeSteps) ||
                        prv_has_widget_type(WidgetTypeDistance) ||
                        prv_has_widget_type(WidgetTypeHeartRate);
    if (needs_health && !s_health_event_handle) {
        prv_health_handler(HealthEventSignificantUpdate, NULL);
        s_health_event_handle = events_health_service_events_subscribe(prv_health_handler, NULL);
    } else if (!needs_health && s_health_event_handle) {
        events_health_service_events_unsubscribe(s_health_event_handle);
        s_health_event_handle = NULL;
    }

    connection_vibes_enable_health(needs_health);
    hourly_vibes_enable_health(needs_health);
#endif

    bool needs_connection = prv_has_widget_type(WidgetTypeConnection);
    if (needs_connection && !s_connection_event_handle) {
        prv_connection_handler(connection_service_peek_pebble_app_connection());
        s_connection_event_handle = events_connection_service_subscribe((ConnectionHandlers) {
            .pebble_app_connection_handler = prv_connection_handler
        });
    } else if (!needs_connection && s_connection_event_handle) {
        events_connection_service_unsubscribe(s_connection_event_handle);
        s_connection_event_handle = NULL;
    }

    if (enamel_get_EXTRA_WIDGETS_ENABLED() && !s_tap_event_handle) {
        s_tap_event_handle = events_accel_tap_service_subscribe(prv_tap_handler);
    } else if (!enamel_get_EXTRA_WIDGETS_ENABLED() && s_tap_event_handle) {
        events_accel_tap_service_unsubscribe(s_tap_event_handle);
        s_tap_event_handle = NULL;
    }

    WidgetType widget_types[] = {
        atoi(enamel_get_WIDGET_NW()),
        atoi(enamel_get_WIDGET_NE()),
        atoi(enamel_get_WIDGET_SW()),
        atoi(enamel_get_WIDGET_SE()),
        atoi(enamel_get_EXTRA_WIDGET_NW()),
        atoi(enamel_get_EXTRA_WIDGET_NE()),
        atoi(enamel_get_EXTRA_WIDGET_SW()),
        atoi(enamel_get_EXTRA_WIDGET_SE()),
    };

    for (uint i = 0; i < ARRAY_LENGTH(s_widget_layers); i++) {
        fctx_text_layer_set_text(s_widget_layers[i], s_widget_buffers[widget_types[i]]);
    }

    window_set_background_color(s_window, enamel_get_COLOR_BACKGROUND());
}

static void prv_window_load(Window *window) {
    logf();
    s_root_layer = window_get_root_fctx_layer(window);

    s_grid_layer = fctx_layer_create(GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    fctx_layer_set_update_proc(s_grid_layer, prv_grid_layer_update_proc);
    fctx_layer_add_child(s_root_layer, s_grid_layer);

    s_time_layer = fctx_text_layer_create(GRect(PBL_IF_APLITE_ELSE(0, PBL_DISPLAY_WIDTH / 2), PBL_IF_APLITE_ELSE(-8, 0), PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    fctx_text_layer_set_font(s_time_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_time_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_time_layer, FTextAnchorTop);
    fctx_text_layer_set_color(s_time_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_time_layer, 56);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_time_layer));

    s_date_layer = fctx_text_layer_create(GRect(PBL_IF_APLITE_ELSE(0, PBL_DISPLAY_WIDTH / 2), PBL_IF_APLITE_ELSE(42, 70), PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    fctx_text_layer_set_font(s_date_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_date_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_date_layer, FTextAnchorBottom);
    fctx_text_layer_set_color(s_date_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_date_layer, 18);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_date_layer));

    s_weather_icon_layer = fctx_layer_create(GRect(0, 74, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    fctx_layer_set_update_proc(s_weather_icon_layer, prv_weather_icon_layer_update_proc);
    fctx_layer_add_child(s_root_layer, s_weather_icon_layer);

#ifdef PBL_PLATFORM_APLITE
    s_temperature_layer = fctx_text_layer_create(GRect(PBL_DISPLAY_WIDTH / 2, 80, PBL_DISPLAY_WIDTH / 2, PBL_DISPLAY_HEIGHT));
#else
    s_temperature_layer = fctx_text_layer_create(GRect(PBL_DISPLAY_WIDTH - (PBL_DISPLAY_WIDTH / 4), 74 + 25, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
#endif
    fctx_text_layer_set_font(s_temperature_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_temperature_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_temperature_layer, FTextAnchorMiddle);
    fctx_text_layer_set_color(s_temperature_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_temperature_layer, 36);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_temperature_layer));

    s_widget_container_layer = fctx_layer_create(GRect(0, 72 + 52, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT));
    fctx_layer_set_update_proc(s_widget_container_layer, prv_widget_container_layer_update_proc);
    fctx_layer_add_child(s_root_layer, s_widget_container_layer);

    uint8_t widget_width = PBL_DISPLAY_WIDTH / 2 - 1;
    for (uint i = 0; i < ARRAY_LENGTH(s_widget_layers); i++) {
        uint8_t y = (22 * (i / 2)) + PBL_IF_APLITE_ELSE(0, 5);
        uint8_t x;
        if (i % 2 == 0) x = PBL_IF_APLITE_ELSE(0, PBL_DISPLAY_WIDTH / 4);
        else x = PBL_IF_APLITE_ELSE(PBL_DISPLAY_WIDTH / 2 + 1, PBL_DISPLAY_WIDTH - (PBL_DISPLAY_WIDTH / 4));
        s_widget_layers[i] = fctx_text_layer_create(GRect(x, y, widget_width, 20));
        fctx_text_layer_set_font(s_widget_layers[i], RESOURCE_ID_TEXT_FFONT);
        fctx_text_layer_set_alignment(s_widget_layers[i], GTextAlignmentCenter);
        fctx_text_layer_set_anchor(s_widget_layers[i], FTextAnchorTop);
        fctx_text_layer_set_color(s_widget_layers[i], GColorWhite);
        fctx_text_layer_set_text_size(s_widget_layers[i], 16);
        fctx_layer_add_child(s_widget_container_layer, fctx_text_layer_get_fctx_layer(s_widget_layers[i]));
    }

    memset(s_widget_buffers, 0, sizeof(s_widget_buffers));

    s_weather_event_handle = events_weather_subscribe(prv_weather_handler, NULL);

    prv_settings_handler(NULL);
    s_settings_event_handle = enamel_settings_received_subscribe(prv_settings_handler, NULL);
}

static void prv_window_unload(Window *window) {
    logf();
    if (s_connection_event_handle) events_connection_service_unsubscribe(s_connection_event_handle);
#ifdef PBL_HEALTH
    if (s_health_event_handle) events_health_service_events_unsubscribe(s_health_event_handle);
#endif
    if (s_battery_state_event_handle) events_battery_state_service_unsubscribe(s_battery_state_event_handle);
    enamel_settings_received_unsubscribe(s_settings_event_handle);
    if (s_tap_event_handle) events_accel_tap_service_unsubscribe(s_tap_event_handle);
    events_weather_unsubscribe(s_weather_event_handle);
    events_tick_timer_service_unsubscribe(s_tick_timer_event_handle);

    for(uint i = 0; i < ARRAY_LENGTH(s_text_layers); i++) fctx_text_layer_destroy(*s_text_layers[i]);

    fctx_layer_destroy(s_weather_icon_layer);
    fctx_layer_destroy(s_widget_container_layer);
    fctx_layer_destroy(s_grid_layer);
    fctx_layer_destroy(s_root_layer);
}

static void prv_init(void) {
    logf();
    setlocale(LC_ALL, "");

    enamel_init();
    weather_init();
    connection_vibes_init();
    hourly_vibes_init();
    uint32_t const pattern[] = { 100 };
    hourly_vibes_set_pattern((VibePattern) {
        .durations = pattern,
        .num_segments = 1
    });

    events_app_message_open();

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload
    });
    window_stack_push(s_window, true);
}

static void prv_deinit(void) {
    logf();
    window_destroy(s_window);

    hourly_vibes_deinit();
    connection_vibes_deinit();
    weather_deinit();
    enamel_deinit();
}

int main(void) {
    logf();
    prv_init();
    app_event_loop();
    prv_deinit();
}
