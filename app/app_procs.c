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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_UNICODE
#include <unicode.h>
#endif 

#include <gtk/gtk.h>
#include <gmodule.h>

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

#include <parser.h>

#ifdef G_OS_WIN32
#include <direct.h>
#define mkdir(s,a) _mkdir(s)
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
#include "dia_dirs.h"
#include "render_eps.h"
#include "sheet.h"
#include "plug-ins.h"
#include "recent_files.h"

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
extern DiaExportFilter png_export_filter;
#endif

static void create_user_dirs(void);
static PluginInitResult internal_plugin_init(PluginInfo *info);

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

void debug_break(void); /* shut gcc up */
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
  gboolean nosplash = FALSE;
#ifdef GNOME
  GnomeClient *client;
#endif
  char *in_file_name = NULL;
  char *export_file_name = NULL;
#ifdef HAVE_POPT
#ifndef GNOME
  int rc;
#endif
  poptContext poptCtx = NULL;
  struct poptOption options[] =
  {
    {"export", 'e', POPT_ARG_STRING, NULL /* &export_file_name */, 0,
     N_("Export loaded file and exit"), N_("OUTPUT")},
    {"nosplash", 0, POPT_ARG_NONE, &nosplash, 0,
     N_("Don't show the splash screen"), NULL },
#ifndef GNOME
    {"help", 'h', POPT_ARG_NONE, 0, 1, N_("Show this help message") },
#endif
    {(char *) NULL, '\0', 0, NULL, 0}
  };
#endif

#ifdef HAVE_POPT
  options[0].arg = &export_file_name;
#endif

  gtk_set_locale();
  setlocale(LC_NUMERIC, "C");
  
#ifdef HAVE_UNICODE
  unicode_init();
#endif 

  bindtextdomain(PACKAGE, LOCALEDIR);
#if defined GTK_TALKS_UTF8 && defined ENABLE_NLS
  bind_textdomain_codeset(PACKAGE,"UTF-8");  
