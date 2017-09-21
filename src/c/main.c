#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include <pebble-connection-vibes/connection-vibes.h>
#include <pebble-hourly-vibes/hourly-vibes.h>
#include <enamel.h>
#include "fctx-layer.h"
#include "fctx-text-layer.h"
#include "weather.h"
#include "logging.h"

#define WIDGET_BUF_LEN 16
#define WIDGET_BUF_SIZEOF(b) sizeof(b) * WIDGET_BUF_LEN

typedef enum {
    WidgetTypeNone = 0,
    WidgetTypeHumidity,
    WidgetTypeFeelsLike,
    WidgetTypeLowTemperature,
    WidgetTypeHighTemperature,
    WidgetTypeEnd
} WidgetType;

static Window *s_window;
static FctxLayer *s_root_layer;
static FctxLayer *s_grid_layer;
static FctxTextLayer *s_time_layer;
static FctxTextLayer *s_date_layer;
static FctxTextLayer *s_weather_icon_layer;
static FctxTextLayer *s_temperature_layer;
static FctxTextLayer *s_widget_nw_layer;
static FctxTextLayer *s_widget_ne_layer;
static FctxTextLayer *s_widget_sw_layer;
static FctxTextLayer *s_widget_se_layer;

static GPoint s_weather_icon_origin;
static FctxTextLayer* s_text_layers[8];
static char s_widget_buffers[WidgetTypeEnd][WIDGET_BUF_LEN];

typedef struct {
    char *s;
    GPoint p;
} WeatherIcon;

static const WeatherIcon s_weather_icon_na = { "I", { 0, -2 }};

static const WeatherIcon const s_weather_icons_day[] = {
    { "A", { -2, -2 }},
    { "B", { 0, 4 }},
    { "C", { 0, -2 }},
    { "D", { 0, -6 }},
    { "E", { 0, -6 }},
    { "F", { 0, -6 }},
    { "G", { 0, -6 }},
    { "H", { 0, -6 }},
};

static const WeatherIcon const s_weather_icons_night[] = {
    { "a", { -2, -2 }},
    { "b", { 0, 0 }},
    { "C", { 0, -2 }},
    { "D", { 0, -6 }},
    { "E", { 0, -6 }},
    { "F", { 0, -6 }},
    { "G", { 0, -6 }},
    { "H", { 0, -6 }},
};

static EventHandle s_tick_timer_event_handle;
static EventHandle s_weather_event_handle;
static EventHandle s_settings_event_handle;

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

    fctx_set_offset(fctx, FPointI(0, 72 + 50 + 2));
    prv_fctx_draw_rect(fctx, line_rect);

    fctx_set_offset(fctx, FPointI(0, 72 + 50 + 20 + 4));
    prv_fctx_draw_rect(fctx, line_rect);

    fctx_set_scale(fctx, FPointOne, FPointI(2, 42));
    fctx_set_offset(fctx, FPointI(PBL_DISPLAY_WIDTH / 2 - 1, PBL_DISPLAY_HEIGHT - 42));
    prv_fctx_draw_rect(fctx, line_rect);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    logf();
    static char buf_time[8];
    strftime(buf_time, sizeof(buf_time), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    if (enamel_get_LEADING_ZERO()) fctx_text_layer_set_text(s_time_layer, buf_time);
    else fctx_text_layer_set_text(s_time_layer, buf_time + ((buf_time[0] == '0') ? 1 : 0));

    if (units_changed & DAY_UNIT) {
        static char buf_date[16];
        strftime(buf_date, sizeof(buf_date), "%a, %b %d", tick_time);
        fctx_text_layer_set_text(s_date_layer, buf_date);
    }
}

static void prv_weather_handler(GenericWeatherInfo *info, GenericWeatherStatus status, void *context) {
    WeatherIcon weather_icon;
    if (info->condition == GenericWeatherConditionUnknown)
        weather_icon = s_weather_icon_na;
    else
        weather_icon = info->day ? s_weather_icons_day[info->condition] : s_weather_icons_night[info->condition];
    fctx_text_layer_set_text(s_weather_icon_layer, weather_icon.s);

    GPoint origin = gpoint_add(s_weather_icon_origin, weather_icon.p);
    fctx_layer_set_origin(fctx_text_layer_get_fctx_layer(s_weather_icon_layer), origin);

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
}

static void prv_settings_handler(void *context) {
    logf();
    time_t now = time(NULL);
    prv_tick_handler(localtime(&now), DAY_UNIT | MONTH_UNIT);

    prv_weather_handler(weather_peek(), weather_status_peek(), NULL);

    connection_vibes_set_state(atoi(enamel_get_CONNECTION_VIBE()));
    hourly_vibes_set_enabled(enamel_get_HOURLY_VIBE());

    for (uint i = 0; i < ARRAY_LENGTH(s_text_layers); i++) {
        fctx_text_layer_set_color(s_text_layers[i], enamel_get_COLOR_TEXT());
    }

    fctx_text_layer_set_text(s_widget_nw_layer, s_widget_buffers[atoi(enamel_get_WIDGET_NW())]);
    fctx_text_layer_set_text(s_widget_ne_layer, s_widget_buffers[atoi(enamel_get_WIDGET_NE())]);
    fctx_text_layer_set_text(s_widget_sw_layer, s_widget_buffers[atoi(enamel_get_WIDGET_SW())]);
    fctx_text_layer_set_text(s_widget_se_layer, s_widget_buffers[atoi(enamel_get_WIDGET_SE())]);

    window_set_background_color(s_window, enamel_get_COLOR_BACKGROUND());
}

