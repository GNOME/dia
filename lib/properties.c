/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * properties.h: property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <string.h>

#include "geometry.h"
#include "arrows.h"
#include "color.h"
#include "font.h"

#include "intl.h"
#include "widgets.h"

#define LIBDIA_COMPILATION
#undef G_INLINE_FUNC
#define G_INLINE_FUNC extern
#define G_CAN_INLINE 1
#include "properties.h"

typedef struct {
  gchar *name;
  PropType_Copy cfunc;
  PropType_Free ffunc;
  PropType_GetWidget wfunc;
  PropType_SetProp sfunc;
  PropType_Load loadfunc;
  PropType_Save savefunc;
} CustomProp;

static GArray *custom_props = NULL;
static GHashTable *custom_props_hash = NULL;

PropType
prop_type_register(const gchar *name, PropType_Copy cfunc,
		   PropType_Free ffunc, PropType_GetWidget wfunc,
		   PropType_SetProp sfunc,
		   PropType_Load loadfunc, PropType_Save savefunc)
{
  CustomProp cprop;
  PropType ret;

  g_return_val_if_fail(name != NULL, PROP_TYPE_INVALID);
  g_return_val_if_fail((wfunc != NULL) == (sfunc != NULL), PROP_TYPE_INVALID);
  g_return_val_if_fail((loadfunc != NULL) == (savefunc != NULL),
		       PROP_TYPE_INVALID);

  if (!custom_props) {
    custom_props = g_array_new(TRUE, TRUE, sizeof(CustomProp));
    custom_props_hash = g_hash_table_new(g_str_hash, g_str_equal);
  }

  cprop.name = g_strdup(name);
  cprop.cfunc = cfunc;
  cprop.ffunc = ffunc;
  cprop.wfunc = wfunc;
  cprop.sfunc = sfunc;
  cprop.loadfunc = loadfunc;
  cprop.savefunc = savefunc;

  ret = custom_props->len + PROP_LAST;
  g_array_append_val(custom_props, cprop);
  g_hash_table_insert(custom_props_hash, cprop.name,
		&g_array_index(custom_props, CustomProp, custom_props->len-1));

  return ret;
}

PropDescription *
prop_desc_lists_union(GList *plists)
{
  GArray *arr = g_array_new(TRUE, TRUE, sizeof(PropDescription));
  PropDescription *ret;
  GList *tmp;

  for (tmp = plists; tmp; tmp = tmp->next) {
    PropDescription *plist = tmp->data;
    int i;

    for (i = 0; plist[i].name != NULL; i++) {
      int j;
      for (j = 0; j < arr->len; j++)
	if (g_array_index(arr, PropDescription, j).quark == plist[i].quark)
	  break;
      if (j == arr->len)
	g_array_append_val(arr, plist[i]);
    }
  }
  ret = (PropDescription *)arr->data;
  g_array_free(arr, FALSE);
  return ret;
}

PropDescription *
prop_desc_lists_intersection(GList *plists)
{
  GArray *arr = g_array_new(TRUE, TRUE, sizeof(PropDescription));
  PropDescription *ret;
  GList *tmp;
  gint i;

  if (plists) {
    ret = plists->data;
    for (i = 0; ret[i].name != NULL; i++)
      g_array_append_val(arr, ret[i]);

    /* check each PropDescription list for intersection */
    for (tmp = plists->next; tmp; tmp = tmp->next) {
      ret = tmp->data;

      /* go through array in reverse so that removals don't stuff things up */
      for (i = arr->len - 1; i >= 0; i--) {
	gint j;

	for (j = 0; ret[j].name != NULL; j++)
	  if (g_array_index(arr, PropDescription, i).quark == ret[j].quark)
	    break;
	if (ret[j].name == NULL)
	  g_array_remove_index(arr, i);
      }
    }
  }
  ret = (PropDescription *)arr->data;
  g_array_free(arr, FALSE);
  return ret;
}

