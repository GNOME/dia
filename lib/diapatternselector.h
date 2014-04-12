#include "pattern.h"
#include <gtk/gtk.h>

GtkWidget *dia_pattern_selector_new (void);
void dia_pattern_selector_set_pattern (GtkWidget *sel, DiaPattern *pat);
DiaPattern *dia_pattern_selector_get_pattern (GtkWidget *sel);
