/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gmodule.h>

#include "config.h"

#if (defined (HAVE_LIBPOPT) && defined (HAVE_POPT_H)) || defined (GNOME)
#define HAVE_POPT
#endif

#ifdef GNOME
#include <gnome.h>
#else
#ifdef HAVE_POPT_H
#include <popt.h>
#endif
#endif

#include "intl.h"
#include "app_procs.h"
#include "object.h"
#include "color.h"
#include "tool.h"
#include "modify_tool.h"
#include "interface.h"
#include "group.h"
#include "message.h"
#include "display.h"
#include "layer_dialog.h"
#include "load_save.h"
#include "preferences.h"
#include "custom.h"

static void register_all_objects(void);
static void register_all_sheets(void);
static int name_is_lib(char *name);
static void create_user_dirs(void);

#ifdef GNOME

static void
session_die (gpointer client_data)
{
  gtk_main_quit ();
}

static int
save_state (GnomeClient        *client,
	    gint                phase,
	    GnomeRestartStyle   save_style,
	    gint                shutdown,
	    GnomeInteractStyle  interact_style,
	    gint                fast,
	    gpointer            client_data)
{
  gchar *argv[20];
  gint i = 0;
  GList *l;
  Diagram *dia;

  argv[i++] = "dia";

  for(l = open_diagrams; l != NULL; l = g_list_next(l)) {
    dia = (Diagram *)l->data;
    if(!dia->unsaved) {
      argv[i++] = dia->filename;
    }
  }

  gnome_client_set_restart_command (client, i, argv);
  gnome_client_set_clone_command (client, i, argv);

  return TRUE;
}
#endif

static int current_version = 0;

void
debug_break(void)
{
  /* Break here. All symbols are loaded. */
}

void
app_init (int argc, char **argv)
{
  Diagram *diagram = NULL;
  DDisplay *ddisp = NULL;
#ifdef GNOME
  GnomeClient *client;
#endif
  char *in_file_name = NULL;
  char *export_file_name = NULL;
#ifdef HAVE_POPT
  int rc;
  poptContext poptCtx;
  struct poptOption options[] =
  {
    {"export-to-ps", 'e', POPT_ARG_STRING, &export_file_name, 0,
     N_("Export loaded file to postscript and exit"),
     N_("OUTPUT")},
#ifndef GNOME
    {"help", 'h', POPT_ARG_NONE, 0, 1, N_("Show this help message") },
#endif
    {(char *) NULL, '\0', 0, NULL, 0}
  };
#endif

  gtk_set_locale();
  setlocale(LC_NUMERIC, "C");
  
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

#ifdef GNOME
  gnome_init_with_popt_table(PACKAGE, VERSION, argc, argv, options,
			     0, &poptCtx);

  client = gnome_master_client();
  if(client == NULL) {
    g_warning(_("Can't connect to session manager!\n"));
  }
  else {
    gtk_signal_connect(GTK_OBJECT (client), "save_yourself",
		       GTK_SIGNAL_FUNC (save_state), NULL);
    gtk_signal_connect(GTK_OBJECT (client), "die",
		       GTK_SIGNAL_FUNC (session_die), NULL);
  }
#else
#ifdef HAVE_POPT
  poptCtx = poptGetContext(PACKAGE, argc, argv, options, 0);
  poptSetOtherOptionHelp(poptCtx, _("[OPTION...] [FILE...]"));
  if((rc = poptGetNextOpt(poptCtx)) < -1) {
    fprintf(stderr, 
	    _("Error on option %s: %s.\nRun '%s --help' to see a full list of available command line options.\n"),
	    poptBadOption(poptCtx, 0),
	    poptStrerror(rc),
	    argv[0]);
    exit(1);
  }
  if(rc == 1) {
    poptPrintHelp(poptCtx, stderr, 0);
    exit(0);
  }
#endif
  gtk_init (&argc, &argv);
  dia_image_init();
#endif

  gtk_rc_parse ("diagtkrc"); 

  /*  enable_core_dumps(); */

  create_user_dirs();

  color_init();

  font_init();

  /* Init cursors: */
  default_cursor = gdk_cursor_new(GDK_LEFT_PTR);
  ddisplay_set_all_cursor(default_cursor);

  object_registry_init();

  register_all_objects();
  load_all_sheets();     /* new mechanism */
  register_all_sheets(); /* old mechanism (to be disabled) */

  debug_break();

  if (object_get_type("Standard - Box") == NULL) {
    message_error(_("Couldn't find standard objects when looking for "
		  "object-libs, exiting...\n"));
    fprintf(stderr, _("Couldn't find standard objects when looking for "
	    "object-libs, exiting...\n"));
    exit(1);
  }

  prefs_load();

  create_layer_dialog();

#ifdef HAVE_POPT
  while (poptPeekArg(poptCtx)) {
    in_file_name = poptGetArg(poptCtx);
    diagram = diagram_load (in_file_name);
    if (export_file_name) {
      if (!diagram) {
	fprintf (stderr, _("Need valid input file\n"));
	exit (1);
      }
      diagram_export_to_eps (diagram, export_file_name);
      exit (0);
    }
    if (diagram != NULL) {
      diagram_update_extents(diagram);
      ddisp = new_display(diagram);
    }
  }
  poptFreeContext(poptCtx);
#endif

  active_tool = create_modify_tool();

  create_toolbox();
}