void
prop_copy(Property *dest, Property *src)
{
  CustomProp *cp;

  g_return_if_fail(src != NULL);
  g_return_if_fail(dest != NULL);

  dest->name = src->name;
  dest->type = src->type;
  dest->extra_data = src->extra_data;
  switch (src->type) {
  case PROP_TYPE_INVALID:
  case PROP_TYPE_CHAR:
  case PROP_TYPE_BOOL:
  case PROP_TYPE_INT:
  case PROP_TYPE_ENUM:
  case PROP_TYPE_REAL:
  case PROP_TYPE_POINT:
  case PROP_TYPE_RECT:
  case PROP_TYPE_LINESTYLE:
  case PROP_TYPE_ARROW:
  case PROP_TYPE_COLOUR:
  case PROP_TYPE_FONT:
    dest->d = src->d;
    break;
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*dest));
    PROP_VALUE_STRING(*dest) = g_strdup(PROP_VALUE_STRING(*src));
    break;
  case PROP_TYPE_POINTARRAY:
    g_free(PROP_VALUE_POINTARRAY(*dest).pts);
    PROP_VALUE_POINTARRAY(*dest).npts = PROP_VALUE_POINTARRAY(*src).npts;
    PROP_VALUE_POINTARRAY(*dest).pts =g_memdup(PROP_VALUE_POINTARRAY(*src).pts,
			sizeof(Point) * PROP_VALUE_POINTARRAY(*src).npts);
    break;
  default:
    if (custom_props == NULL || src->type - PROP_LAST >= custom_props->len) {
      g_warning("prop type id %d out of range!!!", src->type);
      return;
    }
    cp = &g_array_index(custom_props, CustomProp, src->type - PROP_LAST);
    if (cp->ffunc)
      cp->ffunc(dest);
    if (cp->cfunc)
      cp->cfunc(dest, src);
    else
      dest->d = src->d; /* straight copy */
  }
}

void
prop_free(Property *prop)
{
  CustomProp *cp;

  g_return_if_fail(prop != NULL);

  switch (prop->type) {
  case PROP_TYPE_INVALID:
  case PROP_TYPE_CHAR:
  case PROP_TYPE_BOOL:
  case PROP_TYPE_INT:
  case PROP_TYPE_ENUM:
  case PROP_TYPE_REAL:
  case PROP_TYPE_POINT:
  case PROP_TYPE_RECT:
  case PROP_TYPE_LINESTYLE:
  case PROP_TYPE_ARROW:
  case PROP_TYPE_COLOUR:
  case PROP_TYPE_FONT:
    break;
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*prop));
    break;
  case PROP_TYPE_POINTARRAY:
    g_free(PROP_VALUE_POINTARRAY(*prop).pts);
    break;
  default:
    if (custom_props == NULL || prop->type - PROP_LAST >= custom_props->len) {
      g_warning("prop type id %d out of range!!!", prop->type);
      return;
    }
    cp = &g_array_index(custom_props, CustomProp, prop->type - PROP_LAST);
    if (cp->ffunc)
      cp->ffunc(prop);
  }
  /* zero out data member so we don't get problems if we try to refree the
   * property. */
  memset(&prop->d, '\0', sizeof(prop->d));
}

void
prop_list_free(Property *props, guint nprops)
{
  int i;

  g_return_if_fail(props != NULL);

  for (i = 0; i < nprops; i++)
    prop_free(&props[i]);
  g_free(props);
}

static void
bool_toggled(GtkWidget *wid)
{
  if (GTK_TOGGLE_BUTTON(wid)->active)
    gtk_label_set(GTK_LABEL(GTK_BIN(wid)->child), _("Yes"));
  else
    gtk_label_set(GTK_LABEL(GTK_BIN(wid)->child), _("No"));
}

