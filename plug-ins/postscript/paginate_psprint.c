/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * paginate_psprint.[ch] -- pagination code for the postscript backend
 * Copyright (C) 1999 James Henstridge
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

/* so we get popen and sigaction even when compiling with -ansi */
#define _POSIX_C_SOURCE 2
#include <signal.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "intl.h"
#include "message.h"
#include "diagramdata.h"
#include "render_eps.h"
#include "diapsrenderer.h"
#include "paginate_psprint.h"
#include "persistence.h"

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <io.h>
#include "win32print.h"
#define pclose(file) win32_printer_close (file)
#endif

/* keep track of print options between prints */
typedef struct _dia_print_options {
  int printer;
} dia_print_options;

static dia_print_options last_print_options =
{
    1
};

static void
count_objs(DiaObject *obj, DiaRenderer *renderer, int active_layer, guint *nobjs)
{
  (*nobjs)++;
}

static guint
print_page (DiagramData *data, DiaRenderer *diarend, DiaRectangle *bounds)
{
  DiaPsRenderer *rend = DIA_PS_RENDERER(diarend);
  guint nobjs = 0;
  gfloat tmargin = data->paper.tmargin, bmargin = data->paper.bmargin;
  gfloat lmargin = data->paper.lmargin;
  gfloat scale = data->paper.scaling;
  gchar d1_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar d2_buf[G_ASCII_DTOSTR_BUF_SIZE];

  rend->paper = data->paper.name;
  rend->is_portrait = data->paper.is_portrait;

  /* count the number of objects in this region */
  data_render(data, diarend, bounds,
              (ObjectRenderer) count_objs, &nobjs);

  if (nobjs == 0)
    return nobjs;

  /* output a page number comment */
  fprintf(rend->file, "%%%%Page: %d %d\n", rend->pagenum, rend->pagenum);
  rend->pagenum++;

  /* save print context */
  fprintf(rend->file, "gs\n");

  /* transform coordinate system */
  if (data->paper.is_portrait) {
    fprintf(rend->file, "%s %s scale\n",
	    g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", 28.346457*scale),
	    g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", -28.346457*scale) );
    fprintf(rend->file, "%s %s translate\n",
	    g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", lmargin/scale - bounds->left),
	    g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", -bmargin/scale - bounds->bottom) );
  } else {
    fprintf(rend->file, "90 rotate\n");
    fprintf(rend->file, "%s %s scale\n",
	    g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", 28.346457*scale),
	    g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", -28.346457*scale) );
    fprintf(rend->file, "%s %s translate\n",
	    g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", lmargin/scale - bounds->left),
	    g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", tmargin/scale - bounds->top) );
  }

  /* set up clip mask */
  fprintf(rend->file, "n %s %s m ",
	  g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", bounds->left),
	  g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", bounds->top) );
  fprintf(rend->file, "%s %s l ",
	  g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", bounds->right),
	  g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", bounds->top) );
  fprintf(rend->file, "%s %s l ",
	  g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", bounds->right),
	  g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", bounds->bottom) );
  fprintf(rend->file, "%s %s l ",
	  g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", bounds->left),
	  g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", bounds->bottom) );
  fprintf(rend->file, "%s %s l ",
	  g_ascii_formatd(d1_buf, sizeof(d1_buf), "%f", bounds->left),
	  g_ascii_formatd(d2_buf, sizeof(d2_buf), "%f", bounds->top) );
  /* Tip from Dov Grobgeld: Clip does not destroy the path, so we should
     do a newpath afterwards */
  fprintf(rend->file, "clip n\n");

  /* render the region */
  data_render(data, diarend, bounds, NULL, NULL);

  /* restore print context */
  fprintf(rend->file, "gr\n");

  /* print the page */
  fprintf(rend->file, "showpage\n\n");

  return nobjs;
}

void
paginate_psprint(DiagramData *dia, FILE *file)
{
  DiaRenderer *rend;
  DiaRectangle *extents;
  gfloat width, height;
  gfloat x, y, initx, inity;
  guint nobjs = 0;

  rend = new_psprint_renderer(dia, file);

#ifdef DIA_PS_RENDERER_DUAL_PASS
  /* Prepare the prolog (with fonts etc) */
  data_render(dia, DIA_RENDERER(rend), NULL, NULL, NULL);
  eps_renderer_prolog_done(rend);
#endif

  /* the usable area of the page */
  width = dia->paper.width;
  height = dia->paper.height;

  /* get extents, and make them multiples of width / height */
  extents = &dia->extents;
  initx = extents->left;
  inity = extents->top;
  /* make page boundaries align with origin */
  if (!dia->paper.fitto) {
    initx = floor(initx / width)  * width;
    inity = floor(inity / height) * height;
  }

  /* iterate through all the pages in the diagram */
  for (y = inity; y < extents->bottom; y += height) {
    /* ensure we are not producing pages for epsilon */
    if ((extents->bottom - y) < 1e-6)
      break;

    for (x = initx; x < extents->right; x += width) {
      DiaRectangle page_bounds;

      if ((extents->right - x) < 1e-6)
        break;

      page_bounds.left = x;
      page_bounds.right = x + width;
      page_bounds.top = y;
      page_bounds.bottom = y + height;

      nobjs += print_page (dia,rend, &page_bounds);
    }
  }

  g_clear_object (&rend);
}


static void
change_entry_state(GtkToggleButton *radio, GtkWidget *entry)
{
  gtk_widget_set_sensitive(entry, gtk_toggle_button_get_active(radio));
}

static void
ok_pressed(GtkButton *button, gboolean *flag)
{
  *flag = TRUE;
  gtk_main_quit();
}

static gboolean sigpipe_received = FALSE;
static void
pipe_handler(int signum)
{
  sigpipe_received = TRUE;
}


static gboolean
diagram_print_destroy (GtkWidget *widget)
{
  DiagramData *dia;

  if ((dia = g_object_get_data (G_OBJECT (widget), "diagram")) != NULL) {
    g_clear_object (&dia);
    g_object_set_data (G_OBJECT (widget), "diagram", NULL);
  }

  return FALSE;
}


void
diagram_print_ps (DiagramData *dia, const gchar* original_filename)
{
  GtkWidget *dialog;
  GtkWidget *vbox, *frame, *table, *box, *button;
  GtkWidget *iscmd, *isofile;
  GtkWidget *cmd, *ofile;
  gboolean cont = FALSE;
  gchar *printcmd = NULL;
  gchar *orig_command, *orig_file;
  gchar *filename = NULL;
  gchar *printer_filename = NULL;
  gchar *dot = NULL;
  gboolean done = FALSE;
  gboolean write_file = TRUE;	/* Used in prompt to overwrite existing file */

  FILE *file;
  gboolean is_pipe;
#ifndef G_OS_WIN32
  /* all the signal stuff below doesn't compile on win32, but it isn't
   * needed anymore because the pipe handling - which never worked on win32
   * anyway - is replace by "native" postscript printing now ...
   */
  struct sigaction old_sigpipe_action, sigpipe_action;
#endif

  /* create the dialog */
  dialog = gtk_dialog_new ();
  /* the dialog has it's own reference to the diagram */
  g_object_ref (dia);
  g_object_set_data (G_OBJECT (dialog), "diagram", dia);
  g_signal_connect (G_OBJECT (dialog), "destroy",
                    G_CALLBACK (diagram_print_destroy), NULL);
  g_signal_connect (G_OBJECT (dialog), "delete_event",
                    G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (dialog), "delete_event",
                    G_CALLBACK (gtk_true), NULL);
  vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  frame = gtk_frame_new (_("Select Printer"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  iscmd = gtk_radio_button_new_with_label (NULL, _("Printer"));
  gtk_table_attach (GTK_TABLE (table), iscmd, 0,1, 0,1,
                    GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show (iscmd);

  cmd = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), cmd, 1,2, 0,1,
                    GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show (cmd);

  g_signal_connect (G_OBJECT (iscmd), "toggled",
                    G_CALLBACK (change_entry_state), cmd);

  isofile = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (iscmd)),
                                             _("File"));
  gtk_table_attach (GTK_TABLE (table), isofile, 0,1, 1,2,
                    GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show (isofile);

  ofile = gtk_entry_new ();
  gtk_widget_set_sensitive (ofile, FALSE);
  gtk_table_attach (GTK_TABLE (table), ofile, 1,2, 1,2,
                    GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show (ofile);
  g_signal_connect (G_OBJECT (isofile), "toggled",
                    G_CALLBACK (change_entry_state), ofile);

  box = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

  button = gtk_button_new_with_label (_("OK"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (ok_pressed), &cont);
  gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
  gtk_box_pack_start (GTK_BOX(box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /* Set default or old dialog values: */
#ifdef G_OS_WIN32
  gtk_entry_set_text (GTK_ENTRY (cmd), win32_printer_default ());
#else
  {
    const gchar *printer = g_getenv ("PRINTER");

    if (printer) {
      printcmd = g_strdup_printf ("lpr -P%s", printer);
    } else {
      printcmd = g_strdup ("lpr");
    }

    gtk_entry_set_text (GTK_ENTRY (cmd), printcmd);
    g_clear_pointer (&printcmd, g_free);
    printcmd = NULL;
  }
#endif
  persistence_register_string_entry ("printer-command", cmd);
  printcmd = g_strdup (gtk_entry_get_text (GTK_ENTRY (cmd)));
  orig_command = printcmd;

  /* Work out diagram filename and use this as default .ps file */
  filename = g_path_get_basename (original_filename);

  printer_filename = g_malloc (strlen (filename) + 4);
  printer_filename = strcpy (printer_filename, filename);
  dot = strrchr (printer_filename, '.');
  if ((dot != NULL) && (strcmp (dot, ".dia") == 0)) {
    *dot = '\0';
  }
  printer_filename = strcat (printer_filename, ".ps");
  gtk_entry_set_text (GTK_ENTRY (ofile), printer_filename);
  g_clear_pointer (&printer_filename, g_free);
  orig_file = g_strdup (gtk_entry_get_text (GTK_ENTRY (ofile)));

  /* Scaling is already set at creation. */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (iscmd),
                                last_print_options.printer);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (isofile),
                                !last_print_options.printer);

  gtk_widget_show (dialog);

  do {
    cont = FALSE;
    write_file = TRUE;
    gtk_main ();

    if (!dia) {
      gtk_widget_destroy (dialog);
      return;
    }

    if (!cont) {
      persistence_change_string_entry ("printer-command", orig_command, cmd);
      gtk_widget_destroy (dialog);
      g_clear_pointer (&orig_command, g_free);
      g_clear_pointer (&orig_file, g_free);
      return;
    }

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (iscmd))) {
      printcmd = g_strdup (gtk_entry_get_text (GTK_ENTRY (cmd)));
#ifdef G_OS_WIN32
      file = win32_printer_open (printcmd);
#else
      file = popen (printcmd, "w");
#endif
      is_pipe = TRUE;
    } else {
      const char *new_filename = gtk_entry_get_text (GTK_ENTRY (ofile));

      if (g_file_test (new_filename, G_FILE_TEST_EXISTS)) {
        /* Output file exists */
        GtkWidget *confirm_overwrite_dialog = NULL;
        char *utf8filename = NULL;

        if (!g_utf8_validate (new_filename, -1, NULL)) {
          utf8filename = g_filename_to_utf8 (new_filename, -1, NULL, NULL, NULL);

          if (utf8filename == NULL) {
            message_warning (_("Some characters in the filename are neither "
                               "UTF-8\nnor your local encoding.\n"
                               "Some things will break."));
          }
        }

        if (utf8filename == NULL) {
          utf8filename = g_strdup (new_filename);
        }
        confirm_overwrite_dialog =
          gtk_message_dialog_new (GTK_WINDOW (dialog),
                                  GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_YES_NO,
                                  _("The file '%s' already exists.\n"
                                    "Do you want to overwrite it?"),
                                  utf8filename);
        g_clear_pointer (&utf8filename, g_free);
        gtk_window_set_title (GTK_WINDOW (confirm_overwrite_dialog),
                              _("File already exists"));
        gtk_dialog_set_default_response (GTK_DIALOG (confirm_overwrite_dialog),
                                         GTK_RESPONSE_NO);

        if (gtk_dialog_run (GTK_DIALOG (confirm_overwrite_dialog))
                         != GTK_RESPONSE_YES) {
          write_file = FALSE;
          file = 0;
        }

        gtk_widget_destroy (confirm_overwrite_dialog);
      }

      if (write_file) {
        if (!g_path_is_absolute (new_filename)) {
          char *full_filename;

          full_filename = g_build_filename (g_get_home_dir (),
                                            new_filename, NULL);
          file = g_fopen (full_filename, "w");
          g_clear_pointer (&full_filename, g_free);
        } else {
          file = g_fopen (new_filename, "w");
        }
      }

      is_pipe = FALSE;
    }

    /* Store dialog values */
    last_print_options.printer =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (iscmd));

    if (write_file) {
      if (!file) {
        if (is_pipe) {
          message_warning (_("Could not run command '%s': %s"),
                           printcmd, strerror (errno));
          g_clear_pointer (&printcmd, g_free);
        } else
          message_warning (_("Could not open '%s' for writing: %s"),
                           gtk_entry_get_text (GTK_ENTRY (ofile)),
                           strerror(errno));
      } else
        done = TRUE;
    }
  } while (!done);

  g_clear_pointer (&orig_command, g_free);
  g_clear_pointer (&orig_file, g_free);
#ifndef G_OS_WIN32
  /* set up a SIGPIPE handler to catch IO errors, rather than segfaulting */
  sigpipe_received = FALSE;
  memset (&sigpipe_action, 0, sizeof (struct sigaction));
  sigpipe_action.sa_handler = pipe_handler;
  sigaction (SIGPIPE, &sigpipe_action, &old_sigpipe_action);
#endif

  paginate_psprint (dia, file);
  gtk_widget_destroy (dialog);
  if (is_pipe) {
    int exitval = pclose (file);
    if (exitval != 0) {
      message_error (_("Printing error: command '%s' returned %d\n"),
                     printcmd, exitval);
    }
  } else {
    fclose (file);
  }

#ifndef G_OS_WIN32
  /* restore original behaviour */
  sigaction (SIGPIPE, &old_sigpipe_action, NULL);
#endif
  if (sigpipe_received) {
    message_error (_("Printing error: command '%s' caused sigpipe."),
                   printcmd);
  }

  if (is_pipe) {
    g_clear_pointer (&printcmd, g_free);
  }
}
