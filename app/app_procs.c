
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

#include <gtk/gtk.h>
#include <gmodule.h>

#if (defined (HAVE_LIBPOPT) && defined (HAVE_POPT_H)) || defined (GNOME)
#define HAVE_POPT
#endif

#ifdef GNOME
#include <gnome.h>
#else 
#  ifdef HAVE_POPT_H
#    include <popt.h>
#  else
/* sorry about the mess, but one should not use conditional defined types in 
 * unconditional function call in the first place ... --hb */
typedef void* poptContext;
#  endif
#endif

#ifdef HAVE_FREETYPE
#include <pango/pangoft2.h>
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
#include "persistence.h"
#include "sheets.h"
#include "utils.h"

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
extern DiaExportFilter png_export_filter;
#endif

static gboolean
handle_initial_diagram(const char *input_file_name, 
		       const char *export_file_name,
		       const char *export_file_format,
		       const char *size);

static void create_user_dirs(void);
static PluginInitResult internal_plugin_init(PluginInfo *info);
static void process_opts(int argc, char **argv,
#ifdef HAVE_POPT
			 poptContext poptCtx, struct poptOption options[],
#endif
			 GSList **files, char **export_file_name,
			 char **export_file_format, char **size);
static gboolean handle_all_diagrams(GSList *files, char *export_file_name,
				    char *export_file_format, char *size);
static void print_credits(gboolean credits);

static gboolean dia_is_interactive = TRUE;
static void
stderr_message_internal(char *title, const char *fmt,
                        va_list *args,  va_list *args2);

static void
stderr_message_internal(char *title, const char *fmt,
                        va_list *args,  va_list *args2)
{
  static gchar *buf = NULL;
  static gint   alloc = 0;
  gint len;

  len = format_string_length_upper_bound (fmt, args);

  if (len >= alloc) {
    if (buf)
      g_free (buf);
    
    alloc = nearest_pow (MAX(len + 1, 1024));
    
    buf = g_new (char, alloc);
  }
  
  vsprintf (buf, fmt, *args2);
  
  fprintf(stderr,
          "%s: %s\n", 
          title,buf);
}

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

/** Convert infname to outfname, using input filter inf and export filter
 * ef.  If either is null, try to guess them.
 * size might be NULL.
 */