GtkWidget *
prop_get_widget(Property *prop)
{
  GtkWidget *ret = NULL;
  GtkAdjustment *adj;
  gchar buf[256];

  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("invalid property type for `%s'",  prop->name);
    break;
  case PROP_TYPE_CHAR:
    ret = gtk_entry_new();
    buf[0] = PROP_VALUE_CHAR(*prop);
    buf[1] = '\0';
    gtk_entry_set_text(GTK_ENTRY(ret), buf);
    break;
  case PROP_TYPE_BOOL:
    ret = gtk_toggle_button_new_with_label(_("No"));
    gtk_signal_connect(GTK_OBJECT(ret), "toggled",
		       GTK_SIGNAL_FUNC(bool_toggled), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret),
				 PROP_VALUE_BOOL(*prop));
    break;
  case PROP_TYPE_INT:
    if (prop->extra_data) {
      PropNumData *numdata = prop->extra_data;
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_INT(*prop),
					      numdata->min, numdata->max,
					      numdata->step, numdata->step,
					      numdata->step));
    } else
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_INT(*prop),
					      G_MININT, G_MAXINT,
					      1.0, 1.0, 1.0));
    ret = gtk_spin_button_new(adj, 1.0, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret), TRUE);
    break;
  case PROP_TYPE_ENUM:
    if (prop->extra_data) {
      PropEnumData *enumdata = prop->extra_data;
      GtkWidget *menu;
      guint i, pos = 0;

      ret = gtk_option_menu_new();
      menu = gtk_menu_new();
      for (i = 0; enumdata[i].name != NULL; i++) {
	GtkWidget *item = gtk_menu_item_new_with_label(_(enumdata[i].name));

	if (enumdata[i].enumv == PROP_VALUE_ENUM(*prop))
	  pos = i;
	gtk_object_set_user_data(GTK_OBJECT(item),
				 GUINT_TO_POINTER(enumdata[i].enumv));
	gtk_container_add(GTK_CONTAINER(menu), item);
	gtk_widget_show(item);
      }
      gtk_option_menu_set_menu(GTK_OPTION_MENU(ret), menu);
      gtk_option_menu_set_history(GTK_OPTION_MENU(ret), i);
    } else {
      ret = gtk_entry_new(); /* should use spin button/option menu */
      g_snprintf(buf, sizeof(buf), "%d", PROP_VALUE_ENUM(*prop));
      gtk_entry_set_text(GTK_ENTRY(ret), buf);
    }
    break;
  case PROP_TYPE_REAL:
    if (prop->extra_data) {
      PropNumData *numdata = prop->extra_data;
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_REAL(*prop),
					      numdata->min, numdata->max,
					      numdata->step, numdata->step,
					      numdata->step));
    } else
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_REAL(*prop),
					      G_MINFLOAT, G_MAXFLOAT,
					      0.1, 0.1, 0.1));
    ret = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret), TRUE);
    break;
  case PROP_TYPE_STRING:
    ret = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ret), PROP_VALUE_STRING(*prop));
    break;
  case PROP_TYPE_POINT:
  case PROP_TYPE_POINTARRAY:
  case PROP_TYPE_RECT:
    ret = gtk_label_new(_("No edit widget"));
    break;
  case PROP_TYPE_LINESTYLE:
    ret = dia_line_style_selector_new();
    dia_line_style_selector_set_linestyle(DIALINESTYLESELECTOR(ret),
					  PROP_VALUE_LINESTYLE(*prop).style,
					  PROP_VALUE_LINESTYLE(*prop).dash);
    break;
  case PROP_TYPE_ARROW:
    ret = dia_arrow_selector_new();
    dia_arrow_selector_set_arrow(DIAARROWSELECTOR(ret),
				 PROP_VALUE_ARROW(*prop));
    break;
  case PROP_TYPE_COLOUR:
    ret = dia_color_selector_new();
    dia_color_selector_set_color(DIACOLORSELECTOR(ret),
				 &PROP_VALUE_COLOUR(*prop));
    break;
  case PROP_TYPE_FONT:
    ret = dia_font_selector_new();
    dia_font_selector_set_font(DIAFONTSELECTOR(ret), PROP_VALUE_FONT(*prop));
    break;
  default:
    /* custom property */
    if (custom_props == NULL ||
	prop->type - PROP_LAST >= custom_props->len ||
	g_array_index(custom_props, CustomProp,
		      prop->type - PROP_LAST).wfunc == NULL)
      ret = gtk_label_new(_("No edit widget"));
    else
      ret = g_array_index(custom_props, CustomProp,
			  prop->type - PROP_LAST).wfunc(prop);
  }
  return ret;
}

void
prop_set_from_widget(Property *prop, GtkWidget *widget)
{
  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("invalid property type for `%s'",  prop->name);
    break;
  case PROP_TYPE_CHAR:
    PROP_VALUE_CHAR(*prop) = gtk_entry_get_text(GTK_ENTRY(widget))[0];
    break;
  case PROP_TYPE_BOOL:
    PROP_VALUE_BOOL(*prop) = GTK_TOGGLE_BUTTON(widget)->active;
    break;
  case PROP_TYPE_INT:
    PROP_VALUE_INT(*prop) = gtk_spin_button_get_value_as_int(
					GTK_SPIN_BUTTON(widget));
    break;
  case PROP_TYPE_ENUM:
    if (GTK_IS_OPTION_MENU(widget))
      PROP_VALUE_ENUM(*prop) = GPOINTER_TO_UINT(gtk_object_get_user_data(
			GTK_OBJECT(GTK_OPTION_MENU(widget)->menu_item)));
    else
      PROP_VALUE_ENUM(*prop) = strtol(gtk_entry_get_text(GTK_ENTRY(widget)),
				     NULL, 0);
    break;
  case PROP_TYPE_REAL:
    PROP_VALUE_REAL(*prop) = gtk_spin_button_get_value_as_float(
					GTK_SPIN_BUTTON(widget));
    break;
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*prop));
    PROP_VALUE_STRING(*prop) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
    break;
  case PROP_TYPE_POINT:
  case PROP_TYPE_POINTARRAY:
  case PROP_TYPE_RECT:
    /* nothing */
    break;
  case PROP_TYPE_LINESTYLE:
    dia_line_style_selector_get_linestyle(DIALINESTYLESELECTOR(widget),
					  &PROP_VALUE_LINESTYLE(*prop).style,
					  &PROP_VALUE_LINESTYLE(*prop).dash);
    break;
  case PROP_TYPE_ARROW:
    PROP_VALUE_ARROW(*prop) =
      dia_arrow_selector_get_arrow(DIAARROWSELECTOR(widget));
    break;
  case PROP_TYPE_COLOUR:
    dia_color_selector_get_color(DIACOLORSELECTOR(widget),
				 &PROP_VALUE_COLOUR(*prop));
    break;
  case PROP_TYPE_FONT:
     PROP_VALUE_FONT(*prop) =
       dia_font_selector_get_font(DIAFONTSELECTOR(widget));
    break;
  default:
    /* custom property */
    if (custom_props != NULL &&
	prop->type - PROP_LAST < custom_props->len &&
	g_array_index(custom_props, CustomProp,
		      prop->type - PROP_LAST).sfunc != NULL)
      g_array_index(custom_props, CustomProp,
		    prop->type - PROP_LAST).sfunc(prop, widget);
  }
}

