/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 *
 * vdx-import.c: Visio XML import filter for dia
 * Copyright (C) 2006 Ian Redfern
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
#include <errno.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "dia_xml_libxml.h"
#include "intl.h"
#include "create.h"
#include "group.h"
#include "font.h"
#include "vdx.h"
#include "visio-types.h"
#include "bezier_conn.h"

gboolean import_vdx(const gchar *filename, DiagramData *dia, void* user_data);

void static vdx_get_colors(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_facenames(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_fonts(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_masters(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_stylesheets(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_free(VDXDocument *theDoc);

/* Note: we can hold pointers to parts of the parsed XML during import, but
   can't pass them to the rest of Dia, as they will be freed when we finish */

static PropDescription vdx_line_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    PROP_STD_START_ARROW,
    PROP_STD_END_ARROW,
    PROP_DESC_END};

static DiaObject *
create_standard_line(Point *points,
                     Arrow *end_arrow,
                     Arrow *start_arrow) {
    DiaObjectType *otype = object_get_type("Standard - Line");
    DiaObject *new_obj;
    Handle *h1, *h2;
    PointProperty *ptprop;
    GPtrArray *props;

    new_obj = otype->ops->create(&points[0], otype->default_user_data,
                                 &h1, &h2);

    props = prop_list_from_descs(vdx_line_prop_descs, pdtpp_true);
    g_assert(props->len == 4);

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



static DiaObject *
create_vdx_beziergon(int num_points, 
			  BezPoint *points) {
    DiaObjectType *otype = object_get_type("Standard - Beziergon");
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData *bcd;
    BezierConn *bcp;
    int i;


    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    bcd = g_new(BezierCreateData, 1);
    bcd->num_points = num_points;
    bcd->points = points;

    new_obj = otype->ops->create(NULL, bcd,
				 &h1, &h2);

    g_free(bcd);

    bcp = (BezierConn *)new_obj;
    g_debug("bcp.np=%d", bcp->numpoints);
    for (i=0; i<bcp->numpoints; i++) 
    { 
        bcp->corner_types[i] = BEZ_CORNER_CUSP;
    }
    
    return new_obj;
}

static PropDescription vdx_simple_prop_descs_line[] = {
    { "line_width", PROP_TYPE_REAL },
    { "line_colour", PROP_TYPE_COLOUR },
    PROP_DESC_END};

static PropDescription vdx_text_descs[] = {
    { "text", PROP_TYPE_TEXT },
    PROP_DESC_END
    /* Can't do the angle */
    /* Height and length are ignored */
    /* Flags */
};


/** Turns a VDX colour definition into a Dia Color.
 * @param s a string from the VDX
 * @param theDoc the current document (with its colour table)
 * @returns A Dia Color object
 */

Color
vdx_parse_color(const char *s, const VDXDocument *theDoc)
{
    int colorvalues;
    Color c = {0, 0, 0};
    if (s[0] == '#')
    {
        sscanf(s, "#%xd", &colorvalues);
        c.red = ((colorvalues & 0x00ff0000)>>16) / 255.0;
        c.green = ((colorvalues & 0x0000ff00)>>8) / 255.0;
        c.blue = (colorvalues & 0x000000ff) / 255.0;
        return c;
    }
    if (g_ascii_isdigit(s[0]))
    {
        /* Look in colour table */
        unsigned int i = atoi(s);
        if (i < theDoc->Colors->len)
            return g_array_index(theDoc->Colors, Color, i);
    }
    g_debug("Couldn't read color: %s", s);
    message_error(_("Couldn't read color: %s\n"), s);
    return c;
}

/** Reads the colour table from the start of a VDX document
 * @param cur the current XML node
 * @param theDoc the current document (with its colour table)
 */

static void 
vdx_get_colors(xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr ColorEntry;
    g_debug("vdx_get_colors");
    theDoc->Colors = g_array_new(FALSE, TRUE, sizeof (Color));

    for (ColorEntry = cur->xmlChildrenNode; ColorEntry; 
         ColorEntry = ColorEntry->next) {
        Color color;
        struct vdx_ColorEntry temp_ColorEntry;

        if (xmlIsBlankNode(ColorEntry)) { continue; }

        vdx_read_object(ColorEntry, theDoc, &temp_ColorEntry);
        /* Just in case Color entries aren't consecutive starting at 0 */
        color = vdx_parse_color(temp_ColorEntry.RGB, theDoc);
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
 */

static void 
vdx_get_facenames(xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr Face = cur->xmlChildrenNode;
    g_debug("vdx_get_facenames");
    theDoc->FaceNames = 
        g_array_new(FALSE, FALSE, sizeof (struct vdx_FaceName));

    for (Face = cur->xmlChildrenNode; Face; Face = Face->next) {
        struct vdx_FaceName FaceName;

        if (xmlIsBlankNode(Face)) { continue; }

        vdx_read_object(Face, theDoc, &FaceName);
        g_debug("FaceName[%d] = %s", FaceName.ID, FaceName.Name);
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
 */

static void 
vdx_get_fonts(xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr Font = cur->xmlChildrenNode;
    g_debug("vdx_get_fonts");
    theDoc->Fonts = g_array_new(FALSE, FALSE, sizeof (struct vdx_FontEntry));

    for (Font = cur->xmlChildrenNode; Font; Font = Font->next) {
        struct vdx_FontEntry FontEntry;

        if (xmlIsBlankNode(Font)) { continue; }

        vdx_read_object(Font, theDoc, &FontEntry);
        g_debug("Font[%d] = %s", FontEntry.ID, FontEntry.Name);
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
 */

static void 
vdx_get_masters(xmlNodePtr cur, VDXDocument* theDoc)
{
    g_debug("vdx_get_masters");
}

/** Reads the stylesheet table from the start of a VDX document
 * @param cur the current XML node
 * @param theDoc the current document
 */

static void 
vdx_get_stylesheets(xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr StyleSheet = cur->xmlChildrenNode;
    g_debug("vdx_get_stylesheets");
    theDoc->StyleSheets = g_array_new (FALSE, TRUE, 
                                       sizeof (struct vdx_StyleSheet));
    for (StyleSheet = cur->xmlChildrenNode; StyleSheet; 
         StyleSheet = StyleSheet->next)
    {
        struct vdx_StyleSheet newSheet;
        
        if (xmlIsBlankNode(StyleSheet)) { continue; }

        vdx_read_object(StyleSheet, theDoc, &newSheet);
        g_debug("Stylesheet[%d] = %s", newSheet.ID, 
                (newSheet.NameU));
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
    struct vdx_any *Any = (struct vdx_any *)p;
    GSList *child;
    g_assert(p);
    for(child = Any->children; child; child = child->next)
    {
        struct vdx_any *Any_child = (struct vdx_any *)child->data;
        if (!child->data) continue;
        /* g_debug("  Has type %s", vdx_Types[(int)Any_child->type]); */
        if (Any_child->type == type) return Any_child;
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
            g_free(list->data);
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
    g_debug("get_style_child(%s,%d)", vdx_Types[type], style);
    while(1)
    {
        g_assert(style < theDoc->StyleSheets->len);
        theSheet = g_array_index(theDoc->StyleSheets, 
                                 struct vdx_StyleSheet, style);
        g_debug(" Looking in sheet %d [%s]", style, theSheet.NameU);
        Any = find_child(type, &theSheet);
        if (Any) return Any;
        /* Terminate on style 0 (default) */
        if (!style) return 0;
        /* Find a parent style to check */
        style = 0;
        if (type == vdx_types_Fill) style = theSheet.FillStyle;
        else if (type == vdx_types_Line) style = theSheet.LineStyle;
        else style = theSheet.TextStyle;
    }

    return 0;
}


/** General absolute length conversion Visio -> Dia
 * @param a a length
 * @param theDoc the document
 * @returns a in Dia space
 */
  
static double da(double a, VDXDocument* theDoc)
{
    return vdx_Point_Scale*a;
}

/** x conversion Visio -> Dia
 * @param x
 * @param theDoc the document
 * @returns x in Dia space
 */
  
static double dx(double x, VDXDocument* theDoc)
{
    return vdx_Point_Scale*x + vdx_Page_Width*theDoc->Page;
}

/** y conversion Visio -> Dia
 * @param y
 * @param theDoc the document
 * @returns y in Dia space
 */
  
static double dy(double y, VDXDocument* theDoc)
{
    return vdx_Y_Offset + vdx_Y_Flip*vdx_Point_Scale*y;
}


/** Sets simple props
 * @param obj the object
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @bug dash length not yet done - much other work needed
 */
  
static void
vdx_simple_properties(DiaObject *obj,
                      const struct vdx_Fill *Fill, const struct vdx_Line *Line,
                      const VDXDocument* theDoc) 
{
    GPtrArray *props = prop_list_from_descs(vdx_simple_prop_descs_line,
                                            pdtpp_true);
    RealProperty *rprop;
    ColorProperty *cprop;

    g_assert(props->len == 2);

    if (Line)
    {
        rprop = g_ptr_array_index(props,0);
        rprop->real_data = Line->LineWeight;
    
        cprop = g_ptr_array_index(props,1);
        cprop->color_data = Line->LineColor;

        if (!Line->LinePattern) 
        { cprop->color_data = vdx_parse_color("#FFFFFF", theDoc); }

        g_debug("line colour %f,%f,%f", Line->LineColor.red,
                Line->LineColor.green, Line->LineColor.blue);

        if (Line->LinePattern) 
        {
            LinestyleProperty *lsprop = 
                (LinestyleProperty *)make_new_prop("line_style", 
                                                   PROP_TYPE_LINESTYLE,
                                                   PROP_FLAG_DONT_SAVE);
            lsprop->style = LINESTYLE_SOLID;

            if (Line->LinePattern > 1)
                lsprop->style = LINESTYLE_DASHED;
            if (Line->LinePattern == 4)
                lsprop->style = LINESTYLE_DASH_DOT;
            if (Line->LinePattern == 3)
                lsprop->style = LINESTYLE_DOTTED;

            lsprop->dash = vdx_Dash_Length;

            g_ptr_array_add(props,lsprop);
        }
    }

    if (Fill && Fill->FillPattern)
    {
        cprop = 
            (ColorProperty *)make_new_prop("fill_colour",
                                           PROP_TYPE_COLOUR,
                                           PROP_FLAG_DONT_SAVE);
        cprop->color_data = Fill->FillForegnd;
        
        g_ptr_array_add(props,cprop);
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


/* The following functions create the Dia standard objects */

/** Plots a line
 * @param Geom the shape
 * @param XForm any transformations
 * @param XForm1D any linear transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 */
  
static DiaObject *
plot_line(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
          const struct vdx_XForm1D *XForm1D, 
          const struct vdx_Fill *Fill, const struct vdx_Line *Line,
          VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;
    GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    struct vdx_any *Any;
    Point points[2];
    Arrow start_arrow;
    Arrow end_arrow;
    Arrow* start_arrow_p = NULL;
    Arrow* end_arrow_p = NULL;

    Point offset = {0, 0};
    Point current = {0, 0};
    Point end = {0, 0};

    if (Geom->NoLine) return 0;

    item = Geom->children;
    Any = (struct vdx_any *)(item->data);
    
    if (Any->type == vdx_types_MoveTo)
    {
        MoveTo = (struct vdx_MoveTo*)(item->data);
        current.x = MoveTo->X;
        current.y = MoveTo->Y;
        item = item->next;
    }
    LineTo = (struct vdx_LineTo*)(item->data);

    end.x = LineTo->X;
    end.y = LineTo->Y;

    if (XForm1D)
    {
        /* As a special case, we can do rotation and scaling on
           lines, as all we need is the end points */
        current.x = XForm1D->BeginX;
        current.y = XForm1D->BeginY;
        end.x = XForm1D->EndX;
        end.y = XForm1D->EndY;
    }

    points[0].x = dx(current.x + offset.x, theDoc);
    points[0].y = dy(current.y + offset.y, theDoc);
    points[1].x = dx(end.x + offset.x, theDoc);
    points[1].y = dy(end.y + offset.y, theDoc);

    if (Line->BeginArrow) 
    {
        start_arrow.type = ARROW_FILLED_TRIANGLE;
        start_arrow.width = da(Line->BeginArrowSize*vdx_Arrow_Scale, theDoc);
        start_arrow.length = da(Line->BeginArrowSize*vdx_Arrow_Scale, theDoc);
        start_arrow_p = &start_arrow;
    }

    if (Line->EndArrow) 
    {
        end_arrow.type = ARROW_FILLED_TRIANGLE;
        end_arrow.width = da(Line->EndArrowSize*vdx_Arrow_Scale, theDoc);
        end_arrow.length = da(Line->EndArrowSize*vdx_Arrow_Scale, theDoc);
        end_arrow_p = &end_arrow;
    }

    /* Yes, I've swapped start and end arrows */
    newobj = create_standard_line(points, end_arrow_p, start_arrow_p);
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Plots an arc
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 */
  
static DiaObject *
plot_arc(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
         const struct vdx_Fill *Fill, const struct vdx_Line *Line,
         VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;
    GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_ArcTo *ArcTo;
    struct vdx_any *Any;
    Point points[2];
    double chord_length_sq;
    double A;
    double R;
    float radius;
    Arrow start_arrow;
    Arrow end_arrow;
    Arrow* start_arrow_p = NULL;
    Arrow* end_arrow_p = NULL;

    Point offset = {0, 0};
    Point current = {0, 0};

    if (Geom->NoLine) return 0;

    if (XForm)
    {
        offset.x = XForm->PinX - XForm->LocPinX;
        offset.y = XForm->PinY - XForm->LocPinY;
    }

    item = Geom->children;
    Any = (struct vdx_any *)(item->data);
    
    if (Any->type == vdx_types_MoveTo)
    {
        MoveTo = (struct vdx_MoveTo*)(item->data);
        current.x = MoveTo->X;
        current.y = MoveTo->Y;
        item = item->next;
    }
    ArcTo = (struct vdx_ArcTo*)(item->data);
    points[0].x = dx(current.x + offset.x, theDoc);
    points[0].y = dy(current.y + offset.y, theDoc);
    points[1].x = dx(ArcTo->X + offset.x, theDoc);
    points[1].y = dy(ArcTo->Y + offset.y, theDoc);

    chord_length_sq = (ArcTo->X - current.x)*(ArcTo->X - current.x) +
        (ArcTo->Y - current.y)*(ArcTo->Y - current.y);

    /* A = The distance from the arc's midpoint to the midpoint of its chord.
       By Pythagoras, R^2 = (chord_length/2)^2 + (R-A)^2 */

    A = ArcTo->A;

    R = (chord_length_sq + 4*A*A)/(8*A);

    radius = da(R, theDoc);

    if (Line->BeginArrow) 
    {
        start_arrow.type = ARROW_FILLED_TRIANGLE;
        start_arrow.width = da(Line->BeginArrowSize*vdx_Arrow_Scale, theDoc);
        start_arrow.length = da(Line->BeginArrowSize*vdx_Arrow_Scale, theDoc);
        start_arrow_p = &start_arrow;
    }

    if (Line->EndArrow) 
    {
        end_arrow.type = ARROW_FILLED_TRIANGLE;
        end_arrow.width = da(Line->EndArrowSize*vdx_Arrow_Scale, theDoc);
        end_arrow.length = da(Line->EndArrowSize*vdx_Arrow_Scale, theDoc);
        end_arrow_p = &end_arrow;
    }

    newobj = create_standard_arc(points[0].x, points[0].y, points[1].x, 
                                 points[1].y, radius, 
                                 start_arrow_p, end_arrow_p);
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Plots a polyline
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 */
  
static DiaObject *
plot_polyline(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
              const struct vdx_Fill *Fill, const struct vdx_Line *Line,
              VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;
    GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    Point *points;
    unsigned int num_points = 0;
    unsigned int count = 0;

    Point offset = {0, 0};

    if (Geom->NoLine) return 0;

    if (XForm)
    {
        offset.x = XForm->PinX - XForm->LocPinX;
        offset.y = XForm->PinY - XForm->LocPinY;
    }

    for(item = Geom->children; item; item = item->next)
    {
        num_points++;
    }

    points = g_new0(Point, num_points);

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        switch (((struct vdx_any *)(item->data))->type)
        {
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            points[count].x = dx(LineTo->X + offset.x, theDoc);
            points[count].y = dy(LineTo->Y + offset.y, theDoc);
            break;
        case vdx_types_MoveTo:
            MoveTo = (struct vdx_MoveTo*)(item->data);
            points[count].x = dx(MoveTo->X + offset.x, theDoc);
            points[count].y = dy(MoveTo->Y + offset.y, theDoc);
            break;
        default:
            break;
        }
        count++;
    }

    newobj = create_standard_polyline(num_points, points, NULL, NULL);
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Plots a polygon
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 */
  
static DiaObject *
plot_polygon(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
              const struct vdx_Fill *Fill, const struct vdx_Line *Line,
              VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;
    GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    Point *points;
    unsigned int num_points = 0;
    unsigned int count = 0;

    Point offset = {0, 0};

    if (XForm)
    {
        offset.x = XForm->PinX - XForm->LocPinX;
        offset.y = XForm->PinY - XForm->LocPinY;
    }

    for(item = Geom->children; item; item = item->next)
    {
        num_points++;
    }

    points = g_new0(Point, num_points);

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        switch (((struct vdx_any *)(item->data))->type)
        {
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            points[count].x = dx(LineTo->X + offset.x, theDoc);
            points[count].y = dy(LineTo->Y + offset.y, theDoc);
            break;
        case vdx_types_MoveTo:
            MoveTo = (struct vdx_MoveTo*)(item->data);
            points[count].x = dx(MoveTo->X + offset.x, theDoc);
            points[count].y = dy(MoveTo->Y + offset.y, theDoc);
            break;
        default:
            break;
        }
        count++;
    }

    newobj = create_standard_polygon(num_points, points);
    g_free(points);
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Plots an ellipse
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 */
  
static DiaObject *
plot_ellipse(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
             const struct vdx_Fill *Fill, const struct vdx_Line *Line,
             VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;
    GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_Ellipse *Ellipse;
    struct vdx_any *Any;

    Point offset = {0, 0};
    Point current = {0, 0};

    if (XForm)
    {
        offset.x = XForm->PinX - XForm->LocPinX;
        offset.y = XForm->PinY - XForm->LocPinY;
    }

    item = Geom->children;
    Any = (struct vdx_any *)(item->data);
    
    if (Any->type == vdx_types_MoveTo)
    {
        MoveTo = (struct vdx_MoveTo*)(item->data);
        current.x = MoveTo->X;
        current.y = MoveTo->Y;
        item = item->next;
    }

    Ellipse = (struct vdx_Ellipse*)(Geom->children->data);

    /* Dia pins its ellipses in the top left corner, so adjust by
       the vertical radius */
    current.y += Ellipse->D;

    newobj = 
        create_standard_ellipse(
            dx(current.x + offset.x, theDoc), 
            dy(current.y + offset.y, theDoc), 
            da(Ellipse->A, theDoc), da(Ellipse->D, theDoc));

    vdx_simple_properties(newobj, Fill, Line, theDoc);

    return newobj;
}

/** Plots a beziergon
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 * @bug deal with moveto not being first component
 */
  
static DiaObject *
plot_beziergon(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
               const struct vdx_Fill *Fill, const struct vdx_Line *Line,
               VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;
    GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    struct vdx_EllipticalArcTo *EllipticalArcTo;
    struct vdx_any *Any;
    unsigned int num_points = 0;
    unsigned int count = 0;
    BezPoint *bezpoints = 0;

    Point offset = {0, 0};
    Point parallel;
    Point normal;

    if (XForm)
    {
        offset.x = XForm->PinX - XForm->LocPinX;
        offset.y = XForm->PinY - XForm->LocPinY;
    }

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        if ((Any->type == vdx_types_MoveTo && num_points) ||
            (Any->type != vdx_types_MoveTo && ! num_points))
        {
            g_debug("MoveTo not in first position!");
            /* return 0; */
        }
        num_points++;
    }

    bezpoints = g_new0(BezPoint, num_points+1);

    /* Dia always has a BEZ_MOVETO in the first slot, and all the rest are
       BEZ_CURVETO or BEZ_LINETO */

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        switch (Any->type)
        {
        case vdx_types_MoveTo:
            MoveTo = (struct vdx_MoveTo*)(item->data);
            bezpoints[count].type = BEZ_MOVE_TO;
            bezpoints[count].p1.x = dx(MoveTo->X + offset.x, theDoc);
            bezpoints[count].p1.y = dy(MoveTo->Y + offset.y, theDoc);
            g_debug("Moveto(%f,%f)", MoveTo->X, MoveTo->Y);
            break;
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            bezpoints[count].type = BEZ_LINE_TO;
            bezpoints[count].p1.x = dx(LineTo->X + offset.x, theDoc);
            bezpoints[count].p1.y = dy(LineTo->Y + offset.y, theDoc);
            g_debug("Lineto(%f,%f)", LineTo->X, LineTo->Y);
            break;
        case vdx_types_EllipticalArcTo:
            EllipticalArcTo = (struct vdx_EllipticalArcTo*)(item->data);
            bezpoints[count].type = BEZ_CURVE_TO;
            bezpoints[count].p3.x = dx(EllipticalArcTo->X + offset.x, theDoc);
            bezpoints[count].p3.y = dy(EllipticalArcTo->Y + offset.y, theDoc);
            bezpoints[count].p2.x = dx(EllipticalArcTo->A + offset.x, theDoc);
            bezpoints[count].p2.y = dy(EllipticalArcTo->B + offset.y, theDoc);
            if (count > 1) bezpoints[count].p1 = bezpoints[count-1].p3;
            if (count == 1) bezpoints[count].p1 = bezpoints[count-1].p1;
            g_debug("Arcto(%f,%f;%f,%f;%f;%f)", 
                    EllipticalArcTo->X, EllipticalArcTo->Y,
                    EllipticalArcTo->A, EllipticalArcTo->B,
                    EllipticalArcTo->C, EllipticalArcTo->D);
            break;
        default:
            g_debug("Not plotting %s as Bezier", 
                    vdx_Types[(unsigned int)Any->type]);
            break;
        }
        count++;
    }
    bezpoints[num_points].type = BEZ_CURVE_TO;
    bezpoints[num_points].p3 = bezpoints[0].p1;
    bezpoints[num_points].p2 = bezpoints[0].p1;
    bezpoints[num_points].p1 = bezpoints[num_points-1].p3;

    newobj = create_vdx_beziergon(num_points+1, bezpoints);
    g_free(bezpoints); 
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Plots a bexier
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 */
  
static DiaObject *
plot_bezier(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
            const struct vdx_Fill *Fill, const struct vdx_Line *Line,
            VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;
    GSList *item;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    struct vdx_EllipticalArcTo *EllipticalArcTo;
    struct vdx_any *Any;
    unsigned int num_points = 0;
    unsigned int count = 0;
    BezPoint *bezpoints = 0;
    Arrow start_arrow;
    Arrow end_arrow;
    Arrow* start_arrow_p = NULL;
    Arrow* end_arrow_p = NULL;

    Point offset = {0, 0};

    if (XForm)
    {
        offset.x = XForm->PinX - XForm->LocPinX;
        offset.y = XForm->PinY - XForm->LocPinY;
    }

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        if ((Any->type == vdx_types_MoveTo && num_points) ||
            (Any->type != vdx_types_MoveTo && ! num_points))
        {
            g_debug("MoveTo not in first position!");
            /* return 0; */
        }
        num_points++;
    }

    bezpoints = g_new0(BezPoint, num_points);

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        switch (Any->type)
        {
        case vdx_types_MoveTo:
            MoveTo = (struct vdx_MoveTo*)(item->data);
            bezpoints[count].type = BEZ_MOVE_TO;
            bezpoints[count].p1.x = dx(MoveTo->X + offset.x, theDoc);
            bezpoints[count].p1.y = dy(MoveTo->Y + offset.y, theDoc);
            g_debug("Moveto(%f,%f)", MoveTo->X, MoveTo->Y);
            break;
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            bezpoints[count].type = BEZ_LINE_TO;
            bezpoints[count].p1.x = dx(LineTo->X + offset.x, theDoc);
            bezpoints[count].p1.y = dy(LineTo->Y + offset.y, theDoc);
            g_debug("Lineto(%f,%f)", LineTo->X, LineTo->Y);
            break;
        case vdx_types_EllipticalArcTo:
            EllipticalArcTo = (struct vdx_EllipticalArcTo*)(item->data);
            bezpoints[count].type = BEZ_CURVE_TO;
            bezpoints[count].p3.x = dx(EllipticalArcTo->X + offset.x, theDoc);
            bezpoints[count].p3.y = dy(EllipticalArcTo->Y + offset.y, theDoc);
            bezpoints[count].p2.x = dx(EllipticalArcTo->A + offset.x, theDoc);
            bezpoints[count].p2.y = dy(EllipticalArcTo->B + offset.y, theDoc);
            if (count > 1) bezpoints[count].p1 = bezpoints[count-1].p3;
            if (count == 1) bezpoints[count].p1 = bezpoints[count-1].p1;
            g_debug("Arcto(%f,%f)", EllipticalArcTo->X, EllipticalArcTo->Y);
            break;
        default:
            g_debug("Not plotting %s as Bezier", 
                    vdx_Types[(unsigned int)Any->type]);
            break;
        }
        count++;
    }

    if (Line->BeginArrow) 
    {
        start_arrow.type = ARROW_FILLED_TRIANGLE;
        start_arrow.width = da(Line->BeginArrowSize*vdx_Arrow_Scale, theDoc);
        start_arrow.length = da(Line->BeginArrowSize*vdx_Arrow_Scale, theDoc);
        start_arrow_p = &start_arrow;
    }

    if (Line->EndArrow) 
    {
        end_arrow.type = ARROW_FILLED_TRIANGLE;
        end_arrow.width = da(Line->EndArrowSize*vdx_Arrow_Scale, theDoc);
        end_arrow.length = da(Line->EndArrowSize*vdx_Arrow_Scale, theDoc);
        end_arrow_p = &end_arrow;
    }

    newobj = create_standard_bezierline(num_points, bezpoints, 
                                        start_arrow_p, end_arrow_p);
    g_free(bezpoints); 
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Plots a shape
 * @param Geom the shape
 * @param XForm any transformations
 * @param Fill any fill
 * @param Line any line
 * @param theDoc the document
 * @returns the new object
 */
  
enum dia_types { vdx_dia_any = 0, vdx_dia_text, vdx_dia_ellipse, vdx_dia_box,
                 vdx_dia_polyline, vdx_dia_polygon, vdx_dia_bezier, 
                 vdx_dia_beziergon, vdx_dia_arc, vdx_dia_line, vdx_dia_image,
                 vdx_dia_zigzagline };

static DiaObject *
plot_geom(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
          const struct vdx_XForm1D *XForm1D, 
          const struct vdx_Fill *Fill, const struct vdx_Line *Line,
          VDXDocument* theDoc)
{
    GSList *item;
    gboolean all_lines = TRUE;
    unsigned int num_steps = 0;
    struct vdx_any *last_point = 0;
    unsigned int dia_type_choice = vdx_dia_any;

    /* Translate from Visio-space to Dia-space before plotting */

    g_debug("plot_geom");

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        last_point = (struct vdx_any *)(item->data);
        switch (((struct vdx_any *)(item->data))->type)
        {
        case vdx_types_LineTo:
            num_steps++;
            break;
        case vdx_types_MoveTo:
            if (item != Geom->children) { all_lines = FALSE; }
            break;
        default:
            all_lines = FALSE;
            num_steps++;
        }
    }

    if (all_lines)
    {
        if (Geom->NoFill) { dia_type_choice = vdx_dia_polyline; }
        else { dia_type_choice = vdx_dia_polygon; }
    }
    if (num_steps == 1) 
    { 
        if (last_point->type == vdx_types_EllipticalArcTo) 
            dia_type_choice = vdx_dia_arc; 
        if (last_point->type == vdx_types_Ellipse) 
            dia_type_choice = vdx_dia_ellipse; 
        if (last_point->type == vdx_types_LineTo) 
            dia_type_choice = vdx_dia_line; 
    }
    if (dia_type_choice == vdx_dia_any)
    {
        if (Geom->NoFill) { dia_type_choice = vdx_dia_bezier; }
        else { dia_type_choice = vdx_dia_beziergon; }
    }

    g_debug("Choice %d", dia_type_choice);

    switch(dia_type_choice)
    {
    case vdx_dia_line:
        return plot_line(Geom, XForm, XForm1D, Fill, Line, theDoc);
        break;
    case vdx_dia_polyline:
        return plot_polyline(Geom, XForm, Fill, Line, theDoc);
        break;
    case vdx_dia_polygon:
        return plot_polygon(Geom, XForm, Fill, Line, theDoc);
        break;
    case vdx_dia_ellipse:
        return plot_ellipse(Geom, XForm, Fill, Line, theDoc); 
        break;
    case vdx_dia_beziergon:
        return plot_beziergon(Geom, XForm, Fill, Line, theDoc);
        break;
    case vdx_dia_bezier:
        return plot_bezier(Geom, XForm, Fill, Line, theDoc);
        break;
    case vdx_dia_arc:
        return plot_arc(Geom, XForm, Fill, Line, theDoc);
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
 * @bug need to assemble multiple child text objects - much work yet to do
 */
static DiaObject *
plot_text(const struct vdx_Text *Text, const struct vdx_XForm *XForm, 
          const struct vdx_Char *Char, const struct vdx_Para *Para,
          VDXDocument* theDoc)
{
    DiaObject *newobj;
    GPtrArray *props;
    TextProperty *tprop;
    struct vdx_FontEntry FontEntry;
    struct vdx_FaceName FaceName;
    struct vdx_text * text = find_child(vdx_types_text, Text);
    int i;

    g_debug("plot_text");
    if (!Char || !text) { g_debug("Not enough info for text"); return 0; }
    newobj = create_standard_text(dx(XForm->PinX, theDoc), 
                                  dy(XForm->PinY, theDoc));
    props = prop_list_from_descs(vdx_text_descs,pdtpp_true);
    tprop = g_ptr_array_index(props,0);
    tprop->text_data = g_strdup(text->text);

    /* Fix Unicode line breaks */
    for (i=0; tprop->text_data[i]; i++)
    {
        if ((unsigned char)tprop->text_data[i] == 226 && 
            (unsigned char)tprop->text_data[i+1] == 128 && 
            (unsigned char)tprop->text_data[i+2] == 168)
        {
            g_debug("Newline fixup");
            tprop->text_data[i] = 10;
            memmove(&tprop->text_data[i+1], &tprop->text_data[i+3],
                    strlen(&tprop->text_data[i+3])+1);
        }
    }
    tprop->attr.alignment = Para->HorzAlign;
    tprop->attr.position.x = dx(0, theDoc);
    tprop->attr.position.y = dy(0, theDoc);
    if (theDoc->Fonts)
    {
        if (Char->Font < theDoc->Fonts->len)
        {
            FontEntry = 
                g_array_index(theDoc->Fonts, struct vdx_FontEntry, Char->Font);
        }
        else { FontEntry.Name = "Helvetica"; }
        tprop->attr.font = dia_font_new_from_legacy_name(FontEntry.Name);
    }
    else if (theDoc->FaceNames)
    {
        if (Char->Font < theDoc->FaceNames->len)
        {
            FaceName = 
                g_array_index(theDoc->FaceNames, 
                              struct vdx_FaceName, Char->Font);
        }
        else { FaceName.Name = "Helvetica"; }
        tprop->attr.font = dia_font_new_from_legacy_name(FaceName.Name);
    }
    else 
    {
        tprop->attr.font = dia_font_new_from_legacy_name("sans");
    }
    tprop->attr.height = Char->Size*vdx_Font_Size_Conversion; 
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
 * @returns list of objects to add
 */

static GSList *
vdx_plot_shape(struct vdx_Shape *Shape, GSList *objects, 
               struct vdx_XForm* group_XForm,
               VDXDocument* theDoc)
{
    struct vdx_Fill *Fill = 0;
    struct vdx_Char *Char = 0;
    struct vdx_Line *Line = 0;
    struct vdx_Geom *Geom = 0;
    struct vdx_XForm *XForm = 0;
    struct vdx_XForm1D *XForm1D = 0;
    struct vdx_Text *Text = 0;
    struct vdx_Para *Para = 0;

    if (Shape->Del) 
    {
        g_debug("Deleted");
        return objects;
    }

    /* Is there a Fill? */
    Fill = (struct vdx_Fill *)find_child(vdx_types_Fill, Shape);
    Line = (struct vdx_Line *)find_child(vdx_types_Line, Shape);
    Char = (struct vdx_Char *)find_child(vdx_types_Char, Shape);
    XForm = (struct vdx_XForm *)find_child(vdx_types_XForm, Shape);
    XForm1D = (struct vdx_XForm1D *)find_child(vdx_types_XForm1D, Shape);
    Geom = (struct vdx_Geom *)find_child(vdx_types_Geom, Shape);
    Text = (struct vdx_Text *)find_child(vdx_types_Text, Shape);
    Para = (struct vdx_Para *)find_child(vdx_types_Para, Shape);

    if (!Fill && (!Geom || !Geom->NoFill))
        Fill = (struct vdx_Fill *)get_style_child(vdx_types_Fill, 
                                                  Shape->FillStyle,
                                                  theDoc);
    if (!Line && (!Geom || !Geom->NoLine))
        Line = (struct vdx_Line *)get_style_child(vdx_types_Line, 
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
    
    /* If we're in a group, apply an overall XForm to everything */
    if (group_XForm)
    {
        if (XForm)
        {
            /* Change for masters? */
            XForm->PinX += group_XForm->PinX - group_XForm->LocPinX;
            XForm->PinY += group_XForm->PinY - group_XForm->LocPinY;
        }
        else
        {
            XForm = group_XForm;
        }
    }

    /* Am I a group? */
    if (!strcmp(Shape->Type, "Group") && find_child(vdx_types_Shapes, Shape))
    {
        GSList *members = NULL;
        GSList *child;
        GList *group = NULL;
        struct vdx_Shapes* Shapes = 
            (struct vdx_Shapes*)find_child(vdx_types_Shapes, Shape);
        
        /* Create a list of member objects */
        for (child = Shapes->children; child; child = child->next)
        {
            struct vdx_Shape * theShape = (struct vdx_Shape*)(child->data);
            if (!theShape) continue;
            if (theShape->type == vdx_types_Shape)
            {
                members = vdx_plot_shape(theShape, members, XForm, theDoc);
            }
        }
        for (child = members; child; child = child->next)
        {
            if (child->data) group = g_list_append(group, child->data);
        }

        objects = g_slist_append(objects, create_standard_group(group));
        /* g_list_free(group); */
        g_slist_free(members);
    }
    else
    {
        if (Geom)
        {
            objects = 
                g_slist_append(objects,
                               plot_geom(Geom, XForm, XForm1D, 
                                         Fill, Line, theDoc));
        }
        /* Text always after the object it's attached to, 
           so it appears on top */
        if (Text && find_child(vdx_types_text, Text))
        {
            objects = 
                g_slist_append(objects,
                               plot_text(Text, XForm, Char, Para, theDoc));
        }    
    }
    return objects;
}

/** Parses a shape and adds it to the diagram
 * @param Shape the XML to parse
 * @param PageSheet the page property sheet
 * @param theDoc the document
 * @param dia the growing diagram
 */

static void
vdx_parse_shape(xmlNodePtr Shape, struct vdx_PageSheet *PageSheet, 
                VDXDocument* theDoc, DiagramData *dia)
{
    /* All decoding is done in Visio-space */
    struct vdx_Shape theShape = {0, 0 };
    GSList *objects = NULL;
    GSList *object;
    struct vdx_LayerMem *LayerMem = NULL;
    Layer *diaLayer = NULL;

    g_debug("vdx_parse_shape");

    if (PageSheet)
    {
        /* Default styles */
        theShape.TextStyle = PageSheet->TextStyle;
        theShape.FillStyle = PageSheet->FillStyle;
        theShape.LineStyle = PageSheet->LineStyle;
    }
    vdx_read_object(Shape, theDoc, &theShape);

    /* Avoid very bad shapes */
    if (!theShape.Type) return;

    g_debug("Shape %d [%s]", theShape.ID, theShape.NameU);
    /* Ignore Guide */
    /* For debugging purposes, use Del attribute and stop flag */
    if (!strcmp(theShape.Type, "Guide") || theShape.Del || theDoc->stop) 
    {
        g_debug("Ignoring Type = %s", theShape.Type);
        free_children(&theShape);
        return;
    }

    /* Remove NULLs */
    theShape.children = g_slist_remove_all(theShape.children, 0);

    /* What layer are we on, if any? */
    LayerMem = find_child(vdx_types_LayerMem, &theShape);
    if (LayerMem)
    {
        unsigned int layer_num = (unsigned int)atoi(LayerMem->LayerMember);
        if (layer_num < dia->layers->len)
        {
            diaLayer = (Layer *)g_ptr_array_index(dia->layers, layer_num);
        }
    }
    if (!diaLayer) diaLayer = 
                       (Layer *)g_ptr_array_index(dia->layers, 
                                                  dia->layers->len - 1);

    /* Draw the shape (or group) and get list of created objects */
    objects = vdx_plot_shape(&theShape, objects, 0, theDoc);

    /* Add the objects straight into the diagram */
    for (object = objects; object; object = object->next)
    {
        if (!object->data) continue;
        layer_add_object(diaLayer, (DiaObject *)object->data);
    }

    free_children(&theShape);
    g_slist_free(objects);
}

/** Parse the pages of the VDX
 * @param PageSheet the PageSheet
 * @param theDoc the document
 * @param dia the growing diagram
 * @bug This doesn't handle multi-page diagrams very well
 */

static void
vdx_setup_layers(struct vdx_PageSheet* PageSheet, VDXDocument* theDoc,
                 DiagramData *dia)
{
    GSList *child = NULL;
    GSList *layernames = NULL;
    GSList *layername = NULL;
    struct vdx_any* Any;
    struct vdx_Layer *theLayer;
    Layer *diaLayer;
    g_debug("vdx_setup_layers");
    
    /* We need the layers in reverse order */

    for (child = PageSheet->children; child; child = child->next)
    {
        if (!child->data) continue;
        Any = (struct vdx_any *)(child->data);
        if (Any->type != vdx_types_Layer) continue;
        theLayer = (struct vdx_Layer *)child->data;
        layernames = g_slist_prepend(layernames, theLayer->Name);
    }

    for(layername = layernames; layername; layername = layername->next)
    {
        g_debug("Layer %s", (char *)layername->data);
        diaLayer = new_layer(g_strdup((char*)layername->data), dia);
	data_add_layer(dia, diaLayer);
    }
    data_set_active_layer(dia, diaLayer);
}

/** Parse the pages of the VDX
 * @param cur current XML node
 * @param theDoc the document
 * @param dia the growing diagram
 */

static void 
vdx_get_pages(xmlNodePtr cur, VDXDocument* theDoc, DiagramData *dia)
{
    xmlNodePtr Page = cur->xmlChildrenNode;
    xmlNodePtr Shapes;
    g_debug("vdx_get_pages");

    for (Page = cur->xmlChildrenNode; Page; Page = Page->next)
    {
        struct vdx_PageSheet PageSheet;
        if (xmlIsBlankNode(Page)) { continue; }

        for (Shapes = Page->xmlChildrenNode; Shapes; Shapes = Shapes->next)
        {
            xmlNodePtr Shape;
            if (xmlIsBlankNode(Shapes)) { continue; }
            g_debug("Page element: %s", Shapes->name);
            if (!strcmp((char *)Shapes->name, "PageSheet")) {
                vdx_read_object(Shapes, theDoc, &PageSheet);
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
                vdx_parse_shape(Shape, &PageSheet, theDoc, dia);
            }
        }
        theDoc->Page++;
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

    g_debug("vdx_free");
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
    /* Masters */
}


/** Imports the given file
 * @param filename the file to read
 * @param dia the diagram
 * @param user_data unused
 * @returns TRUE if successful, FALSE otherwise
 */

gboolean
import_vdx(const gchar *filename, DiagramData *dia, void* user_data) 
{
    xmlDocPtr doc = xmlParseFile(filename);
    xmlNodePtr root, cur;
    struct VDXDocument *theDoc;
    const char *s;
    int visio_version = 0;

    if (!doc) {
        message_warning("parse error for %s", 
                        dia_message_filename(filename));
        return FALSE;
    }
    /* skip comments */
    for (root = doc->xmlRootNode; root; root = root->next)
    {
        if (root->type == XML_ELEMENT_NODE) { break; }
        g_debug("Skipping node '%s'", root->name);
    }
    if (!root || xmlIsBlankNode(root))
    {
        g_warning("Nothing in document!");
        return FALSE;
    }
    if (strcmp((char *)root->name, "VisioDocument"))
    {
        g_warning("%s not VisioDocument", root->name);
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
    g_debug("Visio version = %d", visio_version);

    theDoc = g_new0(struct VDXDocument, 1);
    theDoc->ok = TRUE;
    
    for (cur = root->xmlChildrenNode; cur; cur = cur->next)
    {
        if (xmlIsBlankNode(cur)) { continue; }
        s = (const char *)cur->name;
        g_debug("Top level node '%s'", s);
        if (!strcmp(s, "Colors")) vdx_get_colors(cur, theDoc);
        if (!strcmp(s, "FaceNames")) vdx_get_facenames(cur, theDoc);
        if (!strcmp(s, "Fonts")) vdx_get_fonts(cur, theDoc);
        if (!strcmp(s, "Masters")) vdx_get_masters(cur, theDoc);
        if (!strcmp(s, "StyleSheets")) vdx_get_stylesheets(cur, theDoc);
        if (!strcmp(s, "Pages")) vdx_get_pages(cur, theDoc, dia);
        /* if (!theDoc->ok) break; */
    }

    /* Get rid of internal strings before returning */
    vdx_free(theDoc);
    xmlFreeDoc(doc);
    return TRUE;
}

/* interface from filter.h */

static const gchar *extensions[] = {"vdx", NULL };
DiaImportFilter vdx_import_filter = {
    N_("Visio XML File Format"),
    extensions,
    import_vdx
};
