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

/* wants to be included early to avoid complains about setjmp.h */
#ifdef HAVE_LIBPNG
#include <png.h> /* just for the version stuff */
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

/* apparently there is no clean way to use glib-2.6 GOption with gnome */
#if GLIB_CHECK_VERSION(2,5,5) && !defined GNOME
#  define USE_GOPTION 1
#else
#  define USE_GOPTION 0
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
		       const char *size,
		       char *show_layers);

static void create_user_dirs(void);
static PluginInitResult internal_plugin_init(PluginInfo *info);
static void process_opts(int argc, char **argv,
#if USE_GOPTION
			 GOptionContext* context, GOptionEntry options[],
#elif defined HAVE_POPT
			 poptContext poptCtx, struct poptOption options[],
#endif
			 GSList **files, char **export_file_name,
			 char **export_file_format, char **size,
			 char **show_layers, gboolean *nosplash);
static gboolean handle_all_diagrams(GSList *files, char *export_file_name,
				    char *export_file_format, char *size, char *show_layers);
static void print_credits(gboolean credits);

static gboolean dia_is_interactive = TRUE;
static void
stderr_message_internal(const char *title, const char *fmt,
                        va_list *args,  va_list *args2);

static void
stderr_message_internal(const char *title, const char *fmt,
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

/* Handle the string between commas. We have either of:
 *
 * 1. XX, the number XX
 * 2. -XX, every number until XX
 * 3. XX-, every number from XX until n_layers
 * 4. XX-YY, every number between XX-YY
 */
static void
show_layers_parse_numbers(DiagramData *diagdata, gboolean *visible_layers, gint n_layers, const char *str)
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
    high = strtoul(str, &p, 10)+1;
    /* This must be a number (otherwise we would have called parse_name) */
    g_assert(p != str);
  }
  else {
    /* Case 1, 3 or 4 */
    low = strtoul(str, &p, 10);
    high = low+1; /* Assume case 1 */
    g_assert(p != str);
    if (*p == '-')
      {
	/* Case 3 or 4 */
	str = p + 1;
	if (*str == '\0') /* Case 3 */
	  high = n_layers;
	else
	  {
	    high = strtoul(str, &p, 10)+1;
	    g_assert(p != str);
	  }
      }
  }

  if ( high <= low ) {
    /* This is not an errror */
    g_print(_("Warning: invalid layer range %lu - %lu\n"), low, high-1 );
    return;
  }
  if (high > n_layers)
    high = n_layers;

  /* Set the visible layers */
  for ( i = low; i < high; i++ )
    {
      Layer *lay = (Layer *)g_ptr_array_index(diagdata->layers, i);

      if (visible_layers[i] == TRUE)
	g_print(_("Warning: Layer %lu (%s) selected more than once.\n"), i, lay->name);
      visible_layers[i] = TRUE;
    }
}

static void
show_layers_parse_word(DiagramData *diagdata, gboolean *visible_layers, gint n_layers, const char *str)
{
  GPtrArray *layers = diagdata->layers;
  gboolean found = FALSE;

  /* Apply --show-layers=LAYER,LAYER,... switch. 13.3.2004 sampo@iki.fi */
  if (layers) {
    int len,k = 0;
    Layer *lay;
    char *p;
    for (k = 0; k < layers->len; k++) {
      lay = (Layer *)g_ptr_array_index(layers, k);

      if (lay->name) {
	len =  strlen(lay->name);
	if ((p = strstr(str, lay->name)) != NULL) {
	  if (((p == str) || (p[-1] == ','))    /* zap false positives */
	      && ((p[len] == 0) || (p[len] == ','))){
	    found = TRUE;
	    if (visible_layers[k] == TRUE)
	      g_print(_("Warning: Layer %d (%s) selected more than once.\n"), k, lay->name);
	    visible_layers[k] = TRUE;
	  }
	}
      }
    }
  }

  if (found == FALSE)
    g_print(_("Warning: There is no layer named %s\n"), str);
}

