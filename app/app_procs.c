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
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef GNOME
#undef GTK_DISABLE_DEPRECATED /* gtk_signal_connect[_object] */
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

#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#ifdef G_OS_WIN32
#include <direct.h>
#define mkdir(s,a) _mkdir(s)
#endif

#include "intl.h"
#include "app_procs.h"
#include "object.h"
#include "color.h"
#include "tool.h"
#include "interface.h"
#include "modify_tool.h"
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
#include "authors.h"
#include "autosave.h"
#include "dynamic_refresh.h"
#include "dia_image.h"

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
extern DiaExportFilter png_export_filter;
#endif

static void create_user_dirs(void);
static PluginInitResult internal_plugin_init(PluginInfo *info);

static gboolean dia_is_interactive = TRUE;

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

  for(l = dia_open_diagrams(); l != NULL; l = g_list_next(l)) {
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

char *
build_output_file_name(const char *infname, const char *format)
{
  /* FIXME: probably overly confident... */
  char *p = strrchr(infname,'.');
  char *tmp;
  if (!p) {
    return g_strconcat(infname,".",format,NULL);
  }
  tmp = g_malloc0(strlen(infname)+1+strlen(format)+1);
  memcpy(tmp,infname,p-infname);
  strcat(tmp,".");
  strcat(tmp,format);
  return tmp;
}

const char *argv0 = NULL;

gboolean 
do_convert(const char *infname,
           const char *outfname)
{
  DiaExportFilter *ef = NULL;
  DiaImportFilter *inf = NULL;
  DiagramData *diagdata = NULL;

  dia_is_interactive = FALSE;

  if (0==strcmp(infname,outfname)) {
    fprintf(stderr,
            _("%s error: input and output file name is identical: %s"),
            argv0, infname);
    exit(1);
  }
  
  diagdata = new_diagram_data(&prefs.new_diagram);
  inf = filter_guess_import_filter(infname);
  if (!inf) 
    inf = &dia_import_filter;
  if (!inf->import(infname,diagdata,inf->user_data)) {
    fprintf(stderr,
            _("%s error: need valid input file %s\n"),
            argv0,infname);
            exit(1);
  }
  ef = filter_guess_export_filter(outfname);
  if (!ef) {
    fprintf(stderr,
            _("%s error: don't know how to export into %s\n"),
            argv0,outfname);
            exit(1);
  }
  ef->export(diagdata, outfname, infname, ef->user_data);
  /* if (!quiet) */ fprintf(stdout,
                      _("%s --> %s\n"),
                        infname,outfname);
  diagram_data_destroy(diagdata);
  return TRUE;
}

void debug_break(void); /* shut gcc up */
int debug_break_dont_optimize = 1;
void
debug_break(void)
{
  if (debug_break_dont_optimize > 0) 
    debug_break_dont_optimize -= 1;
}

gboolean
app_is_interactive(void)
{
  return dia_is_interactive;
}

#ifdef G_OS_WIN32
static void
myXmlErrorReporting (void *ctx, const char* msg, ...)
{
  va_list args;
  gchar *string;

  va_start(args, msg);
  string = g_strdup_vprintf (msg, args);
  g_print (string);
  va_end(args);

  g_free(string);
}
#endif

void
app_init (int argc, char **argv)
{
  gboolean nosplash = FALSE;
  gboolean credits = FALSE;
  gboolean version = FALSE;
#ifdef GNOME
  GnomeClient *client;
#endif
  char *export_file_name = NULL;
  char *export_file_format = NULL;
  gboolean made_conversions = FALSE;

#ifdef HAVE_POPT
#ifndef GNOME
  int rc = 0;
#endif  
  poptContext poptCtx = NULL;
  struct poptOption options[] =
  {
    {"export", 'e', POPT_ARG_STRING, NULL /* &export_file_name */, 0,
     N_("Export loaded file and exit"), N_("OUTPUT")},
    {"export-to-format",'t', POPT_ARG_STRING, NULL /* &export_file_format */,
     0, N_("Export to file format and exit"), N_("eps,png,wmf,cgm,dxf,fig")},
    {"nosplash", 'n', POPT_ARG_NONE, &nosplash, 0,
     N_("Don't show the splash screen"), NULL },
    {"credits", 'c', POPT_ARG_NONE, &credits, 0,
     N_("Display credits list and exit"), NULL },
    {"version", 'v', POPT_ARG_NONE, &version, 0,
     N_("Display version and exit"), NULL },
#ifndef GNOME
    {"help", 'h', POPT_ARG_NONE, 0, 1, N_("Show this help message") },
#endif
    {(char *) NULL, '\0', 0, NULL, 0}
  };
#endif

#ifdef HAVE_POPT
  options[0].arg = &export_file_name;
  options[1].arg = &export_file_format;
#endif

  argv0 = (argc > 0) ? argv[0] : "(none)";
  
  gtk_set_locale();
  setlocale(LC_NUMERIC, "C");
  
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
#if defined ENABLE_NLS
  bind_textdomain_codeset(GETTEXT_PACKAGE,"UTF-8");  
#endif
  textdomain(GETTEXT_PACKAGE);

  if (argv) {
#ifdef GNOME
    GnomeProgram *program =
      gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
			  argc, argv, GNOME_PARAM_POPT_TABLE, options,
			  GNOME_PARAM_NONE);
    g_object_get(program, "popt-context", &poptCtx, NULL);
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

    /* This smaller icon is 48x48, standard Gnome size */
    gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/dia_gnome_icon.png");
#else
    gtk_init (&argc, &argv);
#ifdef HAVE_POPT
    poptCtx = poptGetContext(PACKAGE, argc, (const char **)argv, options, 0);
    poptSetOtherOptionHelp(poptCtx, _("[OPTION...] [FILE...]"));
    while (rc >= 0) {
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

        if (export_file_format && export_file_name) {
            fprintf(stderr,
                    _("%s error: can specify only one of -f or -o."),
                    argv[0]);
            exit(1);
        }

    }
#endif
#endif
  }

  if (version) {
#if (defined __TIME__) && (defined __DATE__)
    /* TRANSLATOR: 2nd and 3rd %s are time and date respectively. */
    printf(_("Dia version %s, compiled %s %s\n"), VERSION, __TIME__, __DATE__);
#else
    printf(_("Dia version %s\n"), VERSION);
#endif
    exit(0);
  }

  /* --credits option. Added by Andrew Ferrier.
  
     Hopefully we're not ignoring anything too crucial by
     quitting directly after the credits.

     The phrasing of the English here might need changing
     if we switch from plural to non-plural (say, only
     one maintainer).
  */

  if (credits) {
      int i;
      const gint nauthors = (sizeof(authors) / sizeof(authors[0])) - 1;
      const gint ndocumentors = (sizeof(documentors) / sizeof(documentors[0])) - 1;

      printf("The original author of Dia was:\n\n");
      for (i = 0; i < NUMBER_OF_ORIG_AUTHORS; i++) {
          printf(authors[i]); printf("\n");
      }

      printf("\nThe current maintainers of Dia are:\n\n");
      for (i = NUMBER_OF_ORIG_AUTHORS; i < NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i++) {
	  printf(authors[i]); printf("\n");
      }

      printf("\nOther authors are:\n\n");
      for (i = NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i < nauthors; i++) {
          printf(authors[i]); printf("\n");
      }

      printf("\nDia is documented by:\n\n");
      for (i = 0; i < ndocumentors; i++) {
          printf(documentors[i]); printf("\n");
      }

      exit(0);
  }

  LIBXML_TEST_VERSION;

#ifdef G_OS_WIN32
  xmlSetGenericErrorFunc(NULL, myXmlErrorReporting);
#endif

  stdprops_init();

  dia_image_init();

  
  gdk_rgb_init();

  gtk_rc_parse ("diagtkrc"); 

  if (!nosplash)
    app_splash_init("");

  create_user_dirs();

  color_init();

#ifdef HAVE_FREETYPE
  dia_font_init(pango_ft2_get_context(10,10));
#else
  dia_font_init(gdk_pango_context_get());
#endif

  /* Init cursors: */
  default_cursor = gdk_cursor_new(GDK_LEFT_PTR);
  ddisplay_set_all_cursor(default_cursor);

  object_registry_init();

  dia_register_plugins();
  dia_register_builtin_plugin(internal_plugin_init);

  load_all_sheets();     /* new mechanism */

  dia_object_defaults_load (NULL, TRUE /* prefs.object_defaults_create_lazy */);

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

  /* Set up autosave to check every 5 minutes */
  gtk_timeout_add(5*60*1000, autosave_check_autosave, NULL);

  create_tree_window();

  /* In current setup, we can't find the autosaved files. */
  /*autosave_restore_documents();*/
  
  if (argv) {
#ifdef HAVE_POPT
      while (poptPeekArg(poptCtx)) {
          Diagram *diagram = NULL;
          DDisplay *ddisp = NULL;
          char *in_file_name = (char *)poptGetArg(poptCtx);

          if (export_file_name) {
              made_conversions |= do_convert(in_file_name,export_file_name);
          } else if (export_file_format) {
              export_file_name = build_output_file_name(in_file_name,
                                                        export_file_format);
              made_conversions |= do_convert(in_file_name,export_file_name);
              g_free(export_file_name);
          } else {
              diagram = diagram_load (in_file_name, NULL);
              if (diagram != NULL) {
                  diagram_update_extents(diagram);
                  layer_dialog_set_diagram(diagram);
                  
                  ddisp = new_display(diagram);
              }
          }
      }
      poptFreeContext(poptCtx);
#else
      int i;

      for (i=1; i<argc; i++) {
          Diagram *diagram = NULL;
          DDisplay *ddisp;
          char *in_file_name = argv[i]; /* unless it's an option... */
          
          if (0==strcmp(argv[i],"-t")) {
              if (i < (argc-1)) {
                  i++;
                  export_file_format = argv[i];
                  continue;
              }
          } else if (0 == strcmp(argv[i],"-e")) {
              if (i < (argc-1)) {
                  i++;
                  export_file_name = argv[i];
                  continue;
              }
          }
          
          if (export_file_name) {
              made_conversions |= do_convert(in_file_name,export_file_name);
          } else if (export_file_format) {
              export_file_name = build_output_file_name(in_file_name,
                                                        export_file_format);
              made_conversions |= do_convert(in_file_name,export_file_name);
              g_free(export_file_name);
          } else {
              diagram = diagram_load(in_file_name, NULL);
              
              if (diagram != NULL) {
                  diagram_update_extents(diagram);
                  layer_dialog_set_diagram(diagram);
                  
                  ddisp = new_display(diagram);
              }
              /* Error messages are done in diagram_load() */
          }
      }
#endif
  }
  if (made_conversions) exit(0);

  dynobj_refresh_init();
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

  /*
   * The following "solves" a crash related to a second call of app_exit,
   * after gtk_main_quit was called. It may be a win32 gtk-1.3.x bug only
   * but the check shouldn't hurt on *ix either.          --hb
   */
  static gboolean app_exit_once = FALSE;

  g_return_if_fail (!app_exit_once);

  if (diagram_modified_exists()) {
    GtkWidget *dialog;
    GtkWidget *button;

    dialog = gtk_message_dialog_new(
	       NULL, GTK_DIALOG_MODAL,
               GTK_MESSAGE_QUESTION,
               GTK_BUTTONS_NONE, /* no standard buttons */
	       _("Modified diagrams exist.\n"
           "Are you sure you want to quit Dia\n"
           "without saving them?"),
	       NULL);
    gtk_window_set_title (GTK_WINDOW(dialog), _("Quit Dia"));

    button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    gtk_dialog_add_action_widget (GTK_DIALOG(dialog), button, GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

    button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
    gtk_dialog_add_action_widget (GTK_DIALOG(dialog), button, GTK_RESPONSE_OK);

    g_signal_connect_after (G_OBJECT (dialog), "response",
		            G_CALLBACK(gtk_widget_destroy),
		            NULL);
    gtk_widget_show_all (dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK)
      return;
  }

  dynobj_refresh_finish();
    
  dia_object_defaults_save (NULL);

  /* Free loads of stuff (toolbox) */

  list = dia_open_diagrams();
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

