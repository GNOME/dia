/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * Font code completely reworked for the Pango conversion
 * Copyright (C) 2002 Cyrille Chepelov 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strlen */
#include <time.h>

#include <pango/pango.h>
#ifdef HAVE_FREETYPE
#include <pango/pangoft2.h>
#endif
#include <gdk/gdk.h>
#include <gtk/gtk.h> /* just for gtk_get_default_language() */
#ifdef GDK_WINDOWING_WIN32
/* avoid namespace clashes caused by inclusion of windows.h */
#define Rectangle Win32Rectangle
#include <pango/pangowin32.h>
#undef Rectangle
#endif
#include "font.h"
#include "message.h"
#include "intl.h"

/** Define this if you want to use the layout cache to speed up output.
 *  Appears to be buggy, see bug #307320. */
#undef LAYOUT_CACHE

static PangoContext* pango_context = NULL;

struct _DiaFont 
{
    GObject parent_instance;

    PangoFontDescription* pfd;    
    /* mutable */ char* legacy_name;    
};


/* This is the global factor that says what zoom factor is 100%.  It's
 * normally 20.0 (and likely to stay that way).  It is defined by how many
 * pixels one cm is represented as.
 */
static real global_zoom_factor = 20.0;

#if 0
/* For debugging: Sort families */
static int
cmp_families (const void *a, const void *b)
{
  const char *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const char *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return g_utf8_collate (a_name, b_name);
}

/* For debugging: List all font families */
static void
list_families()
{
  PangoFontFamily **families;
  int nfamilies;
  int i;

  pango_context_list_families(dia_font_get_context(), &families, &nfamilies);
  qsort (families, nfamilies, sizeof (PangoFontFamily *), cmp_families);
  for (i = 0; i < nfamilies; i++) {
    puts(pango_font_family_get_name(families[i]));
  }
  g_free(families);
}
#endif

static void
dia_font_check_for_font(int font) {
    DiaFont *check;
    PangoFont *loaded;

    check = dia_font_new_from_style(font, 1.0);
    loaded = pango_context_load_font(dia_font_get_context(),
				     check->pfd);
    if (loaded == NULL) {
      message_error(_("Can't load font %s.\n"), dia_font_get_family(check));
    }
}

void
dia_font_init(PangoContext* pcontext)
{
  pango_context = pcontext;
  /* We must have these three fonts! */
  dia_font_check_for_font(DIA_FONT_SANS);
  dia_font_check_for_font(DIA_FONT_SERIF);
  dia_font_check_for_font(DIA_FONT_MONOSPACE);
}

/* We might not need these anymore, when using FT2/Win32 fonts only */
static GList *pango_contexts;

void
dia_font_push_context(PangoContext *pcontext) {
  pango_contexts = g_list_prepend(pango_contexts, pango_context);
  pango_context = pcontext;
  pango_context_set_language (pango_context, gtk_get_default_language ());
  g_object_ref(pcontext);
}

void
dia_font_pop_context() {
  g_object_unref(pango_context);
  pango_context = (PangoContext*)pango_contexts->data;
  pango_context_set_language (pango_context, gtk_get_default_language ());
  pango_contexts = g_list_next(pango_contexts);
}

PangoContext *
dia_font_get_context() {
  if (pango_context == NULL) {
#ifdef HAVE_FREETYPE
    /* This is suggested by new Pango (1.2.4+), but doesn't get us the
     * right resolution:(
     dia_font_push_context(pango_ft2_font_map_create_context(pango_ft2_font_map_new()));
    */
    dia_font_push_context(pango_ft2_get_context(75,75));
#else
    if (gdk_display_get_default ())
      dia_font_push_context(gdk_pango_context_get());
    else {
#  ifdef GDK_WINDOWING_WIN32
      dia_font_push_context(pango_win32_get_context ());
#  else
      g_warning ("dia_font_get_context() : not font context w/o display. Crashing soon.");
#  endif
    }
#endif
  }

  return pango_context;
}

    /* dia centimetres to pango device units */
static gint
dcm_to_pdu(real dcm) { return dcm * global_zoom_factor * PANGO_SCALE; }
    /* pango device units to dia centimetres */
static real
pdu_to_dcm(gint pdu) { return (real)pdu / (global_zoom_factor * PANGO_SCALE); }

