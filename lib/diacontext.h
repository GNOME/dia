#pragma once

#include <glib-object.h>

#include "diatypes.h"

G_BEGIN_DECLS

#define DIA_TYPE_CONTEXT (dia_context_get_type ())

G_DECLARE_FINAL_TYPE (DiaContext, dia_context, DIA, CONTEXT, GObject)


DiaContext *dia_context_new (const char *desc);
void dia_context_reset (DiaContext *context);
void dia_context_release (DiaContext *context);

void dia_context_set_filename (DiaContext *context, const char *filename);
void dia_context_add_message (DiaContext *context, const char *fomat, ...) G_GNUC_PRINTF(2, 3);
void dia_context_add_message_with_errno (DiaContext *context, int nr, const char *fomat, ...) G_GNUC_PRINTF(3, 4);

const char *dia_context_get_filename (DiaContext *context);

G_END_DECLS