static void
set_true_callback(GtkWidget *w, int *data)
{
  *data = TRUE;
}

void
app_exit(void)
{
  GList *list;
  GSList *slist;
  
  if (diagram_modified_exists()) {
    GtkWidget *dialog;
    GtkWidget *label;
    GtkWidget *button;
    int result = FALSE;

    dialog = gtk_dialog_new();
  
    gtk_signal_connect (GTK_OBJECT (dialog), "destroy", 
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    
    gtk_window_set_title (GTK_WINDOW (dialog), _("Quit, are you sure?"));
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 0);
    label = gtk_label_new (_("Modified diagrams exists.\n"
			   "Are you sure you want to quit?"));
  
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
			label, TRUE, TRUE, 0);
  
    gtk_widget_show (label);

    button = gtk_button_new_with_label (_("Quit"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
			button, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC(set_true_callback),
			&result);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dialog));
    gtk_widget_show (button);
    
    button = gtk_button_new_with_label (_("Cancel"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			button, TRUE, TRUE, 0);
    gtk_widget_grab_default (button);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dialog));
    
    gtk_widget_show (button);

    gtk_widget_show (dialog);

    /* Make dialog modal: */
    gtk_widget_grab_focus(dialog);
    gtk_grab_add(dialog);

    gtk_main();

    if (result == FALSE)
      return;
  }
  
  /* Free loads of stuff (toolbox) */

  list = open_diagrams;
  while (list!=NULL) {
    Diagram *dia = (Diagram *)list->data;
    list = g_list_next(list);

    slist = dia->displays;
    while (slist!=NULL) {
      DDisplay *ddisp = (DDisplay *)slist->data;
      slist = g_slist_next(slist);

      gtk_widget_destroy(ddisp->shell);

    }
    /* The diagram is freed when the last display is destroyed */

  }
  

  gtk_main_quit();
}

static void create_user_dirs(void)
{
  gchar *dir, *subdir;

  dir = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S, ".dia", NULL);
  if (mkdir(dir, 0755) && errno != EEXIST)
    g_error(_("Could not create per-user Dia config directory"));

  /* it is no big deal if these directories can't be created */
  subdir = g_strconcat(dir, G_DIR_SEPARATOR_S, "objects", NULL);
  mkdir(subdir, 0755);
  g_free(subdir);
  subdir = g_strconcat(dir, G_DIR_SEPARATOR_S, "shapes", NULL);
  mkdir(subdir, 0755);
  g_free(subdir);
  subdir = g_strconcat(dir, G_DIR_SEPARATOR_S, "sheets", NULL);
  mkdir(subdir, 0755);
  g_free(subdir);

  g_free(dir);
}

static int
name_is_lib(char *name)
{
  int len;
  len = strlen(name);
  if (len < 3)
    return FALSE;
  if ( (len>3) && (strcmp(".so", &name[len-3])==0) )
    return 1;
  if ( (len>9) && (strcmp(".so.0.0.0", &name[len-9])==0) )
    return 1;
  if ( (len>3) && (strcmp(".sl", &name[len-3])==0) )
    return 1;
  if ( (len>4) && (strcmp(".dll", &name[len-4])==0) )
    return 1;
  
  return 0;
}

