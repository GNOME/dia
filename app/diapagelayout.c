
#define PAGELAYOUT_TEST

#include "diapagelayout.h"
#include "diaunitspinner.h"

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

enum {
  CHANGED,
  FITTOPAGE,
  LAST_SIGNAL
};

static guint pl_signals[LAST_SIGNAL] = { 0 };
static GtkObjectClass *parent_class;

static void dia_page_layout_class_init(DiaPageLayoutClass *class);
static void dia_page_layout_init(DiaPageLayout *self);
static void dia_page_layout_destroy(GtkObject *object);

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
  parent_class = gtk_type_class(gtk_table_get_type());

  pl_signals[CHANGED] =
    gtk_signal_new("changed",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(DiaPageLayoutClass, changed),
		   gtk_signal_default_marshaller,
		   GTK_TYPE_NONE, 0);
  pl_signals[FITTOPAGE] =
    gtk_signal_new("fittopage",
		   GTK_RUN_FIRST,
		   object_class->type,
		   GTK_SIGNAL_OFFSET(DiaPageLayoutClass, fittopage),
		   gtk_signal_default_marshaller,
		   GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals(object_class, pl_signals, LAST_SIGNAL);

  object_class->destroy = dia_page_layout_destroy;
}

static void fittopage_pressed(DiaPageLayout *self);
static void darea_size_allocate(DiaPageLayout *self, GtkAllocation *alloc);
static gint darea_expose_event(DiaPageLayout *self, GdkEventExpose *ev);
static void paper_size_change(GtkMenuItem *item, DiaPageLayout *self);
static void orient_changed(DiaPageLayout *self);
static void margin_changed(DiaPageLayout *self);
static void scale_changed(DiaPageLayout *self);

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

  box = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box), 5);
  gtk_container_add(GTK_CONTAINER(frame), box);
  gtk_widget_show(box);

  self->paper_size = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(box), self->paper_size, TRUE, FALSE, 0);

  menu = gtk_menu_new();
  for (i = 0; paper_metrics[i].paper != NULL; i++) {
    menuitem = gtk_menu_item_new_with_label(paper_metrics[i].paper);
    gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(i));
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		       GTK_SIGNAL_FUNC(paper_size_change), self);
    gtk_container_add(GTK_CONTAINER(menu), menuitem);
    gtk_widget_show(menuitem);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(self->paper_size), menu);
  gtk_widget_show(self->paper_size);

  self->paper_label = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(box), self->paper_label, TRUE, TRUE, 0);
  gtk_widget_show(self->paper_label);

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

  self->tmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->tmargin, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->tmargin);

  wid = gtk_label_new(_("Bottom:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->bmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->bmargin, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->bmargin);

  wid = gtk_label_new(_("Left:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 2,3,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->lmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->lmargin, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->lmargin);

  wid = gtk_label_new(_("Right:"));
  gtk_misc_set_alignment(GTK_MISC(wid), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), wid, 0,1, 3,4,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(wid);

  self->rmargin = dia_unit_spinner_new(
	GTK_ADJUSTMENT(gtk_adjustment_new(1, 0,100, 0.1,10,10)),
	2, DIA_UNIT_CENTIMETER);
  gtk_table_attach(GTK_TABLE(table), self->rmargin, 1,2, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(self->rmargin);

  /* Scaling */
  frame = gtk_frame_new(_("Scaling"));
  gtk_table_attach(GTK_TABLE(self), frame, 0,1, 2,3,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(frame);

  box = gtk_vbox_new(FALSE, 5);
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

  /* connect the signal handlers */
  gtk_signal_connect_object(GTK_OBJECT(self->orient_portrait), "toggled",
			    GTK_SIGNAL_FUNC(orient_changed), GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->tmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->bmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->lmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->rmargin), "changed",
			    GTK_SIGNAL_FUNC(margin_changed), GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->scaling), "changed",
			    GTK_SIGNAL_FUNC(scale_changed), GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->fittopage), "pressed",
			    GTK_SIGNAL_FUNC(fittopage_pressed),
			    GTK_OBJECT(self));

  gtk_signal_connect_object(GTK_OBJECT(self->darea), "size_allocate",
			    GTK_SIGNAL_FUNC(darea_size_allocate),
			    GTK_OBJECT(self));
  gtk_signal_connect_object(GTK_OBJECT(self->darea), "expose_event",
			    GTK_SIGNAL_FUNC(darea_expose_event),
			    GTK_OBJECT(self));

  gdk_color_white(gtk_widget_get_colormap(GTK_WIDGET(self)), &self->white);
  gdk_color_black(gtk_widget_get_colormap(GTK_WIDGET(self)), &self->black);
  self->blue.red = 0;
  self->blue.green = 0;
  self->blue.blue = 0x7fff;
  gdk_color_alloc(gtk_widget_get_colormap(GTK_WIDGET(self)), &self->blue);

  self->gc = NULL;
  self->block_changed = FALSE;
}

GtkWidget *
dia_page_layout_new(void)
{
  DiaPageLayout *self = gtk_type_new(dia_page_layout_get_type());

  dia_page_layout_set_paper(self, "A4");
  return GTK_WIDGET(self);
}

const gchar *
dia_page_layout_get_paper(DiaPageLayout *self)
{
  return paper_metrics[self->papernum].paper;
}

void
dia_page_layout_set_paper(DiaPageLayout *self, const gchar *paper)
{
  gint i;

  for (i = 0; paper_metrics[i].paper != NULL; i++) {
    if (!g_strcasecmp(paper_metrics[i].paper, paper))
      break;
  }
  if (paper_metrics[i].paper == NULL)
    i = 1; /* A4 */
  gtk_option_menu_set_history(GTK_OPTION_MENU(self->paper_size), i);
  gtk_menu_item_activate(
	GTK_MENU_ITEM(GTK_OPTION_MENU(self->paper_size)->menu_item));
}

void
dia_page_layout_get_margins(DiaPageLayout *self,
			    gfloat *tmargin, gfloat *bmargin,
			    gfloat *lmargin, gfloat *rmargin)
{
  if (tmargin)
    *tmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
  if (bmargin)
    *bmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
  if (lmargin)
    *lmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
  if (rmargin)
    *rmargin = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
}

void
dia_page_layout_set_margins(DiaPageLayout *self,
			    gfloat tmargin, gfloat bmargin,
			    gfloat lmargin, gfloat rmargin)
{
  self->block_changed = TRUE;
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->tmargin), tmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->bmargin), bmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->lmargin), lmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->rmargin), rmargin);
  self->block_changed = FALSE;

  gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

