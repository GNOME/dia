#include <config.h>

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h> /* just for version */

#ifdef G_OS_WIN32
#define Rectangle Win32Rectangle
#define WIN32_LEAN_AND_MEAN
#include <windows.h> /* native file api */
#undef Rectangle

#include "app_procs.h"
#include "interface.h"

static void dia_redirect_console (void);

/* In case we build this as a windowed application */

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
	 struct HINSTANCE__ *hPrevInstance,
	 char               *lpszCmdLine,
	 int                 nCmdShow)
{
  typedef BOOL (WINAPI *PFN_SetProcessDPIAware) (VOID);
  PFN_SetProcessDPIAware setProcessDPIAware;

  /* Try to claim DPI awareness (should work from Vista). This disables automatic
   * scaling of the user interface elements, so icons might get really small. But
   * for reasonable high dpi that should still look better than, e.g. scaling our
   * buttons to 125%.
   */
  setProcessDPIAware = (PFN_SetProcessDPIAware) GetProcAddress (GetModuleHandle ("user32.dll"),
								"SetProcessDPIAware");
  if (setProcessDPIAware)
    setProcessDPIAware ();

  dia_redirect_console ();

  app_init (__argc, __argv);

  if (!app_is_interactive())
    return 0;

  toolbox_show();

  app_splash_done();

  gtk_main ();

  return 0;
}

void
dia_log_func (const gchar    *log_domain,
              GLogLevelFlags  flags,
              const gchar    *message,
              gpointer        data)
{
  HANDLE file = (HANDLE)data;
  const char* level;
  static char* last_message = NULL;
  DWORD dwWritten; /* looks like being optional in MSDN, but isn't */

  /* some small optimization especially for the ugly tweaked font message */
  if (last_message && (0 == strcmp (last_message, message)))
    return;
  /* not using GLib functions! */
  if (last_message)
    free (last_message);
  last_message = strdup (message); /* finally leaked */

  WriteFile (file, log_domain ? log_domain : "?", log_domain ? strlen(log_domain) : 1, &dwWritten, 0);
  WriteFile (file, "-", 1, &dwWritten, 0);
  level = flags & G_LOG_LEVEL_ERROR ? "Error" :
             flags & G_LOG_LEVEL_CRITICAL ? "Critical" :
             flags & G_LOG_LEVEL_WARNING ? "Warning" :
             flags & G_LOG_LEVEL_INFO ? "Info" :
             flags & G_LOG_LEVEL_DEBUG ? "Debug" :
             flags & G_LOG_LEVEL_MESSAGE ? "Message" : "?";
  WriteFile (file, level, strlen(level), &dwWritten, 0);
  WriteFile (file, ": ", 2, &dwWritten, 0);
  WriteFile (file, message, strlen(message), &dwWritten, 0);
  WriteFile (file, "\r\n", 2, &dwWritten, 0);
  FlushFileBuffers (file);

  if (flags & G_LOG_FATAL_MASK)
    {
      /* should we also exit here or at least do MessageBox ?*/
    }
}

void
dia_print_func (const char* string)
{

}

void
dia_redirect_console (void)
{
  const gchar *log_domains[] =
  {
    "GLib", "GModule", "GThread", "GLib-GObject",
    "Pango", "PangoFT2", "PangoWin32", /* noone sets them ?*/
    "Gtk", "Gdk", "GdkPixbuf",
    "Dia", "DiaLib", "DiaObject", "DiaPlugin", "DiaPython",
    NULL,
  };
  static int MAX_TRIES = 16;
  int i = 0;
  HANDLE file = INVALID_HANDLE_VALUE;
  char logname[] = "dia--" VERSION ".log";
  char* redirected;
  gboolean verbose = TRUE;
  BY_HANDLE_FILE_INFORMATION fi = { 0, };

  if (   (   ((file = GetStdHandle (STD_OUTPUT_HANDLE)) != INVALID_HANDLE_VALUE)
          || ((file = GetStdHandle (STD_ERROR_HANDLE)) != INVALID_HANDLE_VALUE))
      && (GetFileInformationByHandle (file, &fi) && (fi.dwFileAttributes != 0)))
    verbose = FALSE; /* use it but not too much ;-) */
  else do
  {
    /* overwrite at startup */
    redirected = g_build_filename (g_get_tmp_dir (), logname, NULL);
    /* not using standard c runtime functions to
     * deal with possibly multiple instances
     */
    file = CreateFile (redirected, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, 0);
    if (file == INVALID_HANDLE_VALUE)
      {
        i++;
        g_clear_pointer (&redirected, g_free);
        logname[3] = '0' + i;
      }
  } while (file == INVALID_HANDLE_VALUE && i < MAX_TRIES);

  if (file != INVALID_HANDLE_VALUE)
    {
      char* log = g_strdup_printf ("Dia (%s) instance %d started "
                                   "using Gtk %d.%d.%d (%d)\r\n",
                                   VERSION, i + 1,
                                   gtk_major_version, gtk_minor_version, gtk_micro_version, gtk_binary_age);
      DWORD dwWritten; /* looks like being optional in msdn, but isn't */

      if (!verbose || WriteFile (file, log, strlen(log), &dwWritten, 0))
        {
          for (i = 0; i < G_N_ELEMENTS (log_domains); i++)
            g_log_set_handler (log_domains[i],
                               G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR,
                               dia_log_func,
                               file);
          /* don't redirect g_print () yet, it's upcoming API */
          g_set_print_handler (dia_print_func);
        }
      else
        {
          char* emsg = g_win32_error_message (GetLastError ());
          g_printerr ("Logfile %s writing failed: %s", redirected, emsg);
          g_clear_pointer (&emsg, g_free);
        }
      g_clear_pointer (&log, g_free);
    }
}

#endif