void
prop_load(Property *prop, ObjectNode obj_node)
{
  CustomProp *cp;
  AttributeNode attr;
  DataNode data;
  gchar *str;
  guint i;

  g_return_if_fail(prop != NULL);

  /* custom prop types may have some other handling */
  if (!PROP_IS_OTHER(prop->type)) {
    attr = object_find_attribute(obj_node, prop->name);
    if (!attr) {
      g_warning("Could not find attribute %s", prop->name);
      return;
    }
    data = attribute_first_data(attr);
    if (!data) {
      g_warning("Attribute %s contains no data", prop->name);
      return;
    }
  }

  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("Can't load invalid");
    break;
  case PROP_TYPE_CHAR:
    str = data_string(data);
    if (str && str[0]) {
      PROP_VALUE_CHAR(*prop) = str[0];
      g_free(str);
    } else
      g_warning("Could not read character data for attribute %s", prop->name);
    break;
  case PROP_TYPE_BOOL:
    PROP_VALUE_BOOL(*prop) = data_boolean(data);
    break;
  case PROP_TYPE_INT:
    PROP_VALUE_INT(*prop) = data_int(data);
    break;
  case PROP_TYPE_ENUM:
    PROP_VALUE_ENUM(*prop) = data_enum(data);
    break;
  case PROP_TYPE_REAL:
    PROP_VALUE_REAL(*prop) = data_real(data);
    break;
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*prop));
    PROP_VALUE_STRING(*prop) = data_string(data);
    break;
  case PROP_TYPE_POINT:
    data_point(data, &PROP_VALUE_POINT(*prop));
    break;
  case PROP_TYPE_POINTARRAY:
    PROP_VALUE_POINTARRAY(*prop).npts = attribute_num_data(attr);
    g_free(PROP_VALUE_POINTARRAY(*prop).pts);
    PROP_VALUE_POINTARRAY(*prop).pts = g_new(Point,
					PROP_VALUE_POINTARRAY(*prop).npts);
    for (i = 0; i < PROP_VALUE_POINTARRAY(*prop).npts; i++) {
      data_point(data, &PROP_VALUE_POINTARRAY(*prop).pts[i]);
      data = data_next(data);
    }
    break;
  case PROP_TYPE_RECT:
    data_rectangle(data, &PROP_VALUE_RECT(*prop));
    break;
  case PROP_TYPE_LINESTYLE:
    PROP_VALUE_LINESTYLE(*prop).style = data_enum(data);
    PROP_VALUE_LINESTYLE(*prop).dash = 1.0;
    /* don't bother checking dash length if we have a solid line. */
    if (PROP_VALUE_LINESTYLE(*prop).style != LINESTYLE_SOLID) {
      data = data_next(data);
      if (data)
	PROP_VALUE_LINESTYLE(*prop).dash = data_real(data);
      else {
	/* backward compatibility */
	if ((attr = object_find_attribute(obj_node, "dashlength")) &&
	    (data = attribute_first_data(attr)))
	  PROP_VALUE_LINESTYLE(*prop).dash = data_real(data);
      }
    }
    break;
  case PROP_TYPE_ARROW:
    /* Maybe it would be better to store arrows as a single composite
     * attribute rather than three seperate attributes. This would break
     * binary compatibility though.*/
    PROP_VALUE_ARROW(*prop).type = data_enum(data);
    PROP_VALUE_ARROW(*prop).length = 0.8;
    PROP_VALUE_ARROW(*prop).width = 0.8;
    if (PROP_VALUE_ARROW(*prop).type != ARROW_NONE) {
      str = g_strconcat(prop->name, "_length", NULL);
      if ((attr = object_find_attribute(obj_node, str)) &&
	  (data = attribute_first_data(attr)))
	PROP_VALUE_ARROW(*prop).length = data_real(data);
      g_free(str);
      str = g_strconcat(prop->name, "_width", NULL);
      if ((attr = object_find_attribute(obj_node, str)) &&
	  (data = attribute_first_data(attr)))
	PROP_VALUE_ARROW(*prop).width = data_real(data);
      g_free(str);
    }
    break;
  case PROP_TYPE_COLOUR:
    data_color(data, &PROP_VALUE_COLOUR(*prop));
    break;
  case PROP_TYPE_FONT:
    PROP_VALUE_FONT(*prop) = data_font(data);
  default:
    if (custom_props == NULL || prop->type - PROP_LAST >= custom_props->len) {
      g_warning("prop type id %d out of range!!!", prop->type);
      return;
    }
    cp = &g_array_index(custom_props, CustomProp, prop->type - PROP_LAST);
    if (cp->ffunc)
      cp->ffunc(prop);
    if (cp->loadfunc)
      cp->loadfunc(prop, obj_node);
    else
      g_warning("Don't know how to load custom property of type %s", cp->name);
    break;
  }
}