static void
show_layers_parse_string(DiagramData *diagdata, gboolean *visible_layers, gint n_layers, const char *str)
{
  gchar **pp;
  int i;

  pp = g_strsplit(str, ",", 100);

  for (i = 0; pp[i]; i++) {
    gchar *p = pp[i];

    /* Skip the empty string */
    if (strlen(p) == 0)
      continue;

    /* If the string is only numbers and '-' chars, it is parsed as a
     * number range. Otherwise it is parsed as a layer name.
     */
    if (strlen(p) != strspn(p, "0123456789-") )
      show_layers_parse_word(diagdata, visible_layers, n_layers, p);
    else
      show_layers_parse_numbers(diagdata, visible_layers, n_layers, p);
  }

  g_strfreev(pp);
}


static void
handle_show_layers(DiagramData *diagdata, const char *show_layers)
{
  gboolean *visible_layers;
  Layer *layer;
  int i;

  visible_layers = g_malloc(diagdata->layers->len * sizeof(gboolean));
  /* Assume all layers are non-visible */
  for (i=0;i<diagdata->layers->len;i++)
    visible_layers[i] = FALSE;

  /* Split the layer-range by commas */
  show_layers_parse_string(diagdata, visible_layers, diagdata->layers->len,
			   show_layers);

  /* Set the visibility of the layers */
  for (i=0;i<diagdata->layers->len;i++) {
    layer = g_ptr_array_index(diagdata->layers, i);

    if (visible_layers[i] == TRUE)
      layer->visible = TRUE;
    else
      layer->visible = FALSE;
  }
  g_free(visible_layers);
}


const char *argv0 = NULL;

/** Convert infname to outfname, using input filter inf and export filter
 * ef.  If either is null, try to guess them.
 * size might be NULL.
 */
