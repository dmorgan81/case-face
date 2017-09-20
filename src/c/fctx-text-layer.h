#pragma once
#include "fctx-layer.h"

typedef struct FctxTextLayer FctxTextLayer;

FctxTextLayer *fctx_text_layer_create(const GPoint origin);
void fctx_text_layer_destroy(FctxTextLayer *this);
FctxLayer *fctx_text_layer_get_fctx_layer(const FctxTextLayer *this);
void fctx_text_layer_set_text(FctxTextLayer *this, const char *text);
const char *fctx_text_layer_get_text(const FctxTextLayer *this);
void fctx_text_layer_set_font(FctxTextLayer *this, uint32_t font);
void fctx_text_layer_set_color(FctxTextLayer *this, GColor color);
void fctx_text_layer_set_text_size(FctxTextLayer *this, int16_t pixels);
void fctx_text_layer_set_alignment(FctxTextLayer *this, GTextAlignment alignment);
void fctx_text_layer_set_anchor(FctxTextLayer *this, FTextAnchor anchor);