void
prop_save(Property *prop, ObjectNode obj_node)
{
  CustomProp *cp;
  AttributeNode attr;
  gchar buf[32], *str;
  guint i;

  g_return_if_fail(prop != NULL);

  if (!PROP_IS_OTHER(prop->type))
    attr = new_attribute(obj_node, prop->name);

  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("Can't save invalid");
    break;
  case PROP_TYPE_CHAR:
    buf[0] = PROP_VALUE_CHAR(*prop);
    buf[1] = '\0';
    data_add_string(attr, buf);
    break;
  case PROP_TYPE_BOOL:
    data_add_boolean(attr, PROP_VALUE_BOOL(*prop));
    break;
  case PROP_TYPE_INT:
    data_add_int(attr, PROP_VALUE_INT(*prop));
    break;
  case PROP_TYPE_ENUM:
    data_add_enum(attr, PROP_VALUE_ENUM(*prop));
    break;
  case PROP_TYPE_REAL:
    data_add_real(attr, PROP_VALUE_REAL(*prop));
    break;
  case PROP_TYPE_STRING:
    data_add_string(attr, PROP_VALUE_STRING(*prop));
    break;
  case PROP_TYPE_POINT:
    data_add_point(attr, &PROP_VALUE_POINT(*prop));
    break;
  case PROP_TYPE_POINTARRAY:
    for (i = 0; i < PROP_VALUE_POINTARRAY(*prop).npts; i++)
      data_add_point(attr, &PROP_VALUE_POINTARRAY(*prop).pts[i]);
    break;
  case PROP_TYPE_RECT:
    data_add_rectangle(attr, &PROP_VALUE_RECT(*prop));
    break;
  case PROP_TYPE_LINESTYLE:
    data_add_enum(attr, PROP_VALUE_LINESTYLE(*prop).style);
    data_add_real(attr, PROP_VALUE_LINESTYLE(*prop).dash);
    /* for compatibility.  It makes more sense to link the two together */
    data_add_real(new_attribute(obj_node, "dashlength"),
		  PROP_VALUE_LINESTYLE(*prop).dash);
    break;
  case PROP_TYPE_ARROW:
    data_add_enum(attr, PROP_VALUE_ARROW(*prop).type);
    if (PROP_VALUE_ARROW(*prop).type != ARROW_NONE) {
      str = g_strconcat(prop->name, "_length", NULL);
      attr = new_attribute(obj_node, str);
      g_free(str);
      data_add_real(attr, PROP_VALUE_ARROW(*prop).length);
      str = g_strconcat(prop->name, "_width", NULL);
      attr = new_attribute(obj_node, str);
      g_free(str);
      data_add_real(attr, PROP_VALUE_ARROW(*prop).width);
    }
    break;
  case PROP_TYPE_COLOUR:
    data_add_color(attr, &PROP_VALUE_COLOUR(*prop));
    break;
  case PROP_TYPE_FONT:
    data_add_font(attr, PROP_VALUE_FONT(*prop));
    break;
  default:
    if (custom_props == NULL || prop->type - PROP_LAST >= custom_props->len) {
      g_warning("prop type id %d out of range!!!", prop->type);
      return;
    }
    cp = &g_array_index(custom_props, CustomProp, prop->type - PROP_LAST);
    if (cp->savefunc)
      cp->savefunc(prop, obj_node);
    else
      g_warning("Don't know how to save custom property of type %s", cp->name);
  }
}


/* --------------------------------------- */

/* an ObjectChange structure for setting of properties */
typedef struct _ObjectPropChange ObjectPropChange;
struct _ObjectPropChange {
  ObjectChange obj_change;