DiaPageOrientation
dia_page_layout_get_orientation(DiaPageLayout *self)
{
  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active)
    return DIA_PAGE_ORIENT_PORTRAIT;
  else
    return DIA_PAGE_ORIENT_LANDSCAPE;
}

void
dia_page_layout_set_orientation(DiaPageLayout *self,
				DiaPageOrientation orient)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->orient_portrait),
			       orient == DIA_PAGE_ORIENT_PORTRAIT);
}

void
dia_page_layout_get_paper_size(const gchar *paper,
			       gfloat *width, gfloat *height)
{
  gint i;

  for (i = 0; paper_metrics[i].paper != NULL; i++) {
    if (!g_strcasecmp(paper_metrics[i].paper, paper))
      break;
  }
  if (paper_metrics[i].paper == NULL)
    i = 1; /* A4 */
  if (width)
    *width = paper_metrics[i].pswidth;
  if (height)
    *height = paper_metrics[i].psheight;
}

void
dia_page_layout_get_default_margins(const gchar *paper,
				    gfloat *tmargin, gfloat *bmargin,
				    gfloat *lmargin, gfloat *rmargin)
{
  gint i;

  for (i = 0; paper_metrics[i].paper != NULL; i++) {
    if (!g_strcasecmp(paper_metrics[i].paper, paper))
      break;
  }
  if (paper_metrics[i].paper == NULL)
    i = 1; /* A4 */
  if (tmargin)
    *tmargin = paper_metrics[i].tmargin;
  if (bmargin)
    *bmargin = paper_metrics[i].bmargin;
  if (lmargin)
    *lmargin = paper_metrics[i].lmargin;
  if (rmargin)
    *rmargin = paper_metrics[i].rmargin;
}

