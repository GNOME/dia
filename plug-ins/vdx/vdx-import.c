/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 *
 * vdx-import.c: Visio XML import filter for dia
 * Copyright (C) 2006-2009 Ian Redfern
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>
#include <locale.h>

#include "geometry.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "create.h"
#include "group.h"
#include "font.h"
#include "vdx.h"
#include "visio-types.h"
#include "bezier_conn.h"
#include "connection.h"
#include "dia-io.h"
#include "dia-layer.h"

void static vdx_get_colors(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx);
void static vdx_get_facenames(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx);
void static vdx_get_fonts(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx);
void static vdx_get_masters(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx);
void static vdx_get_stylesheets(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx);
void static vdx_free(VDXDocument *theDoc);

/* Note: we can hold pointers to parts of the parsed XML during import, but
   can't pass them to the rest of Dia, as they will be freed when we finish */

/* The following should be in create.h
   Dia interface taken from xfig-import.c */

static PropDescription vdx_line_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    PROP_STD_START_ARROW,
    PROP_STD_END_ARROW,
    PROP_DESC_END};

/** Creates a line - missing from lib/create.c
 * @param points start and end
 * @param start_arrow start arrow
 * @param end_arrow end arrow
 * @returns A Standard - Line object
 */

static DiaObject *
create_standard_line(Point *points,
                     Arrow *start_arrow,
                     Arrow *end_arrow) {
    DiaObjectType *otype = object_get_type("Standard - Line");
    DiaObject *new_obj;
    Handle *h1, *h2;
    PointProperty *ptprop;
    GPtrArray *props;

    new_obj = otype->ops->create(&points[0], otype->default_user_data,
                                 &h1, &h2);

    props = prop_list_from_descs(vdx_line_prop_descs, pdtpp_true);
    if (props->len != 4)
    {
        g_debug("create_standard_line() - props->len != 4");
        return 0;
    }

    ptprop = g_ptr_array_index(props,0);
    ptprop->point_data = points[0];

    ptprop = g_ptr_array_index(props,1);
    ptprop->point_data = points[1];

    if (start_arrow != NULL)
       ((ArrowProperty *)g_ptr_array_index(props, 2))->arrow_data = *start_arrow;
    if (end_arrow != NULL)
       ((ArrowProperty *)g_ptr_array_index(props, 3))->arrow_data = *end_arrow;

    new_obj->ops->set_props(new_obj, props);

    prop_list_free(props);

    return new_obj;
}

/* This is a modified create_standard_beziergon. */

/** Creates a beziergon with cusp corners
 * @param num_points number of points
 * @param points array of points
 * @param ctx to accummulate warning/error messages
 * @return A Standard - Beziergon object
 */
static DiaObject *
create_vdx_beziergon(int num_points,
                     BezPoint *points,
                     DiaContext *ctx)
{
    DiaObjectType *otype = object_get_type("Standard - Beziergon");
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData *bcd;
    BezierConn *bcp;
    int i;


    if (otype == NULL){
	dia_context_add_message(ctx, _("Can't find standard object"));
	return NULL;
    }

    bcd = g_new(BezierCreateData, 1);
    bcd->num_points = num_points;
    bcd->points = points;

    new_obj = otype->ops->create(NULL, bcd,
				 &h1, &h2);

    g_clear_pointer (&bcd, g_free);

    /* Convert all points to cusps - not in API */

    bcp = (BezierConn *)new_obj;
    for (i=0; i<bcp->bezier.num_points; i++)
    {
        if (points[i].type == BEZ_CURVE_TO)
            bcp->bezier.corner_types[i] = BEZ_CORNER_CUSP;
    }

    return new_obj;
}

/* Not in standard includes */

typedef enum _Valign Valign;
enum _Valign {
        VALIGN_TOP,
        VALIGN_BOTTOM,
        VALIGN_CENTER,
        VALIGN_FIRST_LINE
};

/* Vertical alignment is a separate property */

static PropDescription vdx_text_descs[] = {
    { "text", PROP_TYPE_TEXT },
    { "text_vert_alignment", PROP_TYPE_ENUM },
    PROP_DESC_END
};

/* End of code taken from xfig-import.c */


/** Turns a VDX colour definition into a Dia Color.
 * @param s a string from the VDX
 * @param theDoc the current document (with its colour table)
 * @param ctx the context for error/warning messages
 * @return A Dia Color object
 */
Color
vdx_parse_color(const char *s, const VDXDocument *theDoc, DiaContext *ctx)
{
    int colorvalues;
    Color c = {0, 0, 0, 0};
    if (s[0] == '#')
    {
        sscanf(s, "#%xd", &colorvalues);
        /* A wild fabricated guess? */
        /* c.alpha = ((colorvalues & 0xff000000)>>24) / 255.0; */
        c.red = ((colorvalues & 0x00ff0000)>>16) / 255.0;
        c.green = ((colorvalues & 0x0000ff00)>>8) / 255.0;
        c.blue = (colorvalues & 0x000000ff) / 255.0;
        c.alpha = 1.0;
        return c;
    }
    if (g_ascii_isdigit(s[0]))
    {
        /* Look in colour table */
        unsigned int i = atoi(s);
        if (theDoc->Colors && i < theDoc->Colors->len)
            return g_array_index(theDoc->Colors, Color, i);
    }
    /* Colour 0 is always black, so don't warn (OmniGraffle) */
    if (*s != '0')
    {
        dia_context_add_message(ctx, _("Couldn't read color: %s"), s);
        g_debug("Couldn't read color: %s", s);
    }
    return c;
}

/** Reads the colour table from the start of a VDX document
 * @param cur the current XML node
 * @param theDoc the current document (with its colour table)
 * @param ctx the context for error/warning messages
 */
static void
vdx_get_colors(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx)
{
    xmlNodePtr ColorEntry;
    theDoc->Colors = g_array_new(FALSE, TRUE, sizeof (Color));

    for (ColorEntry = cur->xmlChildrenNode; ColorEntry;
         ColorEntry = ColorEntry->next) {
        Color color;
        struct vdx_ColorEntry temp_ColorEntry;

        if (xmlIsBlankNode(ColorEntry)) { continue; }

        vdx_read_object(ColorEntry, theDoc, &temp_ColorEntry, ctx);
        /* Just in case Color entries aren't consecutive starting at 0 */
        color = vdx_parse_color(temp_ColorEntry.RGB, theDoc, ctx);
        if (theDoc->Colors->len <= temp_ColorEntry.IX)
        {
            theDoc->Colors =
                g_array_set_size(theDoc->Colors, temp_ColorEntry.IX+1);
        }
        g_array_index(theDoc->Colors, Color, temp_ColorEntry.IX) = color;
        g_array_append_val(theDoc->Colors, color);
    }
}

/** Reads the face table from the start of a VDX document
 * @param cur the current XML node
 * @param theDoc the current document
 * @param ctx the context for error/warning messages
 */
static void
vdx_get_facenames(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx)
{
    xmlNodePtr Face;
    theDoc->FaceNames =
        g_array_new(FALSE, FALSE, sizeof (struct vdx_FaceName));

    for (Face = cur->xmlChildrenNode; Face; Face = Face->next) {
        struct vdx_FaceName FaceName;

        if (xmlIsBlankNode(Face)) { continue; }

        vdx_read_object(Face, theDoc, &FaceName, ctx);
        /* FaceNames need not be numbered consecutively, or start at 0,
           so make room for the new one in the array */
        if (theDoc->FaceNames->len <= FaceName.ID)
        {
            theDoc->FaceNames =
                g_array_set_size(theDoc->FaceNames, FaceName.ID+1);
        }
        g_array_index(theDoc->FaceNames, struct vdx_FaceName,
                      FaceName.ID) = FaceName;
    }
}

/** Reads the font table from the start of a VDX document
 * @param cur the current XML node
 * @param theDoc the current document
 * @param ctx the context for error/warning messages
 */
static void
vdx_get_fonts(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx)
{
    xmlNodePtr Font;
    theDoc->Fonts = g_array_new(FALSE, FALSE, sizeof (struct vdx_FontEntry));

    for (Font = cur->xmlChildrenNode; Font; Font = Font->next) {
        struct vdx_FontEntry FontEntry;

        if (xmlIsBlankNode(Font)) { continue; }

        vdx_read_object(Font, theDoc, &FontEntry, ctx);
        /* Defensive, in case Fonts are sometimes not consecutive from 0 */
        if (theDoc->Fonts->len <= FontEntry.ID)
        {
            theDoc->Fonts = g_array_set_size(theDoc->Fonts, FontEntry.ID+1);
        }
        g_array_index(theDoc->Fonts, struct vdx_FontEntry, FontEntry.ID) =
            FontEntry;
    }
}

/** Reads the masters table from the start of a VDX document
 * @param cur the current XML node
 * @param theDoc the current document
 * @param ctx the context for error/warning messages
 */
static void
vdx_get_masters(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx)
{
    xmlNodePtr Master;
    theDoc->Masters = g_array_new (FALSE, TRUE,
                                   sizeof (struct vdx_Master));
    for (Master = cur->xmlChildrenNode; Master;
         Master = Master->next)
    {
        struct vdx_Master newMaster;

        if (xmlIsBlankNode(Master)) { continue; }

        vdx_read_object(Master, theDoc, &newMaster, ctx);
        /* Masters need not be numbered consecutively,
           so make room for the new one in the array */
        if (theDoc->Masters->len <= newMaster.ID)
        {
            theDoc->Masters =
                g_array_set_size(theDoc->Masters, newMaster.ID+1);
        }
        g_array_index(theDoc->Masters, struct vdx_Master,
                      newMaster.ID) = newMaster;
    }
}

/** Reads the stylesheet table from the start of a VDX document
 * @param cur the current XML node
 * @param theDoc the current document
 * @param ctx the context for error/warning messages
 */
static void
vdx_get_stylesheets(xmlNodePtr cur, VDXDocument* theDoc, DiaContext *ctx)
{
    xmlNodePtr StyleSheet;
    theDoc->StyleSheets = g_array_new (FALSE, TRUE,
                                       sizeof (struct vdx_StyleSheet));
    for (StyleSheet = cur->xmlChildrenNode; StyleSheet;
         StyleSheet = StyleSheet->next)
    {
        struct vdx_StyleSheet newSheet;

        if (xmlIsBlankNode(StyleSheet)) { continue; }

        vdx_read_object(StyleSheet, theDoc, &newSheet, ctx);
        /* StyleSheets need not be numbered consecutively,
           so make room for the new one in the array */
        if (theDoc->StyleSheets->len <= newSheet.ID)
        {
            theDoc->StyleSheets =
                g_array_set_size(theDoc->StyleSheets, newSheet.ID+1);
        }
        g_array_index(theDoc->StyleSheets, struct vdx_StyleSheet,
                      newSheet.ID) = newSheet;
    }
}

/** Finds a child of an object with a specific type
 * @param type a type code
 * @param p the object
 * @returns An object of the desired type, or NULL
 */
static void *
find_child(unsigned int type, const void *p)
{
    const struct vdx_any *Any = p;
    GSList *child;

    if (!p)
    {
        g_debug("find_child called with p=0");
        return 0;
    }

    for(child = Any->children; child; child = child->next)
    {
        struct vdx_any *Any_child = (struct vdx_any *)child->data;
        if (!child->data) continue;
        if (Any_child->type == type) return Any_child;
    }
    return 0;
}

/** Finds next child of an object with a specific type after a given
 * @param type a type code
 * @param p the object
 * @param given the given child
 * @returns An object of the desired type, or NULL
 */
static void *
find_child_next(unsigned int type, const void *p, const void *given)
{
    struct vdx_any *Any = (struct vdx_any *)p;
    GSList *child;
    gboolean found_given = FALSE;

    if (!p)
    {
        g_debug("find_child_next() called with p=0");
        return 0;
    }
    for(child = Any->children; child; child = child->next)
    {
        struct vdx_any *Any_child = (struct vdx_any *)child->data;
        if (!child->data) continue;
        if (Any_child->type == type)
        {
            if (found_given) return Any_child;
            if (child->data == given) found_given = TRUE;
        }
    }
    return 0;
}

