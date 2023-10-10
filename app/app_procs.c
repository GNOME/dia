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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include <gmodule.h>

#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include <glib/gstdio.h>

#include "app_procs.h"
#include "object.h"
#include "commands.h"
#include "tool.h"
#include "interface.h"
#include "modify_tool.h"
#include "group.h"
#include "message.h"
#include "display.h"
#include "layer-editor/layer_dialog.h"
#include "load_save.h"
#include "preferences.h"
#include "dia_dirs.h"
#include "sheet.h"
#include "plug-ins.h"
#include "recent_files.h"
#include "authors.h"
#include "autosave.h"
#include "dynamic_refresh.h"
#include "persistence.h"
#include "sheet-editor/sheets.h"
#include "exit_dialog.h"
#include "dialib.h"
#include "dia-layer.h"
#include "dia-version-info.h"

static gboolean         handle_initial_diagram (const char *input_file_name,
                                                const char *export_file_name,
                                                const char *export_file_format,
                                                const char *size,
                                                char       *show_layers,
                                                const char *outdir);
static void             create_user_dirs       (void);
static PluginInitResult internal_plugin_init   (PluginInfo *info);
static gboolean         handle_all_diagrams    (GSList     *files,
                                                char       *export_file_name,
                                                char       *export_file_format,
                                                char       *size,
                                                char       *show_layers,
                                                const char *input_dir,
                                                const char *output_dir);
static void             print_credits          (void);
static void             print_filters_list     (gboolean verbose);

static gboolean dia_is_interactive = FALSE;

static char *
build_output_file_name(const char *infname, const char *format, const char *outdir)
{
  const char *pe = strrchr(infname,'.');
  const char *pp = strrchr(infname,G_DIR_SEPARATOR);
  char *tmp;
  if (!pp)
    pp = infname;
  else
    pp += 1;
  if (!pe)
    return g_strconcat(outdir ? outdir : "", pp,".",format,NULL);

  tmp = g_new0 (char, strlen (pp) + 1 + strlen (format) + 1);
  memcpy(tmp,pp,pe-pp);
  strcat(tmp,".");
  strcat(tmp,format);

  if (outdir) {
    char *ret = g_strconcat(outdir, G_DIR_SEPARATOR_S, tmp, NULL);

    g_clear_pointer (&tmp, g_free);

    return ret;
  }

  return tmp;
}

/* Handle the string between commas. We have either of:
 *
 * 1. XX, the number XX
 * 2. -XX, every number until XX
 * 3. XX-, every number from XX until n_layers
 * 4. XX-YY, every number between XX-YY
 */
static void
show_layers_parse_numbers (DiagramData *diagdata,
                           gboolean    *visible_layers,
                           int          n_layers,
                           const char  *str)
{
  char *p;
  unsigned long int low = 0;
  unsigned long int high = n_layers;
  unsigned long int i;

  if (str == NULL)
    return;

  /* Case 2, starts with '-' */
  if (*str == '-') {
    str++;
    low = 0;
    high = strtoul (str, &p, 10) + 1;
    /* This must be a number (otherwise we would have called parse_name) */
    g_assert (p != str);
  } else {
    /* Case 1, 3 or 4 */
    low = strtoul (str, &p, 10);
    high = low + 1; /* Assume case 1 */
    g_assert (p != str);
    if (*p == '-') {
      /* Case 3 or 4 */
      str = p + 1;
      if (*str == '\0') {/* Case 3 */
        high = n_layers;
      } else {
        high = strtoul (str, &p, 10) + 1;
        g_assert (p != str);
      }
    }
  }

  if (high <= low) {
    /* This is not an errror */
    g_warning (_("invalid layer range %lu - %lu"), low, high - 1);
    return;
  }

  if (high > n_layers) {
    high = n_layers;
  }

  /* Set the visible layers */
  for (i = low; i < high; i++) {
    DiaLayer *lay = data_layer_get_nth (diagdata, i);

    if (visible_layers[i] == TRUE) {
      g_warning (_("Layer %lu (%s) selected more than once."),
                 i,
                 dia_layer_get_name (lay));
    }
    visible_layers[i] = TRUE;
  }
}


