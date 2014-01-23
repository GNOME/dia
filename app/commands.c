/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson *
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.\n");

  gchar *dirname = dia_get_data_directory("");
  gchar *filename = g_build_filename (dirname, "dia-splash.png", NULL);
  GdkPixbuf *logo = gdk_pixbuf_new_from_file(filename, NULL);

#if GTK_CHECK_VERSION(2,24,0)
  /* rely on gtk_show_uri doing the right thing internally */
#else
  gtk_about_dialog_set_url_hook ((GtkAboutDialogActivateLinkFunc)activate_url, NULL, NULL);
#endif
  gtk_show_about_dialog (NULL,
	"logo", logo,
        "name", "Dia",
	"version", VERSION,
	"comments", _("A program for drawing structured diagrams."),
	"copyright", "(C) 1998-2011 The Free Software Foundation and the authors",
	"website", "http://live.gnome.org/Dia",
	"authors", authors,
	"documenters", documentors,
	"translator-credits", strcmp (translators, "translator_credits-PLEASE_ADD_YOURSELF_HERE")
			? translators : NULL,
	"license", license,
	NULL);
  g_free (dirname);
  g_free (filename);
  if (logo)
    g_object_unref (logo);
}

void
view_zoom_in_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  
  ddisplay_zoom_middle(ddisp, M_SQRT2);
}

void
view_zoom_out_callback (GtkAction *action)
{
  DDisplay *ddisp;
  
  ddisp = ddisplay_active();
  if (!ddisp) return;
  
  ddisplay_zoom_middle(ddisp, M_SQRT1_2);
}

void 
view_zoom_set_callback (GtkAction *action)
{
  int factor;
  /* HACK the actual factor is a suffix to the action name */
  factor = atoi (gtk_action_get_name (action) + strlen ("ViewZoom"));
  view_zoom_set (factor);
}

void
view_show_cx_pts_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();
  if (!ddisp) return;

  old_val = ddisp->show_cx_pts;
  ddisp->show_cx_pts = gtk_toggle_action_get_active (action);
  
  if (old_val != ddisp->show_cx_pts) {
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

void 
view_unfullscreen (void)
{
  DDisplay *ddisp;
  GtkToggleAction *item;

  ddisp = ddisplay_active();
  if (!ddisp) return;

  /* find the menuitem */
  item = GTK_TOGGLE_ACTION (menus_get_action ("ViewFullscreen"));
  if (item && gtk_toggle_action_get_active (item)) {
    gtk_toggle_action_set_active (item, FALSE);
  }
}

void
view_fullscreen_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  int fs;

  ddisp = ddisplay_active();
  if (!ddisp) return;

  fs = gtk_toggle_action_get_active (action);
  
  if (fs) /* it is already toggled */
    gtk_window_fullscreen(GTK_WINDOW(ddisp->shell));
  else
    gtk_window_unfullscreen(GTK_WINDOW(ddisp->shell));
}

void
view_aa_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  int aa;

  ddisp = ddisplay_active();
  if (!ddisp) return;
 
  aa = gtk_toggle_action_get_active (action);
  
  if (aa != ddisp->aa_renderer) {
    ddisplay_set_renderer(ddisp, aa);
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

void
view_visible_grid_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  guint old_val;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  
  old_val = ddisp->grid.visible;
  ddisp->grid.visible = gtk_toggle_action_get_active (action); 

  if (old_val != ddisp->grid.visible) {
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

void
view_snap_to_grid_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  
  ddisplay_set_snap_to_grid(ddisp, gtk_toggle_action_get_active (action));
}

void
view_snap_to_objects_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  
  ddisplay_set_snap_to_objects(ddisp, gtk_toggle_action_get_active (action));
}

void 
view_toggle_rulers_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  
  ddisp = ddisplay_active();
  if (!ddisp) return;

  if (!gtk_toggle_action_get_active (action)) {
    if (display_get_rulers_showing(ddisp)) {
      display_rulers_hide (ddisp);
    }
  } else {
    if (!display_get_rulers_showing(ddisp)) {
      display_rulers_show (ddisp);
    }
  }
}

extern void
view_new_view_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram();
  if (!dia) return;
  
  new_display(dia);
}

extern void
view_clone_view_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  
  copy_display(ddisp);
}

void
view_show_all_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  
  ddisplay_show_all (ddisp);
}

void
view_redraw_callback (GtkAction *action)
{
  DDisplay *ddisp;
  ddisp = ddisplay_active();
  if (!ddisp) return;
  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);
}

void
view_diagram_properties_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  diagram_properties_show(ddisp->diagram);
}

void
view_main_toolbar_callback (GtkAction *action)
{
  integrated_ui_toolbar_show (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION(action)));
}

void
view_main_statusbar_callback (GtkAction *action)
{
  integrated_ui_statusbar_show (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION(action)));
}

void
view_layers_callback (GtkAction *action)
{
  integrated_ui_layer_view_show (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION(action)));
}

void 
layers_add_layer_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram();
  if (!dia) return;

  diagram_edit_layer (dia, NULL);
}

