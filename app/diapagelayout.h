#ifndef DIAPAGELAYOUT_H
#define DIAPAGELAYOUT_H

#include <gtk/gtk.h>

#define DIA_PAGE_LAYOUT(obj) GTK_CHECK_CAST(obj, dia_page_layout_get_type(), DiaPageLayout)
#define DIA_PAGE_LAYOUT_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, dia_page_layout_get_type(), DiaPageLayoutClass)
#define DIA_IS_PAGE_LAYOUT(obj) GTK_CHECK_TYPE(obj, dia_page_layout_get_type())

typedef struct _DiaPageLayout DiaPageLayout;
typedef struct _DiaPageLayoutClass DiaPageLayoutClass;

struct _DiaPageLayout {
  GtkTable parent;

  /*<private>*/
  GtkWidget *paper_size;
  GtkWidget *orient_portrait, *orient_landscape;
  GtkWidget *tmargin, *bmargin, *lmargin, *rmargin;
  GtkWidget *scaling;
  GtkWidget *fittopage;

  GtkWidget *darea;

  GdkGC *gc;
  GdkColor white, black, blue;
  gint papernum; /* index into page_metrics array */

  /* position of paper preview */
  gint x, y, width, height;

  gboolean block_changed;
};

struct _DiaPageLayoutClass {
  GtkTableClass parent_class;

  void (*changed)(DiaPageLayout *pl);
  void (*fittopage)(DiaPageLayout *pl);
};

GtkType    dia_page_layout_get_type (void);
GtkWidget *dia_page_layout_new      (void);

#endif
