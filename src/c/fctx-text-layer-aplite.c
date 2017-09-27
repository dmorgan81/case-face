#ifdef PBL_PLATFORM_APLITE
#include <pebble.h>
#include "fctx-layer.h"
#include "fctx-text-layer.h"
#include "logging.h"

struct FctxTextLayer {
    FctxLayer *layer;
    TextLayer *text_layer;
};

FctxTextLayer *fctx_text_layer_create(const GRect frame) {
    logf();
    FctxLayer *layer = fctx_layer_create_with_data(GRect(0, 0, PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT), sizeof(FctxTextLayer));
    FctxTextLayer *this = fctx_layer_get_data(layer);
    this->layer = layer;

    this->text_layer = text_layer_create(frame);
    text_layer_set_background_color(this->text_layer, GColorClear);
    layer_set_clips(text_layer_get_layer(this->text_layer), false);
    layer_add_child(fctx_layer_get_layer(layer), text_layer_get_layer(this->text_layer));

    return this;
}

void fctx_text_layer_destroy(FctxTextLayer *this) {
    logf();
    text_layer_destroy(this->text_layer);
    fctx_layer_destroy(this->layer);
}

FctxLayer *fctx_text_layer_get_fctx_layer(const FctxTextLayer *this) {
    logf();
    return this->layer;
}

void fctx_text_layer_set_text(FctxTextLayer *this, const char *text) {
    logf();
    text_layer_set_text(this->text_layer, text);
}

const char *fctx_text_layer_get_text(const FctxTextLayer *this) {
    logf();
    return text_layer_get_text(this->text_layer);
}

void fctx_text_layer_set_font(FctxTextLayer *this, uint32_t font) {
    logf();
    // no-op
}

void fctx_text_layer_set_color(FctxTextLayer *this, GColor color) {
    logf();
    text_layer_set_text_color(this->text_layer, color);
}

void fctx_text_layer_set_text_size(FctxTextLayer *this, int16_t pixels) {
    logf();
    GFont font;
    if (pixels == 56) font = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
    else if (pixels == 36) font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);
    else if (pixels == 18) font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    else font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

    text_layer_set_font(this->text_layer, font);
}

void fctx_text_layer_set_alignment(FctxTextLayer *this, GTextAlignment alignment) {
    logf();
    text_layer_set_text_alignment(this->text_layer, alignment);
}

void fctx_text_layer_set_anchor(FctxTextLayer *this, FTextAnchor anchor) {
    logf();
    // no-op
}
#endif