static void prv_window_load(Window *window) {
    logf();
    s_root_layer = window_get_root_fctx_layer(window);

    s_grid_layer = fctx_layer_create(GPointZero);
    fctx_layer_set_update_proc(s_grid_layer, prv_grid_layer_update_proc);
    fctx_layer_add_child(s_root_layer, s_grid_layer);

    s_time_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH / 2, 0));
    fctx_text_layer_set_font(s_time_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_time_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_time_layer, FTextAnchorTop);
    fctx_text_layer_set_color(s_time_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_time_layer, 56);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_time_layer));

    s_date_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH / 2, 70));
    fctx_text_layer_set_font(s_date_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_date_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_date_layer, FTextAnchorBottom);
    fctx_text_layer_set_color(s_date_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_date_layer, 18);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_date_layer));

    s_weather_icon_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH / 4, 74 + 25));
    fctx_text_layer_set_font(s_weather_icon_layer, RESOURCE_ID_WEATHER_ICONS_FFONT);
    fctx_text_layer_set_alignment(s_weather_icon_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_weather_icon_layer, FTextAnchorMiddle);
    fctx_text_layer_set_color(s_weather_icon_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_weather_icon_layer, 36);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_weather_icon_layer));
    s_weather_icon_origin = fctx_layer_get_origin(fctx_text_layer_get_fctx_layer(s_weather_icon_layer));

    s_temperature_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH - (PBL_DISPLAY_WIDTH / 4), 74 + 25));
    fctx_text_layer_set_font(s_temperature_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_temperature_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_temperature_layer, FTextAnchorMiddle);
    fctx_text_layer_set_color(s_temperature_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_temperature_layer, 36);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_temperature_layer));

    s_widget_nw_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH / 4, 74 + 52 + 4));
    fctx_text_layer_set_font(s_widget_nw_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_widget_nw_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_widget_nw_layer, FTextAnchorTop);
    fctx_text_layer_set_color(s_widget_nw_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_widget_nw_layer, 16);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_widget_nw_layer));

    s_widget_ne_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH - (PBL_DISPLAY_WIDTH / 4), 74 + 52 + 4));
    fctx_text_layer_set_font(s_widget_ne_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_widget_ne_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_widget_ne_layer, FTextAnchorTop);
    fctx_text_layer_set_color(s_widget_ne_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_widget_ne_layer, 16);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_widget_ne_layer));

    s_widget_sw_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH / 4, 74 + 52 + 22 + 4));
    fctx_text_layer_set_font(s_widget_sw_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_widget_sw_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_widget_sw_layer, FTextAnchorTop);
    fctx_text_layer_set_color(s_widget_sw_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_widget_sw_layer, 16);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_widget_sw_layer));

    s_widget_se_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH - (PBL_DISPLAY_WIDTH / 4), 74 + 52 + 22 + 4));
    fctx_text_layer_set_font(s_widget_se_layer, RESOURCE_ID_TEXT_FFONT);
    fctx_text_layer_set_alignment(s_widget_se_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_widget_se_layer, FTextAnchorTop);
    fctx_text_layer_set_color(s_widget_se_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_widget_se_layer, 16);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_widget_se_layer));

    memset(s_widget_buffers, 0, sizeof(s_widget_buffers));

    s_text_layers[0] = s_time_layer;
    s_text_layers[1] = s_date_layer;
    s_text_layers[2] = s_weather_icon_layer;
    s_text_layers[3] = s_temperature_layer;
    s_text_layers[4] = s_widget_nw_layer;
    s_text_layers[5] = s_widget_ne_layer;
    s_text_layers[6] = s_widget_sw_layer;
    s_text_layers[7] = s_widget_se_layer;

    s_tick_timer_event_handle = events_tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
    s_weather_event_handle = events_weather_subscribe(prv_weather_handler, NULL);

    prv_settings_handler(NULL);
    s_settings_event_handle = enamel_settings_received_subscribe(prv_settings_handler, NULL);
}

static void prv_window_unload(Window *window) {
    logf();
    enamel_settings_received_unsubscribe(s_settings_event_handle);
    events_weather_unsubscribe(s_weather_event_handle);
    events_tick_timer_service_unsubscribe(s_tick_timer_event_handle);

    fctx_text_layer_destroy(s_widget_sw_layer);
    fctx_text_layer_destroy(s_widget_se_layer);
    fctx_text_layer_destroy(s_widget_ne_layer);
    fctx_text_layer_destroy(s_widget_nw_layer);
    fctx_text_layer_destroy(s_temperature_layer);
    fctx_text_layer_destroy(s_weather_icon_layer);
    fctx_text_layer_destroy(s_date_layer);
    fctx_text_layer_destroy(s_time_layer);
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
