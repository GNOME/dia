#include "diagram.h"
#include "diagramdata.h"
#include "libgnome/libgnome.h"
#include "paginate_gnomeprint.h"

static void
count_objs(Object *obj, Renderer *renderer, int active_layer, guint *nobjs)
{
  (*nobjs)++;
}


static guint
print_page(DiagramData *data, RendererGPrint *rend, Rectangle *bounds,
	   gdouble lmargin, gdouble bmargin, gdouble scale)
{
  guint nobjs = 0;
  static guint pagenum = 0;

  /* count the number of objects in this region */
  data_render(data, (Renderer *)rend, bounds,
	      (ObjectRenderer) count_objs, &nobjs);

  if (nobjs == 0)
    return nobjs;

  /* save print context */
  gnome_print_gsave(rend->ctx);

  /* transform coordinate system */
  gnome_print_scale(rend->ctx, 28.346457 * scale, -28.346457 * scale);
  gnome_print_translate(rend->ctx, lmargin/scale - bounds->left,
			-bmargin/scale - bounds->bottom);

  /* set up clip mask */
  gnome_print_newpath(rend->ctx);
  gnome_print_moveto(rend->ctx, bounds->left, bounds->top);
  gnome_print_lineto(rend->ctx, bounds->right, bounds->top);
  gnome_print_lineto(rend->ctx, bounds->right, bounds->bottom);
  gnome_print_lineto(rend->ctx, bounds->left, bounds->bottom);
  gnome_print_lineto(rend->ctx, bounds->left, bounds->top);
  gnome_print_clip(rend->ctx);

  /* render the region */
  data_render(data, (Renderer *)rend, bounds, NULL, NULL);

  /* restore print context */
  gnome_print_grestore(rend->ctx);

  /* print the page */
  gnome_print_showpage(rend->ctx);

  return nobjs;
}

void
paginate_gnomeprint(Diagram *dia, GnomePrintContext *ctx,
		    const gchar *paper_name, gdouble scale)
{
  RendererGPrint *rend;
  Rectangle *extents;
  const GnomePaper *paper;
  const GnomeUnit *unit;
  gdouble pswidth, psheight, lmargin, tmargin, rmargin, bmargin;
  gdouble width, height;
  gdouble x, y;
  guint nobjs = 0;

  rend = new_gnomeprint_renderer(dia, ctx);

  /* get the paper metrics */
  if (paper_name)
    paper = gnome_paper_with_name(paper_name);
  else
    paper = gnome_paper_with_name(gnome_paper_name_default());
  if (!paper)
    g_message("paper_name == %s", paper_name?paper_name:gnome_paper_name_default());
  unit = gnome_unit_with_name("Centimeter");
  pswidth = gnome_paper_convert(gnome_paper_pswidth(paper), unit);
  psheight = gnome_paper_convert(gnome_paper_psheight(paper), unit);
  lmargin = gnome_paper_convert(gnome_paper_lmargin(paper), unit);
  tmargin = gnome_paper_convert(gnome_paper_tmargin(paper), unit);
  rmargin = gnome_paper_convert(gnome_paper_rmargin(paper), unit);
  bmargin = gnome_paper_convert(gnome_paper_bmargin(paper), unit);

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

  free(rend);

  gnome_print_context_close(ctx);

  gtk_object_unref(GTK_OBJECT(ctx));
}
		    
