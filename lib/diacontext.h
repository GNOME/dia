#ifndef DIA_CONTEXT_H
#define DIA_CONTEXT_H

#include "diatypes.h"
#include <glib.h>

G_BEGIN_DECLS

DiaContext *dia_context_new (const char *desc);
void dia_context_reset (DiaContext *context);
void dia_context_release (DiaContext *context);

void dia_context_set_filename (DiaContext *context, const char *filename);
void dia_context_add_message (DiaContext *context, const char *fomat, ...) G_GNUC_PRINTF(2, 3);
void dia_context_add_message_with_errno (DiaContext *context, int nr, const char *fomat, ...) G_GNUC_PRINTF(3, 4);

const char *dia_context_get_filename (DiaContext *context);

G_END_DECLS


#endif /* DIA_CONTEXT_H */