static void
show_layers_parse_word (DiagramData *diagdata,
                        gboolean    *visible_layers,
                        int          n_layers,
                        const char  *str)
{
  gboolean found = FALSE;
  int len;
  char *p;
  const char *name;

  /* Apply --show-layers=LAYER,LAYER,... switch. 13.3.2004 sampo@iki.fi */

  DIA_FOR_LAYER_IN_DIAGRAM (diagdata, lay, k, {
    name = dia_layer_get_name (lay);

    if (name) {
      len = strlen (name);
      if ((p = strstr (str, name)) != NULL) {
        if (((p == str) || (p[-1] == ','))    /* zap false positives */
            && ((p[len] == 0) || (p[len] == ','))){
          found = TRUE;
          if (visible_layers[k] == TRUE) {
            g_warning (_("Layer %d (%s) selected more than once."), k, name);
          }
          visible_layers[k] = TRUE;
        }
      }
    }
  });

  if (found == FALSE) {
    g_warning (_("There is no layer named %s."), str);
  }
}

static void
show_layers_parse_string (DiagramData *diagdata,
                          gboolean    *visible_layers,
                          int          n_layers,
                          const char  *str)
{
  gchar **pp;
  int i;

  pp = g_strsplit (str, ",", 100);

  for (i = 0; pp[i]; i++) {
    gchar *p = pp[i];

    /* Skip the empty string */
    if (strlen(p) == 0) {
      continue;
    }

    /* If the string is only numbers and '-' chars, it is parsed as a
     * number range. Otherwise it is parsed as a layer name.
     */
    if (strlen (p) != strspn (p, "0123456789-")) {
      show_layers_parse_word (diagdata, visible_layers, n_layers, p);
    } else {
      show_layers_parse_numbers (diagdata, visible_layers, n_layers, p);
    }
  }

  g_strfreev (pp);
}


static void
handle_show_layers (DiagramData *diagdata,
                    const char  *show_layers)
{
  gboolean *visible_layers;
  DiaLayer *layer;
  int i;

  visible_layers = g_new (gboolean, data_layer_count (diagdata));
  /* Assume all layers are non-visible */
  for (i = 0; i < data_layer_count (diagdata); i++) {
    visible_layers[i] = FALSE;
  }

  /* Split the layer-range by commas */
  show_layers_parse_string (diagdata,
                            visible_layers,
                            data_layer_count (diagdata),
                            show_layers);

  /* Set the visibility of the layers */
  for (i = 0; i < data_layer_count (diagdata); i++) {
    layer = data_layer_get_nth (diagdata, i);

    if (visible_layers[i] == TRUE) {
      dia_layer_set_visible (layer, TRUE);
    } else {
      dia_layer_set_visible (layer, FALSE);
    }
  }
  g_clear_pointer (&visible_layers, g_free);
}


const char *argv0 = NULL;

/*
 * Convert infname to outfname, using input filter inf and export filter
 * ef.  If either is null, try to guess them.
 * size might be NULL.
 */
static gboolean
do_convert (const char      *infname,
            const char      *outfname,
            DiaExportFilter *ef,
            const char      *size,
            char            *show_layers)
{
  DiaImportFilter *inf;
  DiagramData     *diagdata = NULL;
  DiaContext      *ctx;

  inf = filter_guess_import_filter (infname);
  if (!inf) {
    inf = &dia_import_filter;
  }

  if (ef == NULL) {
    ef = filter_guess_export_filter (outfname);
    if (!ef) {
      g_critical (_("%s error: don't know how to export into %s\n"),
                  argv0, outfname);
      exit (1);
    }
  }

  dia_is_interactive = FALSE;

  if (0 == strcmp (infname,outfname)) {
    g_critical (_("%s error: input and output filenames are identical: %s"),
                argv0, infname);
    exit (1);
  }

  diagdata = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);
  ctx = dia_context_new (_("Import"));

  if (!inf->import_func (infname, diagdata, ctx, inf->user_data)) {
    g_critical (_("%s error: need valid input file %s\n"),
                argv0, infname);
    exit (1);
  }

  /* Apply --show-layers */
  if (show_layers) {
    handle_show_layers (diagdata, show_layers);
  }

  /* recalculate before export */
  data_update_extents (diagdata);

  /* Do our best in providing the size to the filter, but don't abuse user_data
   * too much for it. It _must not_ be changed after initialization and there
   * are quite some filter selecting their output format by it. --hb
   */
  if (size) {
    g_warning ("--size parameter unsupported for %s filter",
               ef->unique_name ? ef->unique_name : "selected");
    ef->export_func (diagdata, ctx, outfname, infname, ef->user_data);
  } else {
    ef->export_func (diagdata, ctx, outfname, infname, ef->user_data);
  }
  /* if (!quiet) */
  g_printerr (_("%s --> %s\n"), infname, outfname);
  g_clear_object (&diagdata);
  dia_context_release (ctx);
  return TRUE;
}