#endif
  textdomain(PACKAGE);

  if (argv) {
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
    poptCtx = poptGetContext(PACKAGE, argc, (const char **)argv, options, 0);
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
#endif
  }
   
  LIBXML_TEST_VERSION;

  stdprops_init();

  dia_image_init();

  gdk_rgb_init();

  gtk_rc_parse ("diagtkrc"); 

  if (!nosplash)
    app_splash_init("");

  create_user_dirs();

  color_init();

  font_init();

  /* Init cursors: */
  default_cursor = gdk_cursor_new(GDK_LEFT_PTR);
  ddisplay_set_all_cursor(default_cursor);

  object_registry_init();

  dia_register_plugins();
  dia_register_builtin_plugin(internal_plugin_init);

  load_all_sheets();     /* new mechanism */

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

  /* further initialization *before* reading files */  
  active_tool = create_modify_tool();

  create_toolbox();

  /*fill recent file menu */
  recent_file_history_init();

  if (argv) {
#ifdef HAVE_POPT
    while (poptPeekArg(poptCtx)) {
      in_file_name = (char *)poptGetArg(poptCtx);
      diagram = diagram_load (in_file_name, NULL);
      if (export_file_name) {
	DiaExportFilter *ef;
	if (!diagram) {
	  fprintf (stderr, _("Need valid input file\n"));
	  exit (1);
	}
	ef = filter_guess_export_filter(export_file_name);
	if (!ef)
	  ef = &eps_export_filter;
	ef->export(diagram->data, export_file_name, in_file_name, ef->user_data);
	exit (0);
      }
      if (diagram != NULL) {
	diagram_update_extents(diagram);
	ddisp = new_display(diagram);
      }
    }
    poptFreeContext(poptCtx);
#else
    int i;
    for (i=1; i<argc; i++) {
      Diagram *diagram;
      DDisplay *ddisp;
      
      diagram = diagram_load(argv[i], NULL);
      
      if (diagram != NULL) {
	diagram_update_extents(diagram);
	layer_dialog_set_diagram(diagram);
	
	ddisp = new_display(diagram);
      }
      /* Error messages are done in diagram_load() */
    }
#endif
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
  GList *list;
  GSList *slist;
  gchar *filename;
  /*
   * The following "solves" a crash related to a second call of app_exit,
   * after gtk_main_quit was called. It may be a win32 gtk-1.3.x bug only
   * but the check shouldn't hurt on *ix either.          --hb
   */
  static gboolean app_exit_once = FALSE;

  g_return_if_fail (!app_exit_once);
  
  if (diagram_modified_exists()) {
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *label;
#ifndef GNOME
    GtkWidget *button;
#endif
    int result = FALSE;

#ifdef GNOME
    dialog = gnome_dialog_new(_("Quit, are you sure?"), NULL);
    vbox = GNOME_DIALOG(dialog)->vbox;
    gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);
#else
    dialog = gtk_dialog_new();
    vbox = GTK_DIALOG(dialog)->vbox;
    gtk_window_set_title (GTK_WINDOW (dialog), _("Quit, are you sure?"));
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 0);
#endif

    gtk_signal_connect (GTK_OBJECT (dialog), "destroy", 
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    
    label = gtk_label_new (_("Modified diagrams exists.\n"
			   "Are you sure you want to quit?"));
  
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  
    gtk_widget_show (label);

#ifdef GNOME
    gnome_dialog_append_button_with_pixmap(GNOME_DIALOG(dialog),
					   _("Quit"), GNOME_STOCK_PIXMAP_QUIT);
    gnome_dialog_append_button(GNOME_DIALOG(dialog),GNOME_STOCK_BUTTON_CANCEL);

    result = (gnome_dialog_run(GNOME_DIALOG(dialog)) == 0);
#else
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
#endif

    if (result == FALSE)
      return;
  }
  
  /* Save menu accelerators */
  filename = dia_config_filename("menus" G_DIR_SEPARATOR_S "toolbox");
  if (filename!=NULL) {
    GtkPatternSpec pattern;

    gtk_pattern_spec_init(&pattern, "*<Toolbox>*");

    gtk_item_factory_dump_rc (filename, &pattern, TRUE);
    g_free (filename);
    gtk_pattern_spec_free_segs(&pattern);
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
  
  /* save pluginrc */
  dia_pluginrc_write();

  /* save recent file history */
  recent_file_history_write();
  
  gtk_main_quit();
  app_exit_once = TRUE;
}

static void create_user_dirs(void)
{
  gchar *dir, *subdir;

#ifdef G_OS_WIN32
  /* not necessary to quit the program with g_error, everywhere else
   * dia_config_filename appears to be used. Spit out a warning ...
   */
  if (!g_get_home_dir())
  {
    g_warning(_("Could not create per-user Dia config directory"));
    return; /* ... and return. Probably removes my one and only FAQ. --HB */
  }
#endif
  dir = g_strconcat(g_get_home_dir(), G_DIR_SEPARATOR_S ".dia", NULL);
  if (mkdir(dir, 0755) && errno != EEXIST)
#ifndef G_OS_WIN32
    g_error(_("Could not create per-user Dia config directory"));
#else /* HB: it this really a reason to exit the program on *nix ? */
    g_warning(_("Could not create per-user Dia config directory. Please make "
        "sure that the environment variable HOME points to an existing directory."));
#endif

  /* it is no big deal if these directories can't be created */
  subdir = g_strconcat(dir, G_DIR_SEPARATOR_S "objects", NULL);
  mkdir(subdir, 0755);
  g_free(subdir);
  subdir = g_strconcat(dir, G_DIR_SEPARATOR_S "shapes", NULL);
  mkdir(subdir, 0755);
  g_free(subdir);
  subdir = g_strconcat(dir, G_DIR_SEPARATOR_S "sheets", NULL);
  mkdir(subdir, 0755);
  g_free(subdir);

  g_free(dir);
}

static PluginInitResult
internal_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Internal",
			    _("Objects and filters internal to dia"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  /* register the group object type */
  object_register_type(&group_type);

  /* register import filters */
  filter_register_import(&dia_import_filter);

  /* register export filters */
  filter_register_export(&dia_export_filter);
  filter_register_export(&eps_export_filter);
#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
  filter_register_export(&png_export_filter);
#endif

  return DIA_PLUGIN_INIT_OK;
}

