/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * properties.h: property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "geometry.h"
#include "arrows.h"
#include "color.h"
#include "font.h"
#include "text.h"

#include "intl.h"
#include "widgets.h"
#include "connpoint_line.h"
#include "charconv.h"

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
  PropType_GetProp gfunc;
  PropType_SetProp sfunc;
  PropType_Load loadfunc;
  PropType_Save savefunc;
} CustomProp;

static GArray *custom_props = NULL;
static GHashTable *custom_props_hash = NULL;

PropType
prop_type_register(const gchar *name, PropType_Copy cfunc,
		   PropType_Free ffunc, PropType_GetWidget wfunc,
		   PropType_GetProp gfunc,
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
  cprop.gfunc = gfunc;
  cprop.sfunc = sfunc;
  cprop.loadfunc = loadfunc;
  cprop.savefunc = savefunc;

  ret = custom_props->len + PROP_LAST;
  g_array_append_val(custom_props, cprop);
  g_hash_table_insert(custom_props_hash, cprop.name,
		&g_array_index(custom_props, CustomProp, custom_props->len-1));

  return ret;
}

/* finds the real handler in case there are several levels of indirection */
PropEventHandler 
prop_desc_find_real_handler(const PropDescription *pdesc)
{
  PropEventHandler ret = pdesc->event_handler;
  const PropEventHandlerChain *chain = &pdesc->chain_handler;
  if (!chain->handler) return ret;
  while (chain) {
    if (chain->handler) ret = chain->handler;
    chain = chain->chain;
  }
  return ret;
}

/* free a handler indirection list */
void 
prop_desc_free_handler_chain(PropDescription *pdesc)
{
  if (pdesc) {
    PropEventHandlerChain *chain = pdesc->chain_handler.chain;
    while (chain) {
      PropEventHandlerChain *next = chain->chain;
      g_free(chain);
      chain = next;
    } 
    pdesc->chain_handler.chain = NULL;
    pdesc->chain_handler.handler = NULL;
  }
}

/* free a handler indirection list in a list of descriptors */
void 
prop_desc_list_free_handler_chain(PropDescription *pdesc) 
{
  if (!pdesc) return;
  while (pdesc->name) {
    prop_desc_free_handler_chain(pdesc);
    pdesc++;
  }
}

/* insert an event handler */
void 
prop_desc_insert_handler(PropDescription *pdesc, 
                         PropEventHandler handler)
{
  if ((pdesc->chain_handler.handler) || (pdesc->chain_handler.chain)) {
    /* not the first level. Push things forward. */
    PropEventHandlerChain *pushed = g_new0(PropEventHandlerChain,1);
    *pushed = pdesc->chain_handler;
    pdesc->chain_handler.chain = pushed;
  }
  pdesc->chain_handler.handler = pdesc->event_handler;
  pdesc->event_handler = handler;
}


static const PropDescription null_prop_desc = { NULL };