  Object *obj;
  Property *saved_props;
  guint nsaved_props;
};

static void
object_prop_change_apply_revert(ObjectPropChange *change, Object *obj)
{
  Property *old_props;
  guint i;

  /* create new properties structure with current values */
  old_props = g_new0(Property, change->nsaved_props);
  for (i = 0; i < change->nsaved_props; i++) {
    old_props[i].name       = change->saved_props[i].name;
    old_props[i].type       = change->saved_props[i].type;
    old_props[i].extra_data = change->saved_props[i].extra_data;
  }
  if (change->obj->ops->get_props)
    change->obj->ops->get_props(change->obj, old_props, change->nsaved_props);

  /* set saved property values */
  if (change->obj->ops->set_props)
    change->obj->ops->set_props(change->obj, change->saved_props,
				change->nsaved_props);

  /* move old props to saved properties */
  prop_list_free(change->saved_props, change->nsaved_props);
  change->saved_props = old_props;
}

static void
object_prop_change_free(ObjectPropChange *change)
{
  prop_list_free(change->saved_props, change->nsaved_props);
}

ObjectChange *
object_apply_props(Object *obj, Property *props, guint nprops)
{
  ObjectPropChange *change;
  Property *old_props;
  guint i;

  change = g_new(ObjectPropChange, 1);

  change->obj_change.apply =
    (ObjectChangeApplyFunc) object_prop_change_apply_revert;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) object_prop_change_apply_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) object_prop_change_free;

  change->obj = obj;

  /* create new properties structure with current values */
  old_props = g_new0(Property, nprops);
  for (i = 0; i < nprops; i++) {
    old_props[i].name       = props[i].name;
    old_props[i].type       = props[i].type;
    old_props[i].extra_data = props[i].extra_data;
  }
  if (obj->ops->get_props)
    obj->ops->get_props(obj, old_props, nprops);

  /* set saved property values */
  if (obj->ops->set_props)
    obj->ops->set_props(obj, props, nprops);

  change->saved_props = old_props;
  change->nsaved_props = nprops;

  return (ObjectChange *)change;
}

/* --------------------------------------- */

#define struct_member(sp, off, tp) (*(tp *)(((char *)sp) + off))

gboolean
object_get_props_from_offsets(Object *obj, PropOffset *offsets,
			      Property *props, guint nprops)
{
  guint i, j;
  guint handled = 0;

  for (i = 0; offsets[i].name != NULL; i++)
    if (offsets[i].name_quark == 0)
      offsets[i].name_quark = g_quark_from_string(offsets[i].name);

  for (i = 0; i < nprops; i++) {
    GQuark prop_quark = g_quark_from_string(props[i].name);

    for (j = 0; offsets[j].name != NULL; j++)
      if (offsets[j].name_quark == prop_quark)
	break;
    if (offsets[j].name == NULL || props[i].type != offsets[j].type)
      continue;
    switch (offsets[j].type) {
    case PROP_TYPE_CHAR:
      PROP_VALUE_CHAR(props[i]) = struct_member(obj, offsets[j].offset, gchar);
      break;
    case PROP_TYPE_BOOL:
      PROP_VALUE_BOOL(props[i]) =
	struct_member(obj, offsets[j].offset, gboolean);
      break;
    case PROP_TYPE_INT:
    case PROP_TYPE_ENUM:
      PROP_VALUE_INT(props[i]) =
	struct_member(obj, offsets[j].offset, gint);
      break;
    case PROP_TYPE_REAL:
      PROP_VALUE_REAL(props[i]) =
	struct_member(obj, offsets[j].offset, real);
      break;
    case PROP_TYPE_STRING:
      g_free(PROP_VALUE_STRING(props[i]));
      PROP_VALUE_STRING(props[i]) =
	g_strdup(struct_member(obj, offsets[j].offset, gchar *));
      break;
    case PROP_TYPE_POINT:
      PROP_VALUE_POINT(props[i]) =
	struct_member(obj, offsets[j].offset, Point);
      break;
    case PROP_TYPE_POINTARRAY:
      g_warning("PropType == pointarray not supported");
      break;
    case PROP_TYPE_RECT:
      PROP_VALUE_RECT(props[i]) =
	struct_member(obj, offsets[j].offset, Rectangle);
      break;
    case PROP_TYPE_LINESTYLE:
      PROP_VALUE_LINESTYLE(props[i]).style =
	struct_member(obj, offsets[j].offset, LineStyle);
      PROP_VALUE_LINESTYLE(props[i]).dash =
	struct_member(obj, offsets[j].offset2, real);
      break;
    case PROP_TYPE_ARROW:
      PROP_VALUE_ARROW(props[i]) =
	struct_member(obj, offsets[j].offset, Arrow);
      break;
    case PROP_TYPE_COLOUR:
      PROP_VALUE_COLOUR(props[i]) =
	struct_member(obj, offsets[j].offset, Color);
      break;
    case PROP_TYPE_FONT:
      PROP_VALUE_FONT(props[i]) =
	struct_member(obj, offsets[j].offset, Font *);
      break;
    default:
      g_warning("Prop %s: type == %d not supported", props[i].name,
		offsets[j].type);
      break;
    }
    handled++;
  }
  return handled == nprops;
}