/** Frees all children of an object recursively
 * @param p an object pointer (may be NULL)
 */
static void
free_children(void *p)
{
    if (p)
    {
        struct vdx_any *Any = (struct vdx_any *)p;
        GSList *list;
        for (list = Any->children; list; list = list->next)
        {
            if (!list->data) continue;
            free_children(list->data);
            g_clear_pointer (&list->data, g_free);
        }
        g_slist_free(list);
    }
}

/** Finds the stylesheet style object that applies
 * @param type a type code
 * @param style the starting stylesheet number
 * @param theDoc the document
 * @returns An object of the desired type, or NULL
 */
static void *
get_style_child(unsigned int type, unsigned int style, VDXDocument* theDoc)
{
    struct vdx_StyleSheet theSheet;
    struct vdx_any *Any;
    while(1)
    {
        if (!theDoc->StyleSheets || style >= theDoc->StyleSheets->len)
        {
            /* Ignore style 0 for OmniGraffle */
            if (style)
            {
                g_debug("Unknown stylesheet reference: %d", style);
            }
            return 0;
        }
        theSheet = g_array_index(theDoc->StyleSheets,
                                 struct vdx_StyleSheet, style);
        Any = find_child(type, &theSheet);
        if (Any) return Any;
        /* Terminate on style 0 (default) */
        if (!style) return 0;
        /* Find a parent style to check */
        if (type == vdx_types_Fill) style = theSheet.FillStyle;
        else if (type == vdx_types_Line) style = theSheet.LineStyle;
        else style = theSheet.TextStyle;
        if (theDoc->debug_comments)
            g_debug("style %s=%d", vdx_Types[type], style);
    }

    return 0;
}

/** Finds a shape in any object including its grouped subshapes
 * @param id shape id (0 for first, as no shape has id 0)
 * @param Shapes shape list
 * @param depth only print warning at top level
 * @param ctx to accummulate warning/error messages
 * @return The Shape or NULL
 */
static struct vdx_Shape *
get_shape_by_id(unsigned int id, struct vdx_Shapes *Shapes, unsigned int depth, DiaContext *ctx)
{
    struct vdx_Shape *Shape;
    struct vdx_Shapes *SubShapes;
    GSList *child;

    if (!Shapes)
    {
        g_debug("get_shape_by_id() called with Shapes=0");
        return 0;
    }

    /* A Master has a list of Shapes */
    for(child = Shapes->any.children; child; child = child->next)
    {
        struct vdx_any *Any_child = (struct vdx_any *)child->data;
        if (!child->data) continue;
        if (Any_child->type == vdx_types_Shape)
        {
            Shape = (struct vdx_Shape *)Any_child;
            if (Shape->ID == id || id == 0) return Shape;

            /* Any of the Shapes may have a list of Shapes */
            SubShapes = (struct vdx_Shapes *)find_child(vdx_types_Shapes,
                                                        Shape);
            if (SubShapes)
            {
                Shape = get_shape_by_id(id, SubShapes, depth+1, ctx);
                if (Shape) return Shape;
            }
        }
    }
    if (!depth)
    {
        dia_context_add_message(ctx, _("Couldn't find shape %d"), id);
        g_debug("Couldn't find shape %d", id);
    }
    return 0;
}

/** Finds the master style object that applies
 * @param type a type code
 * @param master the master number
 * @param shape the mastershape number
 * @param theDoc the document
 * @param ctx to accummulate warning/error messages
 * @return The master's shape child
 */
static struct vdx_Shape *
get_master_shape(unsigned int master, unsigned int shape, VDXDocument* theDoc, DiaContext *ctx)
{
    struct vdx_Master theMaster;
    struct vdx_Shapes *Shapes;

    if (!theDoc->Masters || master >= theDoc->Masters->len)
    {
        g_debug("Unknown master reference");
        return 0;
    }

    if (theDoc->debug_comments)
        g_debug("Looking for Master %d Shape %d", master, shape);
    theMaster = g_array_index(theDoc->Masters,
                              struct vdx_Master, master);

    Shapes = find_child(vdx_types_Shapes, &theMaster);
    if (!Shapes) return NULL;

    return get_shape_by_id(shape, Shapes, 0, ctx);
}

/** Convert a Visio point to a Dia one
 * @param p a Visio-space point
 * @param theDoc the VDXDocument
 * @returns a Dia-space point
 */
static Point
dia_point(Point p, const VDXDocument* theDoc)
{
    Point q;
    q.x = vdx_Point_Scale*p.x + vdx_Page_Width*theDoc->Page;
    q.y = vdx_Y_Offset + vdx_Y_Flip*vdx_Point_Scale*p.y;
    return q;
}

/** Convert a Visio absolute length to a Dia one
 * @param length a length
 * @param theDoc the document
 * @returns length in Dia space
 */

static double
dia_length(double length, const VDXDocument* theDoc)
{
    return vdx_Point_Scale*length;
}


/*
 * Sets simple props
 * @param obj the object
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @param ctx the context for error/warning messages
 * @todo dash length not yet done - much other work needed
 */
static void
vdx_simple_properties (DiaObject             *obj,
                       const struct vdx_Fill *Fill,
                       const struct vdx_Line *Line,
                       const VDXDocument     *theDoc,
                       DiaContext            *ctx)
{
  GPtrArray *props = g_ptr_array_new ();

  if (Line) {
    Color color;

    prop_list_add_line_width (props,Line->LineWeight * vdx_Line_Scale);

    color = Line->LineColor;
    color.alpha = 1.0 - Line->LineColorTrans;

    if (!Line->LinePattern) {
      color = vdx_parse_color("#FFFFFF", theDoc, ctx);
    }

    prop_list_add_line_colour (props, &color);

    if (Line->LinePattern) {
      LinestyleProperty *lsprop =
          (LinestyleProperty *) make_new_prop ("line_style",
                                               PROP_TYPE_LINESTYLE,
                                               PROP_FLAG_DONT_SAVE);

      if (Line->LinePattern == 2) {
        lsprop->style = DIA_LINE_STYLE_DASHED;
      } else if (Line->LinePattern == 4) {
        lsprop->style = DIA_LINE_STYLE_DASH_DOT;
      } else if (Line->LinePattern == 3) {
        lsprop->style = DIA_LINE_STYLE_DOTTED;
      } else if (Line->LinePattern == 5) {
        lsprop->style = DIA_LINE_STYLE_DASH_DOT_DOT;
      } else {
        lsprop->style = DIA_LINE_STYLE_SOLID;
      }

      lsprop->dash = vdx_Dash_Length;

      g_ptr_array_add (props,lsprop);
    }

    if (Line->Rounding > 0.0) {
      prop_list_add_real (props,
                          "corner_radius",
                          Line->Rounding * vdx_Line_Scale);
    }
  }

    if (Fill && Fill->FillPattern)
    {
        Color color;

        /* Dia can't do fill patterns, so we have to choose either the
           foreground or background colour.
           I've chosen the background colour for all patterns except solid */

        if (Fill->FillPattern == 1)
	{
            color = Fill->FillForegnd;
	    color.alpha = 1.0 - Fill->FillForegndTrans;
	}
        else
	{
            color = Fill->FillBkgnd;
	    color.alpha = 1.0 - Fill->FillBkgndTrans;
	}

	if (!Line)
	{
	    /* Mostly a matter of rountrip - if set NoLine we still need line
	     * properties because Dia can't do NoLine, just hide it by tiniting
	     * the line with fill color and setting it's width to 0
	     */
	    prop_list_add_line_width (props, 0.0);
	    prop_list_add_line_colour (props, &color);
	}

        if (theDoc->debug_comments)
        {
            g_debug("Fill pattern %d fg %s bg %s", Fill->FillPattern,
                    vdx_string_color(Fill->FillForegnd),
                    vdx_string_color(Fill->FillBkgnd));
        }
	prop_list_add_fill_colour (props, &color);
    }
    else
    {
        BoolProperty *bprop =
            (BoolProperty *)make_new_prop("show_background",
                                          PROP_TYPE_BOOL,PROP_FLAG_DONT_SAVE);
        bprop->bool_data = FALSE;

        g_ptr_array_add(props,bprop);
    }

    obj->ops->set_props(obj, props);
    prop_list_free(props);
}

/** Applies a standard XForm object to a point
 * @param p the point
 * @param XForm the XForm
 * @returns the new point
 */
static Point
apply_XForm(Point p, const struct vdx_XForm *XForm)
{
    Point q = p;
    Point r;
    double sin_theta, cos_theta;

    /* Remove the offset of the rotation pin from the object */
    if (!XForm)
    {
        g_debug("apply_XForm() called with XForm=0");
        return q;
    }

    q.x -= XForm->LocPinX;
    q.y -= XForm->LocPinY;

    /* Apply the flips */
    if (XForm->FlipX)
    {
        q.x = - q.x;
    }
    if (XForm->FlipY)
    {
        q.y = - q.y;
    }

    /* Perform the rotation */
    if (fabs(XForm->Angle) > EPSILON)
    {
        /* Rotate */
        sin_theta = sin(XForm->Angle);
        cos_theta = cos(XForm->Angle);

        r.x = q.x*cos_theta - q.y*sin_theta;
        r.y = q.y*cos_theta + q.x*sin_theta;
        q = r;
    }

    /* Now add the offset of the rotation pin from the page */
    q.x += XForm->PinX;
    q.y += XForm->PinY;

    if (XForm->any.children)
    {
        /* Recurse if we have a list */
        /* This is overloading the children list, but XForms cannot
           have real children, so this list is always empty, and it's a lot
           easier than the composition calculations */
        XForm = (struct vdx_XForm*)XForm->any.children->data;
        if (XForm)
        {
            q = apply_XForm(q, XForm);
        }
    }

    return q;
}

/** Create an arrow
 * @param Line a Visio Line object
 * @param start_end 's' for the start arrow or 'e' for the end arrow
 * @param theDoc the document
 * @returns the arrow
 */
static Arrow *
make_arrow(const struct vdx_Line *Line, char start_end,
           const VDXDocument *theDoc)
{
    Arrow *a = g_new0(Arrow, 1);
    unsigned int fixed_size = 0;
    double size = 0;
    unsigned int type = 0;

    if (!Line)
    {
        g_debug("make_arrow() called with Line=0");
        return 0;
    }
    a->type = ARROW_FILLED_TRIANGLE;

    if (start_end == 's')
    {
        fixed_size = Line->BeginArrowSize;
        type = Line->BeginArrow;
    }
    else
    {
        fixed_size = Line->EndArrowSize;
        type = Line->EndArrow;
    }
    if (type <= 16) a->type = vdx_Arrows[type];

    if (fixed_size > 6) { fixed_size = 0; }
    size = vdx_Arrow_Sizes[fixed_size];

    a->length = dia_length(size*vdx_Arrow_Scale, theDoc);
    if (a->type == ARROW_FILLED_TRIANGLE)
        a->width = a->length * vdx_Arrow_Width_Height_Ratio;
    else
        a->width = a->length;

    if (theDoc->debug_comments)
        g_debug("arrow %c %d", start_end, fixed_size);
    return a;
}

/*!
 * \brief Translate points to rectangle representation if possible
 */