void debug_break (void); /* shut gcc up */
int debug_break_dont_optimize = 1;
void
debug_break (void)
{
  if (debug_break_dont_optimize > 0) {
    debug_break_dont_optimize -= 1;
  }
}

static void
dump_dependencies (void)
{
#ifdef __GNUC__
  g_print ("Compiler: GCC " __VERSION__ "\n");
#elif defined _MSC_VER
  g_print ("Compiler: MSC %d\n", _MSC_VER);
#else
  g_print ("Compiler: unknown\n");
#endif

  /* print out all those dependies, both compile and runtime if possible
   * Note: this is not meant to be complete but does only include libaries
   * which may or have cause(d) us trouble in some versions
   */
  g_print ("Library versions (at compile time)\n");
  {
    const gchar* libxml_rt_version = "?";
#if 0
    /* this is stupid, does not compile on Linux:
     * app_procs.c:504: error: expected identifier before '(' token
     *
     * In fact libxml2 has different ABI for LIBXML_THREAD_ENABLED, this code only compiles without
     * threads enabled, but apparently it does only work when theay are.
     */
    xmlInitParser();
    if (xmlGetGlobalState())
      libxml_rt_version = xmlGetGlobalState()->xmlParserVersion;
#endif
    libxml_rt_version = xmlParserVersion;
    if (atoi (libxml_rt_version)) {
      g_print ("libxml  : %d.%d.%d (%s)\n",
               atoi (libxml_rt_version) / 10000,
               atoi (libxml_rt_version) / 100 % 100,
               atoi (libxml_rt_version) % 100,
               LIBXML_DOTTED_VERSION);
    } else {   /* may include "extra" */
      g_print ("libxml  : %s (%s)\n", libxml_rt_version ? libxml_rt_version : "??", LIBXML_DOTTED_VERSION);
    }
  }
  g_print ("glib    : %d.%d.%d (%d.%d.%d)\n",
           glib_major_version, glib_minor_version, glib_micro_version,
           GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
  g_print ("pango   : %s (%d.%d.%d)\n", pango_version_string(), PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR, PANGO_VERSION_MICRO);
  g_print ("cairo   : %s (%s)\n", cairo_version_string(), CAIRO_VERSION_STRING);
  g_print ("gtk+    : %d.%d.%d (%d.%d.%d)\n",
           gtk_major_version, gtk_minor_version, gtk_micro_version,
           GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
}

gboolean
app_is_interactive (void)
{
  return dia_is_interactive;
}

/*
 * Handle loading of diagrams given on command line, including conversions.
 * Returns %TRUE if any automatic conversions were performed.
 * Note to future hackers: 'size' is currently the only argument that can be
 * sent to exporters. If more arguments are desired, please don't just add
 * even more arguments, but create a more general system.
 */
static gboolean
handle_initial_diagram (const char *in_file_name,
                        const char *out_file_name,
                        const char *export_file_format,
                        const char *size,
                        char       *show_layers,
                        const char *outdir) {
  Diagram *diagram = NULL;
  gboolean made_conversions = FALSE;

  if (export_file_format) {
    char *export_file_name = NULL;
    DiaExportFilter *ef = NULL;

    /* First try guessing based on extension */
    export_file_name = build_output_file_name (in_file_name,
                                               export_file_format,
                                               outdir);

    ef = filter_guess_export_filter (export_file_name);
    if (ef == NULL) {
      ef = filter_export_get_by_name (export_file_format);
      if (ef == NULL) {
        g_critical (_("Can't find output format/filter %s"),
                    export_file_format);
        return FALSE;
      }
      g_clear_pointer (&export_file_name, g_free);
      export_file_name = build_output_file_name (in_file_name,
                                                 ef->extensions[0],
                                                 outdir);
    }
    made_conversions |= do_convert (in_file_name,
                                    (out_file_name != NULL ? out_file_name : export_file_name),
                                    ef,
                                    size,
                                    show_layers);
    g_clear_pointer (&export_file_name, g_free);
  } else if (out_file_name) {
    DiaExportFilter *ef = NULL;
    made_conversions |= do_convert (in_file_name,
                                    out_file_name,
                                    ef,
                                    size,
                                    show_layers);
  } else {
    if (g_file_test (in_file_name, G_FILE_TEST_EXISTS)) {
      diagram = diagram_load (in_file_name, NULL);
    } else {
      GFile *file = g_file_new_for_path (in_file_name);

      diagram = dia_diagram_new (file);

      g_clear_object (&file);
    }

    if (diagram != NULL) {
      diagram_update_extents (diagram);
      if (app_is_interactive ()) {
        layer_dialog_set_diagram (diagram);
        /* the display initial diagram holds two references */
        new_display (diagram);
      } else {
        g_clear_object (&diagram);
      }
    }
  }
  return made_conversions;
}

#ifdef G_OS_WIN32
/* Translators:  This is an option, not to be translated */
#define WMF "wmf, "
#else
#define WMF ""
#endif

static const gchar *input_directory = NULL;
static const gchar *output_directory = NULL;


static gboolean
_check_option_input_directory (const gchar    *option_name,
                               const gchar    *value,
                               gpointer        data,
                               GError        **error)
{
  gchar *directory = g_filename_from_utf8 (value, -1, NULL, NULL, NULL);

  if (g_file_test (directory, G_FILE_TEST_IS_DIR)) {
    input_directory = directory;
    return TRUE;
  }
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               _("Input directory '%s' must exist!\n"), directory);
  g_clear_pointer (&directory, g_free);
  return FALSE;
}


static gboolean
_check_option_output_directory (const gchar    *option_name,
                                const gchar    *value,
                                gpointer        data,
                                GError        **error)
{
  gchar *directory = g_filename_from_utf8 (value, -1, NULL, NULL, NULL);

  if (g_file_test (directory, G_FILE_TEST_IS_DIR)) {
    output_directory = directory;
    return TRUE;
  }
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               _("Output directory '%s' must exist!\n"), directory);
  g_clear_pointer (&directory, g_free);
  return FALSE;
}