void 
layers_rename_layer_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram();
  if (!dia) return;

  diagram_edit_layer (dia, dia->data->active_layer);
}

void
objects_place_over_callback (GtkAction *action)
{
  diagram_place_over_selected(ddisplay_active_diagram());
}

void
objects_place_under_callback (GtkAction *action)
{
  diagram_place_under_selected(ddisplay_active_diagram());
}

void
objects_place_up_callback (GtkAction *action)
{
  diagram_place_up_selected(ddisplay_active_diagram());
}

void
objects_place_down_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  diagram_place_down_selected(ddisplay_active_diagram());
}

void
objects_parent_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  diagram_parent_selected(ddisplay_active_diagram());
}

void
objects_unparent_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  diagram_unparent_selected(ddisplay_active_diagram());
}

void
objects_unparent_children_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  diagram_unparent_children_selected(ddisplay_active_diagram());
}

void
objects_group_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  diagram_group_selected(ddisplay_active_diagram());
  ddisplay_do_update_menu_sensitivity(ddisp);
} 

void
objects_ungroup_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  diagram_ungroup_selected(ddisplay_active_diagram());
  ddisplay_do_update_menu_sensitivity(ddisp);
} 

void
dialogs_properties_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram();
  if (!dia || textedit_mode(ddisplay_active())) return;

  if (dia->data->selected != NULL) {
    object_list_properties_show(dia, dia->data->selected);
  } else {
    diagram_properties_show(dia);
  } 
}

void
dialogs_layers_callback (GtkAction *action)
{
  layer_dialog_set_diagram(ddisplay_active_diagram());
  layer_dialog_show();
}


void
objects_align_h_callback (GtkAction *action)
{
  const gchar *a;
  int align = DIA_ALIGN_LEFT;
  Diagram *dia;
  GList *objects;

  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  /* HACK align is suffix to action name */
  a = gtk_action_get_name (action) + strlen ("ObjectsAlign");
  if (0 == strcmp ("Left", a)) {
	align = DIA_ALIGN_LEFT;
  } 
  else if (0 == strcmp ("Center", a)) {
	align = DIA_ALIGN_CENTER;
  } 
  else if (0 == strcmp ("Right", a)) {
	align = DIA_ALIGN_RIGHT;
  } 
  else if (0 == strcmp ("Spreadouthorizontally", a)) {
	align = DIA_ALIGN_EQUAL;
  } 
  else if (0 == strcmp ("Adjacent", a)) {
	align = DIA_ALIGN_ADJACENT;
  }
  else {
	g_warning ("objects_align_v_callback() called without appropriate align");
	return;
  }

  dia = ddisplay_active_diagram();
  if (!dia) return;
  objects = dia->data->selected;
  
  object_add_updates_list(objects, dia);
  object_list_align_h(objects, dia, align);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_modified(dia);
  diagram_flush(dia);     

  undo_set_transactionpoint(dia->undo);
}

void
objects_align_v_callback (GtkAction *action)
{
  const gchar *a;
  int align;
  Diagram *dia;
  GList *objects;

  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  /* HACK align is suffix to action name */
  a = gtk_action_get_name (action) + strlen ("ObjectsAlign");
  if (0 == strcmp ("Top", a)) {
	align = DIA_ALIGN_TOP;
  } 
  else if (0 == strcmp ("Middle", a)) {
	align = DIA_ALIGN_CENTER;
  } 
  else if (0 == strcmp ("Bottom", a)) {
	align = DIA_ALIGN_BOTTOM;
  } 
  else if (0 == strcmp ("Spreadoutvertically", a)) {
	align = DIA_ALIGN_EQUAL;
  } 
  else if (0 == strcmp ("Stacked", a)) {
	align = DIA_ALIGN_ADJACENT;
  }
  else {
	g_warning ("objects_align_v_callback() called without appropriate align");
	return;
  }

  dia = ddisplay_active_diagram();
  if (!dia) return;
  objects = dia->data->selected;

  object_add_updates_list(objects, dia);
  object_list_align_v(objects, dia, align);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_modified(dia);
  diagram_flush(dia);     

  undo_set_transactionpoint(dia->undo);
}

void
objects_align_connected_callback (GtkAction *action)
{
  Diagram *dia;
  GList *objects;

  dia = ddisplay_active_diagram();
  if (!dia) return;
  objects = dia->data->selected;

  object_add_updates_list(objects, dia);
  object_list_align_connected(objects, dia, 0);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_modified(dia);
  diagram_flush(dia);

  undo_set_transactionpoint(dia->undo);
}

/*! Open a file and show it in a new display */
void
dia_file_open (const gchar *filename,
               DiaImportFilter *ifilter)
{
  Diagram *diagram;

  if (!ifilter)
    ifilter = filter_guess_import_filter(filename);

  diagram = diagram_load(filename, ifilter);
  if (diagram != NULL) {
    diagram_update_extents(diagram);
    layer_dialog_set_diagram(diagram);
    new_display(diagram);
  }
}
