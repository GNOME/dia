#ifndef DIAUNITSPINNER_H
#define DIAUNITSPINNER_H

#include <gtk/gtk.h>

#define DIA_UNIT_SPINNER(obj) GTK_CHECK_CAST(obj, dia_unit_spinner_get_type(), DiaUnitSpinner)
#define DIA_UNIT_SPINNER_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, dia_unit_spinner_get_type(), DiaUnitSpinnerClass)
#define DIA_IS_UNIT_SPINNER(obj) GTK_CHECK_TYPE(obj, dia_unit_spinner_get_type())

typedef struct _DiaUnitSpinner DiaUnitSpinner;
typedef struct _DiaUnitSpinnerClass DiaUnitSpinnerClass;

typedef enum {
  DIA_UNIT_FEET,
  DIA_UNIT_METER,
  DIA_UNIT_DECIMETER,
  DIA_UNIT_MILLIMETER,
  DIA_UNIT_POINT,
  DIA_UNIT_CENTIMETER,
  DIA_UNIT_INCH,
} DiaUnit;

struct _DiaUnitSpinner {
  GtkSpinButton parent;

  DiaUnit unit_num;
};

struct _DiaUnitSpinnerClass {
  GtkSpinButtonClass parent_class;

};

GtkType    dia_unit_spinner_get_type  (void);
GtkWidget *dia_unit_spinner_new       (GtkAdjustment *adjustment,
				       guint digits,
				       DiaUnit adj_unit);
void       dia_unit_spinner_set_value (DiaUnitSpinner *self, gfloat val);
gfloat     dia_unit_spinner_get_value (DiaUnitSpinner *self);

#endif
