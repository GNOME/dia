#ifndef DIAPAGELAYOUT_H
#define DIAPAGELAYOUT_H

#include <gtk/gtk.h>

#define DIA_PAGE_LAYOUT(obj) GTK_CHECK_CAST(obj, dia_page_layout_get_type(), DiaPageLayout)
#define DIA_PAGE_LAYOUT_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, dia_page_layout_get_type(), DiaPageLayoutClass)
#define DIA_IS_PAGE_LAYOUT(obj) GTK_CHECK_TYPE(obj, dia_page_layout_get_type())

typedef struct _DiaPageLayout DiaPageLayout;
typedef struct _DiaPageLayoutClass DiaPageLayoutClass;

typedef enum {
  DIA_PAGE_ORIENT_PORTRAIT,
  DIA_PAGE_ORIENT_LANDSCAPE
} DiaPageOrientation;

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

GtkType      dia_page_layout_get_type    (void);
GtkWidget   *dia_page_layout_new         (void);

const gchar *dia_page_layout_get_paper   (DiaPageLayout *pl);
void         dia_page_layout_set_paper   (DiaPageLayout *pl,
					  const gchar *paper);
void         dia_page_layout_get_margins (DiaPageLayout *pl,
					  gfloat *tmargin, gfloat *bmargin,
					  gfloat *lmargin, gfloat *rmargin);
void         dia_page_layout_set_margins (DiaPageLayout *pl,
					  gfloat tmargin, gfloat bmargin,
					  gfloat lmargin, gfloat rmargin);
DiaPageOrientation dia_page_layout_get_orientation (DiaPageLayout *pl);
void         dia_page_layout_set_orientation (DiaPageLayout *pl,
					      DiaPageOrientation orient);

/* get paper sizes and default margins ... */
void dia_page_layout_get_paper_size      (const gchar *paper,
					  gfloat *width, gfloat *height);
void dia_page_layout_get_default_margins (const gchar *paper,
					  gfloat *tmargin, gfloat *bmargin,
					  gfloat *lmargin, gfloat *rmargin);

#endif