static gboolean
do_convert(const char *infname, 
	   const char *outfname, DiaExportFilter *ef,
	   const char *size,
	   char *show_layers)
{
  DiaImportFilter *inf;
  DiagramData *diagdata = NULL;

  inf = filter_guess_import_filter(infname);
  if (!inf) 
    inf = &dia_import_filter;

  if (ef == NULL) {
    ef = filter_guess_export_filter(outfname);
    if (!ef) {
      g_critical(_("%s error: don't know how to export into %s\n"),
	      argv0,outfname);
      exit(1);
    }
  }

  dia_is_interactive = FALSE;

  if (0==strcmp(infname,outfname)) {
    g_critical(_("%s error: input and output file name is identical: %s"),
            argv0, infname);
    exit(1);
  }
  
  diagdata = g_object_new (DIA_TYPE_DIAGRAM_DATA, NULL);

  if (!inf->import_func(infname,diagdata,inf->user_data)) {
    g_critical(_("%s error: need valid input file %s\n"),
            argv0, infname);
    exit(1);
  }

  /* Apply --show-layers */
  if (show_layers)
    handle_show_layers(diagdata, show_layers);

  /* Do our best in providing the size to the filter, but don't abuse user_data 
   * too much for it. It _must not_ be changed after initialization and there 
   * are quite some filter selecting their output format by it. --hb
   */
  if (size) {
    if (ef == filter_get_by_name ("png-libart")) /* the warning we get is appropriate, don't cast */
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
  g_object_unref(diagdata);
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

static void
dump_dependencies(void)
{
#ifdef __GNUC__
  g_print ("Compiler: GCC " __VERSION__ "\n");
#elif defined _MSC_VER
  g_print ("Compiler: MSC %d\n", _MSC_VER);
#else
  g_print ("Compiler: unknown\n");
#endif
  /* some flags/defines which may be interesting */
  g_print ("  with : "
#ifdef G_THREADS_ENABLED
  "threads "
#endif
#ifdef HAVE_CAIRO
  "cairo "
#endif
#ifdef GNOME
  "gnome "
#endif
#ifdef HAVE_GNOMEPRINT
  "gnomeprint "
#endif
#ifdef HAVE_LIBART
  "libart "
#endif
  "\n");

  /* print out all those dependies, both compile and runtime if possible 
   * Note: this is not meant to be complete but does only include libaries 
   * which may or have cause(d) us trouble in some versions 
   */
  g_print ("Library versions\n");
#ifdef HAVE_LIBPNG
  g_print ("libpng  : %s (%s)\n", png_get_header_ver(NULL), PNG_LIBPNG_VER_STRING);
#endif
#ifdef HAVE_FREETYPE
  {
    FT_Library ft_library;
    FT_Int     ft_major_version;
    FT_Int     ft_minor_version;
    FT_Int     ft_micro_version;

    if (FT_Init_FreeType (&ft_library) == 0) {
      FT_Library_Version (ft_library,
                          &ft_major_version,
                          &ft_minor_version,
                          &ft_micro_version);

      g_print ("freetype: %d.%d.%d\n", ft_major_version, ft_minor_version, ft_micro_version);
      FT_Done_FreeType (ft_library);
    }
    else
      g_print ("freetype: ?.?.?\n");
  }
#endif
  {
    gint   libxml_version   = atoi(xmlParserVersion);

    g_print ("libxml  : %d.%d.%d (%s)\n", 
             libxml_version / 100 / 100, libxml_version / 100 % 100, libxml_version % 100, LIBXML_DOTTED_VERSION);
  }
  g_print ("glib    : %d.%d.%d (%d.%d.%d)\n", 
           glib_major_version, glib_minor_version, glib_micro_version,
           GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
  g_print ("pango   : version not available\n"); /* Pango does not provide such */
#if 0
  {
    gchar  linkedname[1024];
    gint   len = 0;

    /* relying on PREFIX is wrong */
    if ((len = readlink (PREFIX "/lib/libpango-1.0.so", linkedname, 1023)) > 0) {
      /* man 2 readlink : does not append a  NUL  character */
      linkedname[len] = '\0';
      g_print ("%s/%s\n", PREFIX, linkedname);
    }
  }
#endif
  g_print ("gtk+    : %d.%d.%d (%d.%d.%d)\n",
           gtk_major_version, gtk_minor_version, gtk_micro_version,
           GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);

#if 0
  /* we could read $PREFIX/share/gnome-about/gnome-version.xml 
   * but is it worth the effort ? */
  g_print ("gnome   : %d.%d.%d (%d.%d.%d)\n"
           gnome_major_version, gnome_minor_version, gnome_micro_version,
           GNOME_MAJOR_VERSION, GNOME_MINOR_VERSION, GNOME_MICRO_VERSION);
#endif
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
		       const char *size,
		       char* show_layers) {
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
	g_critical(_("Can't find output format/filter %s\n"), export_file_format);
	return FALSE;
      }
      g_free (export_file_name);
      export_file_name = build_output_file_name(in_file_name,
						ef->extensions[0]);
    }
    made_conversions |= do_convert(in_file_name,
      (out_file_name != NULL?out_file_name:export_file_name),
				   ef, size, show_layers);
    g_free(export_file_name);
  } else if (out_file_name) {
    DiaExportFilter *ef = NULL;

    /* if this looks like an ugly hack to you, agreed ;)  */
    if (size && strstr(out_file_name, ".png"))
      ef = filter_get_by_name ("png-libart");
    
    made_conversions |= do_convert(in_file_name, out_file_name, ef,
				   size, show_layers);
  } else {
    if (g_file_test(in_file_name, G_FILE_TEST_EXISTS)) {
      diagram = diagram_load (in_file_name, NULL);
    } else {
      diagram = new_diagram (in_file_name);
    }
	      
    if (diagram != NULL) {
      diagram_update_extents(diagram);
      if (app_is_interactive()) {
	layer_dialog_set_diagram(diagram);
        /* the display initial diagram holds two references */
	ddisp = new_display(diagram);
      } else {
        g_object_unref(diagram);
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
  static gboolean nosplash = FALSE;
  static gboolean nonew = FALSE;
  static gboolean credits = FALSE;
  static gboolean version = FALSE;
  static gboolean verbose = FALSE;
  static gboolean log_to_stderr = FALSE;
#ifdef GNOME
  GnomeClient *client;
#endif
  static char *export_file_name = NULL;
  static char *export_file_format = NULL;
  static char *size = NULL;
  static char *show_layers = NULL;
  gboolean made_conversions = FALSE;
  GSList *files = NULL;

  gchar *export_format_string = 
     /* Translators:  The argument is a list of options, not to be translated */
    g_strdup_printf(_("Select the filter/format out of: %s"),
		    "cgm, dia, dxf, eps, eps-builtin, " EPS_PANGO
		    "fig, mp, plt, hpgl, png ("
#  if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
		    "png-libart, "
#  endif
#  ifdef HAVE_CAIRO
		    "cairo-png, cairo-alpha-png, "
#  endif
		    /* we always have pixbuf but don't know exactly all it's *few* save formats */
		    "pixbuf-png), jpg, "
		    "shape, svg, tex, " WMF
		    "wpg");

#if USE_GOPTION 
  GOptionContext *context = NULL;
  static GOptionEntry options[] =
  {
    {"export", 'e', 0, G_OPTION_ARG_STRING, NULL /* &export_file_name */,
     N_("Export loaded file and exit"), N_("OUTPUT")},
    {"filter",'t', 0, G_OPTION_ARG_STRING, NULL /* &export_file_format */,
     NULL /* &export_format_string */, N_("TYPE") },
    {"size", 's', 0, G_OPTION_ARG_STRING, NULL,
     N_("Export graphics size"), N_("WxH")},
    {"show-layers", 'L', 0, G_OPTION_ARG_STRING, NULL,
     N_("Show only specified layers (e.g. when exporting). Can be either the layer name or a range of layer numbers (X-Y)"),
     N_("LAYER,LAYER,...")},
    {"nosplash", 'n', 0, G_OPTION_ARG_NONE, &nosplash,
     N_("Don't show the splash screen"), NULL },
    {"nonew", 'n', 0, G_OPTION_ARG_NONE, &nonew,
     N_("Don't create empty diagram"), NULL },
    {"log-to-stderr", 'l', 0, G_OPTION_ARG_NONE, &log_to_stderr,
     N_("Send error messages to stderr instead of showing dialogs."), NULL },
    {"credits", 'c', 0, G_OPTION_ARG_NONE, &credits,
     N_("Display credits list and exit"), NULL },
    {"verbose", 0, 0, G_OPTION_ARG_NONE, &verbose,
     N_("Generate verbose output"), NULL },
    {"version", 'v', 0, G_OPTION_ARG_NONE, &version,
     N_("Display version and exit"), NULL },
    { NULL }
  };
#elif defined HAVE_POPT
  poptContext context = NULL;
  struct poptOption options[] =
  {
    {"export", 'e', POPT_ARG_STRING, NULL /* &export_file_name */, 0,
     N_("Export loaded file and exit"), N_("OUTPUT")},
    {"filter",'t', POPT_ARG_STRING, NULL /* &export_file_format */,
     0, export_format_string, N_("TYPE")
    },
    {"size", 's', POPT_ARG_STRING, NULL, 0,
     N_("Export graphics size"), N_("WxH")},
    {"show-layers", 'L', POPT_ARG_STRING, NULL, 0,  /* 13.3.2004 sampo@iki.fi */
     N_("Show only specified layers (e.g. when exporting). Can be either the layer name or a range of layer numbers (X-Y)"),
     N_("LAYER,LAYER,...")},
    {"nosplash", 'n', POPT_ARG_NONE, &nosplash, 0,
     N_("Don't show the splash screen"), NULL },
    {"nonew", 'n', POPT_ARG_NONE, &nonew, 0,
     N_("Don't create empty diagram"), NULL },
    {"log-to-stderr", 'l', POPT_ARG_NONE, &log_to_stderr, 0,
     N_("Send error messages to stderr instead of showing dialogs."), NULL },
    {"credits", 'c', POPT_ARG_NONE, &credits, 0,
     N_("Display credits list and exit"), NULL },
    {"verbose", 0, POPT_ARG_NONE, &verbose, 0,
     N_("Generate verbose output"), NULL },
    {"version", 'v', POPT_ARG_NONE, &version, 0,
     N_("Display version and exit"), NULL },
    {"help", 'h', POPT_ARG_NONE, 0, 1, N_("Show this help message") },
    {(char *) NULL, '\0', 0, NULL, 0}
  };
#endif

#if USE_GOPTION
  options[0].arg_data = &export_file_name;
  options[1].arg_data = &export_file_format;
  options[1].description = export_format_string;
  options[2].arg_data = &size;
  options[3].arg_data = &show_layers;
#elif defined HAVE_POPT
  options[0].arg = &export_file_name;
  options[1].arg = &export_file_format;
  options[2].arg = &size;
  options[3].arg = &show_layers;
#endif

  argv0 = (argc > 0) ? argv[0] : "(none)";

  gtk_set_locale();
  setlocale(LC_NUMERIC, "C");
  
#ifdef G_OS_WIN32
  /* calculate runtime directory */
  {
    gchar* localedir = dia_get_lib_directory ("locale");
    
    bindtextdomain(GETTEXT_PACKAGE, localedir);
    g_free (localedir);
  }
#else
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
#endif
  textdomain(GETTEXT_PACKAGE);

  process_opts(argc, argv, 
#if defined HAVE_POPT || USE_GOPTION
               context, options, 
#endif
               &files,
	     &export_file_name, &export_file_format, &size, &show_layers, &nosplash);

#if defined ENABLE_NLS && defined HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(GETTEXT_PACKAGE,"UTF-8");  
#endif
  textdomain(GETTEXT_PACKAGE);

  if (argv && dia_is_interactive && !version) {
#ifdef GNOME
    GnomeProgram *program =
      gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
			  argc, argv,
			  /* haven't found a quick way to pass GOption here */
			  GNOME_PARAM_POPT_TABLE, options,
			  GNOME_PROGRAM_STANDARD_PROPERTIES,
			  GNOME_PARAM_NONE);
    g_object_get(program, "popt-context", &context, NULL);
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
#  ifdef G_THREADS_ENABLED
    g_thread_init (NULL);
#  endif
    gtk_init(&argc, &argv);
#endif
  }
  else {
#ifdef G_THREADS_ENABLED
    g_thread_init (NULL);
#endif
    g_type_init();
#ifdef GDK_WINDOWING_WIN32
    /*
     * On windoze there is no command line without display so this call is harmless. 
     * But it is needed to avoid failing in gdk functions just because there is a 
     * display check. Still a little hack ...
     */
    gtk_init(&argc, &argv);
#endif
  }

  /* done with option parsing, don't leak */
  g_free(export_format_string);
  
  if (version) {
#if (defined __TIME__) && (defined __DATE__)
    /* TRANSLATOR: 2nd and 3rd %s are time and date respectively. */
    printf(g_locale_from_utf8(_("Dia version %s, compiled %s %s\n"), -1, NULL, NULL, NULL), VERSION, __TIME__, __DATE__);
#else
    printf(g_locale_from_utf8(_("Dia version %s\n"), -1, NULL, NULL, NULL), VERSION);
#endif
    if (verbose)
      dump_dependencies();
    exit(0);
  }

  if (!dia_is_interactive)
    log_to_stderr = TRUE;
  
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

  if (dia_is_interactive)
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
		  "object-libs; exiting...\n"));
    g_critical( _("Couldn't find standard objects when looking for "
	    "object-libs in '%s'; exiting...\n"), dia_get_lib_directory("dia"));
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
					 export_file_format, size, show_layers);
  if (dia_is_interactive && files == NULL && !nonew) {
    gchar *filename = g_filename_from_utf8(_("Diagram1.dia"), -1, NULL, NULL, NULL);
    Diagram *diagram = new_diagram (filename);
    g_free(filename);
    
    if (diagram != NULL) {
      diagram_update_extents(diagram);
      diagram->virtual = TRUE;
      layer_dialog_set_diagram(diagram);
      new_display(diagram);
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
    g_error(_("This shouldn't happen.  Please file a bug report at bugzilla.gnome.org\n"
	      "describing how you can cause this message to appear.\n"));
    return FALSE;
  }

  if (diagram_modified_exists()) {
    GtkWidget *dialog;
    GtkWidget *button;

    dialog = gtk_message_dialog_new(
	       NULL, GTK_DIALOG_MODAL,
               GTK_MESSAGE_QUESTION,
               GTK_BUTTONS_NONE, /* no standard buttons */
	       _("Quitting without saving modified diagrams"));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
		 _("Modified diagrams exist. "
		 "Are you sure you want to quit Dia "
 		 "without saving them?"));

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
  if (dia_is_interactive)
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
  if (mkdir(dir, 0755) && errno != EEXIST) {
#ifndef G_OS_WIN32
    g_critical(_("Could not create per-user Dia config directory"));
    exit(1);
#else /* HB: it this really a reason to exit the program on *nix ? */
    g_warning(_("Could not create per-user Dia config directory. Please make "
        "sure that the environment variable HOME points to an existing directory."));
#endif
  }

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
#if USE_GOPTION
	     GOptionContext *context, GOptionEntry options[],
#elif defined HAVE_POPT
	     poptContext poptCtx, struct poptOption options[],
#endif
	     GSList **files, char **export_file_name,
	     char **export_file_format, char **size,
	     char **show_layers, gboolean* nosplash)
{
#if defined HAVE_POPT && !USE_GOPTION
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
#if defined HAVE_POPT && !USE_GOPTION
      while (poptPeekArg(poptCtx)) {
          char *in_file_name = (char *)poptGetArg(poptCtx);
	  if (*in_file_name != '\0')
	    *files = g_slist_append(*files, in_file_name);
      }
      poptFreeContext(poptCtx);
#elif USE_GOPTION
      GError *error = NULL;
      int i;
      
      context = g_option_context_new(_("[FILE...]"));
      g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
#  if GTK_CHECK_VERSION(2,5,7)
      /* at least Gentoo was providing GLib-2.6 but Gtk+-2.4.14 */
      g_option_context_add_group (context, gtk_get_option_group (FALSE));
#  endif
      if (!g_option_context_parse (context, &argc, &argv, &error)) {
        if (error) { /* IMO !error here is a bug upstream, triggered with --gdk-debug=updates */
	g_print ("%s", error->message);
	  g_error_free (error);
	} else {
	  g_print ("Invalid option?");
	}
	
        g_option_context_free(context);
	exit(0);
      }
      if (argc < 2) {
        g_option_context_free(context);
	return;
      }
      for (i = 1; i < argc; i++) {
	if (!g_file_test (argv[i], G_FILE_TEST_IS_REGULAR)) {
	  g_print (_("'%s' not found!\n"), argv[i]);
          g_option_context_free(context);
	  exit(2);
	}
	*files = g_slist_append(*files, argv[i]);
      }
      g_option_context_free(context);
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
          } else if (0 == strcmp(argv[i],"-L")) {
              if (i < (argc-1)) {
                  i++;
                  *show_layers = argv[i];
                  continue;
              }
          } else if (0 == strcmp(argv[i],"-n")) {
	      *nosplash = 1;
	      continue;
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
		    char *export_file_format, char *size, char *show_layers)
{
  GSList *node = NULL;
  gboolean made_conversions = FALSE;

  for (node = files; node; node = node->next) {
    made_conversions |=
      handle_initial_diagram(node->data, export_file_name,
			     export_file_format, size, show_layers);
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
          g_print("%s\n", authors[i]);
      }

      g_print(_("\nThe current maintainers of Dia are:\n\n"));
      for (i = NUMBER_OF_ORIG_AUTHORS; i < NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i++) {
	  g_print("%s\n", authors[i]);
      }

      g_print(_("\nOther authors are:\n\n"));
      for (i = NUMBER_OF_ORIG_AUTHORS + NUMBER_OF_MAINTAINERS; i < nauthors; i++) {
          g_print("%s\n", authors[i]);
      }

      g_print(_("\nDia is documented by:\n\n"));
      for (i = 0; i < ndocumentors; i++) {
          g_print("%s\n", documentors[i]);
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

int app_is_embedded(void)
{
  return 0;
}
