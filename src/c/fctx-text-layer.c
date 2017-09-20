#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include "fctx-layer.h"
#include "fctx-text-layer.h"
#include "logging.h"

struct FctxTextLayer {
    FctxLayer *layer;
    const char *text;
    uint32_t font;
    GColor color;
    int16_t text_size;
    GTextAlignment alignment;
    FTextAnchor anchor;
};

static void prv_update_proc(FctxLayer *layer, FContext *fctx) {
    logf();
    FctxTextLayer *this = fctx_layer_get_data(layer);
    if (!this->text || this->text_size <= 0 || gcolor_equal(this->color, GColorClear)) return;

    FFont *font = ffont_create_from_resource(this->font);
    if (!font) return;

    fctx_set_text_em_height(fctx, font, this->text_size);
    fctx_set_fill_color(fctx, this->color);

    fctx_begin_fill(fctx);
    fctx_draw_string(fctx, this->text, font, this->alignment, this->anchor);
    fctx_end_fill(fctx);

    ffont_destroy(font);
}

FctxTextLayer *fctx_text_layer_create(const GPoint origin) {
    logf();
    FctxLayer *layer = fctx_layer_create_with_data(origin, sizeof(FctxTextLayer));
    fctx_layer_set_update_proc(layer, prv_update_proc);
    FctxTextLayer *this = fctx_layer_get_data(layer);
    this->layer = layer;
    this->text = NULL;
    this->font = 0;
    this->color = GColorClear;
    this->text_size = 0;
    this->alignment = GTextAlignmentLeft;
    this->anchor = FTextAnchorTop;
    return this;
}

void fctx_text_layer_destroy(FctxTextLayer *this) {
    logf();
    fctx_layer_destroy(this->layer);
}

FctxLayer *fctx_text_layer_get_fctx_layer(const FctxTextLayer *this) {
    logf();
    return this->layer;
}

void fctx_text_layer_set_text(FctxTextLayer *this, const char *text) {
    logf();
    this->text = text;
    fctx_layer_mark_dirty(this->layer);
}

const char *fctx_text_layer_get_text(const FctxTextLayer *this) {
    logf();
    return this->text;
}

void fctx_text_layer_set_font(FctxTextLayer *this, uint32_t font) {
    logf();
    this->font = font;
    fctx_layer_mark_dirty(this->layer);
}

void fctx_text_layer_set_color(FctxTextLayer *this, GColor color) {
    logf();
    this->color = color;
    fctx_layer_mark_dirty(this->layer);
}

void fctx_text_layer_set_text_size(FctxTextLayer *this, int16_t text_size) {
    logf();
    this->text_size = text_size;
    fctx_layer_mark_dirty(this->layer);
}

void fctx_text_layer_set_alignment(FctxTextLayer *this, GTextAlignment alignment) {
    logf();
    this->alignment = alignment;
    fctx_layer_mark_dirty(this->layer);
}

void fctx_text_layer_set_anchor(FctxTextLayer *this, FTextAnchor anchor) {
    logf();
    this->anchor = anchor;
    fctx_layer_mark_dirty(this->layer);
}