void
app_init (int argc, char **argv)
{
  static gboolean nosplash = FALSE;
  static gboolean nonew = FALSE;
  static gboolean use_integrated_ui = TRUE;
  static gboolean credits = FALSE;
  static gboolean list_filters = FALSE;
  static gboolean version = FALSE;
  static gboolean verbose = FALSE;
  static gboolean log_to_stderr = FALSE;
  static char *export_file_name = NULL;
  static char *export_file_format = NULL;
  static char *size = NULL;
  static char *show_layers = NULL;
  gboolean made_conversions = FALSE;
  GSList *files = NULL;
  static const gchar **filenames = NULL;
  int i = 0;
  GOptionContext *context = NULL;
  static GOptionEntry options[] = {
    {"export", 'e', 0, G_OPTION_ARG_FILENAME, NULL /* &export_file_name */,
     N_("Export loaded file and exit"), N_("OUTPUT")},
    {"filter", 't', 0, G_OPTION_ARG_STRING, NULL /* &export_file_format */,
     N_("Select the export filter/format"), N_("TYPE") },
    {"list-filters", 0, 0, G_OPTION_ARG_NONE, &list_filters,
     N_("List export filters/formats and exit"), NULL},
    {"size", 's', 0, G_OPTION_ARG_STRING, NULL,
     N_("Export graphics size"), N_("WxH")},
    {"show-layers", 'L', 0, G_OPTION_ARG_STRING, NULL,
     N_("Show only specified layers (e.g. when exporting). Can be either the layer name or a range of layer numbers (X-Y)"),
     N_("LAYER,LAYER,...")},
    {"nosplash", 'n', 0, G_OPTION_ARG_NONE, &nosplash,
     N_("Don't show the splash screen"), NULL },
    {"nonew", 'n', 0, G_OPTION_ARG_NONE, &nonew,
     N_("Don't create an empty diagram"), NULL },
    {"classic", '\0', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &use_integrated_ui,
     N_("Start classic user interface (no diagrams in tabs)"), NULL },
    {"log-to-stderr", 'l', 0, G_OPTION_ARG_NONE, &log_to_stderr,
     N_("Send error messages to stderr instead of showing dialogs."), NULL },
    {"input-directory", 'I', 0, G_OPTION_ARG_CALLBACK, _check_option_input_directory,
     N_("Directory containing input files"), N_("DIRECTORY")},
    {"output-directory", 'O', 0, G_OPTION_ARG_CALLBACK, _check_option_output_directory,
     N_("Directory containing output files"), N_("DIRECTORY")},
    {"credits", 'c', 0, G_OPTION_ARG_NONE, &credits,
     N_("Display credits list and exit"), NULL },
    {"verbose", 0, 0, G_OPTION_ARG_NONE, &verbose,
     N_("Generate verbose output"), NULL },
    {"version", 'v', 0, G_OPTION_ARG_NONE, &version,
     N_("Display version and exit"), NULL },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, NULL /* &filenames */,
      NULL, NULL },
    { NULL }
  };
  char *localedir = dia_get_locale_directory ();

  /* for users of app_init() the default is interactive */
  dia_is_interactive = TRUE;

  options[0].arg_data = &export_file_name;
  options[1].arg_data = &export_file_format;
  options[3].arg_data = &size;
  options[4].arg_data = &show_layers;
  g_return_if_fail (g_strcmp0 (options[14].long_name, G_OPTION_REMAINING) == 0);
  options[14].arg_data = (void*)&filenames;

  argv0 = (argc > 0) ? argv[0] : "(none)";

  setlocale (LC_NUMERIC, "C");

  bindtextdomain (GETTEXT_PACKAGE, localedir);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_clear_pointer (&localedir, g_free);

  context = g_option_context_new (_("[FILE...]"));
  g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
  /* avoid to add it a second time */
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (argv) {
    GError *error = NULL;

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
      if (error) { /* IMO !error here is a bug upstream, triggered e.g. with --gdk-debug=updates */
        g_printerr ("%s", error->message);
        g_clear_error (&error);
      } else {
        g_printerr (_("Invalid option?"));
      }

      g_clear_pointer (&context, g_option_context_free);
      exit (1);
    }

    /* second level check of command line options, existence of input files etc. */
    if (filenames) {
      while (filenames[i] != NULL) {
        char *filename;
        char *testpath;

        if (g_str_has_prefix (filenames[i], "file://")) {
          filename = g_filename_from_uri (filenames[i], NULL, NULL);
          if (!g_utf8_validate (filename, -1, NULL)) {
            char *tfn = filename;
            filename = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
            g_clear_pointer (&tfn, g_free);
          }
        } else {
          filename = g_filename_to_utf8 (filenames[i], -1, NULL, NULL, NULL);
        }

        if (!filename) {
          g_printerr (_("Filename conversion failed: %s\n"), filenames[i]);
          ++i;
          continue;
        }

        if (g_path_is_absolute (filename)) {
          testpath = filename;
        } else {
          testpath = g_build_filename (input_directory ? input_directory : ".", filename, NULL);
        }

        /* we still have a problem here, if GLib's file name encoding would not be utf-8 */
        if (g_file_test (testpath, G_FILE_TEST_IS_REGULAR)) {
          files = g_slist_append (files, filename);
        } else {
          g_printerr (_("Missing input: %s\n"), filename);
          g_clear_pointer (&filename, g_free);
        }

        if (filename != testpath) {
          g_clear_pointer (&testpath, g_free);
        }

        ++i;
      }
    }
    /* given some files to output (or something;)), we are not starting up the UI */
    if (export_file_name || export_file_format || size || credits || version || list_filters) {
      dia_is_interactive = FALSE;
    }
  }
  g_clear_pointer (&context, g_option_context_free);

  if (argv && dia_is_interactive) {
    g_set_application_name (_("Dia Diagram Editor"));
    gtk_init (&argc, &argv);

    /* TODO: GTK: (Defunct with GtkApplication) */
    gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                      "/org/gnome/Dia/icons/");

    /* Set the icon for Dia windows & dialogs */
    /* GTK3: Use icon-name with GResource fallback */
    gtk_window_set_default_icon_name ("org.gnome.Dia");
  } else {
    /*
     * On Windows there is no command line without display so that gtk_init is harmless.
     * On X11 we need gtk_init_check() to avoid exit() just because there is no display
     * running outside of X11.
     */
    if (!gtk_init_check (&argc, &argv)) {
      dia_log_message ("Running without display");
    }
  }

  if (version) {
    char *ver_str;

#if (defined __TIME__) && (defined __DATE__)
    /* TRANSLATOR: 2nd and 3rd %s are time and date respectively. */
    ver_str = g_strdup_printf (_("Dia version %s, compiled %s %s\n"), dia_version_string (), __TIME__, __DATE__);
#else
    ver_str = g_strdup_printf (_("Dia version %s\n"), dia_version_string ());
#endif

    g_print ("%s\n", ver_str);

    g_clear_pointer (&ver_str, g_free);

    if (verbose) {
      dump_dependencies ();
    }

    exit (0);
  }

  if (!dia_is_interactive) {
    log_to_stderr = TRUE;
  }

  libdia_init ( (dia_is_interactive ? DIA_INTERACTIVE : 0)
               |(log_to_stderr ? DIA_MESSAGE_STDERR : 0)
               |(verbose ? DIA_VERBOSE : 0));

  if (credits) {
    print_credits ();
    exit (0);
  }

  if (dia_is_interactive) {
    create_user_dirs ();

    if (!nosplash) {
      app_splash_init ("");
    }

    /* Init cursors: */
    default_cursor = gdk_cursor_new (GDK_LEFT_PTR);
    ddisplay_set_all_cursor (default_cursor);
  }

  dia_register_plugins ();
  dia_register_builtin_plugin (internal_plugin_init);

  if (list_filters) {
    print_filters_list (verbose);
    exit (0);
  }

  load_all_sheets ();     /* new mechanism */

  dia_log_message ("object defaults");
  {
    DiaContext *ctx = dia_context_new (_("Object Defaults"));
    dia_object_defaults_load (NULL, TRUE /* prefs.object_defaults_create_lazy */, ctx);
    dia_context_release (ctx);
  }
  debug_break ();

  if (object_get_type ("Standard - Box") == NULL) {
    message_error (_("Couldn't find standard objects when looking for "
                     "object-libs; exiting..."));
    g_critical (_("Couldn't find standard objects when looking for "
                  "object-libs in “%s”; exiting..."),
                dia_get_lib_directory ());
    exit (1);
  }

  persistence_load ();

  /** Must load prefs after persistence */
  dia_preferences_init ();

  if (dia_is_interactive) {

    /* further initialization *before* reading files */
    active_tool = create_modify_tool ();

    dia_log_message ("ui creation");
    if (use_integrated_ui) {
      create_integrated_ui ();
    } else {
      create_toolbox ();
      /* for the integrated ui case it is integrated */
      persistence_register_window_create ("layer_window",
                                          (NullaryFunc*) &layer_dialog_create);
    }

    /*fill recent file menu */
    recent_file_history_init ();

    /* Set up autosave to check every 5 minutes */
    g_timeout_add_seconds (5 * 60, autosave_check_autosave, NULL);

#if 0 /* do we really open these automatically in the next session? */
    persistence_register_window_create ("diagram_tree",
                                        &diagram_tree_show);
#endif
    persistence_register_window_create ("sheets_main_dialog",
                                        (NullaryFunc*) &sheets_dialog_create);

    /* In current setup, we can't find the autosaved files. */
    /*autosave_restore_documents();*/
  }

  dia_log_message ("diagrams");
  made_conversions = handle_all_diagrams (files,
                                          export_file_name,
                                          export_file_format,
                                          size,
                                          show_layers,
                                          input_directory,
                                          output_directory);

  if (dia_is_interactive && files == NULL && !nonew) {
    GList *list;

    file_new_callback (NULL);

    list = dia_open_diagrams ();
    if (list) {
      Diagram *diagram = list->data;
      diagram_update_extents (diagram);
      diagram->is_default = TRUE;
      layer_dialog_set_diagram (diagram);
    }
  }
  g_slist_free (files);
  if (made_conversions) {
    exit (0);
  }

  dynobj_refresh_init ();
  dia_log_message ("initialized");
}


