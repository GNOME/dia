#ifndef LIBDIA_H
#define LIBDIA_H

#include <glib.h>

G_BEGIN_DECLS


enum DiaInitFlags {
  DIA_INTERACTIVE = (1<<0),
  DIA_MESSAGE_STDERR = (1<<1),
  DIA_VERBOSE = (1<<2)
};

void libdia_init (guint flags);

G_END_DECLS
#endif /*LIBDIA_H */
