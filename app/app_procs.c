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
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <dlfcn.h>
#include <signal.h>
#include <locale.h>

#include <gtk/gtk.h>

#include "config.h"
#ifdef GNOME
#include <gnome.h>
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

#ifdef RTLD_LAZY        /* Solaris 2. */
# define DLOPEN_MODE   RTLD_LAZY
#else
# ifdef RTLD_NOW
#  define DLOPEN_MODE   RTLD_NOW
# else
#  define DLOPEN_MODE   1  /* Thats what it says in the man page. */
# endif
#endif

static void register_all_objects(void);
static void register_all_sheets(void);
static int name_is_lib(char *name);

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
  /*
  gchar *argv[20];
  gchar *s;
  gint i = 0, j;
  HelpWindow win;
  gint xpos, ypos;
  gint xsize, ysize;

  g_message("Saving myself");
  gdk_window_get_origin(appwin->window, &xpos, &ypos);
  gdk_window_get_size(appwin->window, &xsize, &ysize);

  argv[i++] = program_invocation_name;
  argv[i++] = (char *) "-x";
  s = alloca(20);
  snprintf(s, 20, "%d", xpos);
  argv[i++] = s;
  argv[i++] = (char *) "-y";
  s = alloca(20);
  snprintf(s, 20, "%d", ypos);
  argv[i++] = s;
  argv[i++] = (char *) "-w";
  s = alloca(20);
  snprintf(s, 20, "%d", xsize);
  argv[i++] = s;
  argv[i++] = (char *) "-h";
  s = alloca(20);
  snprintf(s, 20, "%d", ysize);
  argv[i++] = s;

  s = alloca(512);
  snprintf(s, 512, "restart command is");
  for (j=0; j<i; j++) {
    strncat(s, " ", 511);
    strncat(s, argv[j], 511);
  }
  g_message("%s", s);

  gnome_client_set_restart_command (client, i, argv);
  gnome_client_set_clone_command (client, 0, NULL);
  */

  return TRUE;
} 

static GnomeClient
*new_gnome_client()
{
  gchar *buf[1024];
  GnomeClient *client;

  client = gnome_client_new();

  if (!client)
    return NULL;

  getcwd((char *)buf, sizeof(buf));
  gnome_client_set_current_directory(client, (char *)buf);

  gtk_object_ref(GTK_OBJECT(client));
  gtk_object_sink(GTK_OBJECT(client));

  gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
		      GTK_SIGNAL_FUNC (save_state), NULL);
  gtk_signal_connect (GTK_OBJECT (client), "die",
		      GTK_SIGNAL_FUNC (session_die), NULL);
  return client;
}

#endif

static int current_version = 0;

void
app_init (int argc, char **argv)
{
  Diagram *diagram;
  DDisplay *ddisp;
#ifdef GNOME
  GnomeClient client;
  GtkWidget *app;
#endif
  int i;
        
  gtk_set_locale ();
  setlocale(LC_NUMERIC, "C");
  
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

#ifdef GNOME
  gnome_init (PACKAGE, VERSION, argc, argv);
#else
  gtk_init (&argc, &argv);
  gdk_imlib_init();
  /* FIXME:  Is this a good idea? */
  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());
#endif

  /* Here could be popt stuff for options */
    
  gtk_rc_parse ("diagtkrc"); 

  /*  enable_core_dumps(); */

  color_init();

  font_init();

  /* Init cursors: */
  default_cursor = gdk_cursor_new(GDK_LEFT_PTR);
  ddisplay_set_all_cursor(default_cursor);

  object_registry_init();

  register_all_objects();
  register_all_sheets();

  if (object_get_type("Standard - Box") == NULL) {
    message_error(_("Couldn't find standard objects when looking for "
		  "object-libs, exiting...\n"));
    fprintf(stderr, _("Couldn't find standard objects when looking for "
	    "object-libs, exiting...\n"));
    exit(1);
  }

  active_tool = create_modify_tool();

  create_toolbox();

  create_layer_dialog();
    
  /* In case argc > 1 load diagram files */
  i = 1;
  while (i < argc) {
    diagram = diagram_load(argv[i]);
    if (diagram != NULL) {
      diagram_update_extents(diagram);
      ddisp = new_display(diagram);
      diagram_add_ddisplay(diagram, ddisp);
    }
    i++;
  }
}