/**
 * app_exit:
 *
 * Exit the application, but asking the user for confirmation
 * if there are changed diagrams.
 *
 * Returns: %TRUE if the application exits.
 *
 * Since: dawn-of-time
 */
gboolean
app_exit (void)
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
    g_error (_("This shouldn't happen.  Please file a bug report at "
               "https://gitlab.gnome.org/GNOME/dia "
               "describing how you caused this message to appear."));
    return FALSE;
  }

  if (diagram_modified_exists()) {
    if (is_integrated_ui ()) {
      DiaExitDialog *dialog;
      int            result;
      GPtrArray     *items  = NULL;
      GList         *diagrams;
      Diagram       *diagram;

      dialog = dia_exit_dialog_new (GTK_WINDOW (interface_get_toolbox_shell ()));

      diagrams = dia_open_diagrams ();
      while (diagrams) {
        diagram = diagrams->data;

        if (diagram_is_modified (diagram)) {
          const char *name = diagram_get_name (diagram);
          const char *path = diagram->filename;
          dia_exit_dialog_add_item (dialog, name, path, diagram);
        }

        diagrams = g_list_next (diagrams);
      }

      result = dia_exit_dialog_run (dialog, &items);

      g_clear_object (&dialog);

      if (result == DIA_EXIT_DIALOG_CANCEL) {
        return FALSE;
      } else if (result == DIA_EXIT_DIALOG_SAVE) {
        DiaContext *ctx = dia_context_new (_("Save"));

        for (int i = 0; i < items->len; i++) {
          DiaExitDialogItem *item = g_ptr_array_index (items, i);
          char *filename;

          filename = g_filename_from_utf8 (item->data->filename, -1, NULL, NULL, NULL);
          diagram_update_extents (item->data);
          dia_context_set_filename (ctx, filename);

          if (!diagram_save (item->data, filename, ctx)) {
            dia_context_release (ctx);

            g_clear_pointer (&filename, g_free);
            g_clear_pointer (&items, g_ptr_array_unref);

            return FALSE;
          } else {
            dia_context_reset (ctx);
          }

          g_clear_pointer (&filename, g_free);
        }

        dia_context_release (ctx);
      } else if (result == DIA_EXIT_DIALOG_QUIT) {
        diagrams = dia_open_diagrams ();
        while (diagrams) {
          diagram = diagrams->data;

          /* slight hack: don't ask again */
          diagram_set_modified (diagram, FALSE);
          undo_clear (diagram->undo);
          diagrams = g_list_next (diagrams);
        }
      }

      g_clear_pointer (&items, g_ptr_array_unref);
    } else {
      GtkWidget *dialog;
      GtkWidget *button;
      dialog = gtk_message_dialog_new (NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_QUESTION,
                                       GTK_BUTTONS_NONE, /* no standard buttons */
                                       _("Quitting without saving modified diagrams"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                _("Modified diagrams exist. "
                                                  "Are you sure you want to "
                                                  "quit Dia without saving them?"));

      gtk_window_set_title (GTK_WINDOW (dialog), _("Quit Dia"));

      button = gtk_button_new_with_mnemonic (_("_Cancel"));
      gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_CANCEL);
      gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

      button = gtk_button_new_with_mnemonic (_("_Quit"));
      gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);

      gtk_widget_show_all (dialog);

      if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK) {
        gtk_widget_destroy (dialog);
        return FALSE;
      }
      gtk_widget_destroy (dialog);
    }
  }

  persistence_save ();

  dynobj_refresh_finish ();

  {
    DiaContext *ctx = dia_context_new (_("Exit"));
    dia_object_defaults_save (NULL, ctx);
    dia_context_release (ctx);
  }
  /* Free loads of stuff (toolbox) */

  list = dia_open_diagrams ();
  while (list != NULL) {
    Diagram *dia = (Diagram *) list->data;
    list = g_list_next (list);

    slist = dia->displays;
    while (slist != NULL) {
      DDisplay *ddisp = (DDisplay *) slist->data;
      slist = g_slist_next (slist);

      gtk_widget_destroy (ddisp->shell);
    }
    /* The diagram is freed when the last display is destroyed */
  }

  /* save pluginrc */
  if (dia_is_interactive) {
    dia_pluginrc_write ();
  }

  gtk_main_quit ();

  /* This printf seems to prevent a race condition with unrefs. */
  /* Yuck.  -Lars */
  /* Trying to live without it. -Lars 10/8/07*/
  /* g_print(_("Thank you for using Dia.\n")); */
  app_exit_once = TRUE;

  return TRUE;
}