PropDescription *
prop_desc_lists_union(GList *plists)
{
  GArray *arr = g_array_new(TRUE, TRUE, sizeof(PropDescription));
  PropDescription *ret;
  GList *tmp;

  /* make sure the array is allocated */
  g_array_append_val(arr, null_prop_desc);
  g_array_remove_index(arr, 0);

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

  /* make sure the array is allocated */
  g_array_append_val(arr, null_prop_desc);
  g_array_remove_index(arr, 0);

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
        PropDescription cand = g_array_index(arr,PropDescription,i);
        gboolean remove = TRUE;
	for (j = 0; ret[j].name != NULL; j++) {
	  if (cand.quark == ret[j].quark) {
            PropEventHandler 
              rethdl = prop_desc_find_real_handler(&ret[j]),
              candhdl = prop_desc_find_real_handler(&cand);
            remove = FALSE;
            remove |= (((ret[j].flags|cand.flags) & PROP_FLAG_DONT_MERGE)!=0);
            remove |= (rethdl != candhdl);            
          }
        }
	if (remove) g_array_remove_index(arr, i);
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
  case PROP_TYPE_BEZPOINT:
  case PROP_TYPE_RECT:
  case PROP_TYPE_LINESTYLE:
  case PROP_TYPE_ARROW:
  case PROP_TYPE_COLOUR:
  case PROP_TYPE_FONT:
  case PROP_TYPE_CONNPOINT_LINE:
  case PROP_TYPE_ENDPOINTS:
    dest->d = src->d;
    break;
  case PROP_TYPE_NOTEBOOK_BEGIN:
  case PROP_TYPE_NOTEBOOK_END:
  case PROP_TYPE_NOTEBOOK_PAGE:
  case PROP_TYPE_MULTICOL_BEGIN:
  case PROP_TYPE_MULTICOL_END:
  case PROP_TYPE_MULTICOL_COLUMN:
  case PROP_TYPE_FRAME_BEGIN:
  case PROP_TYPE_FRAME_END:
  case PROP_TYPE_STATIC:
  case PROP_TYPE_BUTTON:
    break;
  case PROP_TYPE_MULTISTRING:
  case PROP_TYPE_STRING:
  case PROP_TYPE_FILE:
    g_free(PROP_VALUE_STRING(*dest));
    if (PROP_VALUE_STRING(*src))
      PROP_VALUE_STRING(*dest) = g_strdup(PROP_VALUE_STRING(*src));
    else
      PROP_VALUE_STRING(*dest) = NULL;
    break;
  case PROP_TYPE_POINTARRAY:
    g_free(PROP_VALUE_POINTARRAY(*dest).pts);
    PROP_VALUE_POINTARRAY(*dest).npts = PROP_VALUE_POINTARRAY(*src).npts;
    PROP_VALUE_POINTARRAY(*dest).pts =g_memdup(PROP_VALUE_POINTARRAY(*src).pts,
			sizeof(Point) * PROP_VALUE_POINTARRAY(*src).npts);
    break;
  case PROP_TYPE_ENUMARRAY:
  case PROP_TYPE_INTARRAY:
    g_free(PROP_VALUE_INTARRAY(*dest).vals);
    PROP_VALUE_INTARRAY(*dest).nvals = PROP_VALUE_INTARRAY(*src).nvals;
    PROP_VALUE_INTARRAY(*dest).vals =
      g_memdup(PROP_VALUE_INTARRAY(*src).vals,
               sizeof(gint) * PROP_VALUE_INTARRAY(*src).nvals);
    break;
  case PROP_TYPE_BEZPOINTARRAY:
    g_free(PROP_VALUE_BEZPOINTARRAY(*dest).pts);
    PROP_VALUE_BEZPOINTARRAY(*dest).npts = PROP_VALUE_BEZPOINTARRAY(*src).npts;
    PROP_VALUE_BEZPOINTARRAY(*dest).pts =
      g_memdup(PROP_VALUE_BEZPOINTARRAY(*src).pts,
	       sizeof(BezPoint) * PROP_VALUE_BEZPOINTARRAY(*src).npts);
    break;
  case PROP_TYPE_TEXT:
    g_free(PROP_VALUE_TEXT(*dest).string);
    dest->d = src->d;
    if (PROP_VALUE_TEXT(*src).string) {
      PROP_VALUE_TEXT(*dest).string = g_strdup(PROP_VALUE_TEXT(*src).string);
    } else {
      PROP_VALUE_TEXT(*dest).string = NULL;
    }
    PROP_VALUE_TEXT(*dest).enabled = TRUE;
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
  case PROP_TYPE_BEZPOINT:
  case PROP_TYPE_RECT:
  case PROP_TYPE_LINESTYLE:
  case PROP_TYPE_ARROW:
  case PROP_TYPE_COLOUR:
  case PROP_TYPE_ENDPOINTS:
  case PROP_TYPE_CONNPOINT_LINE:
  case PROP_TYPE_FONT:
  case PROP_TYPE_STATIC:
  case PROP_TYPE_BUTTON:
    break;
  case PROP_TYPE_NOTEBOOK_BEGIN:
  case PROP_TYPE_NOTEBOOK_END:
  case PROP_TYPE_NOTEBOOK_PAGE:
  case PROP_TYPE_MULTICOL_BEGIN:
  case PROP_TYPE_MULTICOL_END:
  case PROP_TYPE_MULTICOL_COLUMN:
  case PROP_TYPE_FRAME_BEGIN:
  case PROP_TYPE_FRAME_END:
    break;
  case PROP_TYPE_MULTISTRING:
  case PROP_TYPE_STRING:
  case PROP_TYPE_FILE:
    g_free(PROP_VALUE_STRING(*prop));
    break;
  case PROP_TYPE_POINTARRAY:
    g_free(PROP_VALUE_POINTARRAY(*prop).pts);
    break;
  case PROP_TYPE_INTARRAY:
  case PROP_TYPE_ENUMARRAY:
    g_free(PROP_VALUE_INTARRAY(*prop).vals);
    break;
  case PROP_TYPE_BEZPOINTARRAY:
    g_free(PROP_VALUE_BEZPOINTARRAY(*prop).pts);
    break;
  case PROP_TYPE_TEXT:
    g_free(PROP_VALUE_TEXT(*prop).string);
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

static void 
property_signal_handler(GtkObject *obj,
                        gpointer func_data)
{
  PropEventData *ped = (PropEventData *)func_data;
  if (ped) {
    const PropDialogData *pdd = ped->dialog;
    Property *prop = &(pdd->props[ped->index]);
    Object *obj = pdd->obj_copy;
    int j;

    g_assert(prop->event_handler);
    g_assert(obj);
    g_assert(obj->ops->set_props);
    g_assert(obj->ops->get_props);

    /* obj is a scratch object ; we can do what we want with it. */
    obj->ops->set_props(obj,pdd->props,pdd->nprops);
    prop->event_handler(obj,prop);
    obj->ops->get_props(obj,pdd->props,pdd->nprops);

    for (j=0; j < pdd->nprops; j++) {
      prop_reset_widget(&pdd->props[j],pdd->widgets[j]);
    } 
  } else {
    g_assert_not_reached();
  }
}

static void 
prophandler_connect(const Property *prop,
                    GtkObject *object,
                    const gchar *signal)
{
  Object *obj = prop->self.dialog->obj_copy;

  if (!prop->event_handler) return;
  if (0==strcmp(signal,"FIXME")) {
    g_warning("signal type unknown for this kind of property (name is %s), \n"
              "handler ignored.",prop->name);
    return;
  } 
  if ((!obj->ops->set_props) || (!obj->ops->get_props)) {
      g_warning("object has no [sg]et_props() routine(s).\n"
                "event handler for property %s ignored.",
                prop->name);
      return;
  }

  gtk_signal_connect(object,
                     signal,
                     GTK_SIGNAL_FUNC(property_signal_handler),
                     (gpointer)(&prop->self));
}

GtkWidget *
prop_get_widget(const Property *prop)
{
  GtkWidget *ret = NULL;
  GtkAdjustment *adj;

  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("invalid property type for `%s'",  prop->name);
    break;
  case PROP_TYPE_CHAR:
    ret = gtk_entry_new();
    prophandler_connect(prop,GTK_OBJECT(ret),"changed");
    break;
  case PROP_TYPE_BOOL:
    ret = gtk_toggle_button_new_with_label(_("No"));
    gtk_signal_connect(GTK_OBJECT(ret), "toggled",
		       GTK_SIGNAL_FUNC(bool_toggled), NULL);
    prophandler_connect(prop,GTK_OBJECT(ret),"toggled");
    break;
  case PROP_TYPE_INT:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_INT(*prop),
                                            G_MININT, G_MAXINT,
                                            1.0, 10.0, 10.0));
    ret = gtk_spin_button_new(adj, 1.0, 0);
    prophandler_connect(prop,GTK_OBJECT(adj),"value_changed");
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
        prophandler_connect(prop,GTK_OBJECT(item),"activate");
      }
      gtk_option_menu_set_menu(GTK_OPTION_MENU(ret), menu);
    } else {
      ret = gtk_entry_new(); /* should use spin button/option menu */
    }
    break;
  case PROP_TYPE_REAL:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_REAL(*prop),
                                            G_MINFLOAT, G_MAXFLOAT,
                                            0.1, 1.0, 1.0));
    ret = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret), TRUE);
    prophandler_connect(prop,GTK_OBJECT(adj),"changed");
    break;
  case PROP_TYPE_NOTEBOOK_BEGIN:
  case PROP_TYPE_NOTEBOOK_END:
  case PROP_TYPE_NOTEBOOK_PAGE:
  case PROP_TYPE_MULTICOL_BEGIN:
  case PROP_TYPE_MULTICOL_END:
  case PROP_TYPE_MULTICOL_COLUMN:
  case PROP_TYPE_FRAME_BEGIN:
  case PROP_TYPE_FRAME_END:
    g_assert_not_reached();
    break;
  case PROP_TYPE_STRING:
    ret = gtk_entry_new();
    prophandler_connect(prop,GTK_OBJECT(ret),"changed");
    break;
  case PROP_TYPE_MULTISTRING: 
    ret = gtk_text_new(NULL,NULL);
    gtk_text_set_editable(GTK_TEXT(ret),TRUE);
    prophandler_connect(prop,GTK_OBJECT(ret),"changed");
    break;  
  case PROP_TYPE_POINT:
  case PROP_TYPE_POINTARRAY:
  case PROP_TYPE_INTARRAY:
  case PROP_TYPE_ENUMARRAY:
  case PROP_TYPE_BEZPOINT:
  case PROP_TYPE_BEZPOINTARRAY:
  case PROP_TYPE_ENDPOINTS:
  case PROP_TYPE_CONNPOINT_LINE:
  case PROP_TYPE_TEXT:
  case PROP_TYPE_RECT:
    ret = gtk_label_new(_("No edit widget"));
    break;
  case PROP_TYPE_STATIC:
    if (prop->descr) {
      ret = gtk_label_new(prop->descr->tooltip);
      gtk_label_set_justify(GTK_LABEL(ret),GTK_JUSTIFY_LEFT);
    }
    break;
  case PROP_TYPE_BUTTON:
    ret = gtk_button_new_with_label(prop->descr->tooltip);
    prophandler_connect(prop,GTK_OBJECT(ret),"clicked");
    break;
  case PROP_TYPE_LINESTYLE:
    ret = dia_line_style_selector_new();
    prophandler_connect(prop,GTK_OBJECT(ret),"FIXME");
    break;
  case PROP_TYPE_ARROW:
    ret = dia_arrow_selector_new();
    prophandler_connect(prop,GTK_OBJECT(ret),"FIXME");
    break;
  case PROP_TYPE_COLOUR:
    ret = dia_color_selector_new();
    prophandler_connect(prop,GTK_OBJECT(ret),"FIXME");
    break;
  case PROP_TYPE_FONT:
    ret = dia_font_selector_new();
    prophandler_connect(prop,GTK_OBJECT(ret),"FIXME");
    break;
  case PROP_TYPE_FILE:
    ret = dia_file_selector_new();
    prophandler_connect(prop,GTK_OBJECT(ret),"FIXME");
    break;
  default:
    /* custom property */
    if (custom_props == NULL ||
	prop->type - PROP_LAST >= custom_props->len ||
	g_array_index(custom_props, CustomProp,
		      prop->type - PROP_LAST).wfunc == NULL) {
      ret = gtk_label_new(_("No edit widget"));
    } else {
      ret = g_array_index(custom_props, CustomProp,
			  prop->type - PROP_LAST).wfunc(prop);
      prophandler_connect(prop,GTK_OBJECT(ret),"FIXME");
    }
  }
  return ret;
}

