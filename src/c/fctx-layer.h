#pragma once
#include <pebble.h>
#include <pebble-fctx/fctx.h>

typedef struct FctxLayer FctxLayer;
typedef void (*FctxLayerUpdateProc)(struct FctxLayer *this, FContext* fctx);

FctxLayer *window_get_root_fctx_layer(const Window *window);
FctxLayer *fctx_layer_create(const GPoint origin);
FctxLayer *fctx_layer_create_with_data(const GPoint origin, const size_t size);
void fctx_layer_set_update_proc(FctxLayer *this, FctxLayerUpdateProc update_proc);
void fctx_layer_destroy(FctxLayer *this);

Layer *fctx_layer_get_layer(const FctxLayer *this);
void *fctx_layer_get_data(const FctxLayer *this);
void fctx_layer_add_child(FctxLayer *this, FctxLayer *child);
void fctx_layer_remove_from_parent(FctxLayer *this);
void fctx_layer_mark_dirty(const FctxLayer *this);

bool fctx_layer_get_hidden(const FctxLayer *this);
void fctx_layer_set_hidden(FctxLayer *this, bool hidden);

GPoint fctx_layer_get_origin(const FctxLayer *this);
