/* 
 * Various laziness support routines to reduce boilerplate code when 
 * dealing with properties. 
 * Copyright(C) 2000 Cyrille Chepelov
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

#ifndef __LAZYPROPS_H
#define __LAZYPROPS_H

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "geometry.h"
#include "font.h"
#include "text.h"
#include "dia_xml.h"
#include "widgets.h"
#include "message.h"

/* I'm sorry, this header file turned out to be much uglier than I
   had expected. */


/* attribute load/save routines. I wish I had some templates. */

#define MAKE_SAVE_FOO(typename,type) \
  inline static void  \
  save_##typename(ObjectNode *obj_node, const gchar *attrname, type value) { \
    data_add_##typename(new_attribute(obj_node,attrname),value); \
  } 

MAKE_SAVE_FOO(int,int)
MAKE_SAVE_FOO(enum,int)
MAKE_SAVE_FOO(real,real)
MAKE_SAVE_FOO(boolean,int)
MAKE_SAVE_FOO(color,Color *)
MAKE_SAVE_FOO(point,Point *)
MAKE_SAVE_FOO(rectangle,Rectangle *)
MAKE_SAVE_FOO(string,char *)
MAKE_SAVE_FOO(font,Font *)
MAKE_SAVE_FOO(text,Text *)
     /* composites are to be handled the hard way */
#undef MAKE_SAVE_FOO

/* The load functions are a bit heavy for inlining 
(processor cache pollution ; code size.) */

#ifndef __LAZYPROPS_C
#define MAKE_LOAD_FOO(typename,type) \
  type load_##typename(ObjectNode *obj_node, const gchar *attrname, \
		  type defaultvalue);

#define MAKE_LOAD_FOO2(typename,type) \
  void  load_##typename(ObjectNode *obj_node, const gchar *attrname, \
			       type *value, type *defaultvalue);
#else
#define MAKE_LOAD_FOO(typename,type) \
  type load_##typename(ObjectNode *obj_node, const gchar *attrname, \
		  type defaultvalue);  \
  type  \
  load_##typename(ObjectNode *obj_node, const gchar *attrname, \
		  type defaultvalue) { \
    AttributeNode attr; \
    attr = object_find_attribute(obj_node,attrname); \
    if (attr != NULL)  \
       return data_##typename( attribute_first_data(attr) ); \
    else \
       return defaultvalue; \
  }
#define MAKE_LOAD_FOO2(typename,type) \
  void  load_##typename(ObjectNode *obj_node, const gchar *attrname, \
			       type *value, type *defaultvalue); \
  void  \
  load_##typename(ObjectNode *obj_node, const gchar *attrname, type *value, \
		  type *defaultvalue) { \
    AttributeNode attr; \
    attr = object_find_attribute(obj_node,attrname); \
    if (attr != NULL)  \
       data_##typename( attribute_first_data(attr), value); \
    else \
       if (defaultvalue) *value = *defaultvalue; \
  }
#endif __LAZYPROPS_C

MAKE_LOAD_FOO(int,int)
MAKE_LOAD_FOO(enum,int)
MAKE_LOAD_FOO(real,real)
MAKE_LOAD_FOO(boolean,int)
MAKE_LOAD_FOO2(color,Color)
MAKE_LOAD_FOO2(point,Point)
MAKE_LOAD_FOO2(rectangle,Rectangle)
MAKE_LOAD_FOO(string,char *)
MAKE_LOAD_FOO(font,Font *)
MAKE_LOAD_FOO(text,Text *)

#undef MAKE_LOAD_FOO
#undef MAKE_LOAD_FOO2

/* More integrated load and save macros. The older one are still useful
 when it comes to fine-tuning (esp. wrt to field names in the saved file) */
