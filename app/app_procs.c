/* xxxxxx -- an diagram creation/manipulation program
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

#include "app_procs.h"
#include "object.h"
#include "color.h"
#include "tool.h"
#include "modify_tool.h"
#include "interface.h"
#include "group.h"
#include "message.h"
#include "display.h"


static void register_all_objects(void);
static int name_is_lib(char *name);

static void
enable_core_dumps(void)
{
  signal (SIGHUP, SIG_DFL);
  signal (SIGINT, SIG_DFL);
  signal (SIGQUIT, SIG_DFL);
  signal (SIGBUS, SIG_DFL);
  signal (SIGSEGV, SIG_DFL);
  signal (SIGPIPE, SIG_DFL);
  signal (SIGTERM, SIG_DFL);
}

void
app_init (int    argc,
           char **argv)
{
  gtk_set_locale ();
  setlocale(LC_NUMERIC, "C");

  gtk_init (&argc, &argv);

  gtk_rc_parse ("diagtkrc"); 

  /*  enable_core_dumps(); */

  color_init();

  font_init();

  /* Init cursors: */
  default_cursor = gdk_cursor_new(GDK_LEFT_PTR);
  ddisplay_set_all_cursor(default_cursor);

  object_registry_init();

  register_all_objects();

  if (object_get_type("Standard - Box") == NULL) {
    message_error("Couldn't find standard objects when looking for object-libs, exiting...\n");
    printf("Couldn't find standard objects when looking for object-libs, exiting...\n");
    exit(1);
  }
    
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
  
  if (diagram_modified_exists()) {
    GtkWidget *dialog;
    GtkWidget *label;
    GtkWidget *button;
    int result = FALSE;

    dialog = gtk_dialog_new();
  
    gtk_signal_connect (GTK_OBJECT (dialog), "destroy", 
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    
    gtk_window_set_title (GTK_WINDOW (dialog), "Quit, are you sure?");
    gtk_container_border_width (GTK_CONTAINER (dialog), 0);
    label = gtk_label_new ("Modified diagrams exists.\n"
			   "Are you sure you want to quit?");
  
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
			label, TRUE, TRUE, 0);
  
    gtk_widget_show (label);

    button = gtk_button_new_with_label ("Yes");
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
    
    button = gtk_button_new_with_label ("No");
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

static void
register_objects_in(char *directory)
{
  struct stat statbuf;
  char file_name[256];
  struct dirent *dirp;
  DIR *dp;
  char *error;
  void *libhandle;
  void (*register_func)(void);
  
  if (stat(directory, &statbuf)<0) {
    /*
      message_notice("Couldn't find any libraries in %s.\n", directory);
    */
    return;
  }

  if (!S_ISDIR(statbuf.st_mode)) {
    message_warning("Couldn't find any libraries to load. %s is not a directory.\n", directory);
    return;
  }

  dp = opendir(directory);
  if ( dp == NULL ) {
    message_warning("Couldn't open \"%s\".\n", directory);
    return;
  }
  
  while ( (dirp = readdir(dp)) ) {
    if ( name_is_lib(dirp->d_name) ) {
      strncpy(file_name, directory, 256);
      strncat(file_name, "/", 256);
      strncat(file_name, dirp->d_name, 256);
      /*   printf("loading library: \"%s\"\n", file_name); */
      
      libhandle = dlopen(file_name, RTLD_NOW);
      if (libhandle==NULL) {
	message_warning("Error loading library: \"%s\":\n %s\n", file_name, dlerror());
	continue;
      }
      
      register_func = dlsym(libhandle, "register_objects");
      if ((error = dlerror()) != NULL)  {
	message_warning("Error loading library: \"%s\":\n %s\n", file_name, error);
	continue;
      }
      
      (*register_func)();

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