static GList *modules_list = NULL;

static void
register_objects_in(char *directory)
{
  struct stat statbuf;
  char file_name[256];
  struct dirent *dirp;
  DIR *dp;
  const gchar *error;
  GModule *libhandle;
  void (*register_func)(void);
  int (*version_func)(void);
  
  if (stat(directory, &statbuf)<0) {
    /*
      message_notice("Couldn't find any libraries in %s.\n", directory);
    */
    return;
  }

  if (!S_ISDIR(statbuf.st_mode)) {
    message_warning(_("Couldn't find any libraries to load.\n"
		    "%s is not a directory.\n"), directory);
    return;
  }

  dp = opendir(directory);
  if ( dp == NULL ) {
    message_warning(_("Couldn't open \"%s\".\n"), directory);
    return;
  }
  
  while ( (dirp = readdir(dp)) ) {
    if ( name_is_lib(dirp->d_name) ) {
      strncpy(file_name, directory, 256);
      strncat(file_name, "/", 256);
      strncat(file_name, dirp->d_name, 256);

      libhandle = g_module_open(file_name, G_MODULE_BIND_LAZY);
      if (libhandle==NULL) {
	error = g_module_error();
	message_warning(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	printf(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	continue;
      }

      if (!g_module_symbol(libhandle, "get_version", (gpointer)&version_func)) {
	message_warning(_("The file \"%s\" is not a Dia object library.\n"), file_name);
	printf(_("The file \"%s\" is not a Dia object library.\n"), file_name);
	g_module_close(libhandle);
	continue;
      }

      if ( (*version_func)() < current_version ) {
	message_warning(_("The object library \"%s\" is from an older version of Dia and cannot be used.\nPlease upgrade it."), file_name);
	printf(_("The object library \"%s\" is from an older version of Dia and cannot be used.\nPlease upgrade it."), file_name);
	g_module_close(libhandle);
	continue;
      }
      
      if ( (*version_func)() > current_version ) {
	message_warning(_("The object library \"%s\" is from an later version of Dia.\nYou need to upgrade Dia to use it."), file_name);
	printf(_("The object library \"%s\" is from an later version of Dia.\nYou need to upgrade Dia to use it."), file_name);
	g_module_close(libhandle);
	continue;
      }

      if (!g_module_symbol(libhandle, "register_objects", (gpointer)&register_func)) {
	error = g_module_error();
	message_warning(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	printf(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	g_module_close(libhandle);
	continue;
      }
      
      (*register_func)();

      g_module_make_resident(libhandle);
      modules_list = g_list_append(modules_list, libhandle);
    } 
  }

  closedir(dp);
}

static void
register_all_objects(void)
{
  char *library_path;
  char *home_path;
  char *path;
  char lib_dir[256];
  
  object_register_type(&group_type);

  custom_register_objects();

  home_path = getenv("HOME");
  library_path = getenv("DIA_LIB_PATH");
  if (library_path != NULL)
    library_path = strdup(library_path);

  if (home_path!=NULL) {
    strncpy(lib_dir, home_path, 256);
    strncat(lib_dir, "/.dia/objects", 256);
    register_objects_in(lib_dir);
  }

  if (library_path != NULL) {
    path = strtok(library_path, ":");
    while ( path != NULL ) {
      register_objects_in(path);
      path = strtok(NULL, ":");
    }
    g_free(library_path);
  } else {
    register_objects_in(LIBDIR "/dia");
  }

}

static void
register_all_sheets(void)
{
  GList *list;
  void (*register_func)(void);
  GModule *libhandle;
  const gchar *error;

  /* XXX This will disappear, yek yek yek... */
  for (list = modules_list; list != NULL; list = g_list_next(list)) {
    libhandle = (GModule *) list->data;

    if (!g_module_symbol(libhandle, "register_sheets",
			 (gpointer)&register_func)) {
      error = g_module_error();
      message_warning(_("Unable to find register_sheets in library:\n%s"),
		      error);
      continue;
    }

    (*register_func)();
  } 

}

