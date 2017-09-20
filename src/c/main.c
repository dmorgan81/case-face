#include <pebble.h>
#include "logging.h"

static Window *s_window;

static void prv_window_load(Window *window) {
    logf();
    window_set_background_color(window, GColorBlack);
}

static void prv_window_unload(Window *window) {
    logf();
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