static gboolean
_is_rect (int count, Point *points, real *x, real *y, real *w, real *h)
{
    int i;
    real minx, miny, maxdx = 0.0, maxdy = 0.0, sumdx = 0.0, sumdy = 0.0;

    if (count != 5)
	return FALSE;
    minx = points[0].x;
    miny = points[0].y;
    for (i = 1; i< 5; ++i)
    {
	real dx = points[i-1].x - points[i].x;
	real dy = points[i-1].y - points[i].y;

	/* must be both 0 for a real rectangle */
	sumdx += dx;
	sumdy += dy;

	dx = fabs (dx);
	dy = fabs (dy);

	if (dx > EPSILON && dy > EPSILON)
	    return FALSE;
	minx = MIN(minx, points[i].x);
	miny = MIN(miny, points[i].y);
	maxdx = MAX(maxdx, dx);
	maxdy = MAX(maxdy, dy);
    }
    if (fabs(sumdx) > EPSILON || fabs(sumdy) > EPSILON)
	return FALSE;
    *x = minx;
    *y = miny;
    *w = maxdx;
    *h = maxdy;
    return TRUE;
}

/* The following functions create the Dia standard objects */

/** Plots a polyline (or line)
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @param ctx the context for error/warning messages
 * @return the new object
 */
static DiaObject *
plot_polyline(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm,
              const struct vdx_Fill *Fill, const struct vdx_Line *Line,
              VDXDocument* theDoc, const GSList **more, Point *current, DiaContext *ctx)
{
    DiaObject *newobj = NULL;
    const GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    struct vdx_PolylineTo *PolylineTo;
    struct vdx_ArcTo *ArcTo;
    struct vdx_any *Any;
    Point *points, p;
    unsigned int num_points = 1;
    unsigned int count = 0;
    Arrow* start_arrow_p = NULL;
    Arrow* end_arrow_p = NULL;
    gboolean done = FALSE;

    if (theDoc->debug_comments) g_debug("plot_polyline()");
    if (!Geom || ((Geom->NoFill || (Fill && !Fill->FillPattern)) &&
                  (Geom->NoLine || (Line && !Line->LinePattern))))
    {
        *more = 0;
        if (theDoc->debug_comments) g_debug("Nothing to plot");
        return 0;
    }

    for(item = *more; item; item = item->next)
    {
        num_points++;
    }

    points = g_new0(Point, num_points);

    for(item = *more; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        switch (Any->type)
        {
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            if (LineTo->Del) continue;
            p.x = LineTo->X; p.y = LineTo->Y;
            if (!count)
            {
                /* Use current as start point */
                points[count++] =
                    dia_point(apply_XForm(*current, XForm), theDoc);
            }
            break;
        case vdx_types_PolylineTo:
            /* FIXME: Temporary fix to avoid looping */
            PolylineTo = (struct vdx_PolylineTo*)(item->data);
            /* if (PolylineTo->Del) continue; */
            p.x = PolylineTo->X; p.y = PolylineTo->Y;
            if (!count)
            {
                /* Use current as start point */
                points[count++] =
                    dia_point(apply_XForm(*current, XForm), theDoc);
            }
            break;
        case vdx_types_MoveTo:
            MoveTo = (struct vdx_MoveTo*)(item->data);
            p.x = MoveTo->X; p.y = MoveTo->Y;
            if (count)
            {
                /* Need to turn it into multiple lines */
                if (p.x != current->x || p.y != current->y)
                {
                    *more = item;
                    done = TRUE;
                    break;
                }
            }
            break;
        default:
            if (Any->type == vdx_types_ArcTo)
            {
                ArcTo = (struct vdx_ArcTo*)Any;
                if (ArcTo->Del) continue;
            }
            if (theDoc->debug_comments)
                g_debug("Unexpected line component: %s",
                        vdx_Types[(unsigned int)Any->type]);
            *more = item;
            done = TRUE;
            break;
        }
        if (done)
        {
            break;
        }
        *current = p;
        points[count++] = dia_point(apply_XForm(p, XForm), theDoc);
    }
    if (!done) *more = 0;

    if (Line && Line->BeginArrow)
    {
        start_arrow_p = make_arrow(Line, 's', theDoc);
    }

    if (Line && Line->EndArrow)
    {
        end_arrow_p = make_arrow(Line, 'e', theDoc);
    }

    if (count > 2)
    {
        /* If we had to break the Geom partway, it's not a polygon */
        if (!Fill || !Fill->FillPattern || Geom->NoFill || done)
        {
            /* Yes, it is end_arrow followed by start_arrow */
            newobj = create_standard_polyline(count, points,
                                              end_arrow_p, start_arrow_p);
        }
        else
        {
	    real x, y, w, h;

	    if (_is_rect (count, points, &x, &y, &w, &h))
	    {
		newobj = create_standard_box(x, y, w, h);
	    }
	    else
	    {
		newobj = create_standard_polygon(count, points);
	    }
        }
    }
    else
    {
        if (count == 2)
        {
            /* Yes, it is start_arrow followed by end_arrow */
            newobj = create_standard_line(points, start_arrow_p, end_arrow_p);
        }
        else
        {
            /* Don't plot lines with only one point */
            if (theDoc->debug_comments)
                g_debug("Empty polyline");
        }
    }
    if (newobj)
        vdx_simple_properties(newobj, Fill, Line, theDoc, ctx);
    g_clear_pointer (&points, g_free);
    g_clear_pointer (&end_arrow_p, g_free);
    g_clear_pointer (&start_arrow_p, g_free);
    return newobj;
}

/** Plots an ellipse
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @param ctx the context for error/warning messages
 * @return the new object
 */
static DiaObject *
plot_ellipse(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm,
             const struct vdx_Fill *Fill, const struct vdx_Line *Line,
             VDXDocument* theDoc, const GSList **more, Point *current, DiaContext *ctx)
{
    DiaObject *newobj = NULL;
    const GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_Ellipse *Ellipse;
    struct vdx_any *Any;

    Point p;

    item = *more;
    current->x = 0; current->y = 0;
    Any = (struct vdx_any *)(item->data);

    if (Any->type == vdx_types_MoveTo)
    {
        MoveTo = (struct vdx_MoveTo*)(item->data);
        current->x = MoveTo->X;
        current->y = MoveTo->Y;
        item = item->next;
        Any = (struct vdx_any *)(item->data);
        *more = item->next;
    }

    if (Any->type == vdx_types_Ellipse)
    {
        Ellipse = (struct vdx_Ellipse*)(item->data);
    }
    else
    {
        dia_context_add_message(ctx, _("Unexpected Ellipse object: %s"),
                      vdx_Types[(unsigned int)Any->type]);
        g_debug("Unexpected Ellipse object: %s",
                vdx_Types[(unsigned int)Any->type]);
        return NULL;
    }
    *more = item->next;

    /* Dia pins its ellipses in the top left corner, but Visio uses the centre,
       so adjust by the vertical radius */
    current->y += Ellipse->D;

    p = dia_point(apply_XForm(*current, XForm), theDoc);
    if (fabs(XForm->Angle > EPSILON))
	dia_context_add_message(ctx, _("Can't rotate ellipse"));

    newobj =
        create_standard_ellipse(p.x, p.y, dia_length(Ellipse->A, theDoc),
                                dia_length(Ellipse->D, theDoc));

    vdx_simple_properties(newobj, Fill, Line, theDoc, ctx);

    return newobj;
}

/** Converts an ArcTo to EllipticalArcTo
 * @param ArcTo the ArcTo
 * @param Start the start point of the arc
 * @param EllipticalArcTo the equivalent EllipticalArcTo
 */
static gboolean
arc_to_ellipticalarc(struct vdx_ArcTo *ArcTo, const Point *Start,
                     struct vdx_EllipticalArcTo *EllipticalArcTo)
{
    Point chord;
    Point perp;
    double length;

    if (!EllipticalArcTo || !ArcTo || !Start)
    {
        g_debug("arc_to_ellipticalarc() called with null parameters");
        return FALSE;
    }

    EllipticalArcTo->any.type = vdx_types_EllipticalArcTo;
    EllipticalArcTo->any.children = 0;

    EllipticalArcTo->X = ArcTo->X;
    EllipticalArcTo->Y = ArcTo->Y;

    /* For a circular arc, the major and minor axes are the same
       and can be any axes, so we choose the X and Y axes.
       Hence C = 0 and D = 1.
    */

    EllipticalArcTo->C = 0;
    EllipticalArcTo->D = 1;

    /* We need a control point.
       A is the distance from the arc's midpoint to the midpoint of the
       equivalent line.
    */

    chord.x = ArcTo->X - Start->x;
    chord.y = ArcTo->Y - Start->y;

    /* Make one of the two perpendiculars */
    perp.y = -chord.x;
    perp.x = chord.y;

    length = sqrt(perp.x*perp.x + perp.y*perp.y);
    if (length < EPSILON)
    {
        g_debug("chord length too small");
        return FALSE;
    }
    perp.x /= length;
    perp.y /= length;

    EllipticalArcTo->A = Start->x + chord.x/2 + ArcTo->A*perp.x;
    EllipticalArcTo->B = Start->y + chord.y/2 + ArcTo->A*perp.y;

    return TRUE;
}

/** Converts an elliptical arc to Bezier points
 * @param p0 start point
 * @param p3 end point
 * @param p4 arc control point
 * @param C angle of the arc's major axis relative to the x-axis of its parent
 * @param D ratio of an arc's major axis to its minor axis
 * @param p1 first Bezier control point
 * @param p2 second Bezier control point
 */
