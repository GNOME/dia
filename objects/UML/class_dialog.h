#pragma once

#include "dia-colour-selector.h"
#include "dia-font-selector.h"

G_BEGIN_DECLS

#define DIA_UML_TYPE_CLASS_OBJECT_CHANGE dia_uml_class_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaUMLClassObjectChange,
                      dia_uml_class_object_change,
                      DIA_UML, CLASS_OBJECT_CHANGE,
                      DiaObjectChange)


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
  DiaColourSelector *text_color;
  DiaColourSelector *line_color;
  DiaColourSelector *fill_color;
  GtkLabel *max_length_label;
  GtkLabel *Comment_length_label;

  GList *disconnected_connections;
  GList *added_connections;
  GList *deleted_connections;

  GtkWidget *attributes;
  GtkListStore *attributes_store;
  GtkEntry *attr_name;
  GtkEntry *attr_type;
  GtkEntry *attr_value;
  GtkTextView *attr_comment;
  GtkTextBuffer *attr_comment_buffer;
  GtkWidget *attr_visible;
  GtkToggleButton *attr_class_scope;

  GtkWidget *operations;
  GtkListStore *operations_store;
  GtkEntry *op_name;
  GtkEntry *op_type;
  GtkEntry *op_stereotype;
  GtkTextView *op_comment;
  GtkTextBuffer *op_comment_buffer;
  GtkWidget *op_visible;
  GtkToggleButton *op_class_scope;
  GtkWidget *op_inheritance_type;
  GtkToggleButton *op_query;

  GtkWidget *parameters;
  GtkListStore *parameters_store;
  GtkEntry *param_name;
  GtkEntry *param_type;
  GtkEntry *param_value;
  GtkTextView *param_comment;
  GtkWidget *param_kind;
  GtkTextBuffer *param_comment_buffer;

  GtkWidget *param_new_button;
  GtkWidget *param_delete_button;
  GtkWidget *param_up_button;
  GtkWidget *param_down_button;

  GtkWidget *templates;
  GtkListStore *templates_store;
  GtkToggleButton *templ_template;
  GtkEntry *templ_name;
  GtkEntry *templ_type;
};

void _umlclass_store_disconnects(UMLClassDialog *prop_dialog, ConnectionPoint *cp);

char *buffer_get_text (GtkTextBuffer *buffer);

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

G_END_DECLS
