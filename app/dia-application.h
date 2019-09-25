#pragma once

#include "diagram.h"

G_BEGIN_DECLS

enum {
  DIAGRAM_CHANGE_NAME   = (1<<0),
  DIAGRAM_CHANGE_LAYER  = (1<<1),
  DIAGRAM_CHANGE_OBJECT = (1<<2)
};

#define DIA_TYPE_APPLICATION (dia_application_get_type ())
G_DECLARE_FINAL_TYPE (DiaApplication, dia_application, DIA, APPLICATION, GObject)

DiaApplication *dia_application_get_default             (void);
void            dia_application_diagram_add             (DiaApplication *self,
                                                         Diagram        *dia);
void            dia_application_diagram_remove          (DiaApplication *self,
                                                         Diagram        *dia);
int             dia_application_diagram_index           (DiaApplication *self,
                                                         Diagram        *dia);
void            dia_application_diagram_change          (DiaApplication *self,
                                                         Diagram        *dia,
                                                         guint           flags,
                                                         gpointer        object);
GListModel     *dia_application_get_diagrams            (DiaApplication *self);

G_END_DECLS