static gboolean
ellipticalarc_to_bezier(Point p0, Point p3, Point p4, double C, double D,
                         Point *p1, Point *p2)
{
    /* We wish to find the unique Bezier that:
       Starts at p0 and ends at p3
       Has the same tangent as the arc at p0 and p3
       Meets the arc at its furthest point and has the same tangent there
    */

    Point P0, P1, P2, P3, P4, P5, Q, T0, T1, T2, T3;
    double R, R2, R3, sinC, cosC;

    double a, b, c, d, e, f, g; /* Scratch variables */

    if (!p1 || !p2)
    {
        g_debug("ellipticalarc_to_bezier() called with null parameters");
        return FALSE;
    }

    /* We assume the arc is not degenerate:
       p0 != p4 != p3 != p0, 0 < D < infty
    */

    if (fabs(p0.x - p3.x) + fabs(p0.y - p3.y) < EPSILON ||
        fabs(p0.x - p4.x) + fabs(p0.y - p4.y) < EPSILON ||
        fabs(p3.x - p4.x) + fabs(p3.y - p4.y) < EPSILON ||
        fabs(D) < EPSILON)
    {
        g_debug("Colinear");
        return FALSE;
    }

    /* First transform to a circle through P0, P3 and P4:
       Rotate by -C, then scale by 1/D along the major (X) axis
    */

    sinC = sin(C); cosC = cos(C);

    P0.x = (p0.x*cosC + p0.y*sinC)/D;
    P0.y = (-p0.x*sinC + p0.y*cosC);

    P3.x = (p3.x*cosC + p3.y*sinC)/D;
    P3.y = (-p3.x*sinC + p3.y*cosC);

    P4.x = (p4.x*cosC + p4.y*sinC)/D;
    P4.y = (-p4.x*sinC + p4.y*cosC);

    /* Centre of circumcircle is Q, radius R
       Q is the intersection of the perpendicular bisectors of the sides */

    /* Thanks to comp.graphics.algorithms FAQ 1.04 */

    a = P3.x - P0.x;
    b = P3.y - P0.y;
    c = P4.x - P0.x;
    d = P4.y - P0.y;
    e = a*(P0.x + P3.x) + b*(P0.y + P3.y);
    f = c*(P0.x + P4.x) + d*(P0.y + P4.y);
    g = 2.0*(a*(P4.y - P3.y) - b*(P4.x - P3.x));
    if (fabs(g) < EPSILON) {
      g_debug("g=%f too small", g);
      return FALSE;
    }
    Q.x = (d*e - b*f)/g;
    Q.y = (a*f - c*e)/g;
    R = sqrt((P0.x - Q.x)*(P0.x - Q.x) + (P0.y - Q.y)*(P0.y - Q.y));
    R2 = sqrt((P3.x - Q.x)*(P3.x - Q.x) + (P3.y - Q.y)*(P3.y - Q.y));
    R3 = sqrt((P4.x - Q.x)*(P4.x - Q.x) + (P4.y - Q.y)*(P4.y - Q.y));

    if (fabs(R-R2) > EPSILON || fabs(R-R3) > EPSILON)
    {
        g_debug("R=%f,R2=%f,R3=%f not equal", R, R2, R3);
        return FALSE;
    }

    /* Construct unit tangents at P0 and P3 - P1 and P2 lie along these */

    T0.y = Q.x - P0.x;
    T0.x = -(Q.y - P0.y);
    a = sqrt(T0.x*T0.x + T0.y*T0.y);
    T0.x /= a;
    T0.y /= a;

    T3.y = Q.x - P3.x;
    T3.x = -(Q.y - P3.y);
    a = sqrt(T3.x*T3.x + T3.y*T3.y);
    T3.x /= a;
    T3.y /= a;

    /* Now, we want T0 and T3 to both either point towards or away from
       their intersection point (assuming they're not parallel)
       So for some a and b, both <0, P0 + aT0 = P3 + bT3
    */

    d = T3.x*T0.y - T3.y*T0.x;
    if (fabs(d) < EPSILON)
    {
        /* Hard case.
           Well, not really, as long as they point in the same direction... */
        T3 = T0;
    }
    else
    {
        a =  (P3.y * T3.x - P0.y * T3.x + T3.y * P0.x - T3.y * P3.x) / d;
        b = -(P0.y * T0.x - P3.y * T0.x + T0.y * P3.x - T0.y * P0.x) / d;

        if (a < 0 && b > 0)
        {
            /* Opposite direction for one of them */
            T0.x = - T0.x;
            T0.y = - T0.y;
        }
        if (a > 0 && b < 0)
        {
            T3.x = - T3.x;
            T3.y = - T3.y;
        }
    }

    /* So for some a and b,
       P1 = P0 + aT0
       P2 = P3 + bT3
    */

    /* But this is a circle. So if we reflect it, we get another solution.
       So a = b (as we have the vectors in the right direction).
       Further, the extreme point of the arc is its midpoint,
       and this must occur with t = 0.5
       - doesn't symmetry make life easy?
    */

    /* Bezier formula is
       P(t) = P0 + [3P1 - 3P0]t + [3P2 - 6P1 + 3P0]t^2 +
              [P3 - 3P2 + 3P1 - P0]t^3
       P(t) = P0 + [3aT0]t + [3P3 - 3P0 + 3bT3 - 6aT0]t^2 +
              [2P0 - 2P3 - 3bT3 + 3aT0]t^3
       P(0.5) = (P0 + P3)/2 + 3a(T0 + T3)/8
    */

    /* Now, to find the mid point of the arc, first find T1,
       the midpoint of P0 and P3.
    */
    T1.x = (P0.x + P3.x)/2.0;
    T1.y = (P0.y + P3.y)/2.0;

    /* Now drop a radius from Q */
    T2.x = T1.x - Q.x;
    T2.y = T1.y - Q.y;
    a = sqrt(T2.x*T2.x + T2.y*T2.y);
    if (fabs(a) < EPSILON)
    {
        /* So T1 = Q. In that case, pick a normal to P3 - P0 */
        T2 = T0;
        a = sqrt(T2.x*T2.x + T2.y*T2.y);
    }
    T2.x /= a;
    T2.y /= a;

    /* Now we want the radius from Q along T2 to be in the direction of P4
       - at least, the angle between them < 90 */

    a = T2.x * (P4.x - Q.x) + T2.y * (P4.y - Q.y);
    if (fabs(a) < EPSILON)
    {
        /* OK, this isn't possible - implies P4 = P0 or P3 */
        g_debug("P4 = P0 or P3?");
        return FALSE;
    }
    if (a < 0) { T2.x = - T2.x; T2.y = - T2.y; }

    /* And now, we have the mid point of the arc */
    P5.x = Q.x + R * T2.x;
    P5.y = Q.y + R * T2.y;

    /* Now we need to solve for a */

    if (fabs(T0.x+T3.x) < EPSILON)
    {
        /* Do it with y */
        a = (P5.y - (P0.y + P3.y)/2.0)*8.0/3.0/(T0.y + T3.y);
    }
    else
    {
        /* Do it with x */
        a = (P5.x - (P0.x + P3.x)/2.0)*8.0/3.0/(T0.x + T3.x);
    }

    /* Construct P1 and P2 */
    P1.x = P0.x + a * T0.x;
    P1.y = P0.y + a * T0.y;
    P2.x = P3.x + a * T3.x;
    P2.y = P3.y + a * T3.y;

    /* Transform back to an elliptical arc */

    p1->x = (P1.x*D*cosC - P1.y*sinC);
    p1->y = (P1.x*D*sinC + P1.y*cosC);
    p2->x = (P2.x*D*cosC - P2.y*sinC);
    p2->y = (P2.x*D*sinC + P2.y*cosC);

    return TRUE;
}

/** Plots a bezier
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @param ctx the context for error/warning messages
 * @return the new object
 */
static DiaObject *
plot_bezier(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm,
            const struct vdx_Fill *Fill, const struct vdx_Line *Line,
            VDXDocument* theDoc, const GSList **more, Point *current, DiaContext *ctx)
{
    DiaObject *newobj = NULL;
    const GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    struct vdx_EllipticalArcTo *EllipticalArcTo;
    struct vdx_EllipticalArcTo EllipticalArcTo_from_ArcTo = { {0, }, };
    struct vdx_ArcTo *ArcTo;
    struct vdx_any *Any;
    unsigned int num_points = 0;
    unsigned int count = 0;
    BezPoint *bezpoints = 0;
    Arrow* start_arrow_p = NULL;
    Arrow* end_arrow_p = NULL;
    gboolean ok;
    gboolean all_lines = TRUE;
    Point *points = NULL;
    unsigned int i;
    gboolean done = FALSE;

    Point p;
    Point p0 = *current;
    Point p1 = p0, p2 = p0, p3 = p0, p4 = p0;

    if (theDoc->debug_comments)
        g_debug("plot_bezier()");

    for(item = *more; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        if (Any->type != vdx_types_MoveTo && ! num_points)
        {
            dia_context_add_message(ctx, _("MoveTo not at start of Bezier"));
            g_debug("MoveTo not at start of Bezier");
            *more = 0; /* FIXME */
            return 0;
        }
        num_points++;
    }

    bezpoints = g_new0(BezPoint, num_points);

    for(item = *more; item; item = item->next)
    {
        if (!item->data)
        {
            *more = item->next;
            continue;
        }
        Any = (struct vdx_any *)(item->data);
        switch (Any->type)
        {
        case vdx_types_MoveTo:
            MoveTo = (struct vdx_MoveTo*)(item->data);
            bezpoints[count].type = BEZ_MOVE_TO;
            p.x = MoveTo->X; p.y = MoveTo->Y;
            bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
            bezpoints[count].p2 = bezpoints[count].p1;
            bezpoints[count].p3 = bezpoints[count].p1;
            p0 = p;
            break;
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            bezpoints[count].type = BEZ_LINE_TO;
            p.x = LineTo->X; p.y = LineTo->Y;
            bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
            /* Essential to set the others or strange things happen */
            bezpoints[count].p2 = bezpoints[count].p1;
            bezpoints[count].p3 = bezpoints[count].p1;
            p0 = p;
            break;
        case vdx_types_EllipticalArcTo:
            EllipticalArcTo = (struct vdx_EllipticalArcTo*)(item->data);
            p3.x = EllipticalArcTo->X; p3.y = EllipticalArcTo->Y;
            p4.x = EllipticalArcTo->A; p4.y = EllipticalArcTo->B;
            if (ellipticalarc_to_bezier(p0, p3, p4, EllipticalArcTo->C,
                                        EllipticalArcTo->D, &p1, &p2))
            {
                bezpoints[count].type = BEZ_CURVE_TO;
                bezpoints[count].p3 = dia_point(apply_XForm(p3, XForm), theDoc);
                bezpoints[count].p2 = dia_point(apply_XForm(p2, XForm), theDoc);
                bezpoints[count].p1 = dia_point(apply_XForm(p1, XForm), theDoc);
                p0 = p3;
                all_lines = FALSE;
            }
            else
            {
                bezpoints[count].type = BEZ_LINE_TO;
                p.x = EllipticalArcTo->X; p.y = EllipticalArcTo->Y;
                bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
                bezpoints[count].p2 = bezpoints[count].p1;
                bezpoints[count].p3 = bezpoints[count].p1;
                p0 = p;
            }
            break;
        case vdx_types_ArcTo:
            ArcTo = (struct vdx_ArcTo*)(item->data);
            /* Transform to EllipticalArcTo */
            EllipticalArcTo = &EllipticalArcTo_from_ArcTo;
            ok = arc_to_ellipticalarc(ArcTo, &p0, EllipticalArcTo);

            p3.x = EllipticalArcTo->X; p3.y = EllipticalArcTo->Y;
            p4.x = EllipticalArcTo->A; p4.y = EllipticalArcTo->B;
            if (ok && ellipticalarc_to_bezier(p0, p3, p4, EllipticalArcTo->C,
                                              EllipticalArcTo->D, &p1, &p2))
            {
                bezpoints[count].type = BEZ_CURVE_TO;
                bezpoints[count].p3 = dia_point(apply_XForm(p3, XForm), theDoc);
                bezpoints[count].p2 = dia_point(apply_XForm(p2, XForm), theDoc);
                bezpoints[count].p1 = dia_point(apply_XForm(p1, XForm), theDoc);
                p0 = p3;
                all_lines = FALSE;
            }
            else
            {
                bezpoints[count].type = BEZ_LINE_TO;
                p.x = ArcTo->X; p.y = ArcTo->Y;
                bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
                bezpoints[count].p2 = bezpoints[count].p1;
                bezpoints[count].p3 = bezpoints[count].p1;
                p0 = p;
            }
            break;
        default:
            done = TRUE;
            break;
        }
        if (done) break;
        *more = item->next;
        *current = p0;
        count++;
    }

    if (Line && Line->BeginArrow)
    {
        start_arrow_p = make_arrow(Line, 's', theDoc);
    }

    if (Line && Line->EndArrow)
    {
        end_arrow_p = make_arrow(Line, 'e', theDoc);
    }

    if (!all_lines)
    {
        /* It's a Bezier */
        if (Geom && Geom->NoFill)
        {
            /* Yes, it is end then start arrow */
            newobj = create_standard_bezierline(num_points, bezpoints,
                                                end_arrow_p, start_arrow_p);
        }
        else
        {
            newobj = create_vdx_beziergon(num_points, bezpoints, ctx);
        }
    }
    else
    {
        /* It's really a line/polyline, and it's far better to
           draw it as such - code from plot_polyline() */
        if (theDoc->debug_comments)
            g_debug("All lines");

        points = g_new0(Point, count);
        for (i=0; i<count; i++)
        {
            points[i].x = bezpoints[i].p1.x;
            points[i].y = bezpoints[i].p1.y;
        }

        if (count > 2)
        {
            if (Geom && Geom->NoFill)
            {
                /* Yes, it is end_arrow followed by start_arrow */
                newobj = create_standard_polyline(count, points,
                                                  end_arrow_p, start_arrow_p);
            }
            else
            {
                newobj = create_standard_polygon(count, points);
            }
        }
        else
        {
            if (count == 2)
            {
                /* Yes, it is start_arrow followed by end_arrow */
                newobj = create_standard_line(points,
                                              start_arrow_p, end_arrow_p);
            }
            else
            {
                if (theDoc->debug_comments)
                    g_debug("Empty polyline");
            }
        }
        g_clear_pointer (&points, g_free);
    }
    g_clear_pointer (&bezpoints, g_free);
    if (newobj)
        vdx_simple_properties(newobj, Fill, Line, theDoc, ctx);
    g_clear_pointer (&end_arrow_p, g_free);
    g_clear_pointer (&start_arrow_p, g_free);
    return newobj;
}

