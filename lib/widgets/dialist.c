/* Minimal reimplementation of GtkList, prototyped in Vala */


#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dialist.h"

typedef struct _DiaListPrivate DiaListPrivate;

struct _DiaListPrivate {
	GtkListBox* real;
};

G_DEFINE_TYPE_WITH_CODE (DiaList, dia_list, GTK_TYPE_FRAME,
						 G_ADD_PRIVATE (DiaList))

enum  {
	DIA_LIST_0_PROPERTY,
	DIA_LIST_CHILDREN_PROPERTY,
	DIA_LIST_SELECTION_PROPERTY,
	DIA_LIST_SELECTION_MODE_PROPERTY,
	DIA_LIST_NUM_PROPERTIES
};

static GParamSpec* dia_list_properties[DIA_LIST_NUM_PROPERTIES];

typedef struct _DiaListItemPrivate DiaListItemPrivate;

struct _DiaListItemPrivate {
	GtkLabel* label;
};

G_DEFINE_TYPE_WITH_CODE (DiaListItem, dia_list_item, GTK_TYPE_LIST_BOX_ROW,
						 G_ADD_PRIVATE (DiaListItem))

enum  {
	DIA_LIST_CHILD_SELECTED_SIGNAL,
	DIA_LIST_SELECTION_CHANGED_SIGNAL,
	DIA_LIST_NUM_SIGNALS
};
static guint dia_list_signals[DIA_LIST_NUM_SIGNALS] = {0};

enum  {
	DIA_LIST_ITEM_0_PROPERTY,
	DIA_LIST_ITEM_VALUE_PROPERTY,
	DIA_LIST_ITEM_NUM_PROPERTIES
};
static GParamSpec* dia_list_item_properties[DIA_LIST_ITEM_NUM_PROPERTIES];


typedef struct _Block1Data Block1Data;

struct _Block1Data {
	int _ref_count_;
	DiaList* self;
	gint i;
};

static void __lambda4_ (DiaList* self,
                 DiaListItem* item);
static void ___lambda4__gfunc (gpointer data,
                        gpointer self);
static Block1Data* block1_data_ref (Block1Data* _data1_);
static void block1_data_unref (void * _userdata_);
static void __lambda5_ (Block1Data* _data1_,
                 DiaListItem* item);
static void ___lambda5__gfunc (gpointer data,
                        gpointer self);
static void __lambda6_ (DiaList* self,
                 DiaListItem* item);
static void ___lambda6__gfunc (gpointer data,
                        gpointer self);
static void __lambda7_ (DiaList* self,
                 GtkWidget* elm);
static void ___lambda7__gtk_callback (GtkWidget* widget,
                               gpointer self);
static void _dia_list___lambda8_ (DiaList* self);
static void __dia_list___lambda8__gtk_list_box_row_selected (GtkListBox* _sender,
                                                      GtkListBoxRow* row,
                                                      gpointer self);
static void _vala_dia_list_get_property (GObject * object,
                                  guint property_id,
                                  GValue * value,
                                  GParamSpec * pspec);
static void _vala_dia_list_set_property (GObject * object,
                                  guint property_id,
                                  const GValue * value,
                                  GParamSpec * pspec);
static void _vala_dia_list_item_get_property (GObject * object,
                                       guint property_id,
                                       GValue * value,
                                       GParamSpec * pspec);
static void _vala_dia_list_item_set_property (GObject * object,
                                       guint property_id,
                                       const GValue * value,
                                       GParamSpec * pspec);
static void
dia_list_finalize (GObject * obj);
static void
dia_list_item_finalize (GObject * obj);

void
dia_list_select_child (DiaList* self,
                       DiaListItem* widget)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (widget != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_list_box_select_row (_tmp0_, (GtkListBoxRow*) widget);
	g_signal_emit (self, dia_list_signals[DIA_LIST_CHILD_SELECTED_SIGNAL], 0, widget);
}


void
dia_list_unselect_child (DiaList* self,
                         DiaListItem* widget)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (widget != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_list_box_unselect_row (_tmp0_, (GtkListBoxRow*) widget);
}


static void
__lambda4_ (DiaList* self,
            DiaListItem* item)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (item != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_container_add ((GtkContainer*) _tmp0_, (GtkWidget*) item);
}


static void
___lambda4__gfunc (gpointer data,
                   gpointer self)
{
	__lambda4_ ((DiaList*) self, (DiaListItem*) data);
}


void
dia_list_append_items (DiaList* self,
                       GList* items)
{
	g_return_if_fail (self != NULL);
	g_list_foreach (items, ___lambda4__gfunc, self);
}


static Block1Data*
block1_data_ref (Block1Data* _data1_)
{
	g_atomic_int_inc (&_data1_->_ref_count_);
	return _data1_;
}


