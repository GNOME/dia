#include <stdio.h>
#include "intl.h"
#include "diagram.h"
#include "diagramdata.h"
#include "render_eps.h"
#include "paginate_psprint.h"

#include <gtk/gtk.h>

/* Paper definitions stollen from gnome-libs.
 * All measurements are in centimetres. */
static const struct _dia_paper_metrics {
  gchar *paper;
  gdouble pswidth, psheight;
  gdouble lmargin, tmargin, rmargin, bmargin;
} paper_metrics[] = {
  { "A3", 29.7, 42.0, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A4", 21.0, 29.7, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "A5", 14.85, 21.0, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "B4", 25.7528, 36.4772, 2.1167, 2.1167, 2.1167, 2.1167 },
  { "B5", 17.6389, 25.0472, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "B5-Japan", 18.2386, 25.7528, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "Letter", 21.59, 27.94, 2.54, 2.54, 2.54, 2.54 },
  { "Legal", 21.59, 35.56, 2.54, 2.54, 2.54, 2.54 },
  { "Half-Letter", 21.59, 14.0, 2.54, 2.54, 2.54, 2.54 },
  { "Executive", 18.45, 26.74, 2.54, 2.54, 2.54, 2.54 },
  { "Tabloid", 28.01, 43.2858, 2.54, 2.54, 2.54, 2.54 },
  { "Monarch", 9.8778, 19.12, 0.3528, 0.3528, 0.3528, 0.3528 },
  { "SuperB", 29.74, 43.2858, 2.8222, 2.8222, 2.8222, 2.8222 },
  { "Envelope-Commercial", 10.5128, 24.2, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "Envelope-Monarch", 9.8778, 19.12, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "Envelope-DL", 11.0, 22.0, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "Envelope-C5", 16.2278, 22.9306, 0.1764, 0.1764, 0.1764, 0.1764 },
  { "EuroPostcard", 10.5128, 14.8167, 0.1764, 0.1764, 0.1764, 0.1764 },
  { NULL, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
};

/* keep track of print options between prints */
typedef struct _dia_print_options {
    int printer;
    char *command;
    char *output;
    int paper;
    float scaling;
} dia_print_options;
static dia_print_options last_print_options = 
{
    0, NULL, NULL, 0, 1.000
};

static void
count_objs(Object *obj, Renderer *renderer, int active_layer, guint *nobjs)
{
  (*nobjs)++;
}

static guint
print_page(DiagramData *data, RendererEPS *rend, Rectangle *bounds,
           gdouble lmargin, gdouble bmargin, gdouble scale)
{
  guint nobjs = 0;

  /* count the number of objects in this region */
  data_render(data, (Renderer *)rend, bounds,
              (ObjectRenderer) count_objs, &nobjs);

  if (nobjs == 0)
    return nobjs;

  /* output a page number comment */
  fprintf(rend->file, "%%%%Page: %d %d\n", rend->pagenum, rend->pagenum);
  rend->pagenum++;

  /* save print context */
  fprintf(rend->file, "gs\n");

  /* transform coordinate system */
  fprintf(rend->file, "%f %f scale\n", 28.346457 * scale, -28.346457 * scale);
  fprintf(rend->file, "%f %f translate\n", lmargin/scale - bounds->left,
	  -bmargin/scale - bounds->bottom);

  /* set up clip mask */
  fprintf(rend->file, "n %f %f m ", bounds->left, bounds->top);
  fprintf(rend->file, "%f %f l ", bounds->right, bounds->top);
  fprintf(rend->file, "%f %f l ", bounds->right, bounds->bottom);
  fprintf(rend->file, "%f %f l ", bounds->left, bounds->bottom);
  fprintf(rend->file, "%f %f l ", bounds->left, bounds->top);
  fprintf(rend->file, "clip\n");

  /* render the region */
  data_render(data, (Renderer *)rend, bounds, NULL, NULL);

  /* restore print context */
  fprintf(rend->file, "gr\n");

  /* print the page */
  fprintf(rend->file, "showpage\n\n");

  return nobjs;
}

void
paginate_psprint(Diagram *dia, FILE *file, const gchar *paper_name,
		 gdouble scale)
{
  RendererEPS *rend;
  Rectangle *extents;
  gint i;
  gdouble pswidth, psheight, lmargin, tmargin, rmargin, bmargin;
  gdouble width, height;
  gdouble x, y;
  guint nobjs = 0;

  rend = new_psprint_renderer(dia, file, paper_name);

  /* default to A4 metrics */
  pswidth = 21.0;
  psheight = 29.7;
  lmargin = rmargin = tmargin = bmargin = 2.82222;

  for (i = 0; paper_metrics[i].paper != NULL; i++)
    if (!strcmp(paper_name, paper_metrics[i].paper)) {
      pswidth  = paper_metrics[i].pswidth;
      psheight = paper_metrics[i].psheight;
      lmargin  = paper_metrics[i].lmargin;
      tmargin  = paper_metrics[i].tmargin;
      rmargin  = paper_metrics[i].rmargin;
      bmargin  = paper_metrics[i].bmargin;
      break;
    }

  /* the usable area of the page */
  width = pswidth - lmargin - rmargin;
  height = psheight - tmargin - bmargin;

  /* scale width/height */
  width  /= scale;
  height /= scale;

  /* get extents, and make them multiples of width / height */
  extents = &dia->data->extents;
  /*extents->left -= extents->left % width;
    extents->right += width - extents->right % width;
    extents->top -= extents->top % height;
    extents->bottom += height - extents->bottom % height;*/

  /* iterate through all the pages in the diagram */
  for (y = extents->top; y < extents->bottom; y += height)
    for (x = extents->left; x < extents->right; x += width) {
      Rectangle page_bounds;

      page_bounds.left = x;
      page_bounds.right = x + width;
      page_bounds.top = y;
      page_bounds.bottom = y + height;

      nobjs += print_page(dia->data,rend, &page_bounds, lmargin,bmargin,scale);
    }

  g_free(rend);
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

void
diagram_print_ps(Diagram *dia)
{
  GtkWidget *dialog;
  GtkWidget *vbox, *frame, *table, *box, *button;
  GtkWidget *iscmd, *isofile;
  GtkWidget *cmd, *ofile;
  GtkWidget *papersel;
  GtkWidget *scaler;
  GList *items = NULL;
  int i;
  gboolean cont = FALSE;
  
  FILE *file;
  gboolean is_pipe;
  gchar *papername;
  gdouble scale;

  /* create the dialog */
  dialog = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
		     GTK_SIGNAL_FUNC(gtk_true), NULL);
  vbox = GTK_DIALOG(dialog)->vbox;

  frame = gtk_frame_new(_("Select Printer"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  table = gtk_table_new(2, 2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(table);

  iscmd = gtk_radio_button_new_with_label(NULL, _("Printer"));
  gtk_table_attach(GTK_TABLE(table), iscmd, 0,1, 0,1,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(iscmd);

  cmd = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(cmd), 
		     last_print_options.command 
		     ? last_print_options.command : "lpr");
  gtk_table_attach(GTK_TABLE(table), cmd, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(cmd);
  gtk_signal_connect(GTK_OBJECT(iscmd), "toggled",
		     GTK_SIGNAL_FUNC(change_entry_state), cmd);

  isofile = gtk_radio_button_new_with_label(GTK_RADIO_BUTTON(iscmd)->group,
					    _("File"));
  gtk_table_attach(GTK_TABLE(table), isofile, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(isofile);

  ofile = gtk_entry_new();
  gtk_widget_set_sensitive(ofile, FALSE);
  gtk_entry_set_text(GTK_ENTRY(ofile), 
		     last_print_options.output 
		     ? last_print_options.output : "output.ps");
  gtk_table_attach(GTK_TABLE(table), ofile, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(ofile);
  gtk_signal_connect(GTK_OBJECT(isofile), "toggled",
		     GTK_SIGNAL_FUNC(change_entry_state), ofile);

  frame = gtk_frame_new(_("Select Paper"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  papersel = gtk_combo_new();
  gtk_combo_set_value_in_list(GTK_COMBO(papersel), TRUE, TRUE);
  for (i = 0; paper_metrics[i].paper != NULL; i++)
    items = g_list_append(items, paper_metrics[i].paper);
  gtk_combo_set_popdown_strings(GTK_COMBO(papersel), items);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(papersel)->entry), "A4");
  gtk_box_pack_start(GTK_BOX(box), papersel, TRUE, TRUE, 0);
  /* TODO: set selection to last_print_options.paper */
  gtk_widget_show(papersel);

  frame = gtk_frame_new(_("Scaling"));
  gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  scaler = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(last_print_options.scaling,0.1,10.0,0.1,0.1,1.0)), 0, 3);
  gtk_box_pack_start(GTK_BOX(box), scaler, TRUE, TRUE, 0);
  gtk_widget_show(scaler);

  box = GTK_DIALOG(dialog)->action_area;

  button = gtk_button_new_with_label(_("OK"));
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     GTK_SIGNAL_FUNC(ok_pressed), &cont);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label(_("Cancel"));
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  gtk_widget_show(dialog);
  gtk_main();

  if (!cont) {
    gtk_widget_destroy(dialog);
    return;
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(iscmd))) {
    file = popen(gtk_entry_get_text(GTK_ENTRY(cmd)), "w");
    is_pipe = TRUE;

    last_print_options.printer = 0;
    if ( last_print_options.command ) 
	g_free( last_print_options.command );
    last_print_options.command = 
	g_strdup( gtk_entry_get_text(GTK_ENTRY(cmd)) );
    
  } else {
    file = fopen(gtk_entry_get_text(GTK_ENTRY(ofile)), "w");
    is_pipe = FALSE;

    last_print_options.printer = 1;
    if ( last_print_options.output ) 
	g_free( last_print_options.output );
    last_print_options.output = 
	g_strdup( gtk_entry_get_text(GTK_ENTRY(ofile)) );
  }

  papername = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(papersel)->entry));
  last_print_options.paper = 
      ( (char*)(GTK_COMBO(papersel)->entry) - (char *)(paper_metrics) ) 
      / sizeof( struct _dia_paper_metrics );

  scale = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(scaler));
  last_print_options.scaling = scale;

  if (!file)
    g_warning("could not open file");
  else
    paginate_psprint(dia, file, papername, scale);
  gtk_widget_destroy(dialog);
  if (is_pipe)
    pclose(file);
  else
    fclose(file);
}