void 
prop_reset_widget(const Property *prop, GtkWidget *widget)
{
  GtkAdjustment *adj;
  gchar buf[256];
#ifdef UNICODE_WORK_IN_PROGRESS
  utfchar *utfbuf;
  gchar *locbuf;
#endif

  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("invalid property type for `%s'",  prop->name);
    break;
  case PROP_TYPE_CHAR:
#ifdef UNICODE_WORK_IN_PROGRESS
    utfbuf = charconv_unichar_to_utf8(PROP_VALUE_CHAR(*prop));/*keep for gtk2*/
    locbuf = charconv_utf8_to_local8(utfbuf);
    gtk_entry_set_text(GTK_ENTRY(widget), locbuf);
    g_free(locbuf);
#else
    buf[0] = PROP_VALUE_CHAR(*prop);
    buf[1] = '\0';
    gtk_entry_set_text(GTK_ENTRY(widget), buf);
#endif
    break;
  case PROP_TYPE_BOOL:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
				 PROP_VALUE_BOOL(*prop));
    break;
  case PROP_TYPE_INT:
    if (prop->extra_data) {
      PropNumData *numdata = prop->extra_data;
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_INT(*prop),
					      numdata->min, numdata->max,
					      numdata->step, 10.0 * numdata->step,
					      10.0 * numdata->step));
    } else {
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_INT(*prop),
					      G_MININT, G_MAXINT,
					      1.0, 10.0, 10.0));
    }
    gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(widget), adj);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    break;
  case PROP_TYPE_ENUM:
    if (prop->extra_data) {
      PropEnumData *enumdata = prop->extra_data;
      guint i, pos = 0;

      for (i = 0; enumdata[i].name != NULL; i++) {
	if (enumdata[i].enumv == PROP_VALUE_ENUM(*prop)) {         
	  pos = i;
          break;
        }
      }
      gtk_option_menu_set_history(GTK_OPTION_MENU(widget), pos);
    } else {
      g_snprintf(buf, sizeof(buf), "%d", PROP_VALUE_ENUM(*prop));
      gtk_entry_set_text(GTK_ENTRY(widget), buf);
    }
    break;
  case PROP_TYPE_REAL:
    if (prop->extra_data) {
      PropNumData *numdata = prop->extra_data;
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_REAL(*prop),
					      numdata->min, numdata->max,
					      numdata->step, 
                                              10.0 * numdata->step,
					      10.0 * numdata->step));
    } else {
      adj = GTK_ADJUSTMENT(gtk_adjustment_new(PROP_VALUE_REAL(*prop),
					      G_MINFLOAT, G_MAXFLOAT,
					      0.1, 1.0, 1.0));
    }
    gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(widget), adj);
    break;
  case PROP_TYPE_NOTEBOOK_BEGIN:
  case PROP_TYPE_NOTEBOOK_END:
  case PROP_TYPE_NOTEBOOK_PAGE:
  case PROP_TYPE_MULTICOL_BEGIN:
  case PROP_TYPE_MULTICOL_END:
  case PROP_TYPE_MULTICOL_COLUMN:
  case PROP_TYPE_FRAME_BEGIN:
  case PROP_TYPE_FRAME_END:
    g_assert_not_reached();
    break;
  case PROP_TYPE_STRING:
    if (PROP_VALUE_STRING(*prop)) {
#if (defined(UNICODE_WORK_IN_PROGRESS) && !defined(GTK_SPEAKS_UTF8))
      locbuf = charconv_utf8_to_local8(PROP_VALUE_STRING(*prop));
      gtk_entry_set_text(GTK_ENTRY(widget), locbuf);
      g_free(locbuf);
#else
      gtk_entry_set_text(GTK_ENTRY(widget), PROP_VALUE_STRING(*prop));
#endif
    }
    break;
  case PROP_TYPE_MULTISTRING: 
    if (PROP_VALUE_STRING(*prop)) {
      gtk_text_set_point(GTK_TEXT(widget), 0);
      gtk_text_forward_delete(GTK_TEXT(widget),
                              gtk_text_get_length(GTK_TEXT(widget)));
#if (defined(UNICODE_WORK_IN_PROGRESS) && !defined(GTK_SPEAKS_UTF8))
      locbuf = charconv_utf8_to_local8(PROP_VALUE_STRING(*prop));
      gtk_text_insert(GTK_TEXT(widget), NULL, NULL, NULL, locbuf,-1);
      g_free(locbuf);
#else
      gtk_text_insert(GTK_TEXT(widget), NULL, NULL, NULL,
                      PROP_VALUE_STRING(*prop),-1);
#endif    
    break;
  }
  case PROP_TYPE_POINT:
  case PROP_TYPE_POINTARRAY:
  case PROP_TYPE_INTARRAY:
  case PROP_TYPE_ENUMARRAY:
  case PROP_TYPE_BEZPOINT:
  case PROP_TYPE_BEZPOINTARRAY:
  case PROP_TYPE_ENDPOINTS:
  case PROP_TYPE_CONNPOINT_LINE:
  case PROP_TYPE_TEXT:
  case PROP_TYPE_RECT:
  case PROP_TYPE_STATIC:
  case PROP_TYPE_BUTTON:
    break;
  case PROP_TYPE_LINESTYLE:
    dia_line_style_selector_set_linestyle(DIALINESTYLESELECTOR(widget),
					  PROP_VALUE_LINESTYLE(*prop).style,
					  PROP_VALUE_LINESTYLE(*prop).dash);
    break;
  case PROP_TYPE_ARROW:
    dia_arrow_selector_set_arrow(DIAARROWSELECTOR(widget),
				 PROP_VALUE_ARROW(*prop));
    break;
  case PROP_TYPE_COLOUR:
    dia_color_selector_set_color(DIACOLORSELECTOR(widget),
				 &PROP_VALUE_COLOUR(*prop));
    break;
  case PROP_TYPE_FONT:
    dia_font_selector_set_font(DIAFONTSELECTOR(widget), 
                               PROP_VALUE_FONT(*prop));
    break;
  case PROP_TYPE_FILE:
    dia_file_selector_set_file(DIAFILESELECTOR(widget), 
                               PROP_VALUE_FILE(*prop));
    break;
  default:
    /* custom property */
    if (!(custom_props == NULL ||
	prop->type - PROP_LAST >= custom_props->len ||
	g_array_index(custom_props, CustomProp,
		      prop->type - PROP_LAST).gfunc == NULL))
      g_array_index(custom_props, CustomProp,
                    prop->type - PROP_LAST).gfunc(prop,widget);
  }
}