/** NURBS basis function N_{i,k}(u)
 * @param i knot number
 * @param k degree
 * @param u point on line
 * @param n number of control points
 * @param knot array of knots
 * @returns the value of N
 */

static float
NURBS_N(unsigned int i, unsigned int k, float u, unsigned int n,
        const float *knot)
{
    float sum = 0.0;

    if (! knot)
    {
        g_debug("NURBS_N() called with knot=0");
        return sum;
    }

    if (k == 0)
    {
        if (knot[i] <= u && u < knot[i+1])
        {
            return 1.0;
        }
        else
        {
            return 0.0;
        }
    }

    if (fabs(knot[i+k]-knot[i]) >= EPSILON)
    {
        sum = (u-knot[i])/(knot[i+k]-knot[i]) * NURBS_N(i, k-1, u, n, knot);
    }

    if (i <= n && fabs(knot[i+k+1]-knot[i+1]) >= EPSILON)
    {
        sum += (knot[i+k+1]-u)/(knot[i+k+1]-knot[i+1]) *
            NURBS_N(i+1, k-1, u, n, knot);
    }

    return sum;
}

/** NURBS function C(u)
 * @param k degree
 * @param u point on line
 * @param n number of control points
 * @param knot array of knots
 * @param control array of control points
 * @returns the value of C
 */

static Point
NURBS_C(unsigned int k, float u, unsigned int n,
        const float *knot, const float *weight, const Point *control)
{
    float top_x = 0;
    float top_y = 0;
    float bottom = 0;
    unsigned int i;
    float N_i_k;
    Point p = {0, 0};

    if (!knot || !weight || !control)
    {
        g_debug("NURBS_C() called with null parameters");
        return p;
    }

    for(i=0; i<=n; i++)
    {
        N_i_k = NURBS_N(i, k, u, n, knot);
        top_x += weight[i]*control[i].x*N_i_k;
        top_y += weight[i]*control[i].y*N_i_k;
        bottom += weight[i]*N_i_k;
    }

    if (fabs(bottom) < EPSILON)
    {
        bottom = EPSILON;
    }

    p.x = top_x/bottom;
    p.y = top_y/bottom;

    return p;
}


/** Plots a NURBS
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @param ctx the context for error/warning messages
 * @return the new object
 */
static DiaObject *
plot_nurbs(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm,
           const struct vdx_Fill *Fill, const struct vdx_Line *Line,
           VDXDocument* theDoc, const GSList **more, Point *current, DiaContext *ctx)
{
    DiaObject *newobj = NULL;
    const GSList *item, *item2;
    struct vdx_MoveTo *MoveTo;
    struct vdx_NURBSTo *NURBSTo;
    struct vdx_any *Any;
    struct vdx_SplineStart *SplineStart;
    struct vdx_SplineKnot *SplineKnot;

    Arrow *start_arrow_p = NULL;
    Arrow *end_arrow_p = NULL;
    Point *control = NULL;
    Point *points = NULL;
    Point p;
    Point end = *current;
    Point begin = *current;
    gboolean know_end = FALSE;
    float *weight = NULL;
    float *knot = NULL;
    unsigned int k = 0;
    unsigned int n = 0;
    float knotLast = 0;
    unsigned int xType;
    unsigned int yType;
    char *c;
    unsigned int i = 0;
    unsigned int num_points = 0;
    float u;
    unsigned int steps = 40;
    float start_u, step_u;

    if (theDoc->debug_comments)
        g_debug("plot_nurbs(), current x=%f y=%f", current->x, current->y);

    item = *more;
    Any = (struct vdx_any *)(item->data);

    if (Any->type == vdx_types_MoveTo)
    {
        MoveTo = (struct vdx_MoveTo*)(item->data);
        current->x = MoveTo->X;
        current->y = MoveTo->Y;
        begin = *current;
        if (theDoc->debug_comments)
            g_debug("MoveTo x=%f y=%f", begin.x, begin.y);
        item = item->next;
        Any = (struct vdx_any *)(item->data);
        *more = item;
    }

    if (Any->type == vdx_types_NURBSTo)
    {
        if (theDoc->debug_comments) g_debug("NURBSTo");
        NURBSTo = (struct vdx_NURBSTo*)(item->data);

        /* E holds the NURBS formula */
        c = NURBSTo->E;

        /* NURBS(knotLast, degree, xType, yType, x1, y1, knot1, weight1, ...) */

        /* There should be 4n + 4 values, and so 4n + 3 commas */
        n = 1;
        while (*c)
        {
            if (*c++ == ',') { n++; }
        }
        if (n % 4 || ! n)
        {
            dia_context_add_message(ctx, _("Invalid NURBS formula"));
            g_debug("Invalid NURBS formula");
            return 0;
        }
        n /= 4;
        n--;

        /* Parse out the first four params */
        c = NURBSTo->E;
        c += strlen("NURBS(");

        knotLast = atof(c);
        c = strchr(c, ',');
        if (!c) { return 0; }

        k = atoi(++c);
        c = strchr(c, ',');
        if (!c) { return 0; }

        xType = atoi(++c);
        c = strchr(c, ',');
        if (!c) return 0;

        yType = atoi(++c);
        c = strchr(c, ',');
        if (!c) return 0;

        /* Setup the arrays for n+1 points, degree k */
        control = g_new(Point, n+1);
        weight = g_new(float, n+1);
        knot = g_new(float, n+k+2);
        num_points = steps+1;
        points = g_new0(Point, num_points);

        /* Some missing data from the NURBSTo */
        weight[n] = NURBSTo->B;
        control[n].x = NURBSTo->X;
        control[n].y = NURBSTo->Y;
        knot[n] = NURBSTo->A;

        end.x = NURBSTo->X; end.y = NURBSTo->Y;
        know_end = TRUE;

        i = 0;
        while(c && *c && i < n)
        {
            if (!c) break;
            control[i].x = atof(++c);
            current->x = control[i].x;
            /* xType = 0 means X is proportion of Width */
            if (xType == 0) control[i].x *= XForm->Width;
            c = strchr(c, ',');

            if (!c) break;
            control[i].y = atof(++c);
            current->y = control[i].y;
            /* yType = 0 means Y is proportion of Height */
            if (yType == 0) control[i].y *= XForm->Height;
            c = strchr(c, ',');

            if (!c) break;
            knot[i] = atof(++c);
            c = strchr(c, ',');

            if (!c) break;
            weight[i] = atof(++c);
            c = strchr(c, ',');
            i++;
        }
        *more = item->next;
    }
    else
    {
        if (Any->type == vdx_types_SplineStart)
        {
            if (theDoc->debug_comments) g_debug("SplineStart");
            SplineStart = (struct vdx_SplineStart*)(item->data);
            item2 = item;
            n = 1;
            while ((item2 = item2->next))
            {
                Any = (struct vdx_any*)(item2->data);
                g_return_val_if_fail (Any != NULL, NULL); /* pathologic case */
                if (Any->type != vdx_types_SplineKnot) break;
                n++;
            }
            k = SplineStart->D;

            /* Setup the arrays for n+1 points, degree k */
            if (theDoc->debug_comments)
                g_debug("%d points degree %d", n, k);
            control = g_new(Point, n+1);
            weight = g_new(float, n+1);
            knot = g_new(float, n+k+2);
            num_points = steps+1;
            points = g_new0(Point, num_points);
            knotLast = SplineStart->C;

            /* Some missing data */
            control[0].x = current->x;
            control[0].y = current->y;
            control[1].x = SplineStart->X;
            control[1].y = SplineStart->Y;
            knot[0] = SplineStart->A;
            knot[1] = SplineStart->B;

            for(i=0; i<=n; i++)
            {
                weight[i] = 1;
            }

            i = 2;
            item = item->next;
            *more = item;
            while(item)
            {
                Any = (struct vdx_any*)(item->data);
                if (Any->type != vdx_types_SplineKnot)
                {
                    if (theDoc->debug_comments)
                        g_debug("Not a knot");
                    break;
                }
                SplineKnot = (struct vdx_SplineKnot *)(item->data);

                control[i].x = SplineKnot->X;
                control[i].y = SplineKnot->Y;
                knot[i] = SplineKnot->A;
                item = item->next;
                *more = item;
                current->x = SplineKnot->X;
                current->y = SplineKnot->Y;
                i++;
            }
        }
        else
        {
            if (theDoc->debug_comments)
                g_debug("Unexpected NURBS component: %s",
                        vdx_Types[(unsigned int)Any->type]);
            if (Any->type == vdx_types_LineTo)
                return plot_polyline(Geom, XForm, Fill, Line, theDoc, more,
                                     current, ctx);
            if (Any->type == vdx_types_EllipticalArcTo ||
                Any->type == vdx_types_ArcTo)
                return plot_bezier(Geom, XForm, Fill, Line, theDoc,
                                   more, current, ctx);
            return 0;
        }
    }

    /* Add in remaining knots */
    for (i=n+1; i<n+k+2; i++) { knot[i] = knotLast; }

    step_u = (knotLast - knot[0] - 4*EPSILON)/steps;
    start_u = knot[0] + 2*EPSILON;

    if (theDoc->debug_comments)
    {
        g_debug("k = %d n = %d", k, n);
        for (i=0; i<=n; i++)
        {
            g_debug("Control point %d x=%f y=%f w=%f", i, control[i].x,
                    control[i].y, weight[i]);
        }
        for (i=0; i<=n+k+1; i++)
        {
            g_debug("Knot %d = %f", i, knot[i]);
        }
    }

    /* Known start point */
    points[0] = dia_point(apply_XForm(begin, XForm), theDoc);
    if (theDoc->debug_comments)
        g_debug("Point 0 VDX x=%f y=%f", begin.x, begin.y);

    for (i=1; i<=steps; i++)
    {
        u = start_u + i*step_u;
        p = NURBS_C(k, u, n, knot, weight, control);
        if (theDoc->debug_comments)
            g_debug("Point %d VDX x=%f y=%f", i, p.x, p.y);
        points[i] = dia_point(apply_XForm(p, XForm), theDoc);
    }

    if (know_end)
    {
        points[num_points-1] = dia_point(apply_XForm(end, XForm), theDoc);
        if (theDoc->debug_comments)
            g_debug("End Point %d VDX x=%f y=%f", num_points-1, end.x, end.y);
        *current = end;
    }

    if (theDoc->debug_comments)
    {
        for (i=0; i<num_points; i++)
        {
            g_debug("Point %d x=%f y=%f", i, points[i].x, points[i].y);
        }
    }

    if (Line && Line->BeginArrow)
    {
        start_arrow_p = make_arrow(Line, 's', theDoc);
    }

    if (Line && Line->EndArrow)
    {
        end_arrow_p = make_arrow(Line, 'e', theDoc);
    }

    newobj = create_standard_polyline(num_points, points, end_arrow_p, start_arrow_p);

    g_clear_pointer (&points, g_free);
    g_clear_pointer (&end_arrow_p, g_free);
    g_clear_pointer (&start_arrow_p, g_free);
    g_clear_pointer (&control, g_free);
    g_clear_pointer (&weight, g_free);
    g_clear_pointer (&knot, g_free);

    vdx_simple_properties(newobj, Fill, Line, theDoc, ctx);
    return newobj;
}

/** Plots a bitmap
 * @param Geom the shape
 * @param XForm any transformations
 * @param Foreign the object location
 * @param ForeignData the object
 * @param theDoc the document
 * @param ctx the context for error/warning messages
 * @return the new object
 */