static void
create_user_dirs (void)
{
  gchar *dir, *subdir;

#ifdef G_OS_WIN32
  /* not necessary to quit the program with g_error, everywhere else
   * dia_config_filename appears to be used. Spit out a warning ...
   */
  if (!g_get_home_dir ()) {
    g_warning (_("Could not create per-user Dia configuration directory"));
    return; /* ... and return. Probably removes my one and only FAQ. --HB */
  }
#endif
  dir = g_strconcat (g_get_home_dir (), G_DIR_SEPARATOR_S ".dia", NULL);
  if (g_mkdir (dir, 0755) && errno != EEXIST) {
#ifndef G_OS_WIN32
    g_critical (_("Could not create per-user Dia configuration directory"));
    exit (1);
#else /* HB: it this really a reason to exit the program on *nix ? */
    g_warning (_("Could not create per-user Dia configuration directory. Please make "
                 "sure that the environment variable HOME points to an existing directory."));
#endif
  }

  /* it is no big deal if these directories can't be created */
  subdir = g_strconcat (dir, G_DIR_SEPARATOR_S "objects", NULL);
  g_mkdir (subdir, 0755);
  g_clear_pointer (&subdir, g_free);
  subdir = g_strconcat (dir, G_DIR_SEPARATOR_S "shapes", NULL);
  g_mkdir (subdir, 0755);
  g_clear_pointer (&subdir, g_free);
  subdir = g_strconcat (dir, G_DIR_SEPARATOR_S "sheets", NULL);
  g_mkdir (subdir, 0755);
  g_clear_pointer (&subdir, g_free);

  g_clear_pointer (&dir, g_free);
}

