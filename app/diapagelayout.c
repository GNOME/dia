#include "diapagelayout.h"

#include "intl.h"

#include "pixmaps/portrait.xpm"
#include "pixmaps/landscape.xpm"

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


static void dia_page_layout_class_init(DiaPageLayoutClass *class);
static void dia_page_layout_init(DiaPageLayout *self);

GtkType
dia_page_layout_get_type(void)
{
  static GtkType pl_type = 0;

  if (!pl_type) {
    GtkTypeInfo pl_info = {
      "DiaPageLayout",
      sizeof(DiaPageLayout),
      sizeof(DiaPageLayoutClass),
      (GtkClassInitFunc) dia_page_layout_class_init,
      (GtkObjectInitFunc) dia_page_layout_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
    };
    pl_type = gtk_type_unique(gtk_table_get_type(), &pl_info);
  }
  return pl_type;
}

static void
dia_page_layout_class_init(DiaPageLayoutClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
}

static void
dia_page_layout_init(DiaPageLayout *self)
{
  GtkWidget *frame, *box, *table, *menu, *menuitem, *wid;
  GdkPixmap *pix;
  GdkBitmap *mask;
  gint i;

  gtk_table_resize(GTK_TABLE(self), 3, 2);
  gtk_table_set_row_spacings(GTK_TABLE(self), 5);
  gtk_table_set_col_spacings(GTK_TABLE(self), 5);

  /* paper size */
  frame = gtk_frame_new(_("Paper Size"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->paper_size = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), self->paper_size, TRUE, FALSE, 0);

  menu = gtk_menu_new();
  for (i = 0; paper_metrics[i].paper != NULL; i++) {
    menuitem = gtk_menu_item_new_with_label(paper_metrics[i].paper);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(i));
    gtk_container_add(GTK_CONTAINER(menu), menuitem);
    gtk_widget_show(menuitem);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(self->paper_size), menu);
  gtk_widget_show(self->paper_size);

  /* orientation */
  frame = gtk_frame_new(_("Orientation"));
  gtk_table_attach(GTK_TABLE(self), frame, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->orient_portrait = gtk_radio_button_new(NULL);
  pix = gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(GTK_WIDGET(self)), &mask, NULL,
		portrait_xpm);
  wid = gtk_pixmap_new(pix, mask);
  gdk_pixmap_unref(pix);
  gdk_bitmap_unref(mask);
  gtk_container_add(GTK_CONTAINER(self->orient_portrait), wid);
  gtk_widget_show(wid);

  gtk_box_pack_start(GTK_BOX(box), self->orient_portrait, TRUE, TRUE, 0);
  gtk_widget_show(self->orient_portrait);

  self->orient_landscape = gtk_radio_button_new(
	gtk_radio_button_group(GTK_RADIO_BUTTON(self->orient_portrait)));
  pix = gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(GTK_WIDGET(self)), &mask, NULL,
		landscape_xpm);
  wid = gtk_pixmap_new(pix, mask);
  gdk_pixmap_unref(pix);
  gdk_bitmap_unref(mask);
  gtk_container_add(GTK_CONTAINER(self->orient_landscape), wid);
  gtk_widget_show(wid);

  gtk_box_pack_start(GTK_BOX(box), self->orient_landscape, TRUE, TRUE, 0);
  gtk_widget_show(self->orient_landscape);

  /* margins */
  frame = gtk_frame_new(_("Margins"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  table = gtk_table_new(4, 2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(table);

  wid = gtk_label_new(_("Top:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 0,1,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->tmargin = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)), 1, 1);
  gtk_table_attach(GTK_TABLE(table), self->tmargin, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->tmargin);

  wid = gtk_label_new(_("Bottom:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->bmargin = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)), 1, 1);
  gtk_table_attach(GTK_TABLE(table), self->bmargin, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->bmargin);

  wid = gtk_label_new(_("Left:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 2,3,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->lmargin = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)), 1, 1);
  gtk_table_attach(GTK_TABLE(table), self->lmargin, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->lmargin);

  wid = gtk_label_new(_("Right:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 3,4,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->rmargin = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)), 1, 1);
  gtk_table_attach(GTK_TABLE(table), self->rmargin, 1,2, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->rmargin);

  /* Scaling */
  frame = gtk_frame_new(_("Scaling"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 2,3,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->scaling = gtk_spin_button_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(100,0,10000, 1,10,10)), 1, 1);
  gtk_box_pack_start(GTK_BOX(box), self->scaling, TRUE, FALSE, 0);
  gtk_widget_show(self->scaling);

  self->fittopage = gtk_button_new_with_label(_("Fit to Page"));
  gtk_box_pack_start(GTK_BOX(box), self->fittopage, TRUE, FALSE, 0);
  gtk_widget_show(self->fittopage);

  /* the drawing area */
  self->darea = gtk_drawing_area_new();
  gtk_table_attach(GTK_TABLE(self), self->darea, 1,2, 1,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->darea);


}

GtkWidget *
dia_page_layout_new(void)
{
  DiaPageLayout *self = gtk_type_new(dia_page_layout_get_type());

  return GTK_WIDGET(self);
}