gboolean
do_convert(const char *infname, 
	   const char *outfname, DiaExportFilter *ef,
	   const char *size)
{
  DiaImportFilter *inf;
  DiagramData *diagdata = NULL;

  inf = filter_guess_import_filter(infname);
  if (!inf) 
    inf = &dia_import_filter;

  if (ef == NULL) {
    ef = filter_guess_export_filter(outfname);
    if (!ef) {
      g_error(_("%s error: don't know how to export into %s\n"),
	      argv0,outfname);
      exit(1);
    }
  }

  dia_is_interactive = FALSE;

  if (0==strcmp(infname,outfname)) {
    g_error(_("%s error: input and output file name is identical: %s"),
            argv0, infname);
    exit(1);
  }
  
  diagdata = new_diagram_data(&prefs.new_diagram);
  if (!inf->import_func(infname,diagdata,inf->user_data)) {
    g_error(_("%s error: need valid input file %s\n"),
            argv0, infname);
    exit(1);
  }
  /* Do our best in providing the size to the filter, but don't abuse user_data 
   * too much for it. It _must not_ be changed after initialization and there 
   * are quite some filter selecting their output format by it. --hb
   */
  if (size) {
    if (ef == filter_get_by_name ("png-libart"))
      ef->export_func(diagdata, outfname, infname, size);
    else {
      g_warning ("--size parameter unsupported for %s filter", 
                 ef->unique_name ? ef->unique_name : "selected");
      ef->export_func(diagdata, outfname, infname, ef->user_data);
    }
  }
  else
    ef->export_func(diagdata, outfname, infname, ef->user_data);
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

/** Handle loading of diagrams given on command line, including conversions.
 * Returns TRUE if any automatic conversions were performed.
 * Note to future hackers:  'size' is currently the only argument that can be
 * sent to exporters.  If more arguments are desired, please don't just add
 * even more arguments, but create a more general system.
 */
static gboolean
handle_initial_diagram(const char *in_file_name, 
		       const char *out_file_name,
		       const char *export_file_format,
		       const char *size) {
  DDisplay *ddisp = NULL;
  Diagram *diagram = NULL;
  gboolean made_conversions = FALSE;

  if (export_file_format) {
    char *export_file_name = NULL;
    DiaExportFilter *ef = NULL;

    /* First try guessing based on extension */
    export_file_name = build_output_file_name(in_file_name,
					      export_file_format);

    /* to make the --size hack even uglier but work again for the only filter supporting it */
    if (   size && strcmp(export_file_format, "png") == 0)
      ef = filter_get_by_name ("png-libart");
    if (!ef)
      ef = filter_guess_export_filter(export_file_name);
    if (ef == NULL) {
      ef = filter_get_by_name(export_file_format);
      if (ef == NULL) {
	g_error(_("Can't find output format/filter %s\n"), export_file_format);
	return FALSE;
      }
      g_free (export_file_name);
      export_file_name = build_output_file_name(in_file_name,
						ef->extensions[0]);
    }
    made_conversions |= do_convert(in_file_name,
      (out_file_name != NULL?out_file_name:export_file_name), ef, size);
    g_free(export_file_name);
  } else if (out_file_name) {
    DiaExportFilter *ef = NULL;

    /* if this looks like an ugly hack to you, agreed ;)  */
    if (size && strstr(out_file_name, ".png"))
      ef = filter_get_by_name ("png-libart");
    
    made_conversions |= do_convert(in_file_name, out_file_name, ef,
				   size);
  } else {
    if (g_file_test(in_file_name, G_FILE_TEST_EXISTS)) {
      diagram = diagram_load (in_file_name, NULL);
    } else
      diagram = new_diagram (in_file_name);
	      
    if (diagram != NULL) {
      diagram_update_extents(diagram);
      if (app_is_interactive()) {
	layer_dialog_set_diagram(diagram);
	ddisp = new_display(diagram);
      }
    }
  }
  return made_conversions;
}

#ifdef G_OS_WIN32
static void
myXmlErrorReporting (void *ctx, const char* msg, ...)
{
  va_list args;
  gchar *string;

  va_start(args, msg);
  string = g_strdup_vprintf (msg, args);
  g_print ("%s", string ? string : "xml error (null)?");
  va_end(args);

  g_free(string);
}
#endif

#ifdef HAVE_FREETYPE
/* Translators:  This is an option, not to be translated */
#define EPS_PANGO "eps-pango, "
#else
#define EPS_PANGO ""
#endif

#ifdef G_OS_WIN32
/* Translators:  This is an option, not to be translated */
#define WMF "wmf, "
#else
#define WMF ""
#endif


void
app_init (int argc, char **argv)
{
  gboolean nosplash = FALSE;
  gboolean credits = FALSE;
  gboolean version = FALSE;
  gboolean log_to_stderr = FALSE;
#ifdef GNOME
  GnomeClient *client;
#endif
  char *export_file_name = NULL;
  char *export_file_format = NULL;
  char *size = NULL;
  gboolean made_conversions = FALSE;
  GSList *files = NULL;

#ifdef HAVE_POPT
  poptContext poptCtx = NULL;
  gchar *export_format_string = 
     /* Translators:  The argument is a list of options, not to be translated */
    g_strdup_printf(_("Select the filter/format.  Supported are: %s"),
		    "cgm, dia, dxf, eps, " EPS_PANGO
		    "fig, mp, plt, hpgl, " "png ("
#  if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
		    "png-libart, "
#  endif
#  ifdef HAVE_CAIRO
		    "png-cairo, "
#  endif
		    /* we always have pixbuf but don't know exactly all it's *few* save formats */
		    "gdkpixbuf), jpg, ras, wbmp, "
		    "shape, svg, tex, " WMF
		    "wpg");

  struct poptOption options[] =
  {
    {"export", 'e', POPT_ARG_STRING, NULL /* &export_file_name */, 0,
     N_("Export loaded file and exit"), N_("OUTPUT")},
    {"filter",'t', POPT_ARG_STRING, NULL /* &export_file_format */,
     0, export_format_string, N_("TYPE")
    },
    {"size", 's', POPT_ARG_STRING, NULL, 0,
     N_("Export graphics size"), N_("WxH")},
    {"nosplash", 'n', POPT_ARG_NONE, &nosplash, 0,
     N_("Don't show the splash screen"), NULL },
    {"log-to-stderr", 'l', POPT_ARG_NONE, &log_to_stderr, 0,
     N_("Send error messages to stderr instead of showing dialogs."), NULL },
    {"credits", 'c', POPT_ARG_NONE, &credits, 0,
     N_("Display credits list and exit"), NULL },
    {"version", 'v', POPT_ARG_NONE, &version, 0,
     N_("Display version and exit"), NULL },
    {"help", 'h', POPT_ARG_NONE, 0, 1, N_("Show this help message") },
    {(char *) NULL, '\0', 0, NULL, 0}
  };
#endif

#ifdef HAVE_POPT
  options[0].arg = &export_file_name;
  options[1].arg = &export_file_format;
  options[2].arg = &size;
#endif

  argv0 = (argc > 0) ? argv[0] : "(none)";

  gtk_set_locale();
  setlocale(LC_NUMERIC, "C");
  
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  textdomain(GETTEXT_PACKAGE);

  process_opts(argc, argv, 
#ifdef HAVE_POPT
               poptCtx, options, 
#endif
               &files,
	     &export_file_name, &export_file_format, &size);

#if defined ENABLE_NLS && defined HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(GETTEXT_PACKAGE,"UTF-8");  
#endif
  textdomain(GETTEXT_PACKAGE);

  if (argv && dia_is_interactive && !version) {
#ifdef GNOME
    GnomeProgram *program =
      gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
			  argc, argv, GNOME_PARAM_POPT_TABLE, options,
			  GNOME_PROGRAM_STANDARD_PROPERTIES,
			  GNOME_PARAM_NONE);
    g_object_get(program, "popt-context", &poptCtx, NULL);
    client = gnome_master_client();
    if(client == NULL) {
      g_warning(_("Can't connect to session manager!\n"));
    }
    else {
      g_signal_connect(GTK_OBJECT (client), "save_yourself",
		       G_CALLBACK (save_state), NULL);
      g_signal_connect(GTK_OBJECT (client), "die",
		       G_CALLBACK (session_die), NULL);
    }

    /* This smaller icon is 48x48, standard Gnome size */
    gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/dia_gnome_icon.png");

#else
    gtk_init(&argc, &argv);
#endif
  }
  else
    g_type_init();

#ifdef HAVE_POPT
  /* done with option parsing, don't leak */
  g_free(export_format_string);
#endif
  
  if (version) {
#if (defined __TIME__) && (defined __DATE__)
    /* TRANSLATOR: 2nd and 3rd %s are time and date respectively. */
    printf(g_locale_from_utf8(_("Dia version %s, compiled %s %s\n"), -1, NULL, NULL, NULL), VERSION, __TIME__, __DATE__);
#else
    printf(g_locale_from_utf8(_("Dia version %s\n"), -1, NULL, NULL, NULL), VERSION);
#endif
    exit(0);
  }

  if (!dia_is_interactive)
    log_to_stderr = TRUE;
  else {
#ifdef G_OS_WIN32
    dia_redirect_console ();
#endif
  }
  
  if (log_to_stderr)
    set_message_func(stderr_message_internal);

  print_credits(credits);

  LIBXML_TEST_VERSION;

#ifdef G_OS_WIN32
  xmlSetGenericErrorFunc(NULL, myXmlErrorReporting);
#endif

  stdprops_init();

  if (dia_is_interactive) {
    dia_image_init();

    gdk_rgb_init();

    gtk_rc_parse("diagtkrc"); 

    if (!nosplash) {
      app_splash_init("");
    }
  }

  create_user_dirs();

  /* Init cursors: */
  if (dia_is_interactive) {
    color_init();
    default_cursor = gdk_cursor_new(GDK_LEFT_PTR);
    ddisplay_set_all_cursor(default_cursor);
  }

  object_registry_init();

  dia_register_plugins();
  dia_register_builtin_plugin(internal_plugin_init);

  load_all_sheets();     /* new mechanism */

  dia_object_defaults_load (NULL, TRUE /* prefs.object_defaults_create_lazy */);

  debug_break();

  if (object_get_type("Standard - Box") == NULL) {
    message_error(_("Couldn't find standard objects when looking for "
		  "object-libs, exiting...\n"));
    g_error( _("Couldn't find standard objects when looking for "
	    "object-libs, exiting...\n"));
    exit(1);
  }

  persistence_load();

  /** Must load prefs after persistence */
  prefs_init();

  if (dia_is_interactive) {

    /* further initialization *before* reading files */  
    active_tool = create_modify_tool();

    create_toolbox();

    persistence_register_window_create("layer_window",
				       (NullaryFunc*)&create_layer_dialog);


    /*fill recent file menu */
    recent_file_history_init();

    /* Set up autosave to check every 5 minutes */
    gtk_timeout_add(5*60*1000, autosave_check_autosave, NULL);

    create_tree_window();

    persistence_register_window_create("sheets_main_dialog",
				       (NullaryFunc*)&sheets_dialog_create);


    /* In current setup, we can't find the autosaved files. */
    /*autosave_restore_documents();*/

  }

  made_conversions = handle_all_diagrams(files, export_file_name,
					 export_file_format, size);
  if (dia_is_interactive && files == NULL) {
    gchar *filename = g_filename_from_utf8(_("Diagram1.dia"), -1, NULL, NULL, NULL);
    Diagram *diagram = new_diagram (filename);
    g_free(filename);
	
	      
    if (diagram != NULL) {
      diagram_update_extents(diagram);
      if (app_is_interactive()) {
	layer_dialog_set_diagram(diagram);
	new_display(diagram);
      }
    }
  }
  g_slist_free(files);
  if (made_conversions) exit(0);

  dynobj_refresh_init();
}

