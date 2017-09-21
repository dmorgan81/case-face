#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include "fctx-layer.h"
#include "fctx-text-layer.h"
#include "logging.h"

static Window *s_window;
static FctxLayer *s_root_layer;
static FctxTextLayer *s_time_layer;

static EventHandle s_tick_timer_event_handle;

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    logf();
    static char s[8];
    strftime(s, sizeof(s), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
    fctx_text_layer_set_text(s_time_layer, s);
}

static void prv_window_load(Window *window) {
    logf();
    s_root_layer = window_get_root_fctx_layer(window);

    s_time_layer = fctx_text_layer_create(GPoint(PBL_DISPLAY_WIDTH / 2, PBL_DISPLAY_HEIGHT / 2));
    fctx_text_layer_set_font(s_time_layer, RESOURCE_ID_ROBOTO_REGULAR_FFONT);
    fctx_text_layer_set_alignment(s_time_layer, GTextAlignmentCenter);
    fctx_text_layer_set_anchor(s_time_layer, FTextAnchorMiddle);
    fctx_text_layer_set_color(s_time_layer, GColorWhite);
    fctx_text_layer_set_text_size(s_time_layer, 50);
    fctx_layer_add_child(s_root_layer, fctx_text_layer_get_fctx_layer(s_time_layer));

    time_t now = time(NULL);
    prv_tick_handler(localtime(&now), MINUTE_UNIT);
    s_tick_timer_event_handle = events_tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);

    window_set_background_color(window, GColorBlack);
}

static void prv_window_unload(Window *window) {
    logf();
    events_tick_timer_service_unsubscribe(s_tick_timer_event_handle);

    fctx_text_layer_destroy(s_time_layer);
    fctx_layer_destroy(s_root_layer);
}

static void prv_init(void) {
    logf();
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
}

int main(void) {
    logf();
    prv_init();
    app_event_loop();
    prv_deinit();
}