static void
block1_data_unref (void * _userdata_)
{
	Block1Data* _data1_;
	_data1_ = (Block1Data*) _userdata_;
	if (g_atomic_int_dec_and_test (&_data1_->_ref_count_)) {
		DiaList* self;
		self = _data1_->self;
		g_object_unref (self);
		g_slice_free (Block1Data, _data1_);
	}
}


static void
__lambda5_ (Block1Data* _data1_,
            DiaListItem* item)
{
	DiaList* self;
	GtkListBox* _tmp0_;
	gint _tmp1_;
	self = _data1_->self;
	g_return_if_fail (item != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	_tmp1_ = _data1_->i;
	_data1_->i = _tmp1_ + 1;
	gtk_list_box_insert (_tmp0_, (GtkWidget*) item, _tmp1_);
}


static void
___lambda5__gfunc (gpointer data,
                   gpointer self)
{
	__lambda5_ (self, (DiaListItem*) data);
}


void
dia_list_insert_items (DiaList* self,
                       GList* items,
                       gint i)
{
	Block1Data* _data1_;
	g_return_if_fail (self != NULL);
	_data1_ = g_slice_new0 (Block1Data);
	_data1_->_ref_count_ = 1;
	_data1_->self = g_object_ref (self);
	_data1_->i = i;
	g_list_foreach (items, ___lambda5__gfunc, _data1_);
	block1_data_unref (_data1_);
	_data1_ = NULL;
}


static void
__lambda6_ (DiaList* self,
            DiaListItem* item)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (item != NULL);
	_tmp0_ = ((DiaListPrivate *) ((DiaListPrivate *) dia_list_get_instance_private (self)))->real;
	gtk_container_remove ((GtkContainer*) _tmp0_, (GtkWidget*) item);
}


static void
___lambda6__gfunc (gpointer data,
                   gpointer self)
{
	__lambda6_ ((DiaList*) self, (DiaListItem*) data);
}


void
dia_list_remove_items (DiaList* self,
                       GList* items)
{
	g_return_if_fail (self != NULL);
	g_list_foreach (items, ___lambda6__gfunc, self);
}


DiaListItem*
dia_list_append (DiaList* self,
                 const gchar* item)
{
	GtkWidget* row = NULL;
	GtkWidget* _tmp0_;
	GtkListBox* _tmp1_;
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (item != NULL, NULL);
	_tmp0_ = dia_list_item_new_with_label (item);
	g_object_ref_sink (_tmp0_);
	row = _tmp0_;
	gtk_widget_show_all ((GtkWidget*) row);
	_tmp1_ = ((DiaListPrivate *) ((DiaListPrivate *) dia_list_get_instance_private (self)))->real;
	gtk_container_add ((GtkContainer*) _tmp1_, (GtkWidget*) row);
	return (DiaListItem *) row;
}


void
dia_list_select_item (DiaList* self,
                      gint i)
{
	GtkListBox* _tmp0_;
	GtkListBoxRow* _tmp1_;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((DiaListPrivate *) ((DiaListPrivate *) dia_list_get_instance_private (self)))->real;
	_tmp1_ = gtk_list_box_get_row_at_index (_tmp0_, i);
	dia_list_select_child (self, G_TYPE_CHECK_INSTANCE_TYPE (_tmp1_, DIA_TYPE_LIST_ITEM) ? ((DiaListItem*) _tmp1_) : NULL);
}


static void
__lambda7_ (DiaList* self,
            GtkWidget* elm)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (elm != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_container_remove ((GtkContainer*) _tmp0_, elm);
}


static void
___lambda7__gtk_callback (GtkWidget* widget,
                          gpointer self)
{
	__lambda7_ ((DiaList*) self, widget);
}


void
dia_list_empty (DiaList* self)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_container_foreach ((GtkContainer*) _tmp0_, ___lambda7__gtk_callback, self);
}


gint
dia_list_child_position (DiaList* self,
                         DiaListItem* row)
{
	gint result = 0;
	g_return_val_if_fail (self != NULL, 0);
	g_return_val_if_fail (row != NULL, 0);
	result = gtk_list_box_row_get_index ((GtkListBoxRow*) row);
	return result;
}


void
dia_list_unselect_all (DiaList* self)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_list_box_unselect_all (_tmp0_);
}


void
dia_list_select_all (DiaList* self)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_list_box_select_all (_tmp0_);
}


DiaList*
dia_list_construct (GType object_type)
{
	DiaList * self = NULL;
	self = (DiaList*) g_object_new (object_type, NULL);
	return self;
}


GtkWidget *
dia_list_new (void)
{
	return (GtkWidget *) dia_list_construct (DIA_TYPE_LIST);
}