void
prop_set_from_widget(Property *prop, GtkWidget *widget)
{
#ifdef UNICODE_WORK_IN_PROGRESS
  gchar *locbuf;
  utfchar *utfbuf;
  unichar uc;
#endif
  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("invalid property type for `%s'",  prop->name);
    break;
  case PROP_TYPE_CHAR:
#ifdef UNICODE_WORK_IN_PROGRESS
    locbuf = gtk_editable_get_chars(GTK_EDITABLE(widget),0,1);
    utfbuf = charconv_local8_to_utf8(locbuf); g_free(locbuf);
    uni_get_utf8(utfbuf,&uc); g_free(utfbuf);
    PROP_VALUE_CHAR(*prop) = uc;
#else
    PROP_VALUE_CHAR(*prop) = gtk_entry_get_text(GTK_ENTRY(widget))[0];
#endif
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
  case PROP_TYPE_MULTISTRING:
    g_free(PROP_VALUE_STRING(*prop));
#ifdef UNICODE_WORK_IN_PROGRESS
    locbuf = gtk_editable_get_chars(GTK_EDITABLE(widget),0, -1);   
    utfbuf = charconv_local8_to_utf8(locbuf); g_free(locbuf);
    PROP_VALUE_STRING(*prop) = utfbuf;  
#else
    PROP_VALUE_STRING(*prop) = 
      gtk_editable_get_chars(GTK_EDITABLE(widget),0, -1);
#endif
    break;
  case PROP_TYPE_NOTEBOOK_BEGIN:
  case PROP_TYPE_NOTEBOOK_END:
  case PROP_TYPE_NOTEBOOK_PAGE:
  case PROP_TYPE_MULTICOL_BEGIN:
  case PROP_TYPE_MULTICOL_END:
  case PROP_TYPE_MULTICOL_COLUMN:
  case PROP_TYPE_FRAME_BEGIN:
  case PROP_TYPE_FRAME_END:
  case PROP_TYPE_STATIC:
  case PROP_TYPE_BUTTON:
  case PROP_TYPE_POINT:
  case PROP_TYPE_POINTARRAY:
  case PROP_TYPE_INTARRAY:
  case PROP_TYPE_ENUMARRAY:
  case PROP_TYPE_BEZPOINT:
  case PROP_TYPE_BEZPOINTARRAY:
  case PROP_TYPE_ENDPOINTS:
  case PROP_TYPE_CONNPOINT_LINE:
  case PROP_TYPE_RECT:
    /* nothing */
    break;
  case PROP_TYPE_TEXT:
    PROP_VALUE_TEXT(*prop).enabled = FALSE;
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
  case PROP_TYPE_FILE:
    g_free(PROP_VALUE_FILE(*prop));
    PROP_VALUE_FILE(*prop) = g_strdup(dia_file_selector_get_file(
					DIAFILESELECTOR(widget)));
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
  AttributeNode attr = NULL;
  DataNode data = NULL;
  gchar *str;
  guint i;
  Text *text;
#ifdef UNICODE_WORK_IN_PROGRESS
  unichar uc;
#endif

  g_return_if_fail(prop != NULL);

  /* custom prop types may have some other handling */
  if (!PROP_IS_OTHER(prop->type)) {
    attr = object_find_attribute(obj_node, prop->name);
    if (!attr) {
      switch(prop->type) {
      case PROP_TYPE_CONNPOINT_LINE:
        PROP_VALUE_CONNPOINT_LINE(*prop) = 1;
        break;
      case PROP_TYPE_BUTTON:
        break;
      default:
        g_warning("Could not find attribute %s", prop->name);
      }
      return;
    }
    data = attribute_first_data(attr);
    if (!data) {
      g_warning("Attribute %s contains no data", prop->name);
      return;
    }
  }

  if ((!attr) || (!data)) {
    g_warning("No attribute %s or no data in this attribute",prop->name);
    return;
  }
  
  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("Can't load invalid");
    break;
  case PROP_TYPE_ENDPOINTS:
    data_point(data, &PROP_VALUE_ENDPOINTS(*prop).endpoints[0]);
    data = data_next(data);
    data_point(data, &PROP_VALUE_ENDPOINTS(*prop).endpoints[1]);
    break;
  case PROP_TYPE_CHAR:
    str = data_string(data);
    if (str && str[0]) {
#ifdef UNICODE_WORK_IN_PROGRESS
      uni_get_utf8(str,&uc);
      PROP_VALUE_CHAR(*prop) = uc;
#else
      PROP_VALUE_CHAR(*prop) = str[0];
#endif
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
  case PROP_TYPE_MULTISTRING:
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*prop));
    PROP_VALUE_STRING(*prop) = data_string(data);
    break;
  case PROP_TYPE_NOTEBOOK_BEGIN:
  case PROP_TYPE_NOTEBOOK_END:
  case PROP_TYPE_NOTEBOOK_PAGE:
  case PROP_TYPE_MULTICOL_BEGIN:
  case PROP_TYPE_MULTICOL_END:
  case PROP_TYPE_MULTICOL_COLUMN:
  case PROP_TYPE_FRAME_BEGIN:
  case PROP_TYPE_FRAME_END:
    break;
  case PROP_TYPE_STATIC:
  case PROP_TYPE_BUTTON:
    break;
  case PROP_TYPE_POINT:
    data_point(data, &PROP_VALUE_POINT(*prop));
    break;
  case PROP_TYPE_TEXT:
    g_free(PROP_VALUE_TEXT(*prop).string);
    text = data_text(data);
    text_get_attributes(text,&PROP_VALUE_TEXT(*prop).attr);
    PROP_VALUE_TEXT(*prop).string = text_get_string_copy(text);
    text_destroy(text);
    PROP_VALUE_TEXT(*prop).enabled = TRUE;
    break;
  case PROP_TYPE_INTARRAY:
    g_free(PROP_VALUE_INTARRAY(*prop).vals);
    PROP_VALUE_INTARRAY(*prop).nvals = attribute_num_data(attr);
    PROP_VALUE_INTARRAY(*prop).vals = g_new0(gint,
                                             PROP_VALUE_INTARRAY(*prop).nvals);
    for (i = 0; i < PROP_VALUE_INTARRAY(*prop).nvals; i++) {
      PROP_VALUE_INTARRAY(*prop).vals[i] = data_int(data);
      data = data_next(data);
    }
    break;
  case PROP_TYPE_ENUMARRAY:
    g_free(PROP_VALUE_INTARRAY(*prop).vals);
    PROP_VALUE_INTARRAY(*prop).nvals = attribute_num_data(attr);
    PROP_VALUE_INTARRAY(*prop).vals = g_new0(gint,
                                             PROP_VALUE_INTARRAY(*prop).nvals);
    for (i = 0; i < PROP_VALUE_INTARRAY(*prop).nvals; i++) {
      PROP_VALUE_INTARRAY(*prop).vals[i] = data_enum(data);
      data = data_next(data);
    }
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
  case PROP_TYPE_BEZPOINT:
  case PROP_TYPE_BEZPOINTARRAY:
    g_warning("haven't written code for this -- must fix");
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
    break;
  case PROP_TYPE_FILE:
    g_free(PROP_VALUE_FILE(*prop));
    PROP_VALUE_FILE(*prop) = data_string(data);
    break;
  case PROP_TYPE_CONNPOINT_LINE:
    PROP_VALUE_CONNPOINT_LINE(*prop) = data_int(data);
    break;
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
  AttributeNode attr = NULL;
  gchar buf[32], *str;
  guint i;
  Text *text;

  g_return_if_fail(prop != NULL);

  if (!PROP_IS_OTHER(prop->type))
    attr = new_attribute(obj_node, prop->name);

  if (!attr) {
    g_warning("Can't save in a null attribute for property %s",prop->name);
    return;
  }

  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("Can't save invalid");
    break;
  case PROP_TYPE_ENDPOINTS:
    data_add_point(attr,&PROP_VALUE_ENDPOINTS(*prop).endpoints[0]);
    data_add_point(attr,&PROP_VALUE_ENDPOINTS(*prop).endpoints[1]);
    break;
  case PROP_TYPE_CHAR:
