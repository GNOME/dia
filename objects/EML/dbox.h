#ifndef DBOX_H
#define DBOX_H

#include <glib.h>
#include "element.h"
#include "eml.h"

#define text_ascent(font, font_height) font_ascent(font, font_height)

typedef struct _EMLBoxT EMLBoxT;
typedef struct _EMLBox EMLBox;

struct _EMLBoxT {
  EMLBox* (* const new) (real , gchar *, gint , real , real , real,
                         ConnectionPoint *, ConnectionPoint *);
  void    (* const destroy) (EMLBox *);
  void    (* const add_el) (EMLBox *, gpointer);
  void    (* const calc_geometry) (EMLBox *, real *, real *);
  real    (* const calc_connections) (EMLBox *, Point *, GList **, real);
  real    (* const draw) (EMLBox *, Renderer *, Point* , real);
};

struct _EMLBox {
  EMLBoxT *ops;
  ConnectionPoint *left_connection;
  ConnectionPoint *right_connection;
  real font_height;
  Font *font;
  gint text_alignment;
  real border_width;
  real separator_width;
  real separator_style;
  GList *els;
};

void emlbox_destroy(EMLBox *);
void emlbox_add_el(EMLBox *, gpointer );
void emlbox_calc_geometry(EMLBox *, real *, real *);
real emlbox_calc_connections(EMLBox *, Point *, GList **, real );
real emlbox_draw(EMLBox *, Renderer *, Element *);

extern EMLBoxT EMLListBox;
extern EMLBoxT EMLTextBox;
#endif /* DBOX_H */
