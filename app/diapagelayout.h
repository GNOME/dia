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

  GtkWidget *paper_size;
  GtkWidget *orient_portrait, *orient_landscape;
  GtkWidget *tmargin, *bmargin, *lmargin, *rmargin;
  GtkWidget *scaling;
  GtkWidget *fittopage;

  GtkWidget *darea;
};

struct _DiaPageLayoutClass {
  GtkTableClass parent_class;
};

GtkType    dia_page_layout_get_type (void);
GtkWidget *dia_page_layout_new      (void);

#endif