#ifdef UNICODE_WORK_IN_PROGRESS
    data_add_string(charconv_unichar_to_utf8(PROP_VALUE_CHAR(*prop)));
#else
    buf[0] = PROP_VALUE_CHAR(*prop);
    buf[1] = '\0';
    data_add_string(attr, buf);
#endif
    break;
  case PROP_TYPE_BOOL:
    data_add_boolean(attr, PROP_VALUE_BOOL(*prop));
    break;
  case PROP_TYPE_TEXT:
    text = new_text(PROP_VALUE_TEXT(*prop).string,
                    PROP_VALUE_TEXT(*prop).attr.font,
                    PROP_VALUE_TEXT(*prop).attr.height,
                    &PROP_VALUE_TEXT(*prop).attr.position,
                    &PROP_VALUE_TEXT(*prop).attr.color,
                    PROP_VALUE_TEXT(*prop).attr.alignment);
    data_add_text(attr,text);
    text_destroy(text);
    PROP_VALUE_TEXT(*prop).enabled = TRUE;
    break;
  case PROP_TYPE_INT:
    data_add_int(attr, PROP_VALUE_INT(*prop));
    break;
  case PROP_TYPE_CONNPOINT_LINE:
    data_add_int(attr,PROP_VALUE_CONNPOINT_LINE(*prop));
    break;
  case PROP_TYPE_ENUM:
    data_add_enum(attr, PROP_VALUE_ENUM(*prop));
    break;
  case PROP_TYPE_REAL:
    data_add_real(attr, PROP_VALUE_REAL(*prop));
    break;
  case PROP_TYPE_STATIC:
  case PROP_TYPE_BUTTON:
  case PROP_TYPE_NOTEBOOK_BEGIN:
  case PROP_TYPE_NOTEBOOK_END:
  case PROP_TYPE_NOTEBOOK_PAGE:
  case PROP_TYPE_MULTICOL_BEGIN:
  case PROP_TYPE_MULTICOL_END:
  case PROP_TYPE_MULTICOL_COLUMN:
  case PROP_TYPE_FRAME_BEGIN:
  case PROP_TYPE_FRAME_END:
    break;
  case PROP_TYPE_MULTISTRING:
  case PROP_TYPE_STRING:
    data_add_string(attr, PROP_VALUE_STRING(*prop));
    break;
  case PROP_TYPE_POINT:
    data_add_point(attr, &PROP_VALUE_POINT(*prop));
    break;
  case PROP_TYPE_ENUMARRAY:
    for (i = 0; i < PROP_VALUE_INTARRAY(*prop).nvals; i++)
      data_add_enum(attr, PROP_VALUE_INTARRAY(*prop).vals[i]);
    break;
  case PROP_TYPE_INTARRAY:
    for (i = 0; i < PROP_VALUE_INTARRAY(*prop).nvals; i++)
      data_add_int(attr, PROP_VALUE_INTARRAY(*prop).vals[i]);
    break;
  case PROP_TYPE_POINTARRAY:
    for (i = 0; i < PROP_VALUE_POINTARRAY(*prop).npts; i++)
      data_add_point(attr, &PROP_VALUE_POINTARRAY(*prop).pts[i]);
    break;
  case PROP_TYPE_BEZPOINT:
  case PROP_TYPE_BEZPOINTARRAY:
    g_warning("haven't written code for this -- must fix");
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
  case PROP_TYPE_FILE:
    data_add_string(attr, PROP_VALUE_FILE(*prop));
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
      PROP_VALUE_CHAR(props[i]) = struct_member(obj,offsets[j].offset,unichar);
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
    case PROP_TYPE_CONNPOINT_LINE:
      PROP_VALUE_CONNPOINT_LINE(props[i]) = 
        struct_member(obj,offsets[j].offset, ConnPointLine *)->num_connections;
      break;
    case PROP_TYPE_REAL:
      PROP_VALUE_REAL(props[i]) =
	struct_member(obj, offsets[j].offset, real);
      break;
    case PROP_TYPE_STATIC:  
    case PROP_TYPE_BUTTON:
    case PROP_TYPE_NOTEBOOK_BEGIN:
    case PROP_TYPE_NOTEBOOK_END:
    case PROP_TYPE_NOTEBOOK_PAGE:
    case PROP_TYPE_MULTICOL_BEGIN:
    case PROP_TYPE_MULTICOL_END:
    case PROP_TYPE_MULTICOL_COLUMN:
    case PROP_TYPE_FRAME_BEGIN:
    case PROP_TYPE_FRAME_END:
      break;
    case PROP_TYPE_MULTISTRING:
    case PROP_TYPE_STRING:
      g_free(PROP_VALUE_STRING(props[i]));
      PROP_VALUE_STRING(props[i]) =
	g_strdup(struct_member(obj, offsets[j].offset, gchar *));
      break;
    case PROP_TYPE_POINT:
      PROP_VALUE_POINT(props[i]) =
	struct_member(obj, offsets[j].offset, Point);
      break;
    case PROP_TYPE_ENDPOINTS:
      memcpy(&PROP_VALUE_ENDPOINTS(props[i]),
             &struct_member(obj,offsets[j].offset,Point),
             sizeof(PROP_VALUE_ENDPOINTS(props[i])));
      break;
    case PROP_TYPE_INTARRAY:
    case PROP_TYPE_ENUMARRAY:
      g_free(PROP_VALUE_INTARRAY(props[i]).vals);
      PROP_VALUE_INTARRAY(props[i]).nvals =
	struct_member(obj, offsets[j].offset2, gint);
      PROP_VALUE_INTARRAY(props[i]).vals =
	g_memdup(struct_member(obj, offsets[j].offset, gint *),
		 sizeof(gint) * PROP_VALUE_INTARRAY(props[i]).nvals);
      break;
    case PROP_TYPE_POINTARRAY:
      g_free(PROP_VALUE_POINTARRAY(props[i]).pts);
      PROP_VALUE_POINTARRAY(props[i]).npts =
	struct_member(obj, offsets[j].offset2, gint);
      PROP_VALUE_POINTARRAY(props[i]).pts =
	g_memdup(struct_member(obj, offsets[j].offset, Point *),
		 sizeof(Point) * PROP_VALUE_POINTARRAY(props[i]).npts);
      break;
    case PROP_TYPE_BEZPOINT:
      PROP_VALUE_BEZPOINT(props[i]) =
	struct_member(obj, offsets[j].offset, BezPoint);
      break;
    case PROP_TYPE_BEZPOINTARRAY:
      g_free(PROP_VALUE_BEZPOINTARRAY(props[i]).pts);
      PROP_VALUE_BEZPOINTARRAY(props[i]).npts =
	struct_member(obj, offsets[j].offset2, gint);
      PROP_VALUE_BEZPOINTARRAY(props[i]).pts =
	g_memdup(struct_member(obj, offsets[j].offset, BezPoint *),
		 sizeof(BezPoint) * PROP_VALUE_BEZPOINTARRAY(props[i]).npts);
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
	struct_member(obj, offsets[j].offset, DiaFont *);
      break;
    case PROP_TYPE_TEXT:
      g_free(PROP_VALUE_TEXT(props[i]).string);
      PROP_VALUE_TEXT(props[i]).string = 
        text_get_string_copy(struct_member(obj,offsets[j].offset,Text *));
      text_get_attributes(struct_member(obj,offsets[j].offset,Text *),
                          &PROP_VALUE_TEXT(props[i]).attr);
      PROP_VALUE_TEXT(props[i]).enabled = TRUE;
      break;
    case PROP_TYPE_FILE:
      g_free(PROP_VALUE_FILE(props[i]));
      PROP_VALUE_FILE(props[i]) =
	g_strdup(struct_member(obj, offsets[j].offset, gchar *));
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
      struct_member(obj, offsets[j].offset,unichar)=PROP_VALUE_CHAR(props[i]);
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
    case PROP_TYPE_CONNPOINT_LINE:
      connpointline_adjust_count(struct_member(obj,offsets[j].offset, 
                                               ConnPointLine *),
                                 PROP_VALUE_CONNPOINT_LINE(props[i]),
                                 &struct_member(obj,offsets[j].offset, 
                                               ConnPointLine *)->end);
      break;
    case PROP_TYPE_REAL:
      struct_member(obj, offsets[j].offset, real) =
	PROP_VALUE_REAL(props[i]);
      break;
    case PROP_TYPE_STATIC:
    case PROP_TYPE_BUTTON:
    case PROP_TYPE_NOTEBOOK_BEGIN:
    case PROP_TYPE_NOTEBOOK_END:
    case PROP_TYPE_NOTEBOOK_PAGE:
    case PROP_TYPE_MULTICOL_BEGIN:
    case PROP_TYPE_MULTICOL_END:
    case PROP_TYPE_MULTICOL_COLUMN:
    case PROP_TYPE_FRAME_BEGIN:
    case PROP_TYPE_FRAME_END:
      break;
    case PROP_TYPE_MULTISTRING:
    case PROP_TYPE_STRING:
      g_free(struct_member(obj, offsets[j].offset, gchar *));
      struct_member(obj, offsets[j].offset, gchar *) =
	g_strdup(PROP_VALUE_STRING(props[i]));
      break;
    case PROP_TYPE_POINT:
      struct_member(obj, offsets[j].offset, Point) =
	PROP_VALUE_POINT(props[i]);
      break;
    case PROP_TYPE_ENDPOINTS:
      memcpy(&struct_member(obj,offsets[j].offset,Point),
             &PROP_VALUE_ENDPOINTS(props[i]),
             sizeof(PROP_VALUE_ENDPOINTS(props[i])));
      break;
    case PROP_TYPE_ENUMARRAY:
    case PROP_TYPE_INTARRAY:
      g_free(struct_member(obj, offsets[j].offset, gint *));
      struct_member(obj, offsets[j].offset, gint *) =
	g_memdup(PROP_VALUE_INTARRAY(props[i]).vals,
		 sizeof(gint) * PROP_VALUE_INTARRAY(props[i]).nvals);
      struct_member(obj, offsets[j].offset2, gint) =
	PROP_VALUE_INTARRAY(props[i]).nvals;
      break;
    case PROP_TYPE_POINTARRAY:
      g_free(struct_member(obj, offsets[j].offset, Point *));
      struct_member(obj, offsets[j].offset, Point *) =
	g_memdup(PROP_VALUE_POINTARRAY(props[i]).pts,
		 sizeof(Point) * PROP_VALUE_POINTARRAY(props[i]).npts);
      struct_member(obj, offsets[j].offset2, gint) =
	PROP_VALUE_POINTARRAY(props[i]).npts;
      break;
    case PROP_TYPE_BEZPOINT:
      struct_member(obj, offsets[j].offset, BezPoint) =
	PROP_VALUE_BEZPOINT(props[i]);
      break;
    case PROP_TYPE_BEZPOINTARRAY:
      g_free(struct_member(obj, offsets[j].offset, BezPoint *));
      struct_member(obj, offsets[j].offset, BezPoint *) =
	g_memdup(PROP_VALUE_BEZPOINTARRAY(props[i]).pts,
		 sizeof(BezPoint) * PROP_VALUE_BEZPOINTARRAY(props[i]).npts);
      struct_member(obj, offsets[j].offset2, gint) =
	PROP_VALUE_BEZPOINTARRAY(props[i]).npts;
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
      struct_member(obj, offsets[j].offset, DiaFont *) =
	PROP_VALUE_FONT(props[i]);
      break;
    case PROP_TYPE_FILE:
      g_free(struct_member(obj, offsets[j].offset, gchar *));
      struct_member(obj, offsets[j].offset, gchar *) =
	g_strdup(PROP_VALUE_FILE(props[i]));
      break;
    case PROP_TYPE_TEXT:
      text_set_string(struct_member(obj,offsets[j].offset,Text *),
                      PROP_VALUE_TEXT(props[i]).string);
      text_set_attributes(struct_member(obj,offsets[j].offset,Text *),
                          &PROP_VALUE_TEXT(props[i]).attr);
      /* we might be enabled, we might be not. */
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