#if 0
/* app_procs.c: warning: `set_true_callback' defined but not used */
static void
set_true_callback(GtkWidget *w, int *data)
{
  *data = TRUE;
}
#endif

gboolean
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

  if (app_exit_once) {
    g_error(_("This shouldn't happen.  Please file a bug report at bugzilla.gnome.org\ndescribing how you can cause this message to appear.\n"));
    return FALSE;
  }

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

    gtk_widget_show_all (dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
      gtk_widget_destroy(dialog);
      return FALSE;
    }
    gtk_widget_destroy(dialog);
  }
  prefs_save();

  persistence_save();

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

  gtk_main_quit();
#ifndef G_OS_WIN32
  /* This printf seems to prevent a race condition with unrefs. */
  /* Yuck.  -Lars */
  g_print(_("Thank you for using Dia.\n"));
#endif
  app_exit_once = TRUE;

  return TRUE;
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
  /* Standard Dia format */
  filter_register_export(&dia_export_filter);
  /* EPS with PS fonts */
  filter_register_export(&eps_export_filter);
#ifdef HAVE_FREETYPE
  /* EPS with Pango rendering */
  filter_register_export(&eps_ft2_export_filter);
#endif
#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
  /* PNG with libart rendering */
  filter_register_export(&png_export_filter);
