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

  GtkEntry *classname;
  GtkEntry *stereotype;
  GtkTextView *comment;

  GtkToggleButton *abstract_class;
  GtkToggleButton *attr_vis;
  GtkToggleButton *attr_supp;
  GtkToggleButton *op_vis;
  GtkToggleButton *op_supp;
  GtkToggleButton *comments_vis;
  GtkToggleButton *op_wrap;
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
  GtkSpinButton *wrap_after_char;  
  GtkSpinButton *comment_line_length;
  GtkToggleButton *comment_tagging;
  GtkSpinButton *line_width;
  DiaColorSelector *text_color;
  DiaColorSelector *line_color;
  DiaColorSelector *fill_color;
  GtkLabel *max_length_label;
  GtkLabel *Comment_length_label;

  GList *disconnected_connections;
  GList *added_connections; 
  GList *deleted_connections; 

  GtkWidget *attr_visible;

  GtkWidget *editor;
};

void _umlclass_store_disconnects(UMLClassDialog *prop_dialog, ConnectionPoint *cp);

const gchar *_class_get_comment(GtkTextView *);
void _class_set_comment(GtkTextView *, gchar *);

void _operations_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog, int connection_index);

void _operations_create_page(GtkNotebook *notebook,  UMLClass *umlclass);