static const gchar *prop_dialogdata_key = "object-props:dialogdata";

static void
object_props_dialog_destroy(GtkWidget *dialog)
{
  PropDialogData *ppd = gtk_object_get_data(GTK_OBJECT(dialog),
                                            prop_dialogdata_key);
  Property *props = ppd->props;
  guint nprops = ppd->nprops;
  GtkWidget **widgets = ppd->widgets;

  prop_list_free(props, nprops);
  g_free(widgets);
  if (ppd->obj_copy) ppd->obj_copy->ops->destroy(ppd->obj_copy);
  g_free(ppd);
}

GtkWidget *
object_create_props_dialog(Object *obj)
{
  const PropDescription *pdesc;
  Property *props;
  guint nprops = 0, i, j;
  GtkWidget **widgets;
  GtkWidget *table, *label;
  GtkWidget *mainvbox;

  static GtkWidget **curcontainer = NULL; 
  static gint cc_allocated = 0; 
  int nestlev = -1;
  gboolean haspage = FALSE, hascolumn = FALSE;

  PropDialogData *ppd = g_new0(PropDialogData,1);
  Object *obj_copy = NULL;

  g_return_val_if_fail(obj->ops->describe_props != NULL, NULL);
  g_return_val_if_fail(obj->ops->get_props != NULL, NULL);
  g_return_val_if_fail(obj->ops->set_props != NULL, NULL);

  pdesc = obj->ops->describe_props(obj);
  g_return_val_if_fail(pdesc != NULL, NULL);

  for (i = 0; pdesc[i].name != NULL; i++)
    if ((pdesc[i].flags & PROP_FLAG_VISIBLE) != 0)
      nprops++;

  props = g_new0(Property, nprops);
  widgets = g_new0(GtkWidget *, nprops);

  /* get the current values of all visible properties */
  for (i = 0, j = 0; pdesc[i].name != NULL; i++)
    if ((pdesc[i].flags & PROP_FLAG_VISIBLE) != 0) {
      props[j].name       = pdesc[i].name;
      props[j].type       = pdesc[i].type;
      props[j].extra_data = pdesc[i].extra_data;
      props[j].event_handler = pdesc[i].event_handler;
      props[j].descr      = &pdesc[i];
      props[j].self.dialog = ppd;
      props[j].self.index = j;
      if ((pdesc[j].event_handler) && (!obj_copy)) {
        ppd->obj_copy = obj_copy = obj->ops->copy(obj);
      }
      j++;
    }
  obj->ops->get_props(obj, props, nprops);

  mainvbox = gtk_vbox_new(FALSE,1);
  table = NULL;
  if (!curcontainer) {
    cc_allocated = 4;
    curcontainer = g_new0(GtkWidget *,cc_allocated);
  } 
  curcontainer[++nestlev] = mainvbox;

  ppd->props = props;
  ppd->nprops = nprops;
  ppd->dialog = mainvbox;
  ppd->widgets = widgets;

  /* construct the widgets table */
  for (i = 0, j = 0; pdesc[i].name != NULL; i++) {
    while ((!curcontainer) || (nestlev+3 >= cc_allocated)) {
      cc_allocated *= 2;
      curcontainer = g_renew(GtkWidget *, curcontainer , cc_allocated); 
    }

    if ((pdesc[i].flags & PROP_FLAG_VISIBLE) != 0) {
      switch(props[j].type) {
      case PROP_TYPE_NOTEBOOK_BEGIN:
        {
          GtkWidget *notebook = gtk_notebook_new();
          gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook),GTK_POS_TOP);
          gtk_container_set_border_width (GTK_CONTAINER(notebook), 1);
          gtk_widget_show(notebook);
          gtk_box_pack_end_defaults(GTK_BOX(curcontainer[nestlev]),notebook);
          curcontainer[++nestlev] = notebook;
          table = NULL;
          haspage = FALSE;
          /* note: there must be at least one page, immediately after this. */
          g_assert(nestlev < cc_allocated);
        } break;
      case PROP_TYPE_NOTEBOOK_END:
        {
          if (haspage) nestlev--;
          nestlev--;
          table = NULL;
          haspage = FALSE;
          g_assert(nestlev >= 0);
        } break;
      case PROP_TYPE_NOTEBOOK_PAGE: 
        {
          /* this >must< happen inside a notebook */         
          GtkWidget *page = gtk_vbox_new(FALSE,1);
          GtkWidget *pagelabel = gtk_label_new(pdesc[i].description);
          gtk_container_set_border_width(GTK_CONTAINER(page),2);
          gtk_widget_show_all(page);
          gtk_widget_show_all(pagelabel);
          if (haspage) nestlev--;
          gtk_notebook_append_page(GTK_NOTEBOOK(curcontainer[nestlev]),
                                   page,pagelabel);
          haspage = TRUE;
          curcontainer[++nestlev] = page;
          table = NULL;
          
          g_assert(nestlev < cc_allocated);
        } break;
      case PROP_TYPE_MULTICOL_BEGIN:
        {
          GtkWidget *multicol = gtk_hbox_new(FALSE,1);
          gtk_widget_show(multicol);
          gtk_box_pack_end_defaults(GTK_BOX(curcontainer[nestlev]),multicol);
          gtk_container_set_border_width (GTK_CONTAINER(multicol), 2);
          curcontainer[++nestlev] = multicol;
          table = NULL;
          hascolumn = FALSE;
          /* note: there should be at least one column, 
             immediately after this. */
          g_assert(nestlev < cc_allocated);          
        } break;
      case PROP_TYPE_MULTICOL_END:
        {
          if (haspage) nestlev--;
          nestlev--;
          table = NULL;
          hascolumn = FALSE; 
          g_assert(nestlev >= 0);
        } break;
      case PROP_TYPE_MULTICOL_COLUMN:
        {
          /* this >must< happen inside a multicolumn */         
          GtkWidget *column = gtk_vbox_new(FALSE,1);
          gtk_container_set_border_width(GTK_CONTAINER(column),2);
          gtk_widget_show_all(column);
          if (hascolumn) nestlev--;
          gtk_box_pack_end_defaults(GTK_BOX(curcontainer[nestlev]),column);
          hascolumn = TRUE;
          curcontainer[++nestlev] = column;
          table = NULL;
          
          g_assert(nestlev < cc_allocated);
        } break;
      case PROP_TYPE_FRAME_BEGIN:
        {
          GtkWidget *frame = gtk_frame_new(pdesc[i].description);
          GtkWidget *vbox = gtk_vbox_new(FALSE,2);

          gtk_widget_show_all(frame);
          gtk_container_set_border_width (GTK_CONTAINER(frame), 2);
          gtk_box_pack_end_defaults(GTK_BOX(curcontainer[nestlev]),frame);
          gtk_widget_show(vbox);
          gtk_container_add (GTK_CONTAINER (frame), vbox);
          curcontainer[++nestlev] = frame;
          curcontainer[++nestlev] = vbox;
          
          table = NULL;
          g_assert(nestlev < cc_allocated);          
        } break;
      case PROP_TYPE_FRAME_END:
        {
          nestlev -= 2;
          table = NULL;
          g_assert(nestlev >= 0);
        } break;
      default:
        widgets[j] = prop_get_widget(&props[j]); 
        prop_reset_widget(&props[j],widgets[j]);

        label = gtk_label_new(_(pdesc[i].description));
        gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

        if (!table) {
          table = gtk_table_new(1, 2, FALSE);
          gtk_table_set_row_spacings(GTK_TABLE(table), 2);
          gtk_table_set_col_spacings(GTK_TABLE(table), 5);
          gtk_widget_show(table);
          gtk_box_pack_end_defaults(GTK_BOX(curcontainer[nestlev]),table);

        }
        gtk_table_attach(GTK_TABLE(table), label, 0,1, j,j+1,
                         GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
        gtk_table_attach(GTK_TABLE(table), widgets[j], 1,2, j,j+1,
                         GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

        gtk_widget_show(label);
        gtk_widget_show(widgets[j]);
        break;
      }
      j++;
    }
  }
  
  gtk_object_set_data(GTK_OBJECT(mainvbox), prop_dialogdata_key, ppd);

  gtk_signal_connect(GTK_OBJECT(mainvbox), "destroy",
		     GTK_SIGNAL_FUNC(object_props_dialog_destroy), NULL);

  return mainvbox;
}