static void
set_true_callback(GtkWidget *w, int *data)
{
  *data = TRUE;
}

void
app_exit(void)
{
  
  if (diagram_modified_exists()) {
    GtkWidget *dialog;
    GtkWidget *label;
    GtkWidget *button;
    int result = FALSE;

    dialog = gtk_dialog_new();
  
    gtk_signal_connect (GTK_OBJECT (dialog), "destroy", 
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    
    gtk_window_set_title (GTK_WINDOW (dialog), _("Quit, are you sure?"));
    gtk_container_border_width (GTK_CONTAINER (dialog), 0);
    label = gtk_label_new (_("Modified diagrams exists.\n"
			   "Are you sure you want to quit?"));
  
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
			label, TRUE, TRUE, 0);
  
    gtk_widget_show (label);

    button = gtk_button_new_with_label (_("Yes"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
			button, TRUE, TRUE, 0);
    gtk_widget_grab_default (button);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC(set_true_callback),
			&result);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dialog));
    gtk_widget_show (button);
    
    button = gtk_button_new_with_label (_("No"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			button, TRUE, TRUE, 0);
    
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

  gtk_main_quit();
}

static int
name_is_lib(char *name)
{
  int len;
  len = strlen(name);
  if (len < 3)
    return FALSE;
  return (strcmp(".so", &name[len-3])==0);
}

static GList *modules_list = NULL;

static void
register_objects_in(char *directory)
{
  struct stat statbuf;
  char file_name[256];
  struct dirent *dirp;
  DIR *dp;
  const char *error;
  void *libhandle;
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
      
      libhandle = dlopen(file_name, DLOPEN_MODE);
      if (libhandle==NULL) {
	error = dlerror();
	message_warning(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	printf(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	continue;
      }

#if defined(USCORE) && !defined(DLSYM_ADDS_USCORE)
      version_func = dlsym(libhandle, "_get_version");
#else
      version_func = dlsym(libhandle, "get_version");
#endif
      if ((error = dlerror()) != NULL)  {
	message_warning(_("The file \"%s\" is not a Dia object library.\n"), file_name);
	printf(_("The file \"%s\" is not a Dia object library.\n"), file_name);
	continue;
      }

      if ( (*version_func)() < current_version ) {
	message_warning(_("The object library \"%s\" is from an older version of Dia and cannot be used.\nPlease upgrade it."), file_name);
	printf(_("The object library \"%s\" is from an older version of Dia and cannot be used.\nPlease upgrade it."), file_name);
	continue;
      }
      
      if ( (*version_func)() > current_version ) {
	message_warning(_("The object library \"%s\" is from an later version of Dia.\nYou need to upgrade Dia to use it."), file_name);
	printf(_("The object library \"%s\" is from an later version of Dia.\nYou need to upgrade Dia to use it."), file_name);
	continue;
      }

      
#if defined(USCORE) && !defined(DLSYM_ADDS_USCORE)
      register_func = dlsym(libhandle, "_register_objects");
#else
      register_func = dlsym(libhandle, "register_objects");
#endif
      if ((error = dlerror()) != NULL)  {
	message_warning(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	printf(_("Error loading library: \"%s\":\n %s\n"), file_name, error);
	continue;
      }
      
      (*register_func)();

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

  home_path = getenv("HOME");
  library_path = getenv("DIA_LIB_PATH");
  if (library_path != NULL)
    library_path = strdup(library_path);

  if (home_path!=NULL) {
    strncpy(lib_dir, home_path, 256);
    strncat(lib_dir, "/.dia_libs", 256);
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
  void *libhandle;
  const char *error;

  list = modules_list;
  while (list != NULL) {
    libhandle = (void *) list->data;

#if defined(USCORE) && !defined(DLSYM_ADDS_USCORE)
    register_func = dlsym(libhandle, "_register_sheets");
#else
    register_func = dlsym(libhandle, "register_sheets");
#endif

    if ((error = dlerror()) != NULL)  {
      message_warning(_("Unable to find register_sheets in library:\n%s"), error);
      list = g_list_next(list);
      continue;
    }

    (*register_func)();
    
    list = g_list_next(list);
  } 

}