static void
fittopage_pressed(DiaPageLayout *self)
{
  gtk_signal_emit(GTK_OBJECT(self), pl_signals[FITTOPAGE]);
}

static void size_page(DiaPageLayout *self, GtkAllocation *a)
{
  self->width = a->width - 3;
  self->height = a->height - 3;

  /* change to correct metrics */
  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    if (self->width * paper_metrics[self->papernum].psheight >
	self->height * paper_metrics[self->papernum].pswidth)
      self->width = self->height * paper_metrics[self->papernum].pswidth /
	paper_metrics[self->papernum].psheight;
    else
      self->height = self->width * paper_metrics[self->papernum].psheight /
	paper_metrics[self->papernum].pswidth;
  } else {
    if (self->width * paper_metrics[self->papernum].pswidth >
	self->height * paper_metrics[self->papernum].psheight)
      self->width = self->height * paper_metrics[self->papernum].psheight /
	paper_metrics[self->papernum].pswidth;
    else
      self->height = self->width * paper_metrics[self->papernum].pswidth /
	paper_metrics[self->papernum].psheight;
  }

  self->x = (a->width - self->width - 3) / 2;
  self->y = (a->height - self->height - 3) / 2;
}

static void
darea_size_allocate(DiaPageLayout *self, GtkAllocation *allocation)
{
  size_page(self, allocation);
}

static gint
darea_expose_event(DiaPageLayout *self, GdkEventExpose *event)
{
  GdkWindow *window= self->darea->window;
  gfloat val;
  gint num;

  if (!window)
    return FALSE;

  if (!self->gc)
    self->gc = gdk_gc_new(window);

  gdk_window_clear_area (window,
                         0, 0,
                         self->darea->allocation.width,
                         self->darea->allocation.height);

  /* draw the page image */
  gdk_gc_set_foreground(self->gc, &self->black);
  gdk_draw_rectangle(window, self->gc, TRUE, self->x+3, self->y+3,
		     self->width, self->height);
  gdk_gc_set_foreground(self->gc, &self->white);
  gdk_draw_rectangle(window, self->gc, TRUE, self->x, self->y,
		     self->width, self->height);
  gdk_gc_set_foreground(self->gc, &self->black);
  gdk_draw_rectangle(window, self->gc, FALSE, self->x, self->y,
		     self->width-1, self->height-1);

  gdk_gc_set_foreground(self->gc, &self->blue);

  /* draw margins */
  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
    num = self->y + val * self->height /paper_metrics[self->papernum].psheight;
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
    num = self->y + self->height -
      val * self->height / paper_metrics[self->papernum].psheight;
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
    num = self->x + val * self->width / paper_metrics[self->papernum].pswidth;
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
    num = self->x + self->width -
      val * self->width / paper_metrics[self->papernum].pswidth;
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);
  } else {
    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->tmargin));
    num = self->y + val * self->height /paper_metrics[self->papernum].pswidth;
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->bmargin));
    num = self->y + self->height -
      val * self->height / paper_metrics[self->papernum].pswidth;
    gdk_draw_line(window, self->gc, self->x+1, num, self->x+self->width-2,num);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->lmargin));
    num = self->x + val * self->width / paper_metrics[self->papernum].psheight;
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);

    val = dia_unit_spinner_get_value(DIA_UNIT_SPINNER(self->rmargin));
    num = self->x + self->width -
      val * self->width / paper_metrics[self->papernum].psheight;
    gdk_draw_line(window, self->gc, num, self->y+1,num,self->y+self->height-2);
  }

  return FALSE;
}

