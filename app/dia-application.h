#include "diagram.h"

typedef struct _DiaApplication DiaApplication;

enum {
  DIAGRAM_CHANGE_NAME   = (1<<0),
  DIAGRAM_CHANGE_LAYER  = (1<<1),
  DIAGRAM_CHANGE_OBJECT = (1<<2)
};

DiaApplication * dia_application_get (void);

void dia_diagram_add    (Diagram *dia);
void dia_diagram_change (Diagram *dia, guint flags, gpointer object);
void dia_diagram_remove (Diagram *dia);
