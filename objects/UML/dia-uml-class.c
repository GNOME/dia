#include "dia-uml-class.h"
#include "dia-uml-operation.h"
#include "dia-uml-attribute.h"
#include "dia-uml-formal-parameter.h"
#include "editor/dia-uml-list-store.h"
#include "class_dialog.h"

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

  /* Attributes: */
  DiaUmlListStore *attributes;

  /* Operators: */
  DiaUmlListStore *operations;

  /* Template: */
  gboolean is_template;
  DiaUmlListStore *formal_params;
};

G_DEFINE_TYPE (DiaUmlClass, dia_uml_class, G_TYPE_OBJECT)

enum {
  PROP_NAME = 1,
  PROP_STEREOTYPE,
  PROP_COMMENT,
  PROP_ABSTRACT,
  PROP_COMMENTS,
  PROP_COMMENT_WRAP_POINT,
  PROP_DOCUMENTATION_TAG,
  PROP_ATTRIBUTES_VISIBLE,
  PROP_ATTRIBUTES_SUPPRESS,
  PROP_OPERATIONS_VISIBLE,
  PROP_OPERATIONS_SUPPRESS,
  PROP_OPERATIONS_WRAP,
  PROP_OPERATIONS_WRAP_POINT,
  PROP_IS_TEMPLATE,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];

static void
clear_attrs (DiaUmlClass *self)
{
  g_clear_object (&self->normal_font);
  g_clear_object (&self->abstract_font);
  g_clear_object (&self->polymorphic_font);
  g_clear_object (&self->classname_font);
  g_clear_object (&self->abstract_classname_font);
  g_clear_object (&self->comment_font);
  
  g_free (self->name);
  g_free (self->stereotype);
  g_free (self->comment);

  g_clear_object (&self->attributes);
  g_clear_object (&self->operations);
  g_clear_object (&self->formal_params);
}

static void
dia_uml_class_finalize (GObject *object)
{
  DiaUmlClass *self = DIA_UML_CLASS (object);

  clear_attrs (self);
}