#endif

  return DIA_PLUGIN_INIT_OK;
}

/* Note: running in locale encoding */
static void
process_opts(int argc, char **argv,
#ifdef HAVE_POPT
	     poptContext poptCtx, struct poptOption options[],
#endif
	     GSList **files, char **export_file_name,
	     char **export_file_format, char **size)
{
#ifdef HAVE_POPT
  int rc = 0;
  poptCtx = poptGetContext(PACKAGE, argc, (const char **)argv, options, 0);
  poptSetOtherOptionHelp(poptCtx, _("[OPTION...] [FILE...]"));
  while (rc >= 0) {
    if((rc = poptGetNextOpt(poptCtx)) < -1) {
      fprintf(stderr,_("Error on option %s: %s.\nRun '%s --help' to see a full list of available command line options.\n"),
	      poptBadOption(poptCtx, 0),
	      poptStrerror(rc),
	      argv[0]);
      exit(1);
    }
    if(rc == 1) {
      poptPrintHelp(poptCtx, stderr, 0);
      exit(0);
    }
  }
#endif
  if (argv) {
#ifdef HAVE_POPT
      while (poptPeekArg(poptCtx)) {
          char *in_file_name = (char *)poptGetArg(poptCtx);
	  if (*in_file_name != '\0')
	    *files = g_slist_append(*files, in_file_name);
      }
      poptFreeContext(poptCtx);
#else
      int i;

      for (i=1; i<argc; i++) {
          char *in_file_name = argv[i]; /* unless it's an option... */

          if (0==strcmp(argv[i],"-t")) {
              if (i < (argc-1)) {
                  i++;
                  *export_file_format = argv[i];
                  continue;
              }
          } else if (0 == strcmp(argv[i],"-e")) {
              if (i < (argc-1)) {
                  i++;
                  *export_file_name = argv[i];
                  continue;
              }
          } else if (0 == strcmp(argv[i],"-s")) {
              if (i < (argc-1)) {
                  i++;
                  *size = argv[i];
                  continue;
              }
          }
	  *files = g_slist_append(*files, in_file_name);
      }
#endif
  }
  if (*export_file_name || *export_file_format || *size)
    dia_is_interactive = FALSE;
}