static DiaObject *
plot_image(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm,
           const struct vdx_Foreign *Foreign,
           const struct vdx_ForeignData *ForeignData,
           VDXDocument* theDoc, const GSList **more, Point *current, DiaContext *ctx)
{
    DiaObject *newobj = NULL;

    Point p;
    float h, w;

    GSList *item;
    struct vdx_any *Any;
    struct vdx_text *text;
    const char *base64_data = 0;

    *more = 0;
    /* We can only take a few formats */
    if (!ForeignData->CompressionType)
    {
        if (ForeignData->ForeignType &&
            !strcmp(ForeignData->ForeignType, "Bitmap"))
        {
            /* BMP */
        }
        else
        {
            dia_context_add_message(ctx, _("Couldn't handle foreign object type %s"),
                          ForeignData->ForeignType ? ForeignData->ForeignType
                          : "Unknown");
            return 0;
        }
    }

    /* Find the data in Base64 encoding in the body of ForeignData */
    for (item = ForeignData->any.children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        if (Any->type == vdx_types_text)
        {
            text = (struct vdx_text *)(item->data);
            base64_data = text->text;
        }
    }

    /* Positioning data */
    p.x = Foreign->ImgOffsetX;
    p.y = Foreign->ImgOffsetY;
    p = dia_point(apply_XForm(p, XForm), theDoc);
    h = dia_length(Foreign->ImgHeight, theDoc);
    w = dia_length(Foreign->ImgWidth, theDoc);

    /* Visio supplies bottom left, but Dia needs top left */
    p.y -= h;

    newobj = create_standard_image(p.x, p.y, w, h, NULL);
    {
	GPtrArray *props = g_ptr_array_new ();
	PixbufProperty *prop = (PixbufProperty *)make_new_prop ("pixbuf", PROP_TYPE_PIXBUF, PROP_FLAG_DONT_SAVE);

	/* In error prop->pixbuf is NULL, aka. broken image */
	prop->pixbuf = pixbuf_decode_base64 (base64_data);
	g_ptr_array_add (props, prop);

	newobj->ops->set_props(newobj, props);
	prop_list_free(props);
    }
    return newobj;
}

/** Plots a shape
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param Foreign foreign object location
 * @param ForeignData foreign object
 * @param theDoc the document
 * @returns the new object
 */

enum dia_types { vdx_dia_any = 0, vdx_dia_text, vdx_dia_ellipse, vdx_dia_box,
                 vdx_dia_polyline, vdx_dia_polygon, vdx_dia_bezier,
                 vdx_dia_beziergon, vdx_dia_arc, vdx_dia_line, vdx_dia_image,
                 vdx_dia_zigzagline, vdx_dia_nurbs };

static DiaObject *
plot_geom(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm,
          const struct vdx_XForm1D *XForm1D,
          const struct vdx_Fill *Fill, const struct vdx_Line *Line,
          const struct vdx_Foreign *Foreign,
          const struct vdx_ForeignData *ForeignData,
          VDXDocument* theDoc, const GSList **more, Point *current,
          DiaContext *ctx)
{
    const GSList *item;
    gboolean all_lines = TRUE;  /* Flag for line/polyline */
    gboolean has_nurbs = FALSE; /* Flag for NURBS */
    unsigned int num_steps = 0; /* Flag for poly */
    struct vdx_any *last_point = 0;
    unsigned int dia_type_choice = vdx_dia_any;
    /* Only ArcTo and LineTo get Del, apparently */
    struct vdx_LineTo *LineTo;
    struct vdx_ArcTo *ArcTo;

    /* Is it disabled? */
    if (!Geom || Geom->NoShow)
    {
        if (theDoc->debug_comments)
            g_debug("NoShow");
        *more = 0;
        return 0;
    }

    /* Determine what kind of Dia object we need */

    for(item = *more; item; item = item->next)
    {
        if (!item->data) continue;
        switch (((struct vdx_any *)(item->data))->type)
        {
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            if (LineTo->Del) continue;
            break;
        case vdx_types_MoveTo:
            /* No problem to have a MoveTo inside an object */
            continue;
        case vdx_types_ArcTo:
            ArcTo = (struct vdx_ArcTo*)(item->data);
            if (ArcTo->Del) continue;
            all_lines = FALSE;
            break;
        case vdx_types_NURBSTo:
        case vdx_types_SplineKnot:
            has_nurbs = TRUE;
            all_lines = FALSE;
            break;
        default:
            all_lines = FALSE;
        }
        last_point = (struct vdx_any *)(item->data);
            num_steps++;
    }

    if (all_lines)
    {
        dia_type_choice = vdx_dia_polyline;
    }
    if (num_steps == 1)
    {
        /* Single object - but what sort? */
        if (last_point->type == vdx_types_EllipticalArcTo)
            dia_type_choice = vdx_dia_bezier;
        if (last_point->type == vdx_types_Ellipse)
            dia_type_choice = vdx_dia_ellipse;
        if (last_point->type == vdx_types_LineTo)
            dia_type_choice = vdx_dia_line;
        if (last_point->type == vdx_types_NURBSTo)
            dia_type_choice = vdx_dia_nurbs;
        if (last_point->type == vdx_types_PolylineTo)
            dia_type_choice = vdx_dia_polyline;
    }
    if (dia_type_choice == vdx_dia_polyline)
    {
        /* Fill determines if we're -line or -gon */
        if (Geom->NoFill) { dia_type_choice = vdx_dia_polyline; }
        else { dia_type_choice = vdx_dia_polygon; }
    }
    if (ForeignData) { dia_type_choice = vdx_dia_image; }
    if (dia_type_choice == vdx_dia_any)
    {
        /* Still undecided? Then it's a Bezier(-gon) */
        if (Geom->NoFill) { dia_type_choice = vdx_dia_bezier; }
        else { dia_type_choice = vdx_dia_beziergon; }
        /* Unless it's a spline */
        if (last_point->type == vdx_types_SplineKnot || has_nurbs)
            dia_type_choice = vdx_dia_nurbs;
    }

    switch(dia_type_choice)
    {
    case vdx_dia_line:
    case vdx_dia_polyline:
    case vdx_dia_polygon:
        return plot_polyline(Geom, XForm, Fill, Line, theDoc, more, current, ctx);
        break;
    case vdx_dia_ellipse:
        return plot_ellipse(Geom, XForm, Fill, Line, theDoc, more, current, ctx);
        break;
    case vdx_dia_beziergon:
    case vdx_dia_bezier:
        return plot_bezier(Geom, XForm, Fill, Line, theDoc, more, current, ctx);
        break;
    case vdx_dia_image:
        return plot_image(Geom, XForm, Foreign, ForeignData, theDoc, more, current, ctx);
        break;
    case vdx_dia_nurbs:
        return plot_nurbs(Geom, XForm, Fill, Line, theDoc, more, current, ctx);
        break;
    default:
        g_debug("Not yet implemented");
        break;
    }
    return 0;
}


/** Draws some text
 * @param Text the text
 * @param XForm any transformations
 * @param Char font info
 * @param Para alignment info
 * @param theDoc the document
 * @returns the new object
 */
static DiaObject *
plot_text(const struct vdx_Text *vdxText, const struct vdx_XForm *XForm,
          const struct vdx_Char *Char, const struct vdx_Para *Para,
          const struct vdx_TextBlock *TextBlock,
          const struct vdx_TextXForm *TextXForm,
          VDXDocument* theDoc)
{
    DiaObject *newobj;
    GPtrArray *props;
    TextProperty *tprop;
    Valign vert_align;
    DiaAlignment alignment;
    EnumProperty *eprop = 0;
    struct vdx_FontEntry FontEntry;
    struct vdx_FaceName FaceName;
    struct vdx_text * text = find_child(vdx_types_text, vdxText);
    Point p;
    int i;
    double height;
    char *font_name = 0;
    DiaFontStyle style = 0;
    DiaFont *font = 0;

    if (!vdxText || !Char || !text || !XForm)
    {
        g_debug("Not enough info for text");
        return 0;
    }
    p.x = 0; p.y = 0;

    /* Setup position for horizontal alignment */
    alignment = DIA_ALIGN_LEFT;
    if (Para && Para->HorzAlign == 1) {
      alignment = DIA_ALIGN_CENTRE;
      p.x += XForm->Width / 2.0;
    }

    if (Para && Para->HorzAlign == 2) {
      alignment = DIA_ALIGN_RIGHT;
      p.x += XForm->Width;
    }

    /* And for vertical */
    vert_align = VALIGN_TOP;
    if (TextBlock && TextBlock->VerticalAlign == 0)
    {
        p.y += XForm->Height;
        vert_align = VALIGN_TOP;
    }
    if (TextBlock && TextBlock->VerticalAlign == 1)
    {
        p.y += XForm->Height/2.0;
        vert_align = VALIGN_CENTER;
    }
    if (TextBlock && TextBlock->VerticalAlign == 2)
    {
        vert_align = VALIGN_BOTTOM;
	/*
	 * Not shifting by height makes text position right for text_tests.vdx
	 * Doing no shift for the other VerticalAlign screws them ...
        p.y -= XForm->Height;
	 */
    }

    height = Char->Size*vdx_Font_Size_Conversion;

    /* Text formatting */
    if (Char->Style & 1) { style |= DIA_FONT_BOLD; }
    if (Char->Style & 2) { style |= DIA_FONT_ITALIC; }
    /* Can't do underline or small caps */

    /* Create the object at position p */
    if (TextXForm)
    {
        p.x -= TextXForm->TxtPinX - TextXForm->TxtLocPinX;
        p.y -= TextXForm->TxtPinY - TextXForm->TxtLocPinY;
        /* height = TextXForm->TxtHeight*vdx_Line_Scale; */
    }

    p = dia_point(apply_XForm(p, XForm), theDoc);
    newobj = create_standard_text(p.x, p.y);

    /* Get the property list */
    props = prop_list_from_descs(vdx_text_descs,pdtpp_true);
    tprop = g_ptr_array_index(props,0);
    /* Vertical alignment gets a separate property */
    eprop = g_ptr_array_index(props,1);
    eprop->enum_data = vert_align;

    /* set up the text property by including all children */
    tprop->text_data = g_strdup(text->text);
    while ((text = find_child_next (vdx_types_text, vdxText, text))) {
      char *s = tprop->text_data;
      tprop->text_data = g_strconcat (tprop->text_data, text->text, NULL);
      g_clear_pointer (&s, g_free);
    }

    /* Fix Unicode line breaks */
    for (i=0; tprop->text_data[i]; i++)
    {
        if ((unsigned char)tprop->text_data[i] == 226 &&
            (unsigned char)tprop->text_data[i+1] == 128 &&
            (unsigned char)tprop->text_data[i+2] == 168)
        {
            tprop->text_data[i] = 10;
            memmove(&tprop->text_data[i+1], &tprop->text_data[i+3],
                    strlen(&tprop->text_data[i+3])+1);
        }
    }

#if 0 /* this is not utf-8 safe - see bug 683700 */
    /* Remove trailing line breaks */
    while (tprop->text_data[0] &&
           isspace(tprop->text_data[strlen(tprop->text_data)-1]))
    {
        tprop->text_data[strlen(tprop->text_data)-1] = 0;
    }
#else
    {
        char *s = tprop->text_data;
        char *srep = NULL;
        while ( (s = g_utf8_strchr(s, -1, '\n')) != NULL ) {
            srep = s;
            s = g_utf8_next_char(s);
            if (*s)
                srep = NULL;
            else
                break;
        }
        if (srep)
            *srep = '\0';
    }
#endif

    /* Other standard text properties */
    tprop->attr.alignment = alignment;
    tprop->attr.position.x = p.x;
    tprop->attr.position.y = p.y;

    font_name = "sans";
    if (theDoc->Fonts)
    {
        if (Char->Font < theDoc->Fonts->len)
        {
            FontEntry =
                g_array_index(theDoc->Fonts, struct vdx_FontEntry, Char->Font);
            font_name = FontEntry.Name;
        }
    }
    else if (theDoc->FaceNames)
    {
        if (Char->Font < theDoc->FaceNames->len)
        {
            FaceName =
                g_array_index(theDoc->FaceNames,
                              struct vdx_FaceName, Char->Font);
            font_name = FaceName.Name;
        }
    }

    font = dia_font_new_from_legacy_name(font_name);
    if (!font)
    {
        g_debug("Unable to find font '%s'; using Arial", font_name);
        font = dia_font_new_from_legacy_name("Arial");
    }
    dia_font_set_weight(font, DIA_FONT_STYLE_GET_WEIGHT(style));
    dia_font_set_slant(font, DIA_FONT_STYLE_GET_SLANT(style));
    dia_font_set_height(font, height);
    tprop->attr.font = font;

    if (theDoc->debug_comments)
        g_debug("Text: %s at %f,%f v=%d h=%d s=%.2x f=%s",
                tprop->text_data, p.x, p.y,
                eprop->enum_data, tprop->attr.alignment, style, font_name);

    tprop->attr.height = height;
    tprop->attr.color = Char->Color;
    newobj->ops->set_props(newobj, props);
    prop_list_free(props);
    return newobj;
}

