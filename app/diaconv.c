/* Diaconv -- a text-mode converter tool for dia (well, sort of).
 * Copyright (C) 2001 Cyrille Chepelov, with lots of bits reused from code
 * Copyright (C) 1998-2000 Alexander Larsson and others.
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
#include <gmodule.h>

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
#include "utils.h"
#include "filter.h"

#if (defined(HAVE_POPT_H))
#include <popt.h>

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
extern DiaExportFilter png_export_filter;
#endif

static PluginInitResult internal_plugin_init(PluginInfo *info);
static void stderr_message_internal(char *title, const char *fmt,
                                    va_list *args,  va_list *args2);

const char *argv0 = NULL;
int quiet = 0;

static char *
build_output_file_name(const char *infname, const char *format)
{
  /* FIXME */
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

static void 
do_convert(const char *infname,
           const char *outfname)
{
  DiaExportFilter *ef = NULL;
  DiaImportFilter *inf = NULL;
  DiagramData *diagdata = NULL;

  if (0==strcmp(infname,outfname)) {
    fprintf(stderr,
            _("%s error: input and output file name is identical: %s"),
            argv0, infname);
    exit(1);
  }
  
  printf("about to get diagram\n");
  diagdata = new_diagram_data();
  printf("to get diagram\n");
  inf = filter_guess_import_filter(infname);
  if (!inf) 
    inf = &dia_import_filter;
  printf("got a filter\n");
  
  if (!inf->import(infname,diagdata,inf->user_data)) {
    fprintf(stderr,
            _("%s error: need valid input file %s\n"),
            argv0,infname);
            exit(1);
  }
  printf("got diagram\n");
  ef = filter_guess_export_filter(outfname);
  printf("got exp filter\n");
  if (!ef) {
    fprintf(stderr,
            _("%s error: don't know how to export into %s\n"),
            argv0,outfname);
            exit(1);
  }
  ef->export(diagdata, outfname, infname, ef->user_data);
  if (!quiet) fprintf(stdout,
                      _("%s --> %s\n"),
                        infname,outfname);
  diagram_data_destroy(diagdata);
}


int 
main(int argc, char **argv)
{
  char *export_file_format = NULL;
  char *export_file_name = NULL;
  const char *in_file_name = NULL;
  int rc = 0;

  poptContext poptCtx = NULL;
  struct poptOption options[] = 
  {
    {"to", 't', POPT_ARG_STRING, NULL /* &export_file_format*/ , 0, 
     N_("Export format to use"), N_("eps,png,wmf,cgm,dxf,fig")},
    {"output",'o', POPT_ARG_STRING, NULL /* &export_file_name*/, 0,
     N_("Export file name to use"), N_("OUTPUT")},
    {"help", 'h', POPT_ARG_NONE, 0, 1, N_("Show this help message") },
    {"quiet", 'q', POPT_ARG_NONE, 0, 2, N_("Quiet operation") },
    {(char *) NULL, '\0', 0, NULL, 0}
  };

  fprintf(stderr,"THIS THING IS BROKEN. IT DOESN'T WORK. IT WILL CRASH.\n");
  options[0].arg = &export_file_format;
  options[1].arg = &export_file_name;

  argv0 = argv[0];

  printf("hi !\n");

  set_message_func(stderr_message_internal);
  setlocale(LC_NUMERIC,"C");
#ifdef HAVE_UNICODE
  unicode_init();
#endif 
  bindtextdomain(PACKAGE, LOCALEDIR);
#ifdef UNICODE_WORK_IN_PROGRESS
  /*  bind_textdomain_codeset(PACKAGE,"UTF-8"); */ /* only for GTK2 -- CC */
#endif
  textdomain(PACKAGE);

  printf("hi !\n");

  if (argv) {
    poptCtx = poptGetContext(PACKAGE, argc, argv, options, 0);
    poptSetOtherOptionHelp(poptCtx, _("[OPTION...] [FILE...]"));
    printf("hi !\n");

    while (rc >= 0) {
      rc = poptGetNextOpt(poptCtx);
      if (rc < -1) {
        fprintf(stderr, 
                _("Error on option %s: %s.\nRun '%s --help' to see a full "
                  "list of available command line options.\n"),
                poptBadOption(poptCtx, 0),
                poptStrerror(rc),
                argv[0]);
        exit(1);
      }    
      switch (rc) {
      case -1: break;
      case 1:
        poptPrintHelp(poptCtx, stdout, 0);
        exit(0);
      case 2:
        quiet = 1;
        break;
      default:
        break;
      }
    }
  } else {
      fprintf(stderr, 
              _("Error: No arguments found.\nRun '%s --help' to see a full "
                "list of available command line options.\n"),
              argv[0]);
      exit(1);
  }
  printf("hop !\n");

  /* we have leftover arguments now to process. */
  if (export_file_format && export_file_name) {
    fprintf(stderr,
            _("%s error: can specify only one of -t or -o."),
            argv[0]);
    exit(1);
  }
  if (!(export_file_format || export_file_name)) {
    fprintf(stderr,
            _("%s error: must specify only one of -t or -o.\n"
              "Run '%s --help' to see a full list "
              "of available command line options.\n"),
            argv[0],argv[0]);
    exit(1);
  }
  in_file_name = poptGetArg(poptCtx);
  if (!in_file_name) {
    fprintf(stderr,
            _("%s error: no input file."),
            argv[0]);
    exit(1);
  }

  printf("hip !\n");
  //dia_image_init();
  printf("hop !\n");
  //color_init();
  color_black.red = color_black.green = color_black.blue = 0.0;
  color_white.red = color_white.green = color_white.blue = 1.0;

  printf("hip !\n");
  font_init();
  printf("hop !\n");
  object_registry_init();
  printf("hap !\n");

  dia_register_plugins();
  printf("hup !\n");
  dia_register_builtin_plugin(internal_plugin_init);
  printf("hip !\n");

  load_all_sheets();
  printf("hzp !\n");

  if (object_get_type("Standard - Box") == NULL) {
    message_error(_("Couldn't find standard objects when looking for "
                  "object-libs, exiting...\n"));
    fprintf(stderr, _("Couldn't find standard objects when looking for "
            "object-libs, exiting...\n"));
    exit(1);
  }


  if (export_file_format) {
    /* we know which target file format. We can now load diagrams and save 
       them in turn. */    
    
    while (in_file_name) {
      g_message("in_file_name = %s",in_file_name);
      export_file_name = build_output_file_name(in_file_name,
                                                export_file_format);
      g_message("export_file_name = %s",export_file_name);
      do_convert(in_file_name,export_file_name);
      g_free(export_file_name);
      in_file_name = poptGetArg(poptCtx);
    }
  } else {
    /* we know one output name, and normally only one input name. */
    const char *next_in_file_name = poptGetArg(poptCtx);
    if (next_in_file_name) {
      fprintf(stderr,
              _("%s error: only one input file expected."),
              argv[0]);
      exit(1);
    }
    do_convert(in_file_name,export_file_name);
  }
  exit(0);
}

#else /* this is ugly. FIXME. */
int 
main() {
  fprintf(stderr,
          _("%s error: popt library not available on this system"),
          argv[0]);
  exit(1);
}
  
#endif /* HAVE_POPT */

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

int app_is_embedded(void) 
{
  return 0;
}

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
          "%s %s: %s\n", 
          argv0,title,buf);
}