static void dia_font_class_init(DiaFontClass* class);
static void dia_font_finalize(GObject* object);
static void dia_font_init_instance(DiaFont*);

GType
dia_font_get_type (void)
{
    static GType object_type = 0;

    if (!object_type) {
        static const GTypeInfo object_info =
            {
                sizeof (DiaFontClass),
                (GBaseInitFunc) NULL,
                (GBaseFinalizeFunc) NULL,
                (GClassInitFunc) dia_font_class_init, /* class_init */
                NULL,           /* class_finalize */
                NULL,           /* class_data */
                sizeof (DiaFont),
                0,              /* n_preallocs */
                (GInstanceInitFunc)dia_font_init_instance
            };
        object_type = g_type_register_static (G_TYPE_OBJECT,
                                              "DiaFont",
                                              &object_info, 0);
    }  
    return object_type;
}
static gpointer parent_class;

static void
dia_font_class_init(DiaFontClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  object_class->finalize = dia_font_finalize;
}

static void 
dia_font_init_instance(DiaFont* font)
{
        /*GObject *gobject = G_OBJECT(font);  */
}

DiaFont*
dia_font_new(const char *family, DiaFontStyle style, real height)
{
  DiaFont* retval = dia_font_new_from_style(style, height);

  pango_font_description_set_family(retval->pfd,family);

  pango_context_load_font(dia_font_get_context(), retval->pfd);

  return retval;
}

static void
dia_pfd_set_family(PangoFontDescription* pfd, DiaFontFamily fam) {
  switch (fam) {
  case DIA_FONT_SANS :
    pango_font_description_set_family(pfd, "sans");
    break;
  case DIA_FONT_SERIF :
    pango_font_description_set_family(pfd, "serif");
    break;
  case DIA_FONT_MONOSPACE :
    pango_font_description_set_family(pfd, "monospace");
    break;
  default :
          /* Pango does allow fonts without a name */
      ;
  }
}

static void
dia_pfd_set_weight(PangoFontDescription* pfd, DiaFontWeight fw) {
  switch (fw) {
  case DIA_FONT_ULTRALIGHT :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_ULTRALIGHT);
    break;
  case DIA_FONT_LIGHT :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_LIGHT);
    break;
  case DIA_FONT_WEIGHT_NORMAL :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_NORMAL);
    break;
  case DIA_FONT_MEDIUM : /* Pango doesn't have this, but 
                            'intermediate values are possible' */
    pango_font_description_set_weight(pfd, 500);
    break;
  case DIA_FONT_DEMIBOLD : /* Pango doesn't have this, ... */
    pango_font_description_set_weight(pfd, 600);
    break;
  case DIA_FONT_BOLD :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_BOLD);
    break;
  case DIA_FONT_ULTRABOLD :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_ULTRABOLD);
    break;
  case DIA_FONT_HEAVY :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_HEAVY);
    break;
  default :
    g_assert_not_reached();
  }
}

static void
dia_pfd_set_slant(PangoFontDescription* pfd, DiaFontSlant fo) {
  switch (fo) {
  case DIA_FONT_NORMAL :
    pango_font_description_set_style(pfd,PANGO_STYLE_NORMAL);
    break;
  case DIA_FONT_OBLIQUE :
    pango_font_description_set_style(pfd,PANGO_STYLE_OBLIQUE);
    break;
  case DIA_FONT_ITALIC :
    pango_font_description_set_style(pfd,PANGO_STYLE_ITALIC);
    break;
  default :
    g_assert_not_reached();
  }
}

static void dia_pfd_set_size(PangoFontDescription* pfd, real height)
{ /* inline candidate... */
  pango_font_description_set_size(pfd, dcm_to_pdu(height) );
}


DiaFont*
dia_font_new_from_style(DiaFontStyle style, real height)
{
  DiaFont* retval;
  /* in the future we could establish Dia's own default font
   * matching to be as (font-)system independent as possible.
   * For now fall back to Pangos configuration --hb
   */
  PangoFontDescription* pfd = pango_font_description_new();
  dia_pfd_set_family(pfd,DIA_FONT_STYLE_GET_FAMILY(style));
  dia_pfd_set_weight(pfd,DIA_FONT_STYLE_GET_WEIGHT(style));
  dia_pfd_set_slant(pfd,DIA_FONT_STYLE_GET_SLANT(style));
  dia_pfd_set_size(pfd,height);
  
  retval = DIA_FONT(g_object_new(DIA_TYPE_FONT, NULL));
  
  retval->pfd = pfd;
  retval->legacy_name = NULL;
  return retval;
}