/** Plots a shape
 * @param Shape the Shape
 * @param objects list of plotted objects
 * @param group_XForm a transform to apply to all objects, or NULL
 * @param theDoc the document
 * @param ctx the context for error/warning messages
 * @return list of objects to add
 */
static GSList *
vdx_plot_shape(struct vdx_Shape *Shape, GSList *objects,
               struct vdx_XForm* group_XForm,
               VDXDocument* theDoc, DiaContext *ctx)
{
    struct vdx_Fill *Fill = 0;
    struct vdx_Fill *ShapeFill = 0;
    struct vdx_Char *Char = 0;
    struct vdx_Line *Line = 0;
    struct vdx_Line *ShapeLine = 0;
    struct vdx_Geom *Geom = 0;
    struct vdx_XForm *XForm = 0;
    struct vdx_XForm1D *XForm1D = 0;
    struct vdx_TextXForm *TextXForm = 0;
    struct vdx_Text *vdxText = 0;
    struct vdx_TextBlock *TextBlock = 0;
    struct vdx_Para *Para = 0;
    struct vdx_Foreign * Foreign = 0;
    struct vdx_ForeignData * ForeignData = 0;
    const GSList *more = 0;
    Point current = {0, 0};

    if (!Shape)
    {
        g_debug("vdx_plot_shape() called with Shape=0");
        return 0;
    }
    theDoc->shape_id = Shape->ID;
    if (Shape->Del)
    {
        if (theDoc->debug_comments) g_debug("Shape %d deleted", Shape->ID);
        return objects;
    }
    if (Shape->Type && !strcmp(Shape->Type, "Guide"))
    {
        if (theDoc->debug_comments) g_debug("Ignoring shape");
        return objects;
    }
    if (Shape->Type && !strcmp(Shape->Type, "Foreign"))
    {
        if (theDoc->debug_comments) g_debug("Shape %d Foreign", Shape->ID);
    }

    /* Is there a local definition? */
    ShapeFill = (struct vdx_Fill *)find_child(vdx_types_Fill, Shape);
    ShapeLine = (struct vdx_Line *)find_child(vdx_types_Line, Shape);
    Char = (struct vdx_Char *)find_child(vdx_types_Char, Shape);
    XForm = (struct vdx_XForm *)find_child(vdx_types_XForm, Shape);
    XForm1D = (struct vdx_XForm1D *)find_child(vdx_types_XForm1D, Shape);
    TextXForm = (struct vdx_TextXForm *)find_child(vdx_types_TextXForm, Shape);
    Geom = (struct vdx_Geom *)find_child(vdx_types_Geom, Shape);
    vdxText = (struct vdx_Text *)find_child(vdx_types_Text, Shape);
    TextBlock = (struct vdx_TextBlock *)find_child(vdx_types_TextBlock, Shape);
    Para = (struct vdx_Para *)find_child(vdx_types_Para, Shape);
    Foreign = (struct vdx_Foreign *)find_child(vdx_types_Foreign, Shape);
    ForeignData =
        (struct vdx_ForeignData *)find_child(vdx_types_ForeignData, Shape);

    /* Is there a Master? */
    if (Shape->Master_exists)
    {
        /* We can pick up Fill, Line and Char from the master */
        struct vdx_Shape *MasterShape = 0;

        if (Shape->MasterShape_exists)
        {
            MasterShape = get_master_shape(Shape->Master, Shape->MasterShape,
                                           theDoc, ctx);
        }
        else
        {
            /* Get the first shape and use that */
            MasterShape = get_master_shape(Shape->Master, 0, theDoc, ctx);
        }

        if (MasterShape)
        {
            if (theDoc->debug_comments) g_debug("Using master");
            if (!ShapeFill)
            {
                if (theDoc->debug_comments) g_debug("Looking for master Fill");

                ShapeFill = (struct vdx_Fill *)find_child(vdx_types_Fill,
                                                          MasterShape);
                if (ShapeFill && theDoc->debug_comments)
                    g_debug("Using master Fill");
                if (!ShapeFill)
                {
                    if (!Shape->FillStyle_exists)
                    {
                        if (theDoc->debug_comments)
                            g_debug("Using master FillStyle");
                        Shape->FillStyle_exists = TRUE;
                        Shape->FillStyle = MasterShape->FillStyle;
                    }
                }
            }

            if (!ShapeLine)
            {
                if (theDoc->debug_comments) g_debug("Looking for master Line");
                ShapeLine = (struct vdx_Line *)find_child(vdx_types_Line,
                                                          MasterShape);
                if (ShapeLine && theDoc->debug_comments)
                    g_debug("Using master Line");
                if (!ShapeLine)
                {
                    if (!Shape->LineStyle_exists)
                    {
                        if (theDoc->debug_comments)
                            g_debug("Using master LineStyle");
                        Shape->LineStyle_exists = TRUE;
                        Shape->LineStyle = MasterShape->LineStyle;
                    }
                }
            }
            if (!Geom)
            {
                if (theDoc->debug_comments) g_debug("Looking for master Geom");
                Geom = (struct vdx_Geom *)find_child(vdx_types_Geom,
                                                     MasterShape);
            }
            if (!Para)
            {
                if (theDoc->debug_comments) g_debug("Looking for master Para");
                Para = (struct vdx_Para *)find_child(vdx_types_Para,
                                                     MasterShape);
            }
            if (!TextBlock)
            {
                if (theDoc->debug_comments)
                    g_debug("Looking for master TextBlock");
                TextBlock = (struct vdx_TextBlock *)
                    find_child(vdx_types_TextBlock,
                               MasterShape);
            }
        }
    }

    if (!ShapeFill)
        ShapeFill = (struct vdx_Fill *)get_style_child(vdx_types_Fill,
                                                       Shape->FillStyle,
                                                       theDoc);
    if (!ShapeLine)
        ShapeLine = (struct vdx_Line *)get_style_child(vdx_types_Line,
                                                       Shape->LineStyle,
                                                       theDoc);
    if (!Char)
        Char = (struct vdx_Char *)get_style_child(vdx_types_Char,
                                                  Shape->TextStyle,
                                                  theDoc);
    if (!Para)
        Para = (struct vdx_Para *)get_style_child(vdx_types_Para,
                                                  Shape->TextStyle,
                                                  theDoc);
    if (!TextBlock)
        TextBlock = (struct vdx_TextBlock *)
            get_style_child(vdx_types_TextBlock,
                            Shape->TextStyle,
                            theDoc);

    /* If we're in a group, apply an overall XForm to everything */
    if (group_XForm)
    {
        if (XForm)
        {
            /* Populate the XForm's children list with the parent XForm */
            XForm->any.children =
                g_slist_append(XForm->any.children, group_XForm);
        }
        else
        {
            XForm = group_XForm;
        }
    }

    /* Am I a group? */
    if (Shape->Type && !strcmp(Shape->Type, "Group")
        && find_child(vdx_types_Shapes, Shape))
    {
        GSList *members = NULL;
        GSList *child;
        GList *group = NULL;
        struct vdx_Shapes* Shapes =
            (struct vdx_Shapes*)find_child(vdx_types_Shapes, Shape);

        /* Create a list of member objects */
        for (child = Shapes->any.children; child; child = child->next)
        {
            struct vdx_Shape * theShape = (struct vdx_Shape*)(child->data);
            if (!theShape) continue;
            if (theShape->any.type == vdx_types_Shape)
            {
                if (!theShape->Master)
                {
                    theShape->Master = Shape->Master;
                    theShape->Master_exists = Shape->Master_exists;
                }
                if (theDoc->debug_comments)
                    g_debug("Child shape %d", theShape->ID);
                members = vdx_plot_shape(theShape, members, XForm, theDoc, ctx);
            }
        }
        for (child = members; child; child = child->next)
        {
            if (child->data) group = g_list_append(group, child->data);
        }

        if (group) /* the above might leave us empty - ignore it: bug 735303 */
            objects = g_slist_append(objects, create_standard_group(group));
        /* g_list_free(group); */
        g_slist_free(members);
    }

    while (Geom)
    {
        Fill = ShapeFill;
        if (Geom->NoFill) Fill = 0;
        Line = ShapeLine;
        if (Geom->NoLine) Line = 0;

        more = Geom->any.children;
        do {
          DiaObject *object = plot_geom (Geom, XForm, XForm1D, Fill, Line,
                                         Foreign, ForeignData, theDoc, &more,
                                         &current, ctx);
            /* object can be NULL for Text */
          if (object) {
            char *id = g_strdup_printf ("%d", Shape->ID);
            objects = g_slist_append (objects, object);
            dia_object_set_meta (object, "id", id);
            g_clear_pointer (&id, g_free);
          }

          if (more && theDoc->debug_comments) {
              g_debug ("Additional Geom");
          }
        } while (more);

        /* Yes, you can have multiple (disconnected) Geoms */
        Geom = find_child_next(vdx_types_Geom, Shape, Geom);
    }

    /* Text always after the object it's attached to,
       so it appears on top */
    if (vdxText && find_child (vdx_types_text, vdxText)) {
      DiaObject *object = plot_text (vdxText, XForm, Char, Para,
                                     TextBlock, TextXForm, theDoc);
      if (object) {
        char *id = g_strdup_printf ("%d", Shape->ID);
        objects = g_slist_append (objects, object);
        dia_object_set_meta (object, "id", id);
        g_clear_pointer (&id, g_free);
      }
    }

    /* Wipe the child XForm list to avoid double-free */
    if (XForm) XForm->any.children = 0;
    return objects;
}

/** Parses a shape and adds it to the diagram
 * @param Shape the XML to parse
 * @param PageSheet the page property sheet
 * @param theDoc the document
 * @param dia the growing diagram
 * @param ctx the context for error/warning messages
 */