static gboolean
handle_all_diagrams(GSList *files, char *export_file_name,
		    char *export_file_format, char *size)
{
  GSList *node = NULL;
  gboolean made_conversions = FALSE;

  for (node = files; node; node = node->next) {
    made_conversions |=
      handle_initial_diagram(node->data, export_file_name,
			     export_file_format, size);
  }
  return made_conversions;
}

/* --credits option. Added by Andrew Ferrier.
  
   Hopefully we're not ignoring anything too crucial by
   quitting directly after the credits.

   The phrasing of the English here might need changing
   if we switch from plural to non-plural (say, only
   one maintainer).
*/
static void
print_credits(gboolean credits)
{
  if (credits) {
      int i;
      const gint nauthors = (sizeof(authors) / sizeof(authors[0])) - 1;
      const gint ndocumentors = (sizeof(documentors) / sizeof(documentors[0])) - 1;

      g_print(_("The original author of Dia was:\n\n"));
      for (i = 0; i < NUMBER_OF_ORIG_AUTHORS; i++) {
          g_print(authors[i]); g_print("\n");
      }

      g_print(_("\nThe current maintainers of Dia are:\n\n"));
      for (i = NUMBER_OF_ORIG_AUTHORS; i < NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i++) {
	  g_print(authors[i]); g_print("\n");
      }

      g_print(_("\nOther authors are:\n\n"));
      for (i = NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i < nauthors; i++) {
          g_print(authors[i]); g_print("\n");
      }

      g_print(_("\nDia is documented by:\n\n"));
      for (i = 0; i < ndocumentors; i++) {
          g_print(documentors[i]); g_print("\n");
      }

      exit(0);
  }
}

/* parses a string of the form "[0-9]*x[0-9]*" and transforms it into
   two long values width and height. */
void
parse_size(gchar *size, long *width, long *height)
{
  if (size) {
    gchar** array = g_strsplit(size, "x", 3);
    *width  = (array[0])? strtol(array[0], NULL, 10): 0;
    *height = (array[1])? strtol(array[1], NULL, 10): 0;
    g_strfreev(array);
  }
  else {
    *width  = 0;
    *height = 0;
  }
}