gboolean
object_set_props_from_offsets(Object *obj, PropOffset *offsets,
			      Property *props, guint nprops)
{
  guint i, j;
  guint handled = 0;

  for (i = 0; offsets[i].name != NULL; i++)
    if (offsets[i].name_quark == 0)
      offsets[i].name_quark = g_quark_from_string(offsets[i].name);

  for (i = 0; i < nprops; i++) {
    GQuark prop_quark = g_quark_from_string(props[i].name);

    for (j = 0; offsets[j].name != NULL; j++)
      if (offsets[j].name_quark == prop_quark)
	break;
    if (offsets[j].name == NULL)
      continue;

    props[i].type = offsets[j].type;
    switch (offsets[j].type) {
    case PROP_TYPE_CHAR:
      struct_member(obj, offsets[j].offset, gchar) = PROP_VALUE_CHAR(props[i]);
      break;
    case PROP_TYPE_BOOL:
      struct_member(obj, offsets[j].offset, gboolean) =
	PROP_VALUE_BOOL(props[i]);
      break;
    case PROP_TYPE_INT:
    case PROP_TYPE_ENUM:
      struct_member(obj, offsets[j].offset, gint) =
	PROP_VALUE_INT(props[i]);
      break;
    case PROP_TYPE_REAL:
      struct_member(obj, offsets[j].offset, real) =
	PROP_VALUE_REAL(props[i]);
      break;
    case PROP_TYPE_STRING:
      g_free(struct_member(obj, offsets[j].offset, gchar *));
      struct_member(obj, offsets[j].offset, gchar *) =
	g_strdup(PROP_VALUE_STRING(props[i]));
      break;
    case PROP_TYPE_POINT:
      struct_member(obj, offsets[j].offset, Point) =
	PROP_VALUE_POINT(props[i]);
      break;
    case PROP_TYPE_POINTARRAY:
      g_warning("PropType == pointarray not supported");
      break;
    case PROP_TYPE_RECT:
      struct_member(obj, offsets[j].offset, Rectangle) =
	PROP_VALUE_RECT(props[i]);
      break;
    case PROP_TYPE_LINESTYLE:
      struct_member(obj, offsets[j].offset, LineStyle) =
	PROP_VALUE_LINESTYLE(props[i]).style;
      struct_member(obj, offsets[j].offset2, real) =
	PROP_VALUE_LINESTYLE(props[i]).dash;
      break;
    case PROP_TYPE_ARROW:
      struct_member(obj, offsets[j].offset, Arrow) =
	PROP_VALUE_ARROW(props[i]);
      break;
    case PROP_TYPE_COLOUR:
      struct_member(obj, offsets[j].offset, Color) =
	PROP_VALUE_COLOUR(props[i]);
      break;
    case PROP_TYPE_FONT:
      struct_member(obj, offsets[j].offset, Font *) =
	PROP_VALUE_FONT(props[i]);
      break;
    default:
      g_warning("Prop %s: type == %d not supported", props[i].name,
		offsets[j].type);
      break;
    }
    handled++;
  }
  return handled == nprops;
}


/* --------------------------------------- */

static const gchar *prop_array_key   = "object-props:props";
static const gchar *prop_num_key     = "object-props:nprops";
static const gchar *prop_widgets_key = "object-props:widgets";

static void
object_props_dialog_destroy(GtkWidget *table)
{
  Property *props = gtk_object_get_data(GTK_OBJECT(table), prop_array_key);
  guint nprops = GPOINTER_TO_UINT(gtk_object_get_data(GTK_OBJECT(table),
						      prop_num_key));
  GtkWidget **widgets = gtk_object_get_data(GTK_OBJECT(table),
					    prop_widgets_key);

  prop_list_free(props, nprops);
  g_free(widgets);
}