GList*
dia_list_get_children (DiaList* self)
{
	GList* result;
	GList* _tmp0_;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = gtk_container_get_children ((GtkContainer*) self);
	result = _tmp0_;
	return result;
}


DiaListItem*
dia_list_get_selection (DiaList* self)
{
	DiaListItem* result;
	GtkListBox* _tmp0_;
	GtkListBoxRow* _tmp1_;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	_tmp1_ = gtk_list_box_get_selected_row (_tmp0_);
	result = G_TYPE_CHECK_INSTANCE_TYPE (_tmp1_, DIA_TYPE_LIST_ITEM) ? ((DiaListItem*) _tmp1_) : NULL;
	return result;
}


GtkSelectionMode
dia_list_get_selection_mode (DiaList* self)
{
	GtkSelectionMode result;
	GtkListBox* _tmp0_;
	GtkSelectionMode _tmp1_;
	GtkSelectionMode _tmp2_;
	g_return_val_if_fail (self != NULL, 0);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	_tmp1_ = gtk_list_box_get_selection_mode (_tmp0_);
	_tmp2_ = _tmp1_;
	result = _tmp2_;
	return result;
}


void
dia_list_set_selection_mode (DiaList* self,
                             GtkSelectionMode value)
{
	GtkListBox* _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_list_box_set_selection_mode (_tmp0_, value);
	g_object_notify_by_pspec ((GObject *) self, dia_list_properties[DIA_LIST_SELECTION_MODE_PROPERTY]);
}


static void
_dia_list___lambda8_ (DiaList* self)
{
	g_signal_emit (self, dia_list_signals[DIA_LIST_SELECTION_CHANGED_SIGNAL], 0);
}


static void
__dia_list___lambda8__gtk_list_box_row_selected (GtkListBox* _sender,
                                                 GtkListBoxRow* row,
                                                 gpointer self)
{
	_dia_list___lambda8_ ((DiaList*) self);
}


static GObject *
dia_list_constructor (GType type,
                      guint n_construct_properties,
                      GObjectConstructParam * construct_properties)
{
	GObject * obj;
	GObjectClass * parent_class;
	DiaList * self;
	GtkListBox* _tmp0_;
	GtkListBox* _tmp1_;
	parent_class = G_OBJECT_CLASS (dia_list_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST, DiaList);
	_tmp0_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) _tmp0_);
	_tmp1_ = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
	g_signal_connect_object (_tmp1_, "row-selected", (GCallback) __dia_list___lambda8__gtk_list_box_row_selected, self, 0);
	return obj;
}


static void
dia_list_class_init (DiaListClass * klass)
{
	dia_list_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->get_property = _vala_dia_list_get_property;
	G_OBJECT_CLASS (klass)->set_property = _vala_dia_list_set_property;
	G_OBJECT_CLASS (klass)->constructor = dia_list_constructor;
	G_OBJECT_CLASS (klass)->finalize = dia_list_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), DIA_LIST_CHILDREN_PROPERTY, dia_list_properties[DIA_LIST_CHILDREN_PROPERTY] = g_param_spec_pointer ("children", "children", "children", G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), DIA_LIST_SELECTION_PROPERTY, dia_list_properties[DIA_LIST_SELECTION_PROPERTY] = g_param_spec_object ("selection", "selection", "selection", DIA_TYPE_LIST_ITEM, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), DIA_LIST_SELECTION_MODE_PROPERTY, dia_list_properties[DIA_LIST_SELECTION_MODE_PROPERTY] = g_param_spec_enum ("selection-mode", "selection-mode", "selection-mode", gtk_selection_mode_get_type (), 0, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | G_PARAM_WRITABLE));
	dia_list_signals[DIA_LIST_CHILD_SELECTED_SIGNAL] = g_signal_new ("child-selected", DIA_TYPE_LIST, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, DIA_TYPE_LIST_ITEM);
	dia_list_signals[DIA_LIST_SELECTION_CHANGED_SIGNAL] = g_signal_new ("selection-changed", DIA_TYPE_LIST, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void
dia_list_init (DiaList * self)
{
	GtkListBox* _tmp0_;
	_tmp0_ = (GtkListBox*) gtk_list_box_new ();
	g_object_ref_sink (_tmp0_);
	((DiaListPrivate *) dia_list_get_instance_private (self))->real = _tmp0_;
}


static void
dia_list_finalize (GObject * obj)
{
	DiaList * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST, DiaList);
	g_object_unref (((DiaListPrivate *) dia_list_get_instance_private (self))->real);
	G_OBJECT_CLASS (dia_list_parent_class)->finalize (obj);
}