ObjectChange *
object_apply_props_from_dialog(Object *obj, GtkWidget *dialog)
{
  PropDialogData *ppd = gtk_object_get_data(GTK_OBJECT(dialog),
                                            prop_dialogdata_key);
  Property *props = ppd->props;
  guint nprops = ppd->nprops;
  GtkWidget **widgets = ppd->widgets;

  guint i;

  for (i = 0; i < nprops; i++)
    prop_set_from_widget(&props[i], widgets[i]);

  return object_apply_props(obj, props, nprops);
}

/* --------------------------------------- */

void 
object_copy_props(Object *dest, Object *src)
{
  const PropDescription *pdesc;
  Property *props;
  guint nprops;

  g_return_if_fail(src != NULL);
  g_return_if_fail(dest != NULL);
  g_return_if_fail(strcmp(src->type->name,dest->type->name)==0);
  g_return_if_fail(src->ops == dest->ops);

  if (src->ops->describe_props == NULL ||
      src->ops->set_props == NULL) {
    g_warning("No describe_props or set_props!");
    return;
  }
  pdesc = src->ops->describe_props(src);
  if (pdesc == NULL) {
    g_warning("No properties!");
    return;
  }
  props = prop_list_from_nonmatching_descs(pdesc, PROP_FLAG_DONT_SAVE,&nprops);
  
  src->ops->get_props(src, props, nprops);
  dest->ops->set_props(dest, props, nprops);

  prop_list_free(props, nprops);  
}

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

#ifdef G_OS_WIN32
/* moved to properties.h ... */
#else
/* standard property extra data members */
PropNumData prop_std_line_width_data = { 0.0, 10.0, 0.01 };
PropNumData prop_std_text_height_data = { 0.1, 10.0, 0.1 };
PropEnumData prop_std_text_align_data[] = {
  { N_("Left"), ALIGN_LEFT },
  { N_("Center"), ALIGN_CENTER },
  { N_("Right"), ALIGN_RIGHT },
  { NULL, 0 }
};
#endif

#ifdef FOR_TRANSLATORS_ONLY
static char *list [] = {
	N_("Line colour"),
	N_("Line style"),
	N_("Fill colour"),
	N_("Draw background"),
	N_("Start arrow"),
	N_("End arrow"),
	N_("Text"),
	N_("Text alignment"),
	N_("Font"),
	N_("Font size"),
	N_("Text colour")
};
#endif