GtkWidget *
object_create_props_dialog(Object *obj)
{
  const PropDescription *pdesc;
  Property *props;
  guint nprops = 0, i, j;
  GtkWidget **widgets;

  GtkWidget *table, *label;

  g_return_val_if_fail(obj->ops->describe_props != NULL, NULL);
  g_return_val_if_fail(obj->ops->get_props != NULL, NULL);
  g_return_val_if_fail(obj->ops->set_props != NULL, NULL);

  pdesc = obj->ops->describe_props(obj);
  for (i = 0; pdesc[i].name != NULL; i++)
    if ((pdesc[i].flags & PROP_FLAG_VISIBLE) != 0)
      nprops++;

  props = g_new0(Property, nprops);
  widgets = g_new(GtkWidget *, nprops);

  /* get the current values of all visible properties */
  for (i = 0, j = 0; pdesc[i].name != NULL; i++)
    if ((pdesc[i].flags & PROP_FLAG_VISIBLE) != 0) {
      props[j].name       = pdesc[i].name;
      props[j].type       = pdesc[i].type;
      props[j].extra_data = pdesc[i].extra_data;
      j++;
    }
  obj->ops->get_props(obj, props, nprops);

  table = gtk_table_new(1, 2, FALSE);

  /* construct the widgets table */
  for (i = 0, j = 0; pdesc[i].name != NULL; i++)
    if ((pdesc[i].flags & PROP_FLAG_VISIBLE) != 0) {
      widgets[j] = prop_get_widget(&props[j]);

      label = gtk_label_new(_(pdesc[i].description));
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      gtk_table_attach(GTK_TABLE(table), label, 0,1, j,j+1,
		       GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
      gtk_table_attach(GTK_TABLE(table), widgets[j], 1,2, j,j+1,
		       GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

      gtk_widget_show(label);
      gtk_widget_show(widgets[j]);

      j++;
    }
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);

  gtk_object_set_data(GTK_OBJECT(table), prop_array_key,   props);
  gtk_object_set_data(GTK_OBJECT(table), prop_num_key,
		      GUINT_TO_POINTER(nprops));
  gtk_object_set_data(GTK_OBJECT(table), prop_widgets_key, widgets);

  gtk_signal_connect(GTK_OBJECT(table), "destroy",
		     GTK_SIGNAL_FUNC(object_props_dialog_destroy), NULL);

  return table;
}

ObjectChange *
object_apply_props_from_dialog(Object *obj, GtkWidget *table)
{
  Property *props = gtk_object_get_data(GTK_OBJECT(table), prop_array_key);
  guint nprops = GPOINTER_TO_UINT(gtk_object_get_data(GTK_OBJECT(table),
						      prop_num_key));
  GtkWidget **widgets = gtk_object_get_data(GTK_OBJECT(table),
					    prop_widgets_key);
  guint i;

  for (i = 0; i < nprops; i++)
    prop_set_from_widget(&props[i], widgets[i]);

  return object_apply_props(obj, props, nprops);
}

/* --------------------------------------- */

void
object_load_props(Object *obj, ObjectNode obj_node)
{
  const PropDescription *pdesc;
  Property *props;
  guint nprops, i;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(obj_node != NULL);

  if (obj->ops->describe_props == NULL ||
      obj->ops->set_props == NULL) {
    g_warning("No describe_props or set_props!");
    return;
  }
  pdesc = obj->ops->describe_props(obj);
  if (pdesc == NULL) {
    g_warning("No properties!");
    return;
  }
  props = prop_list_from_nonmatching_descs(pdesc, PROP_FLAG_DONT_SAVE,&nprops);

  for (i = 0; i < nprops; i++)
    prop_load(&props[i], obj_node);

  obj->ops->set_props(obj, props, nprops);
  prop_list_free(props, nprops);
}

void
object_save_props(Object *obj, ObjectNode obj_node)
{
  const PropDescription *pdesc;
  Property *props;
  guint nprops = 42, i;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(obj_node != NULL);

  if (obj->ops->describe_props == NULL ||
      obj->ops->get_props == NULL) {
    g_warning("No describe_props!");
    return;
  }
  pdesc = obj->ops->describe_props(obj);
  if (pdesc == NULL) {
    g_warning("No properties!");
    return;
  }
  props = prop_list_from_nonmatching_descs(pdesc, PROP_FLAG_DONT_SAVE,&nprops);

  obj->ops->get_props(obj, props, nprops);
  for (i = 0; i < nprops; i++)
    prop_save(&props[i], obj_node);
  prop_list_free(props, nprops);
}

/* --------------------------------------- */

/* standard property extra data members */
PropNumData prop_std_line_width_data = { 0.0, 10.0, 0.01 };
PropNumData prop_std_text_height_data = { 0.1, 10.0, 0.1 };
PropEnumData prop_std_text_align_data[] = {
  { N_("Left"), ALIGN_LEFT },
  { N_("Center"), ALIGN_CENTER },
  { N_("Right"), ALIGN_RIGHT },
  { NULL, 0 }
};
