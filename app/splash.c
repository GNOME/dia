#include "config.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "app_procs.h"
#include "dia-version-info.h"

static GtkWidget *splash = NULL;


static gboolean
splash_quit (gpointer data)
{
  gtk_main_quit ();

  return G_SOURCE_REMOVE;
}


static void
splash_realize (GtkWidget *widget, GdkEventExpose *event)
{
  /* this gets called after the splash screen gets realized ... */
  g_idle_add (splash_quit, NULL);
}


void
app_splash_init (const char *fname)
{
  GtkWidget *vbox;
  GtkWidget *gpixmap;
  GtkWidget *label;
  GtkWidget *frame;
  gchar *str;
  gulong signal_id;

  splash = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_role (GTK_WINDOW (splash), "start_dialog");
  gtk_window_set_title (GTK_WINDOW (splash), _("Loading …"));
  gtk_window_set_resizable (GTK_WINDOW (splash), FALSE);
  gtk_window_set_position (GTK_WINDOW (splash), GTK_WIN_POS_CENTER);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER(splash), vbox);

  gpixmap = gtk_image_new_from_resource ("/org/gnome/Dia/dia-splash.png");

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 1);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 1);

  gtk_container_add (GTK_CONTAINER(frame), gpixmap);

  str = g_strdup_printf (_("Dia v%s"), dia_version_string ());
  label = gtk_label_new (str);
  g_clear_pointer (&str, g_free);

  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 1);

  signal_id = g_signal_connect_after (G_OBJECT (splash),
                                      "realize", G_CALLBACK (splash_realize),
                                      NULL);

  gtk_widget_show_all (splash);

  /* splash_realize gets us out of this */
  gtk_main();

  g_signal_handler_disconnect (G_OBJECT (splash), signal_id);
}


void
app_splash_done (void)
{
  if (splash) {
    gtk_widget_destroy (splash);
    splash = NULL;
  }
}