DiaFont* dia_font_copy(const DiaFont* font)
{
    if (!font) return NULL;
    return dia_font_new(dia_font_get_family(font),
                            dia_font_get_style(font),
                            dia_font_get_height(font));
}

void
dia_font_finalize(GObject* object)
{
    DiaFont* font;
    font = DIA_FONT(object);
    if (font->pfd) pango_font_description_free(font->pfd);
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

DiaFont* dia_font_ref(DiaFont* font)
{
    g_object_ref(G_OBJECT(font));
    return font;
}

void dia_font_unref(DiaFont* font)
{ 
    g_object_unref(G_OBJECT(font));
}

DiaFontStyle 
dia_font_get_style(const DiaFont* font)
{
  guint style;

  static int weight_map[] = {
    DIA_FONT_ULTRALIGHT, DIA_FONT_LIGHT,
    DIA_FONT_WEIGHT_NORMAL, /* intentionaly ==0 */
    DIA_FONT_MEDIUM, DIA_FONT_DEMIBOLD, /* not yet in Pango */
    DIA_FONT_BOLD, DIA_FONT_ULTRABOLD, DIA_FONT_HEAVY
  };

  PangoStyle pango_style = pango_font_description_get_style(font->pfd);
  PangoWeight pango_weight = pango_font_description_get_weight(font->pfd);

  g_assert(PANGO_WEIGHT_ULTRALIGHT <= pango_weight && pango_weight <= PANGO_WEIGHT_HEAVY); 
  g_assert(PANGO_WEIGHT_ULTRALIGHT == 200);
  g_assert(PANGO_WEIGHT_NORMAL == 400);
  g_assert(PANGO_WEIGHT_BOLD == 700);

  style  = weight_map[(pango_weight - PANGO_WEIGHT_ULTRALIGHT) / 100];
  style |= (pango_style << 2);

  return style;
}

G_CONST_RETURN char*
dia_font_get_family(const DiaFont* font)
{
  return pango_font_description_get_family(font->pfd);
}

G_CONST_RETURN PangoFontDescription *
dia_font_get_description(const DiaFont* font)
{
  return font->pfd;
}

real
dia_font_get_height(const DiaFont* font)
{
  return pdu_to_dcm(pango_font_description_get_size(font->pfd));
}

void
dia_font_set_height(DiaFont* font, real height)
{
  pango_font_description_set_size(font->pfd, dcm_to_pdu(height));    
}


G_CONST_RETURN char*
dia_font_get_psfontname(const DiaFont *font)
{
    /* FIXME: this will very likely not work ! */
  return dia_font_get_legacy_name(font);
}

void dia_font_set_any_family(DiaFont* font, const char* family)
{
  g_assert(font != NULL);
  pango_font_description_set_family(font->pfd, family);
  if (font->legacy_name) {
      g_free(font->legacy_name);
      font->legacy_name = NULL;
  }
}

void dia_font_set_family(DiaFont* font, DiaFontFamily family)
{
  g_assert(font != NULL);
  dia_pfd_set_family(font->pfd,family);  
  if (font->legacy_name) {
      g_free(font->legacy_name);
      font->legacy_name = NULL;
  }
}

void dia_font_set_weight(DiaFont* font, DiaFontWeight weight)
{
  g_assert(font != NULL);    
  dia_pfd_set_weight(font->pfd,weight);
}

void dia_font_set_slant(DiaFont* font, DiaFontSlant slant)
{
  g_assert(font != NULL);    
  dia_pfd_set_slant(font->pfd,slant);
}


struct weight_name { DiaFontWeight fw; const char *name; };    
static const struct weight_name weight_names[] = {
    {DIA_FONT_ULTRALIGHT, "200"},
    {DIA_FONT_LIGHT,"300"},
    {DIA_FONT_WEIGHT_NORMAL,"normal"},
    {DIA_FONT_WEIGHT_NORMAL,"400"},
    {DIA_FONT_MEDIUM, "500"},
    {DIA_FONT_DEMIBOLD, "600"},
    {DIA_FONT_BOLD, "700"},
    {DIA_FONT_ULTRABOLD, "800"},
    {DIA_FONT_HEAVY, "900"},
    {0,NULL}};

G_CONST_RETURN char *dia_font_get_weight_string(const DiaFont* font)
{
    const struct weight_name* p;
    DiaFontWeight fw = DIA_FONT_STYLE_GET_WEIGHT(dia_font_get_style(font));
    
    for (p = weight_names; p->name != NULL; ++p) {
        if (p->fw == fw) return p->name;
    }
    return "normal";
}

void dia_font_set_weight_from_string(DiaFont* font, const char* weight) {
    DiaFontWeight fw = DIA_FONT_WEIGHT_NORMAL;
    const struct weight_name* p;

    for (p = weight_names; p->name != NULL; ++p) {
        if (0 == strncmp(weight,p->name,8)) {
            fw = p->fw;
            break;
        }
    }

    dia_font_set_weight(font,fw);
}


struct slant_name { DiaFontSlant fo; const char *name; };    
static const struct slant_name slant_names[] = {
    { DIA_FONT_NORMAL, "normal"},
    { DIA_FONT_OBLIQUE, "oblique"},
    { DIA_FONT_ITALIC, "italic"},
    { 0, NULL} };

G_CONST_RETURN char *
dia_font_get_slant_string(const DiaFont* font)
{
    const struct slant_name* p;
    DiaFontSlant fo =
    DIA_FONT_STYLE_GET_SLANT(dia_font_get_style(font));
    
    for (p = slant_names; p->name != NULL; ++p) {
        if (p->fo == fo) return p->name;
    }
    return "normal";
}

void dia_font_set_slant_from_string(DiaFont* font, const char* obli) {
    DiaFontSlant fo = DIA_FONT_NORMAL;
    const struct slant_name* p;

    DiaFontStyle old_style;
    DiaFontSlant old_fo;
    old_style = dia_font_get_style(font);
    old_fo = DIA_FONT_STYLE_GET_SLANT(old_style);
    
    for (p = slant_names; p->name != NULL; ++p) {
        if (0 == strncmp(obli,p->name,8)) {
            fo = p->fo;
            break;
        }
    }
    dia_font_set_slant(font,fo);
}


/* ************************************************************************ */
/* Non-scaled versions of the utility routines                              */
/* ************************************************************************ */

real
dia_font_string_width(const char* string, DiaFont *font, real height)
{
    return dia_font_scaled_string_width(string,font,height,global_zoom_factor);
}

real
dia_font_ascent(const char* string, DiaFont* font, real height)
{
    return dia_font_scaled_ascent(string,font,height,global_zoom_factor);
}

real dia_font_descent(const char* string, DiaFont* font, real height)
{
    return dia_font_scaled_descent(string,font,height,global_zoom_factor);
}

typedef struct {
  gchar *string;
  DiaFont *font;
  PangoLayout *layout;
  int usecount;
} LayoutCacheItem;

static GHashTable *layoutcache;

static gboolean
layout_cache_equals(gconstpointer e1, gconstpointer e2) 
{
  LayoutCacheItem *i1 = (LayoutCacheItem*)e1,
    *i2 = (LayoutCacheItem*)e2;

  /* don't try strcmp() with null pointers ... */
  if (!i1->string || i2->string)
    return FALSE;
  
  return strcmp(i1->string, i2->string) == 0 &&
    pango_font_description_equal(i1->font->pfd, i2->font->pfd);
}

static guint
layout_cache_hash(gconstpointer el) 
{
  LayoutCacheItem *item = (LayoutCacheItem*)el;

  return g_str_hash(item->string) ^
    pango_font_description_hash(item->font->pfd);
}

static long layout_cache_last_use;

static gboolean
layout_cache_cleanup_entry(gpointer key, gpointer value, gpointer data)
{
  LayoutCacheItem *item = (LayoutCacheItem*)value;
  /** Remove unused items */
  if (item->usecount == 0) return TRUE;
  item->usecount = 0;
  return FALSE;
}

/** The actual hash cleanup, called when idle. */
static gboolean
layout_cache_cleanup_idle(gpointer data)
{
  GHashTable *table = (GHashTable*)(data);

  g_hash_table_foreach_remove(table, layout_cache_cleanup_entry, NULL);
  return FALSE;
}

/** Every ten minutes, clean up those strings that haven't seen use since
 * last cleanup.
 */
static gboolean
layout_cache_cleanup(gpointer data)
{
  /* Only cleanup if there has been font activity since last cleanup */
  if (time(0) - layout_cache_last_use < 10) {
    /* Don't go directly to cleanup, wait till there's a pause. */
    g_idle_add(layout_cache_cleanup_idle, data);
  }
  /* Keep doing this */
  return TRUE;
}

static void
layout_cache_free_key(gpointer data)
{
    LayoutCacheItem *item = (LayoutCacheItem*)data;

    if (item->string != NULL) {
      g_free(item->string);
      item->string = NULL;
    }

    if (item->font != NULL) {
      dia_font_unref(item->font);
      item->font = NULL;
    }

    if (item->layout != NULL) {
      g_object_unref(item->layout);
      item->layout = NULL;
    }
    g_free(item);
}


PangoLayout*
dia_font_build_layout(const char* string, DiaFont* font, real height)
{
    PangoLayout* layout;
    PangoAttrList* list;
    PangoAttribute* attr;
    guint length;
    gchar *desc = NULL;

#ifdef LAYOUT_CACHE
    LayoutCacheItem *cached, *item;

    layout_cache_last_use = time(0);
    if (layoutcache == NULL) {
      /** Note that key and value are the same -- it's a HashSet */
      layoutcache = g_hash_table_new_full(layout_cache_hash,
					  layout_cache_equals,
					  layout_cache_free_key,
					  NULL);
      /** Check for cache cleanup every 10 seconds. */
      /** This frequent a check is really a hack while we figure out the
       *  exact problems with reffing the fonts.
       *  Note to self:  The equals function should compare pfd's, but
       *  then DiaFonts are freed too early. 
       */
      g_timeout_add(10*1000, layout_cache_cleanup, (gpointer)layoutcache);
    }
#endif

    height *= 0.7;
    dia_font_set_height(font, height);

#ifdef LAYOUT_CACHE
    item = g_new0(LayoutCacheItem,1);
    item->string = g_strdup(string);
    item->font = font;
    
    /* If it's in the cache, use that instead. */
    cached = g_hash_table_lookup(layoutcache, item);
    if (cached != NULL) {
      g_object_ref(cached->layout);
      g_free(item->string);
      g_free(item);
      cached->usecount ++;
      return cached->layout;
    }

    item->font = dia_font_copy(font);
    dia_font_ref(item->font);
#endif

    /* This could should account for DPI, but it doesn't do right.  Grrr...
    {
      GdkScreen *screen = gdk_screen_get_default();
      real dpi = gdk_screen_get_width(screen)/
		 (gdk_screen_get_width_mm(screen)/25.4);
      printf("height = %f, dpi = %f, new height = %f\n",
	     height, dpi, height * (dpi/100));
      height *= dpi/100;
    }
    */

    layout = pango_layout_new(dia_font_get_context());

    length = string ? strlen(string) : 0;
    pango_layout_set_text(layout, string, length);
        
    list = pango_attr_list_new();
    desc = g_utf8_strdown(pango_font_description_get_family(font->pfd), -1);
    pango_font_description_set_family(font->pfd, desc);
    g_free(desc);
    attr = pango_attr_font_desc_new(font->pfd);
    attr->start_index = 0;
    attr->end_index = length;
    pango_attr_list_insert(list,attr); /* eats attr */
    
    pango_layout_set_attributes(layout,list);
    pango_attr_list_unref(list);

    pango_layout_set_indent(layout,0);
    pango_layout_set_justify(layout,FALSE);
    pango_layout_set_alignment(layout,PANGO_ALIGN_LEFT);
  
#ifdef LAYOUT_CACHE
    item->layout = layout;
    g_object_ref(layout);
    item->usecount = 1;
    g_hash_table_replace(layoutcache, item, item);
#endif

    return layout;
}

/* ************************************************************************ */
/* scaled versions of the utility routines                                  */
/* ************************************************************************ */

void
dia_font_set_nominal_zoom_factor(real size_one)
{ global_zoom_factor = size_one; }



real
dia_font_scaled_string_width(const char* string, DiaFont *font, real height,
                            real zoom_factor)
{
    int lw,lh;
    real result;
    PangoLayout* layout;

    if (string == NULL || string[0] == '\0') {
      return 0.0;
    }

    layout = dia_font_scaled_build_layout(string, font, height, zoom_factor);
    pango_layout_get_size(layout,&lw,&lh);
    g_object_unref(G_OBJECT(layout));
    
    result = pdu_to_dcm(lw);
    /* Scale the result back for the zoom factor */
    result /= (zoom_factor/global_zoom_factor);
    return result;
}

static gboolean
dia_font_vertical_extents(const char* string, DiaFont* font,
                          real height, real zoom_factor,
                          guint line_no,
                          real* top, real* baseline, real* bottom)
{
    PangoRectangle ink_rect,logical_rect;
    PangoLayout* layout;
    PangoLayoutIter* iter;
    guint i;

    if (string == NULL || string[0] == '\0') {
      return FALSE;
    }

    layout = dia_font_scaled_build_layout(string, font,
                                          height, zoom_factor);
    iter = pango_layout_get_iter(layout);
    for (i = 0; i < line_no; ++i) {
       if (!pango_layout_iter_next_line(iter)) {
           pango_layout_iter_free(iter);
           g_object_unref(G_OBJECT(layout));
           return FALSE;
       }
    }
    
    pango_layout_iter_get_line_extents(iter,&ink_rect,&logical_rect);

    *top = pdu_to_dcm(logical_rect.y);
    *bottom = pdu_to_dcm(logical_rect.y + logical_rect.height);
    *baseline = pdu_to_dcm(pango_layout_iter_get_baseline(iter));

    pango_layout_iter_free(iter);
    g_object_unref(G_OBJECT(layout));

    return TRUE;
}
    

real
dia_font_scaled_ascent(const char* string, DiaFont* font, real height,
                      real zoom_factor)
{
  real top,bline,bottom;
  if (!string || string[0] == '\0') {
    /* This hack won't work for fonts that don't cover ASCII */
    dia_font_vertical_extents("XjgM149",font,height,zoom_factor,
			      0,&top,&bline,&bottom);
  } else {
    dia_font_vertical_extents(string,font,height,zoom_factor,
			      0,&top,&bline,&bottom);
  }
  return (bline-top)/(zoom_factor/global_zoom_factor);
}

real dia_font_scaled_descent(const char* string, DiaFont* font,
                            real height, real zoom_factor)
{
  real top,bline,bottom;

  if (!string || string[0] == '\0') {
    /* This hack won't work for fonts that don't cover ASCII */
    dia_font_vertical_extents("XjgM149",font,height,zoom_factor,
			      0,&top,&bline,&bottom);
  } else {
    dia_font_vertical_extents(string,font,height,zoom_factor,
                              0,&top,&bline,&bottom);
  }
  return (bottom-bline)/(zoom_factor/global_zoom_factor);
}

PangoLayout*
dia_font_scaled_build_layout(const char* string, DiaFont* font,
                            real height, real zoom_factor)
{
    DiaFont* altered_font;
    real scaling;
    real nozoom_width;
    real target_zoomed_width;
    PangoLayout* layout;
    real altered_scaling;
    real real_width;

    scaling = zoom_factor / global_zoom_factor;
    if (fabs(1.0 - scaling) < 1E-7) {
        return dia_font_build_layout(string,font,height);
    }

    nozoom_width = dia_font_string_width(string,font,height);
    target_zoomed_width = nozoom_width * scaling;

        /* First try: no tweaks. */
    real_width = dia_font_string_width(string,font, height * scaling);
    if (real_width <= target_zoomed_width) {
        return dia_font_build_layout(string,font,height*scaling);
    }

    altered_font = dia_font_copy(font);

        /* Last try. Using the "reduce overall size" strategy. */
    for (altered_scaling = scaling;
         altered_scaling > (scaling / 2);
         altered_scaling *= (target_zoomed_width/real_width>0.98?0.98:
			     target_zoomed_width/real_width) ) {
      real_width = dia_font_string_width(string,font,
					 height * altered_scaling);

        if (real_width <= target_zoomed_width) {
            layout = dia_font_build_layout(string,altered_font,
                                          height*altered_scaling);
            dia_font_unref(altered_font);
            return layout;
        }
    }

    /* Everything has failed. Returning non-tweaked variant. */
    g_warning("Failed to appropriately tweak zoomed font for zoom factor %f.", zoom_factor);
    dia_font_unref(altered_font);
    return dia_font_build_layout(string,font,height*scaling);    
}

/**
 * Compatibility with older files out of pre Pango Time. 
 * Make old files look as similar as possible
 * List should be kept alphabetically sorted by oldname, in case of
 * duplicates the one with the preferred newname comes first.
 *
 * FIXME: DIA_FONT_FAMILY_ANY in the list below does mean noone knows better
 */
static struct _legacy_font {
  gchar*       oldname;
  gchar*       newname;
  DiaFontStyle style;   /* the DIA_FONT_FAMILY() is used as falback only */
} legacy_fonts[] = {
  { "AvantGarde-Book",        "AvantGarde", DIA_FONT_SERIF },
  { "AvantGarde-BookOblique", "AvantGarde", DIA_FONT_SERIF | DIA_FONT_OBLIQUE },
  { "AvantGarde-Demi",        "AvantGarde", DIA_FONT_SERIF | DIA_FONT_DEMIBOLD },
  { "AvantGarde-DemiOblique", "AvantGarde", DIA_FONT_SERIF | DIA_FONT_OBLIQUE | DIA_FONT_DEMIBOLD },
  { "Batang", "Batang", DIA_FONT_FAMILY_ANY },
  { "Bookman-Demi",        "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_DEMIBOLD },
  { "Bookman-DemiItalic",  "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_DEMIBOLD | DIA_FONT_ITALIC },
  { "Bookman-Light",       "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_LIGHT },
  { "Bookman-LightItalic", "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_LIGHT | DIA_FONT_ITALIC },
  { "BousungEG-Light-GB", "BousungEG-Light-GB", DIA_FONT_FAMILY_ANY },
  { "Courier",             "monospace", DIA_FONT_MONOSPACE },
  { "Courier",             "Courier New", DIA_FONT_MONOSPACE },
  { "Courier-Bold",        "monospace", DIA_FONT_MONOSPACE | DIA_FONT_BOLD },
  { "Courier-Bold",        "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_BOLD },
  { "Courier-BoldOblique", "monospace", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Courier-BoldOblique", "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Courier-Oblique",     "monospace", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC},
  { "Courier-Oblique",     "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC },
  { "Dotum", "Dotum", DIA_FONT_FAMILY_ANY },
  { "GBZenKai-Medium", "GBZenKai-Medium", DIA_FONT_FAMILY_ANY }, 
  { "GothicBBB-Medium", "GothicBBB-Medium", DIA_FONT_FAMILY_ANY },
  { "Gulim", "Gulim", DIA_FONT_FAMILY_ANY }, 
  { "Headline", "Headline", DIA_FONT_FAMILY_ANY },
  { "Helvetica",             "sans", DIA_FONT_SANS },
  { "Helvetica",             "Arial", DIA_FONT_SANS },
  { "Helvetica-Bold",        "sans", DIA_FONT_SANS | DIA_FONT_BOLD },
  { "Helvetica-Bold",        "Arial", DIA_FONT_SANS | DIA_FONT_BOLD },
  { "Helvetica-BoldOblique", "sans", DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "Helvetica-BoldOblique", "Arial", DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "Helvetica-Narrow",             "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM },
  { "Helvetica-Narrow-Bold",        "Arial Narrow", DIA_FONT_SANS | DIA_FONT_DEMIBOLD },
  { "Helvetica-Narrow-BoldOblique", "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM | DIA_FONT_OBLIQUE },
  { "Helvetica-Narrow-Oblique",     "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM | DIA_FONT_OBLIQUE },
  { "Helvetica-Oblique",     "sans", DIA_FONT_SANS | DIA_FONT_ITALIC },
  { "Helvetica-Oblique",     "Arial", DIA_FONT_SANS | DIA_FONT_ITALIC },
  { "MOESung-Medium", "MOESung-Medium", DIA_FONT_FAMILY_ANY },
  { "NewCenturySchoolbook-Bold",       "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_BOLD },
  { "NewCenturySchoolbook-BoldItalic", "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "NewCenturySchoolbook-Italic",     "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_ITALIC },
  { "NewCenturySchoolbook-Roman",      "Century Schoolbook SWA", DIA_FONT_SERIF },
  { "Palatino-Bold",       "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_BOLD }, 
  { "Palatino-BoldItalic", "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_BOLD | DIA_FONT_ITALIC }, 
  { "Palatino-Italic",     "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_ITALIC }, 
  { "Palatino-Roman",      "Palatino", DIA_FONT_FAMILY_ANY },
  { "Ryumin-Light",      "Ryumin", DIA_FONT_FAMILY_ANY | DIA_FONT_LIGHT },
  { "ShanHeiSun-Light",  "ShanHeiSun", DIA_FONT_FAMILY_ANY | DIA_FONT_LIGHT },
  { "Song-Medium",       "Song-Medium", DIA_FONT_FAMILY_ANY | DIA_FONT_MEDIUM },
  { "Symbol",           "Symbol", DIA_FONT_SANS | DIA_FONT_MEDIUM },
  { "Times-Bold",       "serif", DIA_FONT_SERIF | DIA_FONT_BOLD }, 
  { "Times-Bold",       "Times New Roman", DIA_FONT_SERIF | DIA_FONT_BOLD },
  { "Times-BoldItalic", "serif", DIA_FONT_SERIF | DIA_FONT_ITALIC | DIA_FONT_BOLD }, 
  { "Times-BoldItalic", "Times New Roman", DIA_FONT_SERIF | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Times-Italic",     "serif", DIA_FONT_SERIF | DIA_FONT_ITALIC }, 
  { "Times-Italic",     "Times New Roman", DIA_FONT_SERIF | DIA_FONT_ITALIC },
  { "Times-Roman",      "serif", DIA_FONT_SERIF }, 
  { "Times-Roman",      "Times New Roman", DIA_FONT_SERIF }, 
  { "ZapfChancery-MediumItalic", "Zapf Calligraphic 801 SWA", DIA_FONT_SERIF | DIA_FONT_MEDIUM },
  { "ZapfDingbats", "Zapf Calligraphic 801 SWA", DIA_FONT_SERIF },
  { "ZenKai-Medium", "ZenKai", DIA_FONT_FAMILY_ANY | DIA_FONT_MEDIUM },
};


/**
 * Given a legacy name as stored until Dia-0.90 construct
 * a new DiaFont which is as similar as possible
 */
DiaFont*
dia_font_new_from_legacy_name(const char* name)
{
  /* do NOT translate anything here !!! */
  DiaFont* retval;
  struct _legacy_font* found = NULL;
  real height = 1.0;
  int i;

  for (i = 0; i < G_N_ELEMENTS(legacy_fonts); i++) {
    if (!strcmp(name, legacy_fonts[i].oldname)) {
      found = &legacy_fonts[i];
      break;
    }
  }
  if (found) {
    retval = dia_font_new (found->newname, found->style, height);
    retval->legacy_name = found->oldname;
  } else {      
    /* We tried our best, let Pango complain */
    retval = dia_font_new (name, DIA_FONT_WEIGHT_NORMAL, height);
    retval->legacy_name = NULL;
  }
  
  return retval;
}

G_CONST_RETURN char*
dia_font_get_legacy_name(const DiaFont *font)
{
  const char* matched_name = NULL;
  const char* family;
  DiaFontStyle style;
  int i;

  /* if we have loaded it from an old file, use the old name */    
  if (font->legacy_name)
    return font->legacy_name;

  family = dia_font_get_family (font);
  style = dia_font_get_style (font);
  for (i = 0; i < G_N_ELEMENTS(legacy_fonts); i++) {
    if (0 == g_strcasecmp (legacy_fonts[i].newname, family)) {
      /* match weight and slant */
      DiaFontStyle st = legacy_fonts[i].style;
      if ((DIA_FONT_STYLE_GET_SLANT(style) | DIA_FONT_STYLE_GET_WEIGHT(style))
          == (DIA_FONT_STYLE_GET_SLANT(st) | DIA_FONT_STYLE_GET_WEIGHT(st))) {
        return legacy_fonts[i].oldname; /* exact match */
      } else if (0 == (DIA_FONT_STYLE_GET_SLANT(st) | DIA_FONT_STYLE_GET_WEIGHT(st))) {
        matched_name = legacy_fonts[i].oldname;
        /* 'unmodified' font, continue matching */
      }
    }
  }
  return matched_name ? matched_name : "Courier";
}