static void
paper_size_change(GtkMenuItem *item, DiaPageLayout *self)
{
  gchar buf[512];

  self->papernum = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(item)));
  size_page(self, &self->darea->allocation);
  gtk_widget_queue_draw(self->darea);

  self->block_changed = TRUE;
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->tmargin),
			     paper_metrics[self->papernum].tmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->bmargin),
			     paper_metrics[self->papernum].bmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->lmargin),
			     paper_metrics[self->papernum].lmargin);
  dia_unit_spinner_set_value(DIA_UNIT_SPINNER(self->rmargin),
			     paper_metrics[self->papernum].rmargin);

  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      paper_metrics[self->papernum].psheight;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      paper_metrics[self->papernum].psheight;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      paper_metrics[self->papernum].pswidth;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      paper_metrics[self->papernum].pswidth;
  } else {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      paper_metrics[self->papernum].pswidth;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      paper_metrics[self->papernum].pswidth;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      paper_metrics[self->papernum].psheight;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      paper_metrics[self->papernum].psheight;
  }
  self->block_changed = FALSE;

  g_snprintf(buf, sizeof(buf), _("%0.3gcm x %0.3gcm"),
	     paper_metrics[self->papernum].pswidth,
	     paper_metrics[self->papernum].psheight);
  gtk_label_set(GTK_LABEL(self->paper_label), buf);

  gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
orient_changed(DiaPageLayout *self)
{
  size_page(self, &self->darea->allocation);
  gtk_widget_queue_draw(self->darea);

  if (GTK_TOGGLE_BUTTON(self->orient_portrait)->active) {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      paper_metrics[self->papernum].psheight;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      paper_metrics[self->papernum].psheight;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      paper_metrics[self->papernum].pswidth;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      paper_metrics[self->papernum].pswidth;
  } else {
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->tmargin))->upper =
      paper_metrics[self->papernum].pswidth;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->bmargin))->upper =
      paper_metrics[self->papernum].pswidth;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->lmargin))->upper =
      paper_metrics[self->papernum].psheight;
    gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(self->rmargin))->upper =
      paper_metrics[self->papernum].psheight;
  }

  if (!self->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
margin_changed(DiaPageLayout *self)
{
  gtk_widget_queue_draw(self->darea);
  if (!self->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
scale_changed(DiaPageLayout *self)
{
  if (!self->block_changed)
    gtk_signal_emit(GTK_OBJECT(self), pl_signals[CHANGED]);
}

static void
dia_page_layout_destroy(GtkObject *object)
{
  DiaPageLayout *self = DIA_PAGE_LAYOUT(object);

  if (self->gc)
    gdk_gc_unref(self->gc);

  if (parent_class->destroy)
    (* parent_class->destroy)(object);
}

#ifdef PAGELAYOUT_TEST

void
changed_signal(DiaPageLayout *self)
{
  g_message("changed");
}
void
fittopage_signal(DiaPageLayout *self)
{
  g_message("fit to page");
}

void
main(int argc, char **argv)
{
  GtkWidget *win, *pl;

  gtk_init(&argc, &argv);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), _("Page Setup"));
  gtk_signal_connect(GTK_OBJECT(win), "destroy",
		     GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  pl = dia_page_layout_new();
  gtk_container_set_border_width(GTK_CONTAINER(pl), 5);
  gtk_container_add(GTK_CONTAINER(win), pl);
  gtk_widget_show(pl);

  gtk_signal_connect(GTK_OBJECT(pl), "changed",
		     GTK_SIGNAL_FUNC(changed_signal), NULL);
  gtk_signal_connect(GTK_OBJECT(pl), "fittopage",
		     GTK_SIGNAL_FUNC(fittopage_signal), NULL);

  gtk_widget_show(win);
  gtk_main();
}

#endif