static void
vdx_parse_shape(xmlNodePtr Shape, struct vdx_PageSheet *PageSheet,
                VDXDocument* theDoc, DiagramData *dia, DiaContext *ctx)
{
    /* All decoding is done in Visio-space */
    struct vdx_Shape theShape = { {0, }, };
    GSList *objects = NULL;
    GSList *object;
    struct vdx_LayerMem *LayerMem = NULL;
    unsigned int dia_layer_num = 0;
    DiaLayer *diaLayer = NULL;
    char *name = NULL;

    if (theDoc->PageLayers)
        dia_layer_num = theDoc->PageLayers->len;

    if (PageSheet)
    {
        /* Default styles */
        theShape.TextStyle = PageSheet->TextStyle;
        theShape.FillStyle = PageSheet->FillStyle;
        theShape.LineStyle = PageSheet->LineStyle;
    }
    vdx_read_object(Shape, theDoc, &theShape, ctx);

    /* Avoid very bad shapes */
    if (!theShape.Type)
    {
        g_debug("Can't parse shape");
        return;
    }

    /* Name of shape could be in Unicode, or not, or missing */
    name = theShape.NameU;
    if (!name) name = theShape.Name;
    if (!name) name = "Unnamed";
    if (theDoc->debug_comments) g_debug("Shape %d [%s]", theShape.ID, name);

    /* Ignore Guide */
    /* For debugging purposes, use Del attribute and stop flag */
    if (!strcmp(theShape.Type, "Guide") || theShape.Del || theDoc->stop)
    {
        if (theDoc->debug_comments) g_debug("Ignoring shape");
        free_children(&theShape);
        return;
    }

    /* Remove NULLs */
    theShape.any.children = g_slist_remove_all(theShape.any.children, 0);

    /* What layer are we on, if any? */
    LayerMem = find_child(vdx_types_LayerMem, &theShape);
    if (LayerMem && theDoc->PageLayers)
    {
        /* Imported layer number */
        unsigned int layer_num = 0;
        if (LayerMem->LayerMember)
        {
            layer_num = (unsigned int)atoi(LayerMem->LayerMember);
        }
        /* Translate to Dia layer number */
        if (layer_num < theDoc->PageLayers->len)
            dia_layer_num = g_array_index(theDoc->PageLayers, unsigned int,
                                          layer_num);
        if (theDoc->debug_comments)
            g_debug("Layer %d -> %d", layer_num, dia_layer_num);
  } else {
      if (theDoc->debug_comments)
          g_debug("Layer %d", dia_layer_num);
  }
  diaLayer = data_layer_get_nth (dia, dia_layer_num);

  /* Draw the shape (or group) and get list of created objects */
  objects = vdx_plot_shape (&theShape, objects, 0, theDoc, ctx);

  /* Add the objects straight into the diagram */
  /* This isn't strictly correct as a child object can be on a
      different layer from its parent. */
  for (object = objects; object; object = object->next) {
    if (!object->data) continue;

    dia_layer_add_object (diaLayer, DIA_OBJECT (object->data));
  }

  free_children (&theShape);
  g_slist_free (objects);
}


/**
 * vdx_setup_layers:
 * @PageSheet: the PageSheet
 * @theDoc: the document
 * @dia: the growing diagram
 *
 * Parse the pages of the VDX
 *
 * Bug: This doesn't handle multi-page diagrams very well
 */
static void
vdx_setup_layers (struct vdx_PageSheet *PageSheet,
                  VDXDocument          *theDoc,
                  DiagramData          *dia)
{
  GSList *child = NULL;
  GSList *layernames = NULL;
  GSList *layername = NULL;
  struct vdx_any* Any;
  struct vdx_Layer *theLayer;
  DiaLayer *diaLayer = 0;
  unsigned int found_layer, page_layer;
  gboolean found;

  /* What layers are on this page? */

  if (!PageSheet) {
    g_debug ("vdx_setup_layers() called with PageSheet=0");
    return;
  }

  for (child = PageSheet->any.children; child; child = child->next) {
    if (!child || !child->data) {
      continue;
    }

    Any = (struct vdx_any *) (child->data);
    if (Any->type != vdx_types_Layer) {
      continue;
    }

    theLayer = (struct vdx_Layer *) child->data;
    layernames = g_slist_prepend (layernames, theLayer->Name);
  }

  /* Add any missing layers to Dia's list
     Must be back to front
     Also construct translation table for this page's layers */

  if (theDoc->PageLayers) {
    g_array_free (theDoc->PageLayers, TRUE);
  }
  theDoc->PageLayers = g_array_new (FALSE, TRUE, sizeof (unsigned int));

  if (!theDoc->LayerNames) {
    theDoc->LayerNames = g_array_new(FALSE, TRUE, sizeof (char *));
  }

  page_layer = 0;
  for (layername = layernames; layername; layername = layername->next) {
    found = FALSE;
    for (found_layer = 0; found_layer < theDoc->LayerNames->len; found_layer++) {
      if (layername->data &&
          g_array_index (theDoc->LayerNames, char *, found_layer) &&
          !strcmp ((char *) layername->data,
                   g_array_index (theDoc->LayerNames, char *, found_layer)))
      {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      g_array_append_val (theDoc->LayerNames, layername->data);
      g_clear_object (&diaLayer);
      diaLayer = dia_layer_new (((char*) layername->data), dia);
      data_add_layer (dia, diaLayer);
    }
    page_layer++;
    g_array_prepend_val (theDoc->PageLayers, page_layer);
  }

  data_set_active_layer (dia, diaLayer);
  g_clear_object (&diaLayer);
}

/** Parse the pages of the VDX
 * @param cur current XML node
 * @param theDoc the document
 * @param dia the growing diagram
 * @param ctx the context for error/warning messages
 */
static void
vdx_get_pages(xmlNodePtr cur, VDXDocument* theDoc, DiagramData *dia, DiaContext *ctx)
{
    xmlNodePtr Page;
    xmlNodePtr Shapes;

    for (Page = cur->xmlChildrenNode; Page; Page = Page->next)
    {
        struct vdx_PageSheet PageSheet;
        xmlAttrPtr attr;
        gboolean background = FALSE;
        memset(&PageSheet, 0, sizeof(PageSheet));
        if (xmlIsBlankNode(Page)) { continue; }

        for (Shapes = Page->xmlChildrenNode; Shapes; Shapes = Shapes->next)
        {
            xmlNodePtr Shape;
            if (xmlIsBlankNode(Shapes)) { continue; }
            if (!strcmp((char *)Shapes->name, "PageSheet")) {
                vdx_read_object(Shapes, theDoc, &PageSheet, ctx);
                vdx_setup_layers(&PageSheet, theDoc, dia);
                continue;
            }

            if (strcmp((char *)Shapes->name, "Shapes")) {
                /* Ignore non-shapes for now */
                continue;
            }
            for (Shape = Shapes->xmlChildrenNode; Shape; Shape=Shape->next)
            {
                if (xmlIsBlankNode(Shape)) { continue; }
                vdx_parse_shape(Shape, &PageSheet, theDoc, dia, ctx);
            }
        }
        for (attr = Page->properties; attr; attr = attr->next)
        {
            if (!strcmp((char *)attr->name, "Background"))
            {
                background = TRUE;
            }
        }
        if (!background) theDoc->Page++;
        free_children(&PageSheet);
    }
}

/** Free a document
 * @param theDoc the document
 */

static void
vdx_free(VDXDocument *theDoc)
{
    int i;
    struct vdx_StyleSheet theSheet;
    struct vdx_Master theMaster;

    if (theDoc->Colors) g_array_free(theDoc->Colors, TRUE);
    if (theDoc->Fonts) g_array_free(theDoc->Fonts, TRUE);
    if (theDoc->FaceNames) g_array_free(theDoc->FaceNames, TRUE);

    if (theDoc->StyleSheets)
    {
        for (i=0; i<theDoc->StyleSheets->len; i++) {
            theSheet = g_array_index(theDoc->StyleSheets,
                                     struct vdx_StyleSheet, i);
            free_children(&theSheet);
        }
        g_array_free(theDoc->StyleSheets, TRUE);
    }
    if (theDoc->Masters)
    {
        for (i=0; i<theDoc->Masters->len; i++) {
            theMaster = g_array_index(theDoc->Masters,
                                     struct vdx_Master, i);
            free_children(&theMaster);
        }
        g_array_free(theDoc->Masters, TRUE);
    }
    if (theDoc->LayerNames) g_array_free(theDoc->LayerNames, TRUE);
    if (theDoc->PageLayers) g_array_free(theDoc->PageLayers, TRUE);
    g_clear_pointer (&theDoc->debug_shape_ids, g_free);
    g_clear_pointer (&theDoc, g_free);
}


/** Imports the given file
 * @param filename the file to read
 * @param dia the diagram
 * @param user_data unused
 * @returns TRUE if successful, FALSE otherwise
 */
static gboolean
import_vdx (const char  *filename,
            DiagramData *dia,
            DiaContext  *ctx,
            void        *user_data)
{
  xmlDocPtr doc = dia_io_load_document (filename, ctx, NULL);
  xmlNodePtr root, cur;
  struct VDXDocument *theDoc;
  const char *s;
  int visio_version = 0;
  const char *debug = 0;
  unsigned int debug_shapes = 0;
  char *old_locale;

  if (!doc) {
    dia_context_add_message (ctx,
                             _("VDX parser error for %s"),
                             dia_context_get_filename (ctx));

    return FALSE;
  }

  /* skip comments */
    for (root = doc->xmlRootNode; root; root = root->next)
    {
        if (root->type == XML_ELEMENT_NODE) { break; }
    }
    if (!root || xmlIsBlankNode(root))
    {
        dia_context_add_message(ctx, _("Nothing in document!"));
        return FALSE;
    }
    if (strcmp((char *)root->name, "VisioDocument"))
    {
        dia_context_add_message(ctx, _("Expecting VisioDocument, got %s"), root->name);
        return FALSE;
    }
    if (root->ns && root->ns->href &&
        !strcmp((char *)root->ns->href,
               "urn:schemas-microsoft-com:office:visio"))
    {
        visio_version = 2002;
    }
    if (root->ns && root->ns->href &&
        !strcmp((char *)root->ns->href,
                "http://schemas.microsoft.com/visio/2003/core"))
    {
        visio_version = 2003;
    }
    theDoc = g_new0(struct VDXDocument, 1);
    theDoc->ok = TRUE;


    /* ugly, but still better than bogus sizes */
    old_locale = setlocale(LC_NUMERIC, "C");

    /* VDX_DEBUG sets verbose per-shape debugging on */
    if (g_getenv("VDX_DEBUG")) theDoc->debug_comments = TRUE;

    /* VDX_DEBUG_SHAPES=all turns on shape identification colouring
       VDX_DEBUG_SHAPES=1,2,3,4 for specific shape IDs */
    if ((debug = g_getenv("VDX_DEBUG_SHAPES")))
    {
        s = debug;
        if (strchr(s, '=')) { s = strchr(debug, '=') + 1; }
        for (; *s; s++)
        {
            if (*s == ',') debug_shapes++;
        }

        theDoc->debug_shape_ids = g_new0(unsigned int, debug_shapes+1);

        debug_shapes = 0;
        s = debug;
        if (strchr(s, '=')) { s = strchr(debug, '=') + 1; }
        theDoc->debug_shape_ids[0] = atoi(s);
        for (; *s; s++)
        {
            if (*s == ',')
            {
                debug_shapes++;
                theDoc->debug_shape_ids[debug_shapes] = atoi(s);
            }
        }
        theDoc->debug_shape_ids[debug_shapes] = 0;
        /* If array is length 0, all shapes are coloured */
    }

    if (theDoc->debug_comments)
        g_debug("Visio version = %d", visio_version);

    for (cur = root->xmlChildrenNode; cur; cur = cur->next)
    {
        if (xmlIsBlankNode(cur)) { continue; }
        s = (const char *)cur->name;
        if (!strcmp(s, "Colors")) vdx_get_colors(cur, theDoc, ctx);
        if (!strcmp(s, "FaceNames")) vdx_get_facenames(cur, theDoc, ctx);
        if (!strcmp(s, "Fonts")) vdx_get_fonts(cur, theDoc, ctx);
        if (!strcmp(s, "Masters")) vdx_get_masters(cur, theDoc, ctx);
        if (!strcmp(s, "StyleSheets")) vdx_get_stylesheets(cur, theDoc, ctx);
        if (!strcmp(s, "Pages")) vdx_get_pages(cur, theDoc, dia, ctx);
        /* if (!theDoc->ok) break; */
    }

    /* Get rid of internal strings before returning */
    vdx_free(theDoc);
    xmlFreeDoc(doc);

    /* dont screw Dia's global state */
    setlocale(LC_NUMERIC, old_locale);

    return TRUE;
}

/* interface from filter.h */

static const char *extensions[] = {"vdx", NULL };
DiaImportFilter vdx_import_filter = {
    N_("Visio XML File Format"),
    extensions,
    import_vdx
};
