#include <gtk/gtk.h>
#include "uml.h"
#include "class.h"

#ifndef UML_CLASS_H
#define UML_CLASS_H

G_BEGIN_DECLS

#define DIA_UML_TYPE_CLASS (dia_uml_class_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlClass, dia_uml_class, DIA_UML, CLASS, GObject)

struct _DiaUmlClass {
  GObject parent;

  double font_height;
  double abstract_font_height;
  double polymorphic_font_height;
  double classname_font_height;
  double abstract_classname_font_height;
  double comment_font_height;

  DiaFont *normal_font;
  DiaFont *abstract_font;
  DiaFont *polymorphic_font;
  DiaFont *classname_font;
  DiaFont *abstract_classname_font;
  DiaFont *comment_font;
  
  char *name;
  char *stereotype;
  char *comment;

  gboolean abstract;
  gboolean suppress_attributes;
  gboolean suppress_operations;
  gboolean visible_attributes;
  gboolean visible_operations;
  gboolean visible_comments;

  int wrap_operations;
  int wrap_after_char;
  int comment_line_length;
  int comment_tagging;
  
  double line_width;
  GdkRGBA line_color;
  GdkRGBA fill_color;
  GdkRGBA text_color;

  /* Maybe we could use GListStore for these? */

  /* Attributes: */
  GList *attributes;

  /* Operators: */
  GList *operations;

  /* Template: */
  gboolean template_;
  GList *formal_params;
};

DiaUmlClass *dia_uml_class_new              (UMLClass        *klass);
void         dia_uml_class_load             (DiaUmlClass     *self,
                                             UMLClass        *klass);
void         dia_uml_class_store            (DiaUmlClass     *self,
                                             UMLClass        *klass);
GList       *dia_uml_class_get_operations   (DiaUmlClass     *self);
void         dia_uml_class_remove_operation (DiaUmlClass     *self,
                                             DiaUmlOperation *operation);
void         dia_uml_class_insert_operation (DiaUmlClass     *self,
                                             DiaUmlOperation *operation,
                                             int              index);

G_END_DECLS

#endif