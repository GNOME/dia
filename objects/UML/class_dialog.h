#include "widgets.h"
#include "widgets/dialist.h"
#include "diafontselector.h"
#include "editor/dia-uml-class-editor.h"

/**
 * \brief Very special user interface for UMLClass parametrization
 *
 * There is a (too) tight coupling between the UMLClass and it's user interface. 
 * And the dialog is too huge in code as well as on screen.
 */
struct _UMLClassDialog {
  GtkWidget *dialog;

  DiaFontSelector *normal_font;
  DiaFontSelector *abstract_font;
  DiaFontSelector *polymorphic_font;
  DiaFontSelector *classname_font;
  DiaFontSelector *abstract_classname_font;
  DiaFontSelector *comment_font;
  GtkSpinButton *normal_font_height;
  GtkSpinButton *abstract_font_height;
  GtkSpinButton *polymorphic_font_height;
  GtkSpinButton *classname_font_height;
  GtkSpinButton *abstract_classname_font_height;
  GtkSpinButton *comment_font_height;
  GtkSpinButton *line_width;
  DiaColorSelector *text_color;
  DiaColorSelector *line_color;
  DiaColorSelector *fill_color;

  GtkWidget *editor;
};

void _umlclass_store_disconnects (ConnectionPoint  *cp,
                                  GList           **disconnected);
