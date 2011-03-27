#include <glib.h>

gboolean vmem_avail (guint64 *size);
gboolean vmem_reserve (guint64 size);
void vmem_release (void);


