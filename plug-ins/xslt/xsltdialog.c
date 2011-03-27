/*
 * File: plug-ins/xslt/xsltdialog.c
 * 
 * Made by Matthieu Sozeau <mattam@netcourrier.com>
 * 
 * Started on  Thu May 16 20:30:42 2002 Matthieu Sozeau
 * Last update Fri May 17 00:30:24 2002 Matthieu Sozeau
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
 *
 */

/*
 * Opens a dialog for export options
 */

#undef GTK_DISABLE_DEPRECATED /* GTK_OPTION_MENU, ... */
#include "xslt.h"
#include <stdio.h>

#include <gtk/gtk.h>


static void
from_deactivate(fromxsl_t *xsls);


static void
from_activate(GtkWidget *widget, fromxsl_t *xsls)
{
	toxsl_t *to_f = xsls->xsls;
	
	from_deactivate(xsl_from);

	xsl_from = xsls;
	xsl_to = to_f;
	
	gtk_menu_item_activate(GTK_MENU_ITEM(to_f->item));
	while(to_f != NULL)
	{
		gtk_widget_set_sensitive(to_f->item, 1);
		to_f = to_f->next;
	}
}

static void
from_deactivate(fromxsl_t *xsls)
{
	toxsl_t *to_f = xsls->xsls;
	
	while(to_f != NULL)
	{
		gtk_widget_set_sensitive(to_f->item, 0);
		to_f = to_f->next;
	}
}


static void
to_update(GtkWidget *widget, toxsl_t *lng)
{
        /* printf("To: %s\n", lng->name); */
	xsl_to = lng;
}

static GtkWidget *dialog;

static void xslt_dialog_respond(GtkWidget *widget,
                                gint response_id,
                                gpointer user_data);


void
xslt_dialog_create(void) {
	GtkWidget *box, *vbox;
	
	GtkWidget *omenu, *menu, *menuitem;
	GSList *group;
	GtkWidget *label;	

	fromxsl_t *cur_f = froms;
	toxsl_t *cur_to = NULL;

	g_return_if_fail(froms != NULL);

	dialog = gtk_dialog_new_with_buttons(
             _("Export through XSLT"),
             NULL, 0,
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
             GTK_STOCK_OK, GTK_RESPONSE_OK,
             NULL);
	
	gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  
	box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_container_set_border_width (GTK_CONTAINER (box), 10);

	label = gtk_label_new(_("From:"));

 	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	group = NULL;
	
	while(cur_f != NULL)
	{
		menuitem = gtk_radio_menu_item_new_with_label (group, cur_f->name);
		g_signal_connect (G_OBJECT (menuitem), "activate",
				  G_CALLBACK (from_activate), cur_f);
		group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
		gtk_menu_append (GTK_MENU (menu), menuitem);
		gtk_widget_show (menuitem);
		cur_f = cur_f->next;
	}
	

	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_widget_show(menu);
	gtk_widget_show(omenu);
	gtk_widget_show(label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), omenu, FALSE, TRUE, 0);
	
	gtk_widget_show_all(vbox);

	gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, TRUE, 0);
	
	cur_f = froms;

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_container_set_border_width (GTK_CONTAINER (box), 10);

	label = gtk_label_new(_("To:"));

 	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	group = NULL;
	
	while(cur_f != NULL)
	{
		cur_to = cur_f->xsls;
		while(cur_to != NULL)
		{
			menuitem = gtk_radio_menu_item_new_with_label (group, cur_to->name);
			g_signal_connect (G_OBJECT (menuitem), "activate",
					  G_CALLBACK (to_update), cur_to );
			group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
			gtk_menu_append (GTK_MENU (menu), menuitem);
			gtk_widget_show (menuitem);
			cur_to->item = menuitem;
			cur_to = cur_to->next;
		}
		cur_f = cur_f->next;		
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_widget_show(menu);
	gtk_widget_show(omenu);
	gtk_widget_show(label);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), omenu, FALSE, TRUE, 0);
	
	gtk_widget_show_all(vbox);

	gtk_box_pack_start (GTK_BOX (box), vbox, FALSE, TRUE, 0);
	
	gtk_widget_show_all(box);

	g_signal_connect(G_OBJECT(dialog), "response",
                   G_CALLBACK(xslt_dialog_respond),
                   NULL);
	g_signal_connect(G_OBJECT(dialog), "delete_event",
                   G_CALLBACK(gtk_widget_hide), NULL);


	gtk_widget_show(dialog);	

	cur_f = froms->next;
	while(cur_f != NULL)
	{
		from_deactivate(cur_f);
		cur_f = cur_f->next;
	}

}

void xslt_clear(void) {
	gtk_widget_destroy(dialog);
}

void xslt_dialog_respond(GtkWidget *widget,
                         gint response_id,
                         gpointer user_data) {

    gtk_widget_hide(dialog);
    if (response_id == GTK_RESPONSE_OK) xslt_ok();
}