static void
_vala_dia_list_get_property (GObject * object,
                             guint property_id,
                             GValue * value,
                             GParamSpec * pspec)
{
	DiaList * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST, DiaList);
	switch (property_id) {
		case DIA_LIST_CHILDREN_PROPERTY:
		g_value_set_pointer (value, dia_list_get_children (self));
		break;
		case DIA_LIST_SELECTION_PROPERTY:
		g_value_set_object (value, dia_list_get_selection (self));
		break;
		case DIA_LIST_SELECTION_MODE_PROPERTY:
		g_value_set_enum (value, dia_list_get_selection_mode (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
_vala_dia_list_set_property (GObject * object,
                             guint property_id,
                             const GValue * value,
                             GParamSpec * pspec)
{
	DiaList * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST, DiaList);
	switch (property_id) {
		case DIA_LIST_SELECTION_MODE_PROPERTY:
		dia_list_set_selection_mode (self, g_value_get_enum (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


DiaListItem*
dia_list_item_construct_with_label (GType object_type,
                                    const gchar* lbl)
{
	DiaListItem * self = NULL;
	g_return_val_if_fail (lbl != NULL, NULL);
	self = (DiaListItem*) g_object_new (object_type, NULL);
	dia_list_item_set_value (self, lbl);
	return self;
}


GtkWidget *
dia_list_item_new_with_label (const gchar* lbl)
{
	return (GtkWidget *) dia_list_item_construct_with_label (DIA_TYPE_LIST_ITEM, lbl);
}


DiaListItem*
dia_list_item_construct (GType object_type)
{
	DiaListItem * self = NULL;
	self = (DiaListItem*) g_object_new (object_type, NULL);
	return self;
}


GtkWidget *
dia_list_item_new (void)
{
	return (GtkWidget *) dia_list_item_construct (DIA_TYPE_LIST_ITEM);
}


const gchar*
dia_list_item_get_value (DiaListItem* self)
{
	const gchar* result;
	GtkLabel* _tmp0_;
	const gchar* _tmp1_;
	const gchar* _tmp2_;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = ((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label;
	_tmp1_ = gtk_label_get_label (_tmp0_);
	_tmp2_ = _tmp1_;
	result = _tmp2_;
	return result;
}


void
dia_list_item_set_value (DiaListItem* self,
                         const gchar* value)
{
	GtkLabel* _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label;
	gtk_label_set_label (_tmp0_, value);
	g_object_notify_by_pspec ((GObject *) self, dia_list_item_properties[DIA_LIST_ITEM_VALUE_PROPERTY]);
}


static GObject *
dia_list_item_constructor (GType type,
                           guint n_construct_properties,
                           GObjectConstructParam * construct_properties)
{
	GObject * obj;
	GObjectClass * parent_class;
	DiaListItem * self;
	GtkLabel* _tmp0_;
	parent_class = G_OBJECT_CLASS (dia_list_item_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST_ITEM, DiaListItem);
	_tmp0_ = ((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label;
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) _tmp0_);
	return obj;
}


static void
dia_list_item_class_init (DiaListItemClass * klass)
{
	dia_list_item_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->get_property = _vala_dia_list_item_get_property;
	G_OBJECT_CLASS (klass)->set_property = _vala_dia_list_item_set_property;
	G_OBJECT_CLASS (klass)->constructor = dia_list_item_constructor;
	G_OBJECT_CLASS (klass)->finalize = dia_list_item_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), DIA_LIST_ITEM_VALUE_PROPERTY, dia_list_item_properties[DIA_LIST_ITEM_VALUE_PROPERTY] = g_param_spec_string ("value", "value", "value", NULL, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void
dia_list_item_init (DiaListItem * self)
{
	GtkLabel* _tmp0_;
	_tmp0_ = (GtkLabel*) gtk_label_new (NULL);
	g_object_ref_sink (_tmp0_);
	((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label = _tmp0_;
}


static void
dia_list_item_finalize (GObject * obj)
{
	DiaListItem * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST_ITEM, DiaListItem);
	g_object_unref (((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label);
	G_OBJECT_CLASS (dia_list_item_parent_class)->finalize (obj);
}

static void
_vala_dia_list_item_get_property (GObject * object,
                                  guint property_id,
                                  GValue * value,
                                  GParamSpec * pspec)
{
	DiaListItem * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST_ITEM, DiaListItem);
	switch (property_id) {
		case DIA_LIST_ITEM_VALUE_PROPERTY:
		g_value_set_string (value, dia_list_item_get_value (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void
_vala_dia_list_item_set_property (GObject * object,
                                  guint property_id,
                                  const GValue * value,
                                  GParamSpec * pspec)
{
	DiaListItem * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST_ITEM, DiaListItem);
	switch (property_id) {
		case DIA_LIST_ITEM_VALUE_PROPERTY:
		dia_list_item_set_value (self, g_value_get_string (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}



