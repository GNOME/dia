/* 
 * Various laziness routines to minimise boilerplate code
 * Copyright(C) 2000 Cyrille Chepelov
 *
 * I stole most of the gtk code here.
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

#define __LAZYPROPS_C
#include "lazyprops.h"

void 
__propdlg_build_enum(GtkWidget *dialog, 
		     const gchar *desc,
		     PropDlgEnumEntry *enumentries)
{
  GtkWidget *label,*vbox,*button; 
  GSList *group; 
  PropDlgEnumEntry *p; 
  
  p = enumentries;

  vbox = gtk_vbox_new(FALSE,0); 
    
  label = gtk_label_new((desc)); 
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,TRUE,0); 
  
  gtk_widget_show(label); 
  
  group = NULL; 
  while (p->desc) { 
    button = gtk_radio_button_new_with_label(group, _(p->desc)); 
    gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0); 
    gtk_widget_show(button); 
    p->button = GTK_TOGGLE_BUTTON(button); 
    if (!group) gtk_toggle_button_set_active(p->button,TRUE); /* first */ 
    group = gtk_radio_button_group(GTK_RADIO_BUTTON(button)); 
    p++; 
  }
  gtk_box_pack_start(GTK_BOX(dialog),vbox,TRUE,TRUE,0); 
  gtk_widget_show(vbox); 
}

void 
__propdlg_set_enum(PropDlgEnumEntry *enumentries, int value)
{
  PropDlgEnumEntry *p = enumentries;
  while (p->desc) { 
    if (value == p->value) { 
      gtk_toggle_button_set_active(p->button,TRUE); 
      break; 
    } 
    p++; 
  } 
} 
  
int
__propdlg_get_enum(PropDlgEnumEntry *enumentries) 
{
  PropDlgEnumEntry *p = enumentries;
  while (p->desc) { 
    if (gtk_toggle_button_get_active(p->button))
      return p->value;
    p++;
  }
  g_assert_not_reached(); 
  return 0; 
}

StringAttribute 
__propdlg_build_string(GtkWidget *dialog, const gchar *desc)
{
  GtkWidget *label,*entry,*hbox;

  hbox = gtk_hbox_new(FALSE, 5);
  
  label = gtk_label_new(desc);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  entry = gtk_entry_new();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  return GTK_ENTRY(entry);
}

MultiStringAttribute 
__propdlg_build_multistring(GtkWidget *dialog, const gchar *desc, int numlines)
{
  GtkWidget *label,*entry,*hbox;

  hbox = gtk_hbox_new(FALSE, numlines);
  
  label = gtk_label_new(desc);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  entry = gtk_text_new(NULL,NULL);
  gtk_text_set_editable(GTK_TEXT(entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);
  
  return GTK_TEXT(entry);
}

void 
__propdlg_build_separator(GtkWidget *dialog)
{ 
  GtkWidget *label; 
  label = gtk_hseparator_new (); 
  gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0); 
  gtk_widget_show (label); 
}

void 
__propdlg_build_static(GtkWidget *dialog, 
		       const gchar *desc)
{ 
  GtkWidget *label; 

  label = gtk_label_new((desc)); 
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_widget_show (label); 
  gtk_box_pack_start(GTK_BOX(dialog), label, FALSE, TRUE, 0); 
  gtk_widget_show(label);
}

ArrowAttribute 
__propdlg_build_arrow(GtkWidget *dialog, const gchar *desc)
{ 
  GtkWidget *hbox, *label, *align, *arrow; 

  hbox = gtk_hbox_new(FALSE, 5); 
  label = gtk_label_new((desc)); 
  align = gtk_alignment_new(0.0,0.0,0.0,0.0); 
  gtk_container_add(GTK_CONTAINER(align), label); 
  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0); 
  gtk_widget_show (label); 
  gtk_widget_show(align); 
  arrow = dia_arrow_selector_new(); 
  gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0); 
  gtk_widget_show (arrow); 
  gtk_widget_show(hbox); 
  gtk_box_pack_start (GTK_BOX(dialog), hbox, TRUE, TRUE, 0); 

  return DIAARROWSELECTOR(arrow); 
}

LineStyleAttribute
__propdlg_build_linestyle(GtkWidget *dialog, const gchar *desc)
{ 
  GtkWidget *hbox,*label,*linestyle; 
  hbox = gtk_hbox_new(FALSE, 5); 
  label = gtk_label_new((desc)); 
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0); 
  gtk_widget_show (label); 
  linestyle = dia_line_style_selector_new(); 
  gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0); 
  gtk_widget_show (linestyle); 
  gtk_widget_show(hbox); 
  gtk_box_pack_start (GTK_BOX(dialog), hbox, TRUE, TRUE, 0); 
  
  return DIALINESTYLESELECTOR(linestyle); 
}    


ColorAttribute 
__propdlg_build_color(GtkWidget *dialog, const gchar *desc)
{
  GtkWidget *hbox,*label,*color; 
  
  hbox = gtk_hbox_new(FALSE, 5); 
  label = gtk_label_new((desc)); 
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0); 
  gtk_widget_show (label); 
  color = dia_color_selector_new(); 
  gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0); 
  gtk_widget_show (color); 
  gtk_widget_show(hbox); 
  gtk_box_pack_start (GTK_BOX(dialog), hbox, TRUE, TRUE, 0); 
  
  return DIACOLORSELECTOR(color); 
} 
  
BoolAttribute 
__propdlg_build_bool(GtkWidget *dialog, const gchar *desc)
{ 
  GtkWidget *hbox, *cb; 

  hbox = gtk_hbox_new(FALSE,5); 
  cb = gtk_check_button_new_with_label((desc));
  gtk_box_pack_start(GTK_BOX(hbox),cb,TRUE,TRUE,0); 
  gtk_widget_show(cb);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(dialog), hbox, FALSE, TRUE, 0); 
  return GTK_TOGGLE_BUTTON(cb);   
}

FontAttribute 
__propdlg_build_font(GtkWidget *dialog, const gchar *desc)
{ 
  GtkWidget *hbox, *label, *font; 

  hbox = gtk_hbox_new(FALSE, 5); 
  label = gtk_label_new((desc)); 
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0); 
  gtk_widget_show (label); 
  font = dia_font_selector_new(); 
  gtk_box_pack_start (GTK_BOX (hbox), font, TRUE, TRUE, 0); 
  gtk_widget_show (font); 
  gtk_widget_show(hbox); 
  gtk_box_pack_start (GTK_BOX(dialog), hbox, TRUE, TRUE, 0); 
  return DIAFONTSELECTOR(font); 
}
									    

RealAttribute
__propdlg_build_real(GtkWidget *dialog, const gchar *desc,
		     gfloat lower, gfloat upper, gfloat step_increment)
{ 
  GtkWidget *hbox, *label, *spin; 
  GtkAdjustment *adj; 
  
  hbox = gtk_hbox_new(FALSE,5); 
  label = gtk_label_new( (desc) ); 
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0); 
  gtk_widget_show(label); 
  adj = (GtkAdjustment *) gtk_adjustment_new(0.1, lower, upper, 
					     step_increment, 10.0 * step_increment, 0.0); 
  spin = gtk_spin_button_new(adj, 1.0, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin), TRUE); 
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE); 
  gtk_box_pack_start(GTK_BOX (hbox), spin, TRUE, TRUE, 0); 
  gtk_widget_show (spin); 
  gtk_widget_show(hbox); 
  gtk_box_pack_start (GTK_BOX(dialog), hbox, TRUE, TRUE, 0); 
  return GTK_SPIN_BUTTON(spin); 
} 