static PluginInitResult
internal_plugin_init (PluginInfo *info)
{
  if (!dia_plugin_info_init (info,
                             "Internal",
                             _("Objects and filters internal to Dia"),
                             NULL,
                             NULL)) {
    return DIA_PLUGIN_INIT_ERROR;
  }

  /* register the group object type */
  object_register_type (&group_type);

  /* register import filters */
  filter_register_import (&dia_import_filter);

  /* register export filters */
  /* Standard Dia format */
  filter_register_export (&dia_export_filter);

  return DIA_PLUGIN_INIT_OK;
}

static gboolean
handle_all_diagrams (GSList     *files,
                     char       *export_file_name,
                     char       *export_file_format,
                     char       *size,
                     char       *show_layers,
                     const char *input_dir,
                     const char *output_dir)
{
  GSList *node = NULL;
  gboolean made_conversions = FALSE;

  for (node = files; node; node = node->next) {
    gchar *inpath = input_dir ? g_build_filename (input_dir, node->data, NULL) : node->data;
    made_conversions |=
      handle_initial_diagram (inpath,
                              export_file_name,
                              export_file_format,
                              size,
                              show_layers,
                              output_dir);
    if (inpath != node->data) {
      g_clear_pointer (&inpath, g_free);
    }
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
print_credits (void)
{
  int i;
  const int nauthors = (sizeof (authors) / sizeof (authors[0])) - 1;
  const int ndocumentors = (sizeof (documentors) / sizeof (documentors[0])) - 1;

  g_print (_("The original author of Dia was:\n\n"));
  for (i = 0; i < NUMBER_OF_ORIG_AUTHORS; i++) {
    g_print ("%s\n", authors[i]);
  }

  g_print (_("\nThe current maintainers of Dia are:\n\n"));
  for (i = NUMBER_OF_ORIG_AUTHORS; i < NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i++) {
    g_print ("%s\n", authors[i]);
  }

  g_print (_("\nOther authors are:\n\n"));
  for (i = NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i < nauthors; i++) {
    g_print ("%s\n", authors[i]);
  }

  g_print (_("\nDia is documented by:\n\n"));
  for (i = 0; i < ndocumentors; i++) {
    g_print ("%s\n", documentors[i]);
  }
}

typedef struct _PairExtensionFilter PairExtensionFilter;
struct _PairExtensionFilter {
  const char *ext;
  const DiaExportFilter *ef;
};

static int
_cmp_filter (const void *a, const void *b)
{
  const PairExtensionFilter *pa = a;
  const PairExtensionFilter *pb = b;

  return strcmp (pa->ext, pb->ext);
}

/*!
 * \brief Dump available export filters
 *
 * Almost all of Dia's export formats are realized with plug-ins, either
 * implemented in C or Python. Instead of a static help string which tries
 * to cover these at compile time, this function queries the registered
 * filters to provide the complete list for the current configuration.
 *
 * This list still might not cover all available filters for two reasons:
 *  - only configured filters are registered
 *  - some filters are only registered for the interactive (or X11) case
 */
static void
print_filters_list (gboolean verbose)
{
  GList *list;
  int j;
  GArray *by_extension;

  g_print ("%s\n", _("Available Export Filters (for --filter)"));
  g_print ("%10s%20s %s\n",
           /* Translators: be brief or mess up the table for --list-filters */
           _("Extension"),
           _("Identifier"),
           _("Description"));
  by_extension = g_array_new (FALSE, FALSE, sizeof (PairExtensionFilter));
  for (list = filter_get_export_filters (); list != NULL; list = list->next) {
    const DiaExportFilter *ef = list->data;

    if (!ef->extensions) {
      continue;
    }

    for (j = 0; ef->extensions[j] != NULL; ++j) {
      PairExtensionFilter pair = { ef->extensions[j], ef };

      g_array_append_val (by_extension, pair);
      if (!verbose) {
        break; /* additional extensions don't provide significant information */
      }
    }
  }
  g_array_sort (by_extension, _cmp_filter);
  for (j = 0; j < by_extension->len; ++j) {
    PairExtensionFilter *pair = &g_array_index (by_extension, PairExtensionFilter, j);

    g_print ("%10s%20s %s\n",
             pair->ext,
             pair->ef->unique_name ? pair->ef->unique_name : "",
             pair->ef->description);
  }
  g_array_free (by_extension, TRUE);
}
