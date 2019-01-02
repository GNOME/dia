#include "dia-uml-class.h"

G_DEFINE_TYPE (DiaUmlClass, dia_uml_class, G_TYPE_OBJECT)

static void
clear_attrs (DiaUmlClass *self)
{
  GList *list;

  g_clear_object (&self->normal_font);
  g_clear_object (&self->abstract_font);
  g_clear_object (&self->polymorphic_font);
  g_clear_object (&self->classname_font);
  g_clear_object (&self->abstract_classname_font);
  g_clear_object (&self->comment_font);
  
  g_free (self->name);
  g_free (self->stereotype);
  g_free (self->comment);

  list = self->attributes;
  while (list) {
    uml_attribute_destroy ((UMLAttribute *) list->data);
    list = g_list_next (list);
  }
  g_list_free (self->attributes);

  list = self->operations;
  while (list) {
    g_object_unref (list->data);
    list = g_list_next (list);
  }
  g_list_free (self->operations);

  list = self->formal_params;
  while (list) {
    uml_formalparameter_destroy ((UMLFormalParameter *) list->data);
    list = g_list_next (list);
  }
  g_list_free (self->formal_params);
}

static void
dia_uml_class_finalize (GObject *object)
{
  DiaUmlClass *self = DIA_UML_CLASS (object);

  clear_attrs (self);
}

static void
dia_uml_class_class_init (DiaUmlClassClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_uml_class_finalize;
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
  
  self->attributes = NULL;
  list = klass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *)list->data;
    UMLAttribute *attr_copy;
      
    attr_copy = uml_attribute_copy(attr);
    /* Looks wrong, but needed fro proper restore */
    attr_copy->left_connection = attr->left_connection;
    attr_copy->right_connection = attr->right_connection;

    self->attributes = g_list_append(self->attributes, attr_copy);
    list = g_list_next(list);
  }

  /* TODO: Why? */
  self->operations = NULL;
  list = klass->operations;
  while (list != NULL) {
    DiaUmlOperation *op = (DiaUmlOperation *)list->data;
    DiaUmlOperation *copy = dia_uml_operation_copy (op);

    /* Looks wrong but is required for the complicate connections memory management */
    copy->l_connection = op->l_connection;
    copy->r_connection = op->r_connection;

    self->operations = g_list_append(self->operations, copy);
    list = g_list_next(list);
  }

  self->template_ = klass->template;
  
  self->formal_params = NULL;
  list = klass->formal_params;
  while (list != NULL) {
    UMLFormalParameter *param = (UMLFormalParameter *) list->data;
    UMLFormalParameter *param_copy;
    
    param_copy = uml_formalparameter_copy (param);
    self->formal_params = g_list_append (self->formal_params, param_copy);
    
    list = g_list_next(list);
  }
}

/* Sync from self to klass */
void
dia_uml_class_store (DiaUmlClass *self,
                     UMLClass    *klass)
{
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

  klass->line_color = self->line_color;
  klass->fill_color = self->fill_color;
  klass->text_color = self->text_color;

  /* TODO: List stuff */
  klass->attributes = self->attributes;
  klass->operations = self->operations;
  klass->template = self->template_;
  klass->formal_params = self->formal_params;
}

GList *
dia_uml_class_get_operations (DiaUmlClass *self)
{
  return self->operations;
}

void
dia_uml_class_insert_operation (DiaUmlClass     *self,
                                DiaUmlOperation *operation,
                                int              index)
{
  self->operations = g_list_insert (self->operations,
                                    g_object_ref (operation),
                                    index);
}

void
dia_uml_class_remove_operation (DiaUmlClass     *self,
                                DiaUmlOperation *operation)
{
  self->operations = g_list_remove (self->operations, operation);
  g_object_unref (operation);
}
