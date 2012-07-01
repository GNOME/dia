#include "widgets.h"
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

  GtkList *attributes_list;
  GtkListItem *current_attr;
  GtkEntry *attr_name;
  GtkEntry *attr_type;
  GtkEntry *attr_value;
  GtkTextView *attr_comment;
  GtkWidget *attr_visible;
  GtkToggleButton *attr_class_scope;
  
  GtkList *operations_list;
  GtkListItem *current_op;
  GtkEntry *op_name;
  GtkEntry *op_type;
  GtkEntry *op_stereotype;
  GtkTextView *op_comment;

  GtkWidget *op_visible;
  GtkToggleButton *op_class_scope;
  GtkWidget *op_inheritance_type;
  GtkToggleButton *op_query;  
  
  GtkList *parameters_list;
  GtkListItem *current_param;
  GtkEntry *param_name;
  GtkEntry *param_type;
  GtkEntry *param_value;
  GtkTextView *param_comment;
  GtkWidget *param_kind;
  GtkWidget *param_new_button;
  GtkWidget *param_delete_button;
  GtkWidget *param_up_button;
  GtkWidget *param_down_button;
  
  GtkList *templates_list;
  GtkListItem *current_templ;
  GtkToggleButton *templ_template;
  GtkEntry *templ_name;
  GtkEntry *templ_type;
};

void _umlclass_store_disconnects(UMLClassDialog *prop_dialog, ConnectionPoint *cp);

const gchar *_class_get_comment(GtkTextView *);
void _class_set_comment(GtkTextView *, gchar *);

void _attributes_get_current_values(UMLClassDialog *prop_dialog);
void _operations_get_current_values(UMLClassDialog *prop_dialog);
void _templates_get_current_values(UMLClassDialog *prop_dialog);

void _attributes_fill_in_dialog(UMLClass *umlclass);
void _operations_fill_in_dialog(UMLClass *umlclass);
void _templates_fill_in_dialog(UMLClass *umlclass);

void _attributes_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog, int connection_index);
void _operations_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog, int connection_index);
void _templates_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog);

void _attributes_create_page(GtkNotebook *notebook,  UMLClass *umlclass);
void _operations_create_page(GtkNotebook *notebook,  UMLClass *umlclass);
void _templates_create_page(GtkNotebook *notebook,  UMLClass *umlclass);