static void
dia_uml_class_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  DiaUmlClass *self = DIA_UML_CLASS (object);

  switch (property_id) {
    case PROP_NAME:
      self->name = g_value_dup_string (value);
      break;
    case PROP_STEREOTYPE:
      self->stereotype = g_value_dup_string (value);
      break;
    case PROP_COMMENT:
      self->comment = g_value_dup_string (value);
      break;
    case PROP_ABSTRACT:
      self->abstract = g_value_get_boolean (value);
      break;
    case PROP_COMMENTS:
      self->visible_comments = g_value_get_boolean (value);
      break;
    case PROP_COMMENT_WRAP_POINT:
      self->comment_line_length = g_value_get_int (value);
      break;
    case PROP_DOCUMENTATION_TAG:
      self->comment_tagging = g_value_get_boolean (value);
      break;
    case PROP_ATTRIBUTES_VISIBLE:
      self->visible_attributes = g_value_get_boolean (value);
      break;
    case PROP_ATTRIBUTES_SUPPRESS:
      self->suppress_attributes = g_value_get_boolean (value);
      break;
    case PROP_OPERATIONS_VISIBLE:
      self->visible_operations = g_value_get_boolean (value);
      break;
    case PROP_OPERATIONS_SUPPRESS:
      self->suppress_operations = g_value_get_boolean (value);
      break;
    case PROP_OPERATIONS_WRAP:
      self->wrap_operations = g_value_get_boolean (value);
      break;
    case PROP_OPERATIONS_WRAP_POINT:
      self->wrap_after_char = g_value_get_int (value);
      break;
    case PROP_IS_TEMPLATE:
      self->is_template = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_class_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  DiaUmlClass *self = DIA_UML_CLASS (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_STEREOTYPE:
      g_value_set_string (value, self->stereotype ? self->stereotype : "");
      break;
    case PROP_COMMENT:
      g_value_set_string (value, self->comment ? self->comment : "");
      break;
    case PROP_ABSTRACT:
      g_value_set_boolean (value, self->abstract);
      break;
    case PROP_COMMENTS:
      g_value_set_boolean (value, self->visible_comments);
      break;
    case PROP_COMMENT_WRAP_POINT:
      g_value_set_int (value, self->comment_line_length);
      break;
    case PROP_DOCUMENTATION_TAG:
      g_value_set_boolean (value, self->comment_tagging);
      break;
    case PROP_ATTRIBUTES_VISIBLE:
      g_value_set_boolean (value, self->visible_attributes);
      break;
    case PROP_ATTRIBUTES_SUPPRESS:
      g_value_set_boolean (value, self->suppress_attributes);
      break;
    case PROP_OPERATIONS_VISIBLE:
      g_value_set_boolean (value, self->visible_operations);
      break;
    case PROP_OPERATIONS_SUPPRESS:
      g_value_set_boolean (value, self->suppress_operations);
      break;
    case PROP_OPERATIONS_WRAP:
      g_value_set_boolean (value, self->wrap_operations);
      break;
    case PROP_OPERATIONS_WRAP_POINT:
      g_value_set_int (value, self->wrap_after_char);
      break;
    case PROP_IS_TEMPLATE:
      g_value_set_boolean (value, self->is_template);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_class_class_init (DiaUmlClassClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_uml_class_finalize;
  object_class->set_property = dia_uml_class_set_property;
  object_class->get_property = dia_uml_class_get_property;

  properties[PROP_NAME] = g_param_spec_string ("name",
                                               "Name",
                                               "Class name",
                                               "",
                                               G_PARAM_READWRITE);
  properties[PROP_STEREOTYPE] = g_param_spec_string ("stereotype",
                                                     "Stereotype",
                                                     "Parent type",
                                                     "",
                                                     G_PARAM_READWRITE);
  properties[PROP_COMMENT] = g_param_spec_string ("comment",
                                                  "Comment",
                                                  "Class comment",
                                                  "",
                                                  G_PARAM_READWRITE);
  properties[PROP_ABSTRACT] = g_param_spec_boolean ("abstract",
                                                    "Abstract",
                                                    "Is an abstract class",
                                                    FALSE,
                                                    G_PARAM_READWRITE);
  properties[PROP_COMMENTS] = g_param_spec_boolean ("comments",
                                                    "Comments",
                                                    "Show comments",
                                                    FALSE,
                                                    G_PARAM_READWRITE);
  properties[PROP_COMMENT_WRAP_POINT] = g_param_spec_int ("comment-wrap-point",
                                                          "Comments",
                                                          "Show comments",
                                                           17,
                                                           200,
                                                           17,
                                                           G_PARAM_READWRITE);
  properties[PROP_DOCUMENTATION_TAG] = g_param_spec_boolean ("documentation-tag",
                                                             "Documentation tag",
                                                             "Show documentation tag",
                                                             FALSE,
                                                             G_PARAM_READWRITE);
  properties[PROP_ATTRIBUTES_VISIBLE] = g_param_spec_boolean ("attributes-visible",
                                                              "Attributes visible",
                                                              "Are attributes visible",
                                                              TRUE,
                                                              G_PARAM_READWRITE);
  properties[PROP_ATTRIBUTES_SUPPRESS] = g_param_spec_boolean ("attributes-suppress",
                                                               "Attributes suppress",
                                                               "Are attributes supressed",
                                                               FALSE,
                                                               G_PARAM_READWRITE);
  properties[PROP_OPERATIONS_VISIBLE] = g_param_spec_boolean ("operations-visible",
                                                              "Operations visible",
                                                              "Are operations visible",
                                                              TRUE,
                                                              G_PARAM_READWRITE);
  properties[PROP_OPERATIONS_SUPPRESS] = g_param_spec_boolean ("operations-suppress",
                                                               "Operations suppress",
                                                               "Are operations supressed",
                                                               FALSE,
                                                               G_PARAM_READWRITE);
  properties[PROP_OPERATIONS_WRAP] = g_param_spec_boolean ("operations-wrap",
                                                           "Operations wrap",
                                                           "Should operations be wrapped",
                                                           TRUE,
                                                           G_PARAM_READWRITE);
  properties[PROP_OPERATIONS_WRAP_POINT] = g_param_spec_int ("operations-wrap-point",
                                                             "Operations wrap point",
                                                             "Where operations should be wrapped",
                                                             0,
                                                             200,
                                                             40,
                                                             G_PARAM_READWRITE);
  properties[PROP_IS_TEMPLATE] = g_param_spec_boolean ("is-template",
                                                       "Is template",
                                                       "Is a template class",
                                                       FALSE,
                                                       G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);
}

static void
dia_uml_class_init (DiaUmlClass *self)
{
}

DiaUmlClass *
dia_uml_class_new (UMLClass *klass)
{
  DiaUmlClass *self = g_object_new (DIA_UML_TYPE_CLASS, NULL);

  dia_uml_class_load (self, klass);

  return self;
}

/* Sync from klass to self */
void
dia_uml_class_load (DiaUmlClass *self,
                    UMLClass    *klass)
{
  GList *list;

  clear_attrs (self);

  self->font_height = klass->font_height;
  self->abstract_font_height = klass->abstract_font_height;
  self->polymorphic_font_height = klass->polymorphic_font_height;
  self->classname_font_height = klass->classname_font_height;
  self->abstract_classname_font_height = klass->abstract_classname_font_height;
  self->comment_font_height = klass->comment_font_height;

  self->normal_font = g_object_ref (klass->normal_font);
  self->abstract_font = g_object_ref (klass->abstract_font);
  self->polymorphic_font = g_object_ref (klass->polymorphic_font);
  self->classname_font = g_object_ref (klass->classname_font);
  self->abstract_classname_font = g_object_ref (klass->abstract_classname_font);
  self->comment_font = g_object_ref (klass->comment_font);
  
  g_free (self->name);
  self->name = g_strdup(klass->name);
  g_free (self->stereotype);
  self->stereotype = g_strdup(klass->stereotype);
  g_free (self->comment);
  self->comment = g_strdup(klass->comment);

  self->abstract = klass->abstract;
  self->suppress_attributes = klass->suppress_attributes;
  self->suppress_operations = klass->suppress_operations;
  self->visible_attributes = klass->visible_attributes;
  self->visible_operations = klass->visible_operations;
  self->visible_comments = klass->visible_comments;

  self->wrap_operations = klass->wrap_operations;
  self->wrap_after_char = klass->wrap_after_char;
  self->comment_line_length = klass->comment_line_length;
  self->comment_tagging = klass->comment_tagging;

  self->line_color = klass->line_color;
  self->fill_color = klass->fill_color;
  self->text_color = klass->text_color;
  
  list = klass->attributes;
  self->attributes = dia_uml_list_store_new ();
  while (list != NULL) {
    DiaUmlAttribute *attr = (DiaUmlAttribute *)list->data;
    DiaUmlAttribute *attr_copy;
      
    attr_copy = dia_uml_attribute_copy (attr);
    /* Looks wrong, but needed fro proper restore */
    attr_copy->left_connection = attr->left_connection;
    attr_copy->right_connection = attr->right_connection;

    dia_uml_list_store_add (self->attributes, DIA_UML_LIST_DATA (attr_copy));
    list = g_list_next(list);
  }

  list = klass->operations;
  self->operations = dia_uml_list_store_new ();
  while (list != NULL) {
    DiaUmlOperation *op = (DiaUmlOperation *)list->data;
    DiaUmlOperation *copy = dia_uml_operation_copy (op);

    dia_uml_operation_connection_thing (copy, op);

    dia_uml_list_store_add (self->operations, DIA_UML_LIST_DATA (copy));
    list = g_list_next(list);
  }

  self->is_template = klass->template;

  list = klass->formal_params;
  self->formal_params = dia_uml_list_store_new ();
  while (list != NULL) {
    DiaUmlFormalParameter *param = (DiaUmlFormalParameter *)list->data;
    DiaUmlFormalParameter *copy = dia_uml_formal_parameter_copy (param);

    dia_uml_list_store_add (self->formal_params, DIA_UML_LIST_DATA (copy));
    list = g_list_next(list);
  }
}

/* Sync from self to klass */
void
dia_uml_class_store (DiaUmlClass *self,
                     UMLClass    *klass,
                     GList      **added,
                     GList      **removed,
                     GList      **disconnected)
{
  GListModel *list_store;
  DiaUmlListData *itm;
  DiaObject *obj;
  gboolean attr_visible = TRUE;
  gboolean op_visible = TRUE;
  int i = 0;
  int connection_index = UMLCLASS_CONNECTIONPOINTS;

  /* Cast to DiaObject (UMLClass -> Element -> DiaObject) */
  obj = &klass->element.object;

  klass->font_height = self->font_height;
  klass->abstract_font_height = self->abstract_font_height;
  klass->polymorphic_font_height = self->polymorphic_font_height;
  klass->classname_font_height = self->classname_font_height;
  klass->abstract_classname_font_height = self->abstract_classname_font_height;
  klass->comment_font_height = self->comment_font_height;

  /* transfer ownership, but don't leak the previous font */
  g_clear_object (&klass->normal_font);
  klass->normal_font = g_object_ref (self->normal_font);
  g_clear_object (&klass->abstract_font);
  klass->abstract_font = g_object_ref (self->abstract_font);
  g_clear_object (&klass->polymorphic_font);
  klass->polymorphic_font = g_object_ref (self->polymorphic_font);
  g_clear_object (&klass->classname_font);
  klass->classname_font = g_object_ref (self->classname_font);
  g_clear_object (&klass->abstract_classname_font);
  klass->abstract_classname_font = g_object_ref (self->abstract_classname_font);
  g_clear_object (&klass->comment_font);
  klass->comment_font = g_object_ref (self->comment_font);
  
  klass->name = self->name;
  klass->stereotype = self->stereotype;
  klass->comment = self->comment;

  klass->abstract = self->abstract;
  klass->suppress_attributes = self->suppress_attributes;
  klass->suppress_operations = self->suppress_operations;
  klass->visible_attributes = self->visible_attributes;
  klass->visible_operations = self->visible_operations;
  klass->visible_comments = self->visible_comments;

  klass->wrap_operations = self->wrap_operations;
  klass->wrap_after_char = self->wrap_after_char;
  klass->comment_line_length = self->comment_line_length;
  klass->comment_tagging = self->comment_tagging;

  klass->template = self->is_template;

  klass->line_color = self->line_color;
  klass->fill_color = self->fill_color;
  klass->text_color = self->text_color;

  /* Free current attributes: */
  g_list_free_full (klass->attributes, g_object_unref);
  klass->attributes = NULL;

  /* Free current operations: */
  /* Clear those already stored */
  g_list_free_full (klass->operations, g_object_unref);
  klass->operations = NULL;

  /* Free current formal parameters: */
  g_list_free_full (klass->formal_params, g_object_unref);
  klass->formal_params = NULL;

  /* If attributes visible and not suppressed */
  attr_visible = (self->visible_attributes) && (!self->suppress_attributes);

  /* Insert new attributes and remove them from gtklist: */
  i = 0;
  list_store = dia_uml_class_get_attributes (self);
  /* Insert new operations and remove them from gtklist: */
  while ((itm = g_list_model_get_item (list_store, i))) {
    DiaUmlAttribute *attr = DIA_UML_ATTRIBUTE (itm);

    klass->attributes = g_list_append (klass->attributes, g_object_ref (attr));
    
    if (attr->left_connection == NULL) {
      dia_uml_attribute_ensure_connection_points (attr, obj);

      *added = g_list_prepend (*added, attr->left_connection);
      *added = g_list_prepend (*added, attr->right_connection);
    }

    if (attr_visible) { 
      obj->connections[connection_index] = attr->left_connection;
      connection_index++;
      obj->connections[connection_index] = attr->right_connection;
      connection_index++;
    } else {
      _umlclass_store_disconnects (attr->left_connection, disconnected);
      object_remove_connections_to(attr->left_connection);
      _umlclass_store_disconnects (attr->right_connection, disconnected);
      object_remove_connections_to(attr->right_connection);
    }

    i++;
  }

  /* If operations visible and not suppressed */
  op_visible = (self->visible_operations) && (!self->suppress_operations);

  i = 0;
  list_store = dia_uml_class_get_operations (self);
  /* Insert new operations and remove them from gtklist: */
  while ((itm = g_list_model_get_item (list_store, i))) {
    DiaUmlOperation *op = DIA_UML_OPERATION (itm);

    klass->operations = g_list_append(klass->operations, g_object_ref (op));

    if (op->l_connection == NULL) {
      dia_uml_operation_ensure_connection_points (op, obj);
      
      *added = g_list_prepend (*added, op->l_connection);
      *added = g_list_prepend (*added, op->r_connection);
    }

    if (op_visible) { 
      obj->connections[connection_index] = op->l_connection;
      connection_index++;
      obj->connections[connection_index] = op->r_connection;
      connection_index++;
    } else {
      _umlclass_store_disconnects (op->l_connection, disconnected);
      object_remove_connections_to(op->l_connection);
      _umlclass_store_disconnects (op->r_connection, disconnected);
      object_remove_connections_to(op->r_connection);
    }

    i++;
  }

  /* Insert new formal params and remove them from gtklist: */
  i = 0;
  list_store = dia_uml_class_get_formal_parameters (self);
  /* Insert new operations and remove them from gtklist: */
  while ((itm = g_list_model_get_item (list_store, i))) {
    DiaUmlFormalParameter *param = DIA_UML_FORMAL_PARAMETER (itm);

    klass->formal_params = g_list_append(klass->formal_params, g_object_ref (param));

    i++;
  }
}

gboolean
dia_uml_class_is_template (DiaUmlClass *klass)
{
  return klass->is_template;
}

GListModel *
dia_uml_class_get_attributes (DiaUmlClass *self)
{
  return G_LIST_MODEL (self->attributes);
}

GListModel *
dia_uml_class_get_operations (DiaUmlClass *self)
{
  return G_LIST_MODEL (self->operations);
}

GListModel *
dia_uml_class_get_formal_parameters (DiaUmlClass *self)
{
  return G_LIST_MODEL (self->formal_params);
}

/*
 * Don't rely on these six being called!
 * 
 * The DiaUmlListStore can/will be edited directly (e.g. by DiaUmlClassEditor)
 * so connect to items-changed if you want to observe these!
 */

void
dia_uml_class_insert_operation (DiaUmlClass     *self,
                                DiaUmlOperation *operation,
                                int              index)
{
  dia_uml_list_store_insert (self->operations, DIA_UML_LIST_DATA (operation), index);
}

void
dia_uml_class_remove_operation (DiaUmlClass     *self,
                                DiaUmlOperation *operation)
{
  dia_uml_list_store_remove (self->operations, DIA_UML_LIST_DATA (operation));
}

void
dia_uml_class_insert_attribute (DiaUmlClass     *self,
                                DiaUmlAttribute *attribute,
                                int              index)
{
  dia_uml_list_store_insert (self->attributes, DIA_UML_LIST_DATA (attribute), index);
}

void
dia_uml_class_remove_attribute (DiaUmlClass     *self,
                                DiaUmlAttribute *attribute)
{
  dia_uml_list_store_remove (self->attributes, DIA_UML_LIST_DATA (attribute));
}

void
dia_uml_class_insert_formal_parameter (DiaUmlClass           *self,
                                       DiaUmlFormalParameter *param,
                                       int                    index)
{
  dia_uml_list_store_insert (self->formal_params, DIA_UML_LIST_DATA (param), index);
}

void
dia_uml_class_remove_formal_parameter (DiaUmlClass           *self,
                                       DiaUmlFormalParameter *param)
{
  dia_uml_list_store_remove (self->formal_params, DIA_UML_LIST_DATA (param));
}