#define SAVE_INT(obj_node,object,field) \
   save_int((obj_node),#field,(object)->field);
#define LOAD_INT(obj_node,object,defaults,field) \
   (object)->field = load_int(obj_node,#field,(defaults)->field);
#define SAVE_REAL(obj_node,object,field) \
   save_real((obj_node),#field,(object)->field);
#define LOAD_REAL(obj_node,object,defaults,field) \
   (object)->field = load_real(obj_node,#field,(defaults)->field);
#define SAVE_BOOL(obj_node,object,field) \
   save_boolean((obj_node),#field,(object)->field);
#define LOAD_BOOL(obj_node,object,defaults,field) \
   (object)->field = load_boolean(obj_node,#field,(defaults)->field);
#define SAVE_COLOR(obj_node,object,field) \
   save_color((obj_node),#field,&(object)->field); 
#define LOAD_COLOR(obj_node,object,defaults,field) \
   load_color(obj_node,#field,&(object)->field,&(defaults)->field);
#define SAVE_POINT(obj_node,object,field) \
   save_point((obj_node),#field,&(object)->field); 
#define LOAD_POINT(obj_node,object,defaults,field) \
   load_point(obj_node,#field,&(object)->field,&(defaults)->field);
#define SAVE_RECT(obj_node,object,field) \
   save_rectangle((obj_node),#field,&(object)->field); 
#define LOAD_RECT(obj_node,object,defaults,field) \
   load_rectangle(obj_node,#field,&(object)->field,&(defaults)->field);
#define SAVE_STRING(obj_node,object,field) \
   save_string((obj_node),#field,(object)->field); 
#define LOAD_STRING(obj_node,object,defaults,field) \
   (object)->field = load_string(obj_node,#field,NULL); \
   if (!(object)->field) (object)->field = g_strdup((defaults)->field);
#define SAVE_FONT(obj_node,object,field) \
   save_font((obj_node),#field,(object)->field); 
#define LOAD_FONT(obj_node,object,defaults,field) \
   (object)->field = load_font(obj_node,#field,(defaults)->field);
#define SAVE_FONTHEIGHT(obj_node,object,field) \
   save_real((obj_node),#field,(object)->field);
#define LOAD_FONTHEIGHT(obj_node,object,defaults,field) \
   (object)->field = load_real(obj_node,#field,(defaults)->field);
#define SAVE_TEXT(obj_node,object,field) \
   save_text((obj_node),#field,(object)->field);
#define LOAD_TEXT(obj_node,object,defaults,field) \
   (object)->text = load_text(obj_node,#field,NULL);


/* Attribute editing dialog ("Properties.*" stuff) */
/* Goal : no, or almost no gtk garb^Wstuff in an object's code. Or at worst, 
   really nicely hidden. */

/* contract : 
    - object type Foo has a single object properties dialog (FooProperties), 
    which will be pointed to by the first parameter "PropertiesDialog" 
    (or "propdlg", in short) of all the following macros.
    - "propdlg" has a single AttributeDialog field, called "dialog", which
    in fact is a gtk VBox.
    - "propdlg" has a single Foo * pointer, called "parent", which traces back
    to the Foo object structure being edited, or NULL.

    - All of (Foo *)'s fields which are to be edited by the property dialog
    has a matching "propdlg" field of a related type.
    - Description strings are to be passed with _( ) around it (i18n). 

    - all type-specific macros take at least the following arguments : 
       * propdlg   the name of the propdlg variable
       * field     the name of the field being changed (in both propdlg and
                                                        propdlg->parent)
       * desc      an i18n-able description string. 

   For the text font, font height and color attributes, the fields in the 
   dialog structure must respectively be called foo_font, foo_fontheight and
   foo_color (where "foo" is the name of the text field in the parent object).

*/

#define PROPDLG_TYPE GtkWidget *
typedef struct {
  GtkWidget *d;
  GtkWidget *label;
  gboolean ready;
} AttributeDialog; /* This is intended to be opaque */

/* to be called to create a new, empty dialog. */
#define PROPDLG_CREATE(propdlg,parentvalue) \
  if (!propdlg) { \
     (propdlg) = g_malloc0(sizeof(*(propdlg)) ); \
     (propdlg)->dialog.d = gtk_vbox_new(FALSE,5);  \
     gtk_object_ref(GTK_OBJECT((propdlg)->dialog.d)); \
     gtk_object_sink(GTK_OBJECT((propdlg)->dialog.d)); \
     gtk_container_set_border_width(GTK_CONTAINER((propdlg)->dialog.d), 10); \
     /* (propdlg)->dialog.ready = FALSE; */ \
  } \
  (propdlg)->parent = (parentvalue);

/* to be called when the dialog is almost ready to show (bracket the rest with
   PROPDLG_CREATE and PROPDLG_READY) */
#define PROPDLG_READY(propdlg) \
  if (!(propdlg)->dialog.ready) { \
     gtk_widget_show((propdlg)->dialog.d); \
     (propdlg)->dialog.ready = TRUE; \
  }

/* The return type of get_foo_properties() */
#define PROPDLG_RETURN(propdlg) \
    return propdlg->dialog.d;
/* This defines a notebook inside a dialog. */
typedef GtkWidget *AttributeNotebook;
#define PROPDLG_NOTEBOOK_CREATE(propdlg,field) \
   if (!(propdlg)->dialog.ready) {\
      (propdlg)->field = gtk_notebook_new (); \
      gtk_notebook_set_tab_pos (GTK_NOTEBOOK((propdlg)->field), GTK_POS_TOP); \
      gtk_box_pack_start (GTK_BOX ((propdlg)->dialog.d), \
			  (propdlg)->field, TRUE, TRUE, 0); \
      gtk_container_set_border_width (GTK_CONTAINER((propdlg)->field), 10); \
   }
#define PROPDLG_NOTEBOOK_READY(propdlg,field) \
   if (!(propdlg)->dialog.ready) {\
      gtk_widget_show((propdlg)->field); \
   }

/* This defines a notebook page.
   The "field" must be a pointer to an ad-hoc struct similar to the normal
   property dialog structs. */
typedef AttributeDialog AttributePage;
#define PROPDLG_PAGE_CREATE(propdlg,notebook,field,desc) \
   if (!(propdlg)->dialog.ready) { \
     (propdlg)->field = g_malloc0(sizeof(*((propdlg)->field))); \
     (propdlg)->field->dialog.d = gtk_vbox_new(FALSE,5);  \
     /*(propdlg)->field->dialog.ready = FALSE; */ \
     gtk_container_set_border_width(GTK_CONTAINER((propdlg)->field->dialog.d),\
				    10); \
     (propdlg)->field->dialog.label = gtk_label_new((desc));  \
  } \
  (propdlg)->field->parent = (propdlg)->parent;

#define PROPDLG_PAGE_READY(propdlg,notebook,field) \
  if (!(propdlg)->field->dialog.ready) { \
     gtk_widget_show_all((propdlg)->field->dialog.d); \
     gtk_widget_show_all((propdlg)->field->dialog.label); \
     gtk_widget_show((propdlg)->field->dialog.d); \
     gtk_notebook_append_page(GTK_NOTEBOOK((propdlg)->notebook), \
		     (propdlg)->field->dialog.d, \
		     (propdlg)->field->dialog.label); \
     (propdlg)->field->dialog.ready = TRUE; \
  }

/* This defines a real attribute. */
typedef GtkSpinButton *RealAttribute;
RealAttribute __propdlg_build_real(GtkWidget *dialog, 
				   const gchar *desc,
				   gfloat lower,
				   gfloat upper,
				   gfloat step_increment);

#define __PROPDLG_BUILD_REAL(propdlg, field, desc, lower, upper, \
                             step_increment)\
   (propdlg)->field = __propdlg_build_real((propdlg)->dialog.d,(desc), \
                                               (lower),(upper), \
                                               (step_increment)); 
#define __PROPDLG_SHOW_REAL(propdlg,field) \
   gtk_spin_button_set_value((propdlg)->field, \
	                     (propdlg)->parent->field); 
#define PROPDLG_SHOW_REAL(propdlg, field, desc, lower, upper, step_increment)\
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_REAL((propdlg),field,(desc),(lower), \
                         (upper),(step_increment)) ;} \
   __PROPDLG_SHOW_REAL((propdlg),field)

#define PROPDLG_APPLY_REAL(propdlg, field) \
  (propdlg)->parent->field = \
    gtk_spin_button_get_value_as_float((propdlg)->field);

/* This defines an int attribute. */
typedef GtkSpinButton *IntAttribute; 
#define __PROPDLG_BUILD_INT(propdlg, field, desc, lower, upper, step_increment)\
   (propdlg)->field = __propdlg_build_real((propdlg)->dialog.d,(desc), \
                                             (guint)(lower), \
					     (guint)(upper), \
					     (guint)(step_increment));

#define __PROPDLG_SHOW_INT(propdlg, field) \
  gtk_spin_button_set_value((propdlg)->field, \
			    (propdlg)->parent->field); 
#define PROPDLG_SHOW_INT(propdlg, field, desc, lower, upper, step_increment)\
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_INT((propdlg),field,(desc),(lower), \
                         (upper),(step_increment)) ;}\
   __PROPDLG_SHOW_INT((propdlg),field)

#define PROPDLG_APPLY_INT(propdlg, field) \
  (propdlg)->parent->field = \
    gtk_spin_button_get_value_as_int((propdlg)->field);



/* This defines a Boolean attribute : */
typedef GtkToggleButton *BoolAttribute; 
BoolAttribute __propdlg_build_bool(GtkWidget *dialog, 
				   const gchar *desc);
#define __PROPDLG_BUILD_BOOL(propdlg, field, desc)  \
   (propdlg)->field = __propdlg_build_bool((propdlg)->dialog.d,(desc));
#define __PROPDLG_SHOW_BOOL(propdlg,field) \
  gtk_toggle_button_set_active((propdlg)->field, (propdlg)->parent->field);

#define PROPDLG_SHOW_BOOL(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_BOOL((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_BOOL((propdlg),field)
#define PROPDLG_APPLY_BOOL(propdlg,field) \
  (propdlg)->parent->field = (propdlg)->field->active;


/* This defines a Font attribute : */
typedef DiaFontSelector *FontAttribute;
typedef DiaFontSelector *TextFontAttribute;

FontAttribute __propdlg_build_font(GtkWidget *dialog, 
				   const gchar *desc);
#define __PROPDLG_BUILD_FONT(propdlg,field,desc) \
   (propdlg)->field = __propdlg_build_font((propdlg)->dialog.d,(desc));
#define __PROPDLG_BUILD_TEXTFONT(propdlg,field,desc) \
         __PROPDLG_BUILD_FONT((propdlg),field##_font,(desc))

#define __PROPDLG_SHOW_FONT(propdlg,field) \
  dia_font_selector_set_font((propdlg)->field,(propdlg)->parent->field);
#define __PROPDLG_SHOW_TEXTFONT(propdlg,field) \
  dia_font_selector_set_font((propdlg)->field##_font, \
			     (propdlg)->parent->field->font);

#define PROPDLG_SHOW_FONT(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_FONT((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_FONT((propdlg),field)
#define PROPDLG_SHOW_TEXTFONT(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_TEXTFONT((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_TEXTFONT((propdlg),field)

#define PROPDLG_APPLY_FONT(propdlg,field) \
  (propdlg)->parent->field = dia_font_selector_get_font((propdlg)->field);
#define PROPDLG_APPLY_TEXTFONT(propdlg,field) \
  text_set_font((propdlg)->parent->field, \
                dia_font_selector_get_font((propdlg)->field##_font));

/* This defines a Font Height attribute : */
typedef RealAttribute FontHeightAttribute;
typedef RealAttribute TextFontHeightAttribute;

#define __PROPDLG_BUILD_FONTHEIGHT(propdlg,field,desc) \
  __PROPDLG_BUILD_REAL((propdlg),field,(desc),0.0,10.0,0.1)
#define __PROPDLG_BUILD_TEXTFONTHEIGHT(propdlg,field,desc) \
  __PROPDLG_BUILD_REAL((propdlg),field##_fontheight,(desc),0.0,10.0,0.1)
#define __PROPDLG_SHOW_FONTHEIGHT(propdlg,field) \
  __PROPDLG_SHOW_REAL((propdlg),field)  
#define __PROPDLG_SHOW_TEXTFONTHEIGHT(propdlg,field) \
  gtk_spin_button_set_value((propdlg)->field##_fontheight, \
			    (propdlg)->parent->field->height);

#define PROPDLG_SHOW_FONTHEIGHT(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_FONTHEIGHT((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_FONTHEIGHT((propdlg),field)
#define PROPDLG_SHOW_TEXTFONTHEIGHT(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_TEXTFONTHEIGHT((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_TEXTFONTHEIGHT((propdlg),field)

#define PROPDLG_APPLY_FONTHEIGHT(propdlg,field) \
  PROPDLG_APPLY_REAL((propdlg),field)  
#define PROPDLG_APPLY_TEXTFONTHEIGHT(propdlg,field) \
  text_set_height((propdlg)->parent->field, \
		  gtk_spin_button_get_value_as_float( \
                              (propdlg)->field##_fontheight));

/* This defines a Color attribute : */
typedef DiaColorSelector *ColorAttribute;
typedef DiaColorSelector *TextColorAttribute; 
ColorAttribute __propdlg_build_color(GtkWidget *dialog,
				     const gchar *desc);
#define __PROPDLG_BUILD_COLOR(propdlg,field,desc) \
   (propdlg)->field = __propdlg_build_color((propdlg)->dialog.d,(desc));

#define __PROPDLG_BUILD_TEXTCOLOR(propdlg,field,desc) \
    __PROPDLG_BUILD_COLOR((propdlg),field##_color,(desc))

#define __PROPDLG_SHOW_COLOR(propdlg,field) \
  dia_color_selector_set_color((propdlg)->field, \
                              &((propdlg)->parent->field));
#define __PROPDLG_SHOW_TEXTCOLOR(propdlg,field) \
  dia_color_selector_set_color((propdlg)->field##_color, \
                               &((propdlg)->parent->field->color));

#define PROPDLG_SHOW_COLOR(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) \
      __PROPDLG_BUILD_COLOR((propdlg),field,(desc)) \
   __PROPDLG_SHOW_COLOR((propdlg),field)
#define PROPDLG_SHOW_TEXTCOLOR(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_TEXTCOLOR((propdlg),field,(desc)) ;} \
   __PROPDLG_SHOW_TEXTCOLOR((propdlg),field)

#define PROPDLG_APPLY_COLOR(propdlg,field) \
  dia_color_selector_get_color((propdlg)->field, \
                              &((propdlg)->parent->field));
#define PROPDLG_APPLY_TEXTCOLOR(propdlg,field) \
  G_STMT_START { \
    Color col; \
    dia_color_selector_get_color((propdlg)->field##_color, &col); \
    text_set_color((propdlg)->parent->field,&col); \
  } G_STMT_END


/* This defines a Line Width attribute : */
typedef RealAttribute LineWidthAttribute;

#define __PROPDLG_BUILD_LINEWIDTH(propdlg,field,desc) \
   __PROPDLG_BUILD_REAL((propdlg),field,(desc),0.00,10.00,0.01)
#define __PROPDLG_SHOW_LINEWIDTH(propdlg,field) \
   __PROPDLG_SHOW_REAL((propdlg),field)
#define PROPDLG_SHOW_LINEWIDTH(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_LINEWIDTH((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_LINEWIDTH((propdlg),field)
#define PROPDLG_APPLY_LINEWIDTH(propdlg,field,desc) \
   PROPDLG_APPLY_REAL(propdlg,field)

/* This defines a Line Style attribute. If that field is named "foo", then
 it will demand two fields in the parent object, foo_style and 
 foo_dashlength, of the correct types (LineStyle and real).
 (I'm not really happy about this).*/
typedef DiaLineStyleSelector *LineStyleAttribute;
LineStyleAttribute __propdlg_build_linestyle(GtkWidget *dialog,
					     const gchar *desc);
#define __PROPDLG_BUILD_LINESTYLE(propdlg,field,desc) \
   (propdlg)->field = __propdlg_build_linestyle((propdlg)->dialog.d,(desc));

#define __PROPDLG_SHOW_LINESTYLE(propdlg,field) \
  dia_line_style_selector_set_linestyle((propdlg)->field, \
                         (propdlg)->parent->field##_style, \
                         (propdlg)->parent->field##_dashlength);
#define PROPDLG_SHOW_LINESTYLE(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_LINESTYLE((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_LINESTYLE((propdlg),field)
#define PROPDLG_APPLY_LINESTYLE(propdlg,field) \
  dia_line_style_selector_get_linestyle((propdlg)->field, \
                         &((propdlg)->parent->field##_style), \
                         &((propdlg)->parent->field##_dashlength));

/* This defines an Arrow attribute. */
typedef DiaArrowSelector *ArrowAttribute;
ArrowAttribute __propdlg_build_arrow(GtkWidget *dialog, 
				     const gchar *desc);
#define __PROPDLG_BUILD_ARROW(propdlg,field,desc) \
   (propdlg)->field = __propdlg_build_arrow((propdlg)->dialog.d,(desc));

#define __PROPDLG_SHOW_ARROW(propdlg,field) \
  dia_arrow_selector_set_arrow((propdlg)->field, \
                               (propdlg)->parent->field); 
#define PROPDLG_SHOW_ARROW(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_LINEWIDTH((propdlg),field,(desc)) ;}\
   __PROPDLG_SHOW_LINEWIDTH((propdlg),field)
#define PROPDLG_APPLY_ARROW(propdlg,field) \
  (propdlg)->parent->field = \
     dia_arrow_selector_get_arrow((propdlg)->field);

/* Enum attributes are a bit trickier. They need to provide a way to tell
   how to build them, ie the human description of all possible values. 
   To make things funnier, there's a variable number of possible values...
 */
typedef struct {
  const gchar *desc;
  int value;
  GtkToggleButton *button; /* should be initialised as NULL */
} PropDlgEnumEntry;
typedef PropDlgEnumEntry *EnumAttribute;

/* enumentries is a pointer to a NULL-terminated array of PropDlgEnumEntry,
   to which the _BUILD_ function has the right to write once (the button field)
*/
void __propdlg_build_enum(GtkWidget *dialog,
			  const gchar *desc,
			  PropDlgEnumEntry *enumentries);
void __propdlg_set_enum(PropDlgEnumEntry *enumentries,int value);
int __propdlg_get_enum(PropDlgEnumEntry *enumentries);

#define __PROPDLG_BUILD_ENUM(propdlg,field,desc,enumentries) \
    (propdlg)->field = (enumentries); \
    __propdlg_build_enum((propdlg)->dialog.d,(desc),(enumentries));
#define __PROPDLG_SHOW_ENUM(propdlg,field) \
  __propdlg_set_enum((propdlg)->field,(propdlg)->parent->field);

#define PROPDLG_SHOW_ENUM(propdlg, field, desc,enumentries) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_ENUM((propdlg),field,(desc),(enumentries)) ;}\
   __PROPDLG_SHOW_ENUM((propdlg),field)
#define PROPDLG_APPLY_ENUM(propdlg,field) \
  (propdlg)->parent->field = __propdlg_get_enum((propdlg)->field);

/* This defines a string (gchar *) attribute. */
typedef GtkEntry *StringAttribute;
StringAttribute __propdlg_build_string(GtkWidget *dialog,
				       const gchar *desc);
#define __PROPDLG_BUILD_STRING(propdlg,field,desc) \
 (propdlg)->field = __propdlg_build_string((propdlg)->dialog.d,(desc));
#define __PROPDLG_SHOW_STRING(propdlg,field) \
 gtk_entry_set_text((propdlg)->field,(propdlg)->parent->field);
#define PROPDLG_SHOW_STRING(propdlg, field, desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_STRING((propdlg),field,(desc)) ;} \
   __PROPDLG_SHOW_STRING((propdlg),field)
#define PROPDLG_APPLY_STRING(propdlg,field) \
 g_free((propdlg)->parent->field); \
 (propdlg)->parent->field = strdup(gtk_entry_get_text((propdlg)->field));

/* This defines a string (gchar *) attribute. */
typedef GtkText *MultiStringAttribute;
MultiStringAttribute __propdlg_build_multistring(GtkWidget *dialog,
						 const gchar *desc,
						 int numlines);
#define __PROPDLG_BUILD_MULTISTRING(propdlg,field,desc,numlines) \
 (propdlg)->field = __propdlg_build_multistring((propdlg)->dialog.d, \
                (desc),(numlines));
#define __PROPDLG_SHOW_MULTISTRING(propdlg,field) \
 gtk_text_set_point((propdlg)->field, 0 ); \
 gtk_text_forward_delete((propdlg)->field, \
                         gtk_text_get_length((propdlg)->field)); \
 gtk_text_insert((propdlg)->field, NULL, NULL, NULL, \
                     g_strdup((propdlg)->parent->field), -1);

#define PROPDLG_SHOW_MULTISTRING(propdlg, field, desc,numlines) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_MULTISTRING((propdlg),field,(desc),(numlines)) ;} \
   __PROPDLG_SHOW_MULTISTRING((propdlg),field)
#define PROPDLG_APPLY_MULTISTRING(propdlg,field) \
 g_free((propdlg)->parent->field); \
 (propdlg)->parent->field = strdup(gtk_editable_get_chars( \
               GTK_EDITABLE((propdlg)->field),0, -1));

#define __PROPDLG_BUILD_TEXTTEXT(propdlg,field,desc,numlines) \
   __PROPDLG_BUILD_MULTISTRING((propdlg),field,(desc),(numlines))
#define __PROPDLG_SHOW_TEXTTEXT(propdlg,field) \
 gtk_text_set_point((propdlg)->field, 0 ); \
 gtk_text_forward_delete((propdlg)->field, \
                         gtk_text_get_length((propdlg)->field)); \
 gtk_text_insert((propdlg)->field, NULL, NULL, NULL, \
                     text_get_string_copy((propdlg)->parent->field), -1);

#define PROPDLG_SHOW_TEXTTEXT(propdlg, field, desc,numlines) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_TEXTTEXT((propdlg),field,(desc),(numlines)) ;} \
   __PROPDLG_SHOW_TEXTTEXT((propdlg),field)
#define PROPDLG_APPLY_TEXTTEXT(propdlg,field) \
 text_set_string((propdlg)->parent->field, gtk_editable_get_chars( \
               GTK_EDITABLE((propdlg)->field),0, -1));

void  __propdlg_build_static(GtkWidget *dialog, const gchar *desc);
#define __PROPDLG_BUILD_STATIC(propdlg,desc) \
  __propdlg_build_static((propdlg)->dialog.d,(desc));
#define PROPDLG_SHOW_STATIC(propdlg,desc) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_STATIC((propdlg),(desc)); \
   }

/* This simply puts a separator line. */
void __propdlg_build_separator(GtkWidget *dialog);

#define __PROPDLG_BUILD_SEPARATOR(propdlg) \
  __propdlg_build_separator((propdlg)->dialog.d);
#define PROPDLG_SHOW_SEPARATOR(propdlg) \
   if (!(propdlg)->dialog.ready) {\
      __PROPDLG_BUILD_SEPARATOR((propdlg)); \
   }


/* This embeds a sanity check. Shouldn't that made a simple, rude and 
   brutal g_assert( (probableparent) == (propdlg)->parent ); ? */
#define PROPDLG_SANITY_CHECK(propdlg, probableparent) \
  if ( (probableparent) != (propdlg)->parent) { \
    message_warning("%s/%s dialog problem:  %p != %p\n", \
      ((probableparent)?((Object *)(probableparent))->type->name:NULL), \
      ((propdlg)->parent?((Object *)((propdlg)->parent))->type->name:NULL), \
      (probableparent), (propdlg)->parent); \
    (probableparent) = (propdlg)->parent; \
  }

/* State load/save/free macros. Most of the free macros are intentionally 
   blank. This whole thing assumes the fields have the same names in the
   object and state structures.

   These macros are designed to make writing the three foo_*_state() as
   fast as possible (using a lot of cut'n'paste).
*/

#define OBJECT_SET_INT(object,state,field) (object)->field = (state)->field;
#define OBJECT_GET_INT(object,state,field) (state)->field = (object)->field;
#define OBJECT_FREE_INT(state,field)
#define OBJECT_SET_REAL(object,state,field) (object)->field = (state)->field;
#define OBJECT_GET_REAL(object,state,field) (state)->field = (object)->field;
#define OBJECT_FREE_REAL(state,field)
#define OBJECT_SET_BOOL(object,state,field) (object)->field = (state)->field;
#define OBJECT_GET_BOOL(object,state,field) (state)->field = (object)->field;
#define OBJECT_FREE_BOOL(state,field)
#define OBJECT_SET_COLOR(object,state,field) (object)->field = (state)->field;
#define OBJECT_GET_COLOR(object,state,field) (state)->field = (object)->field;
#define OBJECT_FREE_COLOR(state,field)
#define OBJECT_SET_POINT(object,state,field) (object)->field = (state)->field;
#define OBJECT_GET_POINT(object,state,field) (state)->field = (object)->field;
#define OBJECT_FREE_POINT(state,field)
#define OBJECT_SET_RECT(object,state,field) (object)->field = (state)->field;
#define OBJECT_GET_RECT(object,state,field) (state)->field = (object)->field;
#define OBJECT_FREE_RECT(state,field)
#define OBJECT_SET_STRING(object,state,field) \
   if ((object)->field) g_free((object)->field); \
   (object)->field = (state)->field; 
#define OBJECT_GET_STRING(object,state,field) \
   (state)->field = g_strdup((object)->field);
#define OBJECT_FREE_STRING(state,field) g_free((state)->field);
#define OBJECT_SET_MULTISTRING(object,state,field) \
   OBJECT_SET_STRING(object,state,field)
#define OBJECT_GET_MULTISTRING(object,state,field) \
   OBJECT_GET_STRING(object,state,field)
#define OBJECT_FREE_MULTISTRING(state,field) \
   OBJECT_FREE_STRING(state,field)
#define OBJECT_SET_FONT(object,state,field) (object)->field = (state)->field;
#define OBJECT_GET_FONT(object,state,field) (state)->field = (object)->field;
#define OBJECT_FREE_FONT(state,field)
#define OBJECT_SET_FONTHEIGHT(object,state,field) \
  (object)->field = (state)->field;
#define OBJECT_GET_FONTHEIGHT(object,state,field) \
  (state)->field = (object)->field;
#define OBJECT_FREE_FONTHEIGHT(state,field)
#define OBJECT_SET_TEXTATTR(object,state,field) \
  text_set_attributes((object)->field, &(state)->field##_attrib);
#define OBJECT_GET_TEXTATTR(object,state,field) \
  text_get_attributes((object)->field, &(state)->field##_attrib);
#define OBJECT_FREE_TEXTATTR(state,field);

#endif __LAZYPROPS_H








