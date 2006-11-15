/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
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
#include <glib/gstdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>
#include <sys/stat.h>

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
#include "connection.h"

gboolean import_vdx(const gchar *filename, DiagramData *dia, void* user_data);

void static vdx_get_colors(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_facenames(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_fonts(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_masters(xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_stylesheets(xmlNodePtr cur, VDXDocument* theDoc);
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

/* This is a modified create_standard_beziergon. */

/** Creates a beziergon with cusp corners
 * @param num_points number of points
 * @param points array of points
 * @returns A Standard - Beziergon object
 */

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

    /* Convert all points to cusps - not in API */

    bcp = (BezierConn *)new_obj;
    for (i=0; i<bcp->numpoints; i++) 
    { 
        bcp->corner_types[i] = BEZ_CORNER_CUSP;
    }
    
    return new_obj;
}

/* These are for later */

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

/* End of code taken from xfig-import.c */


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
    theDoc->FaceNames = 
        g_array_new(FALSE, FALSE, sizeof (struct vdx_FaceName));

    for (Face = cur->xmlChildrenNode; Face; Face = Face->next) {
        struct vdx_FaceName FaceName;

        if (xmlIsBlankNode(Face)) { continue; }

        vdx_read_object(Face, theDoc, &FaceName);
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
    theDoc->Fonts = g_array_new(FALSE, FALSE, sizeof (struct vdx_FontEntry));

    for (Font = cur->xmlChildrenNode; Font; Font = Font->next) {
        struct vdx_FontEntry FontEntry;

        if (xmlIsBlankNode(Font)) { continue; }

        vdx_read_object(Font, theDoc, &FontEntry);
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
    xmlNodePtr Master = cur->xmlChildrenNode;
    theDoc->Masters = g_array_new (FALSE, TRUE, 
                                   sizeof (struct vdx_Master));
    for (Master = cur->xmlChildrenNode; Master; 
         Master = Master->next)
    {
        struct vdx_Master newMaster;
        
        if (xmlIsBlankNode(Master)) { continue; }

        vdx_read_object(Master, theDoc, &newMaster);
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
 */

static void 
vdx_get_stylesheets(xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr StyleSheet = cur->xmlChildrenNode;
    theDoc->StyleSheets = g_array_new (FALSE, TRUE, 
                                       sizeof (struct vdx_StyleSheet));
    for (StyleSheet = cur->xmlChildrenNode; StyleSheet; 
         StyleSheet = StyleSheet->next)
    {
        struct vdx_StyleSheet newSheet;
        
        if (xmlIsBlankNode(StyleSheet)) { continue; }

        vdx_read_object(StyleSheet, theDoc, &newSheet);
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
    g_assert(p);
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
    while(1)
    {
        g_assert(style < theDoc->StyleSheets->len);
        theSheet = g_array_index(theDoc->StyleSheets, 
                                 struct vdx_StyleSheet, style);
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

/** Finds a shape in any object including its grouped subshapes
 * @param id shape id (0 for first, as no shape has id 0)
 * @param Shapes shape list
 * @returns The Shape or NULL
 */
static struct vdx_Shape *
get_shape_by_id(unsigned int id, struct vdx_Shapes *Shapes)
{
    struct vdx_Shape *Shape;
    struct vdx_Shapes *SubShapes;
    GSList *child;
    g_assert(Shapes);

    /* A Master has a list of Shapes */
    for(child = Shapes->children; child; child = child->next)
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
                Shape = get_shape_by_id(id, SubShapes);
                if (Shape) return Shape;
            }
        }
    }
    message_error(_("Couldn't find shape %d\n"), id);
    return 0;    
}

/** Finds the master style object that applies
 * @param type a type code
 * @param master the master number
 * @param shape the mastershape number
 * @param theDoc the document
 * @returns The master's shape child
 */
static struct vdx_Shape *
get_master_shape(unsigned int master, unsigned int shape, VDXDocument* theDoc)
{
    struct vdx_Master theMaster;
    struct vdx_Shapes *Shapes;

    g_assert(master < theDoc->Masters->len);
    theMaster = g_array_index(theDoc->Masters, 
                              struct vdx_Master, master);

    Shapes = find_child(vdx_types_Shapes, &theMaster);
    if (!Shapes) return NULL;

    return get_shape_by_id(shape, Shapes);
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
        rprop->real_data = Line->LineWeight * vdx_Line_Scale;
    
        cprop = g_ptr_array_index(props,1);
        cprop->color_data = Line->LineColor;

        if (!Line->LinePattern) 
        { cprop->color_data = vdx_parse_color("#FFFFFF", theDoc); }

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

        /* Dia can't do fill patterns, so we have to choose either the
           foreground or background colour.
           I've chosen the background colour for all patterns except solid */

        if (Fill->FillPattern == 1)
            cprop->color_data = Fill->FillForegnd;
        else
            cprop->color_data = Fill->FillBkgnd;
        
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
    q.x -= XForm->LocPinX;
    q.y -= XForm->LocPinY;

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

    /* Then the flips */
    if (XForm->FlipX) { q.x = - q.x; }
    if (XForm->FlipY) { q.y = - q.y; }

    /* Now add the offset of the rotation pin from the page */
    q.x += XForm->PinX;
    q.y += XForm->PinY;

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

    a->type = ARROW_FILLED_TRIANGLE;
    
    if (start_end == 's') { fixed_size = Line->BeginArrowSize; }
    else { fixed_size = Line->EndArrowSize; }

    if (fixed_size > 6) { fixed_size = 0; }
    size = vdx_Arrow_Sizes[fixed_size];

    a->width = dia_length(size*vdx_Arrow_Scale, theDoc);
    a->length = dia_length(size*vdx_Arrow_Scale, theDoc);

    return a;
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
    Arrow* start_arrow_p = NULL;
    Arrow* end_arrow_p = NULL;

    Point current = {0, 0};
    Point end = {0, 0};

    if (Geom->NoLine) return 0;

    item = Geom->children;
    Any = (struct vdx_any *)(item->data);
    
    if (XForm1D)
    {
        /* As a special case, we can do rotation and scaling on
           lines, as all we need is the end points */
        current.x = XForm1D->BeginX;
        current.y = XForm1D->BeginY;
        end.x = XForm1D->EndX;
        end.y = XForm1D->EndY;

        /* This is wrong if the object contains curves! */
    }
    else
    {
        if (Any->type == vdx_types_MoveTo)
        {
            MoveTo = (struct vdx_MoveTo*)(item->data);
            current.x = MoveTo->X;
            current.y = MoveTo->Y;
            item = item->next;
            Any = (struct vdx_any *)(item->data);
        }
        if (Any->type == vdx_types_LineTo)
        {
            LineTo = (struct vdx_LineTo*)(item->data);
            
            end.x = LineTo->X;
            end.y = LineTo->Y;
        }
        else
        {
            message_error(_("Unexpected LineTo object: %s\n"), 
                          vdx_Types[(unsigned int)Any->type]);
            return NULL;
        }
        if (item->next)
        {
            message_error(_("Unexpected LineTo additional objects\n"));
        }
        current = apply_XForm(current, XForm);
        end = apply_XForm(end, XForm);
    }

    points[0] = dia_point(current, theDoc);
    points[1] = dia_point(end, theDoc);

    if (Line->BeginArrow) 
    {
        start_arrow_p = make_arrow(Line, 's', theDoc);
    }

    if (Line->EndArrow) 
    {
        end_arrow_p = make_arrow(Line, 'e', theDoc);
    }

    newobj = create_standard_line(points, start_arrow_p, end_arrow_p);
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
    struct vdx_any *Any;
    Point *points, p;
    unsigned int num_points = 0;
    unsigned int count = 0;

    if (Geom->NoLine) return 0;

    for(item = Geom->children; item; item = item->next)
    {
        num_points++;
    }

    points = g_new0(Point, num_points);

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        switch (Any->type)
        {
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            p.x = LineTo->X; p.y = LineTo->Y;
            break;
        case vdx_types_MoveTo:
            if (count) 
            {         
                message_error(_("MoveTo after start of polyline\n"));
            }
            MoveTo = (struct vdx_MoveTo*)(item->data);
            p.x = MoveTo->X; p.y = MoveTo->Y;
            break;
        default:
            message_error(_("Unexpected polyline object: %s\n"), 
                          vdx_Types[(unsigned int)Any->type]);
            p.x = 0; p.y = 0;
            break;
        }
        points[count++] = dia_point(apply_XForm(p, XForm), theDoc);
    }

    /* FIXME Arrows */

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
    struct vdx_any *Any;
    Point *points, p;
    unsigned int num_points = 0;
    unsigned int count = 0;

    for(item = Geom->children; item; item = item->next)
    {
        num_points++;
    }

    points = g_new0(Point, num_points);

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        switch (Any->type)
        {
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            p.x = LineTo->X; p.y = LineTo->Y;
            break;
        case vdx_types_MoveTo:
            if (count) 
            {         
                message_error(_("MoveTo after start of polygon\n"));
            }
            MoveTo = (struct vdx_MoveTo*)(item->data);
            p.x = MoveTo->X; p.y = MoveTo->Y;
            break;
        default:
            message_error(_("Unexpected polygon object: %s\n"), 
                          vdx_Types[(unsigned int)Any->type]);
            p.x = 0; p.y = 0;
            break;
        }
        points[count++] = dia_point(apply_XForm(p, XForm), theDoc);
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

    Point current = {0, 0};
    Point p;

    item = Geom->children;
    Any = (struct vdx_any *)(item->data);
    
    if (Any->type == vdx_types_MoveTo)
    {
        MoveTo = (struct vdx_MoveTo*)(item->data);
        current.x = MoveTo->X;
        current.y = MoveTo->Y;
        item = item->next;
        Any = (struct vdx_any *)(item->data);
    }

    if (Any->type == vdx_types_Ellipse)
    {
        Ellipse = (struct vdx_Ellipse*)(item->data);
    }
    else
    {
        message_error(_("Unexpected Ellipse object: %s\n"), 
                      vdx_Types[(unsigned int)Any->type]);
        return NULL;
    }
    if (item->next)
    {
        message_error(_("Unexpected Ellipse additional objects\n"));
    }

    /* Dia pins its ellipses in the top left corner, but Visio uses the centre,
       so adjust by the vertical radius */
    current.y += Ellipse->D;

    p = dia_point(apply_XForm(current, XForm), theDoc);
    if (fabs(XForm->Angle > EPSILON))
	message_error(_("Can't rotate ellipse\n"));

    newobj = 
        create_standard_ellipse(p.x, p.y, dia_length(Ellipse->A, theDoc), 
                                dia_length(Ellipse->D, theDoc));

    vdx_simple_properties(newobj, Fill, Line, theDoc);

    return newobj;
}

/** Converts an arc to Bezier points
 * @param p0 start point
 * @param p3 end point
 * @param p4 arc control point
 * @param C angle of the arc's major axis relative to the x-axis of its parent
 * @param D ratio of an arc's major axis to its minor axis
 * @param p1 first Bezier control point
 * @param p2 second Bezier control point
 */
static gboolean
arc_to_bezier(Point p0, Point p3, Point p4, double C, double D,
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
    if (fabs(g) < EPSILON) { g_debug("g=%f too small", g); return FALSE; }
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

    Point p;
    Point p0 = {0, 0};
    Point p1 = p0, p2 = p0, p3 = p0, p4 = p0;

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        if ((Any->type == vdx_types_MoveTo && num_points) ||
            (Any->type != vdx_types_MoveTo && ! num_points))
        {
            message_error(_("MoveTo not at start of Bezier\n"));
            /* return 0; */
        }
        num_points++;
    }

    bezpoints = g_new0(BezPoint, num_points);

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
            p.x = MoveTo->X; p.y = MoveTo->Y;
            bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
            p0 = p;
            break;
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            bezpoints[count].type = BEZ_LINE_TO;
            p.x = LineTo->X; p.y = LineTo->Y;
            bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
            p0 = p;
            break;
        case vdx_types_EllipticalArcTo:
            EllipticalArcTo = (struct vdx_EllipticalArcTo*)(item->data);
            p3.x = EllipticalArcTo->X; p3.y = EllipticalArcTo->Y;
            p4.x = EllipticalArcTo->A; p4.y = EllipticalArcTo->B;
            arc_to_bezier(p0, p3, p4, EllipticalArcTo->C, 
                          EllipticalArcTo->D, &p1, &p2);
            bezpoints[count].type = BEZ_CURVE_TO;
            bezpoints[count].p3 = dia_point(apply_XForm(p3, XForm), theDoc);
            bezpoints[count].p2 = dia_point(apply_XForm(p2, XForm), theDoc);
            bezpoints[count].p1 = dia_point(apply_XForm(p1, XForm), theDoc);
            p0 = p3;
            break;
        default:
            message_error(_("Unexpected Bezier object: %s\n"), 
                          vdx_Types[(unsigned int)Any->type]);
            break;
        }
        count++;
    }

    newobj = create_vdx_beziergon(num_points, bezpoints);
    g_free(bezpoints); 
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Plots a bezier
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
    Arrow* start_arrow_p = NULL;
    Arrow* end_arrow_p = NULL;

    Point p;
    Point p0 = {0, 0};
    Point p1 = p0, p2 = p0, p3 = p0, p4 = p0;

    for(item = Geom->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        if ((Any->type == vdx_types_MoveTo && num_points) ||
            (Any->type != vdx_types_MoveTo && ! num_points))
        {
            message_error(_("MoveTo not at start of Bezier\n"));
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
            p.x = MoveTo->X; p.y = MoveTo->Y;
            bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
            p0 = p;
            break;
        case vdx_types_LineTo:
            LineTo = (struct vdx_LineTo*)(item->data);
            bezpoints[count].type = BEZ_LINE_TO;
            p.x = LineTo->X; p.y = LineTo->Y;
            bezpoints[count].p1 = dia_point(apply_XForm(p, XForm), theDoc);
            p0 = p;
            break;
        case vdx_types_EllipticalArcTo:
            EllipticalArcTo = (struct vdx_EllipticalArcTo*)(item->data);
            p3.x = EllipticalArcTo->X; p3.y = EllipticalArcTo->Y;
            p4.x = EllipticalArcTo->A; p4.y = EllipticalArcTo->B;
            arc_to_bezier(p0, p3, p4, EllipticalArcTo->C, 
                          EllipticalArcTo->D, &p1, &p2);
            bezpoints[count].type = BEZ_CURVE_TO;
            bezpoints[count].p3 = dia_point(apply_XForm(p3, XForm), theDoc);
            bezpoints[count].p2 = dia_point(apply_XForm(p2, XForm), theDoc);
            bezpoints[count].p1 = dia_point(apply_XForm(p1, XForm), theDoc);
            p0 = p3;
            break;
        default:
            message_error(_("Unexpected Beziergon object: %s\n"), 
                          vdx_Types[(unsigned int)Any->type]);
            break;
        }
        count++;
    }

    if (Line->BeginArrow) 
    {
        start_arrow_p = make_arrow(Line, 's', theDoc);
    }

    if (Line->EndArrow) 
    {
        end_arrow_p = make_arrow(Line, 'e', theDoc);
    }

    newobj = create_standard_bezierline(num_points, bezpoints, 
                                        start_arrow_p, end_arrow_p);
    g_free(bezpoints); 
    vdx_simple_properties(newobj, Fill, Line, theDoc);
    return newobj;
}

/** Converts Base64 data to binary and writes to file
 * @param filename file to write
 * @param b64 Base64 encoded data
 * @note glibc 2.12 offers g_base64_decode()
 */

static void
write_base64_file(const char *filename, const char *b64)
{
    FILE *f;
    const char *c;
    char d = 0;
    char buf[4];                /* For 4 decoded 6-bit chunks */
    unsigned int buf_len = 0;

    f = g_fopen(filename, "w+b");
    if (!f)
    {
        message_error(_("Couldn't write file %s"), filename); 
        return;
    }

    for (c = b64; *c; c++)
    {
        /* Ignore whitespace, = padding at end etc. */
        if (!isalnum(*c) && *c != '+' && *c != '/') continue;

        if (*c >= 'A' && *c <= 'Z') { d = *c - 'A'; }
        if (*c >= 'a' && *c <= 'z') { d = *c - 'a' + 26; }
        if (*c >= '0' && *c <= '9') { d = *c - '0' + 52; }
        if (*c == '+') { d = 62; }
        if (*c == '/') { d = 63; }

        buf[buf_len++] = d;
        if (buf_len == 4)
        {
            /* We now have 3 bytes in 4 6-bit chunks */
            fputc(buf[0] << 2 | buf[1] >> 4, f);
            fputc(buf[1] << 4 | buf[2] >> 2, f);
            fputc(buf[2] << 6 | buf[3], f);
            buf_len = 0;
        }
    }

    /* Deal with any chunks left over */
    if (buf_len)
    {
        fputc(buf[0] << 2 | buf[1] >> 4, f);
        if (buf_len > 1)
        {
            fputc(buf[1] << 4 | buf[2] >> 2, f);
            if (buf_len > 2)
            {
                /* This one can't happen */
                fputc(buf[2] << 6 | buf[3], f);
            }
        }
    }

    fclose(f);
}

/** Plots a bitmap
 * @param Geom the shape
 * @param XForm any transformations
 * @param Foreign the object location
 * @param ForeignData the object
 * @param theDoc the document
 * @returns the new object
 */
  
static DiaObject *
plot_image(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
           const struct vdx_Foreign *Foreign, 
           const struct vdx_ForeignData *ForeignData,
           VDXDocument* theDoc)
{
    DiaObject *newobj = NULL;

    Point p;
    float h, w;
    static char *image_dir = 0;

    GSList *item;
    struct vdx_any *Any;
    struct vdx_text *text;
    const char *base64_data = 0;
    static unsigned int file_counter = 0;
    char suffix[5];
    int i;
    char *filename;

    /* We can only take a few formats */
    if (!ForeignData->CompressionType) return 0;
    if (strcmp(ForeignData->CompressionType, "GIF") &&
        strcmp(ForeignData->CompressionType, "JPEG") &&
        strcmp(ForeignData->CompressionType, "PNG") &&
        strcmp(ForeignData->CompressionType, "TIFF"))
    {
        message_error(_("Couldn't handle foreign object type %s"), 
                      ForeignData->CompressionType);
        return 0;
    }

    /* Create the filename for the embedded object */

    file_counter++;
    strcpy(suffix, ForeignData->CompressionType);
    for (i=0; suffix[i]; i++)
    {
        suffix[i] = tolower(suffix[i]);
    }

    if (!image_dir)
    {
        /* Security: don't trust tempnam to be unique, but use it as a 
           directory name. If the mkdir succeeds, we can't be subject
           to a symlink attack (assuming /tmp is sticky) */
        /* Functional: Dia includes bitmaps by reference, and we're
           putting these bitmaps in a temporary location, so they'll be lost
           on reboot. We could write them to the directory the file came
           from or the current directory - both are problematic */
        image_dir = (char *)tempnam(0, "dia");
        if (!image_dir) return 0;
        if (g_mkdir(image_dir, 0700))
        {
            message_error(_("Couldn't make object dir %s"), image_dir);
            return 0;
        }
    }
    filename = g_new(char, strlen(image_dir) + strlen(suffix) + 10);
    sprintf(filename, "%s/%d.%s", image_dir, file_counter, suffix);
    g_debug("Writing file %s", filename);

    /* Find the data in Base64 encoding in the body of ForeignData */
    for (item = ForeignData->children; item; item = item->next)
    {
        if (!item->data) continue;
        Any = (struct vdx_any *)(item->data);
        if (Any->type == vdx_types_text)
        {
            text = (struct vdx_text *)(item->data);
            base64_data = text->text;
        }
    }

    write_base64_file(filename, base64_data);

    /* Positioning data */
    p.x = Foreign->ImgOffsetX;
    p.y = Foreign->ImgOffsetY;
    p = dia_point(apply_XForm(p, XForm), theDoc);
    h = dia_length(Foreign->ImgHeight, theDoc);
    w = dia_length(Foreign->ImgWidth, theDoc);

    /* Visio supplies bottom left, but Dia needs top left */
    p.y -= h;

    newobj = create_standard_image(p.x, p.y, w, h, filename);

    g_free(filename);
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
                 vdx_dia_zigzagline };

static DiaObject *
plot_geom(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
          const struct vdx_XForm1D *XForm1D, 
          const struct vdx_Fill *Fill, const struct vdx_Line *Line,
          const struct vdx_Foreign *Foreign, 
          const struct vdx_ForeignData *ForeignData,
          VDXDocument* theDoc)
{
    GSList *item;
    gboolean all_lines = TRUE;  /* Flag for line/polyline */
    unsigned int num_steps = 0; /* Flag for poly */
    struct vdx_any *last_point = 0;
    unsigned int dia_type_choice = vdx_dia_any;

    /* Determine what kind of Dia object we need */

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
            /* First step can be a MoveTo, but not others */
            if (item != Geom->children) { all_lines = FALSE; }
            break;
        default:
            all_lines = FALSE;
            num_steps++;
        }
    }

    if (all_lines)
    {
        /* Fill determines if we're -line or -gon */
        if (Geom->NoFill) { dia_type_choice = vdx_dia_polyline; }
        else { dia_type_choice = vdx_dia_polygon; }
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
        if (XForm1D) { dia_type_choice = vdx_dia_line; }
    }
    if (ForeignData) { dia_type_choice = vdx_dia_image; }
    if (dia_type_choice == vdx_dia_any)
    {
        /* Still undecided? Then it's a Bezier(-gon) */
        if (Geom->NoFill) { dia_type_choice = vdx_dia_bezier; }
        else { dia_type_choice = vdx_dia_beziergon; }
    }

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
    case vdx_dia_image:
        return plot_image(Geom, XForm, Foreign, ForeignData, theDoc);
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
    Point p;
    int i;

    if (!Char || !text) { g_debug("Not enough info for text"); return 0; }
    p.x = 0; p.y = 0;
    p = dia_point(apply_XForm(p, XForm), theDoc);
    newobj = create_standard_text(p.x, p.y);
    props = prop_list_from_descs(vdx_text_descs,pdtpp_true);
    tprop = g_ptr_array_index(props,0);
    tprop->text_data = g_strdup(text->text);
    while((text = find_child_next(vdx_types_text, Text, text)))
    {
        char *s = tprop->text_data;
        tprop->text_data = g_strconcat(tprop->text_data, text->text, NULL);
        g_free(s);
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
    tprop->attr.alignment = 
        (Para->HorzAlign < 3) ? Para->HorzAlign : ALIGN_LEFT;
    tprop->attr.position.x = p.x;
    tprop->attr.position.y = p.y;
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
    g_debug("Text: %s at %f,%f", tprop->text_data, p.x, p.y);
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
    struct vdx_Foreign * Foreign = 0;
    struct vdx_ForeignData * ForeignData = 0;

    if (Shape->Del) 
    {
        return objects;
    }

    /* Is there a local definition? */
    Fill = (struct vdx_Fill *)find_child(vdx_types_Fill, Shape);
    Line = (struct vdx_Line *)find_child(vdx_types_Line, Shape);
    Char = (struct vdx_Char *)find_child(vdx_types_Char, Shape);
    XForm = (struct vdx_XForm *)find_child(vdx_types_XForm, Shape);
    XForm1D = (struct vdx_XForm1D *)find_child(vdx_types_XForm1D, Shape);
    Geom = (struct vdx_Geom *)find_child(vdx_types_Geom, Shape);
    Text = (struct vdx_Text *)find_child(vdx_types_Text, Shape);
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
                                           theDoc);
        }
        else
        {
            /* Get the first shape and use that */
            MasterShape = get_master_shape(Shape->Master, 0, theDoc);
        }

        if (MasterShape)
        {
            if (!Fill && (!Geom || !Geom->NoFill))
                Fill = (struct vdx_Fill *)find_child(vdx_types_Fill, 
                                                     MasterShape);
            
            if (!Line && (!Geom || !Geom->NoLine))
                Line = (struct vdx_Line *)find_child(vdx_types_Line, 
                                                     MasterShape);
            if (!Char)
                Char = (struct vdx_Char *)find_child(vdx_types_Char,
                                                     MasterShape);
        }
    }

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
                if (!theShape->Master) 
                { 
                    theShape->Master = Shape->Master; 
                    theShape->Master_exists = Shape->Master_exists;
                }
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
        while (Geom)
        {
            objects = 
                g_slist_append(objects,
                               plot_geom(Geom, XForm, XForm1D, Fill, 
                                         Line, Foreign, ForeignData, theDoc));
            /* Yes, you can have multiple (disconnected) Geoms */
            Geom = find_child_next(vdx_types_Geom, Shape, Geom);
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
    char *name = NULL;

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

    /* Name of shape could be in Unicode, or not, or missing */
    name = theShape.NameU;
    if (!name) name = theShape.Name;
    if (!name) name = "Unnamed";
    g_debug("Shape %d [%s]", theShape.ID, name);

    /* Ignore Guide */
    /* For debugging purposes, use Del attribute and stop flag */
    if (!strcmp(theShape.Type, "Guide") || theShape.Del || theDoc->stop) 
    {
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
        layer_num += theDoc->Background_Layers;
        if (layer_num < dia->layers->len)
        {
            diaLayer = (Layer *)g_ptr_array_index(dia->layers, layer_num);
        }
    }
    if (!diaLayer) diaLayer = 
                       (Layer *)g_ptr_array_index(dia->layers, 0);

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
    Layer *diaLayer = 0;
    
    /* We need the layers in reverse order */

    for (child = PageSheet->children; child; child = child->next)
    {
        if (!child || !child->data) continue;
        Any = (struct vdx_any *)(child->data);
        if (Any->type != vdx_types_Layer) continue;
        theLayer = (struct vdx_Layer *)child->data;
        layernames = g_slist_prepend(layernames, theLayer->Name);
    }

    for(layername = layernames; layername; layername = layername->next)
    {
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
        for (attr = Page->properties; attr; attr = attr->next) 
        {
            if (!strcmp((char *)attr->name, "Background"))
            {
                background = TRUE;
                theDoc->Background_Layers = dia->layers->len;
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
