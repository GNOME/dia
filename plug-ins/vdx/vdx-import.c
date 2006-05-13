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

gboolean import_vdx(const gchar *filename, DiagramData *dia, void* user_data);

void static vdx_get_colors(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_facenames(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_fonts(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_masters(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_get_stylesheets(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc);
void static vdx_free(VDXDocument *theDoc);


/* Stolen from xfig-import.c */


static DiaObject *
create_standard_text(real xpos, real ypos,
		     DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Text");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    
    return new_obj;
}

static PropDescription xfig_element_prop_descs[] = {
    { "elem_corner", PROP_TYPE_POINT },
    { "elem_width", PROP_TYPE_REAL },
    { "elem_height", PROP_TYPE_REAL },
    PROP_DESC_END};

static GPtrArray *make_element_props(real xpos, real ypos,
                                     real width, real height) 
{
    GPtrArray *props;
    PointProperty *pprop;
    RealProperty *rprop;

    props = prop_list_from_descs(xfig_element_prop_descs,pdtpp_true);
    g_assert(props->len == 3);
    
    pprop = g_ptr_array_index(props,0);
    pprop->point_data.x = xpos;
    pprop->point_data.y = ypos;
    rprop = g_ptr_array_index(props,1);
    rprop->real_data = width;
    rprop = g_ptr_array_index(props,2);
    rprop->real_data = height;
    
    return props;
}

static DiaObject *
create_standard_ellipse(real xpos, real ypos, real width, real height,
			DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Ellipse");
    DiaObject *new_obj;
    Handle *h1, *h2;
    
    GPtrArray *props;
    Point point;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*   layer_add_object(dia->active_layer, new_obj); */
  
    props = make_element_props(xpos,ypos,width,height);
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);

    return new_obj;
}


static DiaObject *
create_standard_box(real xpos, real ypos, real width, real height,
		    DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Box");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*  layer_add_object(dia->active_layer, new_obj); */
  
    props = make_element_props(xpos,ypos,width,height);
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);

    return new_obj;
}

static PropDescription xfig_line_prop_descs[] = {
    PROP_STD_START_ARROW,
    PROP_STD_END_ARROW,
    PROP_DESC_END};

static DiaObject *
create_standard_polyline(int num_points, 
			 Point *points,
			 Arrow *end_arrow,
			 Arrow *start_arrow,
			 DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - PolyLine");
    DiaObject *new_obj;
    Handle *h1, *h2;
    MultipointCreateData *pcd;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    pcd = g_new(MultipointCreateData, 1);
    pcd->num_points = num_points;
    pcd->points = points;

    new_obj = otype->ops->create(NULL, pcd,
				 &h1, &h2);

    g_free(pcd);

    props = prop_list_from_descs(xfig_line_prop_descs,pdtpp_true);
    g_assert(props->len == 2);
    
    if (start_arrow != NULL)
	((ArrowProperty *)g_ptr_array_index(props, 0))->arrow_data = *start_arrow;
    if (end_arrow != NULL)
	((ArrowProperty *)g_ptr_array_index(props, 1))->arrow_data = *end_arrow;

    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);
    
    return new_obj;
}

static DiaObject *
create_standard_polygon(int num_points, 
			Point *points,
			DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Polygon");
    DiaObject *new_obj;
    Handle *h1, *h2;
    MultipointCreateData *pcd;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    pcd = g_new(MultipointCreateData, 1);
    pcd->num_points = num_points;
    pcd->points = points;

    new_obj = otype->ops->create(NULL, pcd, &h1, &h2);

    g_free(pcd);
    
    return new_obj;
}

static DiaObject *
create_standard_bezierline(int num_points, 
			   BezPoint *points,
			   Arrow *end_arrow,
			   Arrow *start_arrow,
			   DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - BezierLine");
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData *bcd;
    GPtrArray *props;

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
    
    props = prop_list_from_descs(xfig_line_prop_descs,pdtpp_true);
    g_assert(props->len == 2);
    
    if (start_arrow != NULL)
	((ArrowProperty *)g_ptr_array_index(props, 0))->arrow_data = *start_arrow;
    if (end_arrow != NULL)
	((ArrowProperty *)g_ptr_array_index(props, 1))->arrow_data = *end_arrow;

    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);
    
    return new_obj;
}

static DiaObject *
create_standard_beziergon(int num_points, 
			  BezPoint *points,
			  DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Beziergon");
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData *bcd;

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
    
    return new_obj;
}


static PropDescription xfig_arc_prop_descs[] = {
    { "curve_distance", PROP_TYPE_REAL },
    PROP_STD_START_ARROW,
    PROP_STD_END_ARROW,
    PROP_DESC_END};

static DiaObject *
create_standard_arc(real x1, real y1, real x2, real y2,
		    real radius, 
		    Arrow *end_arrow,
		    Arrow *start_arrow,
		    DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Arc");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = x1;
    point.y = y1;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*    layer_add_object(dia->active_layer, new_obj); */
    
    props = prop_list_from_descs(xfig_arc_prop_descs,pdtpp_true);
    g_assert(props->len == 3);
    
    ((RealProperty *)g_ptr_array_index(props,0))->real_data = radius;
    if (start_arrow != NULL)
	((ArrowProperty *)g_ptr_array_index(props, 1))->arrow_data = *start_arrow;
    if (end_arrow != NULL)
	((ArrowProperty *)g_ptr_array_index(props, 2))->arrow_data = *end_arrow;

    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);

    return new_obj;
}

static PropDescription xfig_file_prop_descs[] = {
    { "image_file", PROP_TYPE_FILE },
    PROP_DESC_END};

static DiaObject *
create_standard_image(real xpos, real ypos, real width, real height,
		      char *file, DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Image");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;
    StringProperty *sprop;

    if (otype == NULL){
	message_error(_("Can't find standard object"));
	return NULL;
    }

    point.x = xpos;
    point.y = ypos;

    new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
    /*    layer_add_object(dia->active_layer, new_obj); */
    
    props = make_element_props(xpos,ypos,width,height);
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);


    props = prop_list_from_descs(xfig_file_prop_descs,pdtpp_true);
    g_assert(props->len == 1);    
    sprop = g_ptr_array_index(props,0);
    g_free(sprop->string_data);
    sprop->string_data = g_strdup(file);
    new_obj->ops->set_props(new_obj, props);
    prop_list_free(props);

    return new_obj;
}

static DiaObject *
create_standard_group(GList *items, DiagramData *dia) {
    DiaObject *new_obj;

    new_obj = group_create((GList*)items);

    /*    layer_add_object(dia->active_layer, new_obj); */

    return new_obj;
}













static Color
vdx_get_color(const char *s, VDXDocument *theDoc)
{
    int colorvalues;
    Color c;
    if (s[0] == '#')
    {
        if (sscanf(s, "#%xd", &colorvalues) != 1) {
            message_error(_("Couldn't read color: %s\n"), s);
            theDoc->ok = FALSE;
            return c;
        }

        c.red = ((colorvalues & 0x00ff0000)>>16) / 255.0;
        c.green = ((colorvalues & 0x0000ff00)>>8) / 255.0;
        c.blue = (colorvalues & 0x000000ff) / 255.0;
        return c;
    }
    if (g_ascii_isdigit(*s))
    {
        /* Look in colour table */
        Color *master = &g_array_index(theDoc->Colors, Color, atoi(s));
        memcpy(&c, master, sizeof(c));
        return c;
    }
    message_error(_("Couldn't read color: %s\n"), s);
    return c;
}

static void 
vdx_get_colors(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr ColorEntry = cur->xmlChildrenNode;
    g_debug("vdx_get_colors");
    theDoc->Colors = g_array_new (FALSE, FALSE, sizeof (Color));

    while(ColorEntry) {
        Color color;
        xmlAttrPtr RGB;
        if (xmlIsBlankNode(ColorEntry)) { 
            ColorEntry = ColorEntry->next;
            continue;
        }
        if (strcmp((char *)ColorEntry->name, "ColorEntry")) {
            g_debug("Expecting ColorEntry got %s", ColorEntry->name);
            ColorEntry = ColorEntry->next;
            continue;
        }
        RGB = xmlHasProp(ColorEntry, (xmlChar *)"RGB");
        if (!RGB) {
            message_error(_("Couldn't read color: %s\n"), RGB);
            theDoc->ok = FALSE;
            return;
        }
        color = vdx_get_color((char *)RGB->children->content, theDoc);

        g_array_append_val(theDoc->Colors, color);

        ColorEntry = ColorEntry->next;
    }
}

static void 
vdx_get_facenames(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc)
{
    g_debug("vdx_get_facenames");
}

static void 
vdx_get_fonts(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr Font = cur->xmlChildrenNode;
    g_debug("vdx_get_fonts");
    theDoc->Fonts = g_array_new(FALSE, FALSE, 
                                sizeof (struct vdx_FontEntry));
    while(Font)
    {
        struct vdx_FontEntry FontEntry;
        xmlAttrPtr attr = Font->properties;
        if (xmlIsBlankNode(Font)) { Font = Font->next; continue; }
        while(attr) {
            if (!strcmp((char *)attr->name, "Name")) {
                FontEntry.Name = (char *)attr->children->content;
                g_debug("Font name %s", FontEntry.Name);
            }
            attr = attr->next;
        }
        g_array_append_val(theDoc->Fonts, FontEntry);
        Font = Font->next;
    }        
}

static void 
vdx_get_masters(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc)
{
    g_debug("vdx_get_masters");
}

static void 
vdx_get_stylesheets(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc)
{
    xmlNodePtr StyleSheet = cur->xmlChildrenNode;
    unsigned int sheet_count = 0;
    g_debug("vdx_get_stylesheets");
    theDoc->StyleSheets = g_array_new (FALSE, FALSE, 
                                       sizeof (struct vdx_StyleSheet));

    while(StyleSheet)
    {
        struct vdx_StyleSheet newSheet;
        xmlNodePtr node = StyleSheet->xmlChildrenNode;
        xmlAttrPtr attr = StyleSheet->properties;

        newSheet.children = NULL;
        newSheet.type = vdx_types_StyleSheet;
        if (xmlIsBlankNode(StyleSheet)) { 
            StyleSheet = StyleSheet->next;
            continue;
        }
        if (strcmp((char *)StyleSheet->name, "StyleSheet")) {
            g_debug("Expecting StyleSheet got %s", StyleSheet->name);
            StyleSheet = StyleSheet->next;
            continue;
        }

        /* Setup basic props as attributes */
        while(attr) {
            if (!strcmp((char *)attr->name, "NameU") || 
                !strcmp((char *)attr->name, "Name")) {
                newSheet.Name = (char *)attr->children->content;
                g_debug("StyleSheet[%d] = %s", sheet_count, newSheet.Name);
            }
            if (!strcmp((char *)attr->name, "LineStyle")) {
                newSheet.LineStyle = atoi((char *)attr->children->content);
            }
            if (!strcmp((char *)attr->name, "FillStyle")) {
                newSheet.FillStyle = atoi((char *)attr->children->content);
            }
            if (!strcmp((char *)attr->name, "TextStyle")) {
                newSheet.TextStyle = atoi((char *)attr->children->content);
            }
            attr = attr->next;
        }
        

        while(node)
        {
            if (xmlIsBlankNode(node)) { 
                node = node->next;
                continue;
            }
            if (!strcmp((char *)node->name, "Line"))
            {
                struct vdx_Line * LinePtr = g_new0(struct vdx_Line, 1);
                xmlNodePtr subnode = node->xmlChildrenNode;

                LinePtr->type = vdx_types_Line;
                
                while(subnode) {
                    if (xmlIsBlankNode(subnode)) { 
                        subnode = subnode->next;
                        continue;
                    }
                    if (!strcmp((char *)subnode->name, "BeginArrow")) {
                        LinePtr->BeginArrow = 
                            atoi((char *)subnode->children->content);
                    }
                    if (!strcmp((char *)subnode->name, "BeginArrowSize")) {
                        LinePtr->BeginArrowSize = 
                            atoi((char *)subnode->children->content);
                    }
                    if (!strcmp((char *)subnode->name, "EndArrow")) {
                        LinePtr->EndArrow = 
                            atoi((char *)subnode->children->content);
                    }
                    if (!strcmp((char *)subnode->name, "EndArrowSize")) {
                        LinePtr->EndArrowSize = 
                            atoi((char *)subnode->children->content);
                    }
                    if (!strcmp((char *)subnode->name, "LineCap")) {
                        if (subnode->children->content[0] != '0')
                            LinePtr->LineCap = TRUE;
                    }
                    if (!strcmp((char *)subnode->name, "LineColor")) {
                        LinePtr->LineColor = 
                            vdx_get_color((char *)subnode->children->content, 
                                          theDoc);
                    }
                    if (!strcmp((char *)subnode->name, "LineColorTrans")) {
                        LinePtr->LineColorTrans = 
                            atof((char *)subnode->children->content);
                    }
                    if (!strcmp((char *)subnode->name, "LinePattern")) {
                        LinePtr->LinePattern = 
                            atoi((char *)subnode->children->content);
                    }
                    if (!strcmp((char *)subnode->name, "LineWeight")) {
                        LinePtr->LineWeight = 
                            atof((char *)subnode->children->content);
                    }
                    if (!strcmp((char *)subnode->name, "Rounding")) {
                        LinePtr->Rounding = 
                            atof((char *)subnode->children->content);
                    }
                    subnode = subnode->next;
                }
                newSheet.children = 
                    g_slist_prepend(newSheet.children, LinePtr);
            }
            if (!strcmp((char *)node->name, "Fill"))
            {
                struct vdx_Fill * FillPtr = g_new0(struct vdx_Fill, 1);
                xmlNodePtr subnode = node->xmlChildrenNode;

                FillPtr->type = vdx_types_Fill;
                
                while(subnode) {
                    if (xmlIsBlankNode(subnode)) { 
                        subnode = subnode->next;
                        continue;
                    }
                    if (!strcmp((char *)subnode->name, "FillBkgnd")) {
                        FillPtr->FillBkgnd = 
                            vdx_get_color((char *)subnode->children->content, 
                                          theDoc);
                    }
                    if (!strcmp((char *)subnode->name, "FillForegnd")) {
                        FillPtr->FillForegnd = 
                            vdx_get_color((char *)subnode->children->content, 
                                          theDoc);
                    }
                    subnode = subnode->next;
                }
                newSheet.children = 
                    g_slist_prepend(newSheet.children, FillPtr);
            }


            

            node = node->next;
        }

        g_array_append_val(theDoc->StyleSheets, newSheet);
        StyleSheet = StyleSheet->next;
        sheet_count++;
    }
}

static void *
get_style_child(unsigned int type, unsigned int style, VDXDocument* theDoc)
{
    struct vdx_StyleSheet theSheet;
    GSList *p;
    struct vdx_Fill *Fillptr;
    g_debug("get_style_child(%s,%d)", vdx_Types[type], style);
    while(1)
    {
        theSheet = g_array_index(theDoc->StyleSheets, 
                                 struct vdx_StyleSheet, style);
        g_debug(" Looking in sheet %d [%s]", style, theSheet.Name);
        p = theSheet.children;
        while(p)
        {
            Fillptr = (struct vdx_Fill*)p->data;
            g_debug("  Has type %s", vdx_Types[(int)Fillptr->type]);
            if (Fillptr->type == type) return p->data;
            p = p->next;
        }
        if (!style) return 0;
        style = 0;
        if (type == vdx_types_Fill) style = theSheet.FillStyle;
        if (type == vdx_types_Line) style = theSheet.LineStyle;
        if (type == vdx_types_TextBlock) style = theSheet.TextStyle;
    }

    return 0;
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


static void
vdx_simple_properties(DiaObject *obj,
		      int line_style,
		      float dash_length,
		      int thickness,
		      const Color *pen_colour,
		      const Color *fill_colour, 
                      VDXDocument* theDoc) 
{
    GPtrArray *props = prop_list_from_descs(vdx_simple_prop_descs_line,
                                            pdtpp_true);
    RealProperty *rprop;
    ColorProperty *cprop;

    g_assert(props->len == 2);
    
    rprop = g_ptr_array_index(props,0);
    rprop->real_data = thickness;
    
    cprop = g_ptr_array_index(props,1);
    if (pen_colour) cprop->color_data = *pen_colour;

    if (line_style != -1) {
        LinestyleProperty *lsprop = 
            (LinestyleProperty *)make_new_prop("line_style", 
                                               PROP_TYPE_LINESTYLE,
                                               PROP_FLAG_DONT_SAVE);
        lsprop->dash = 0;
        lsprop->style = LINESTYLE_SOLID;

        g_ptr_array_add(props,lsprop);
    }

    if (! fill_colour) {
        BoolProperty *bprop = 
            (BoolProperty *)make_new_prop("show_background",
                                          PROP_TYPE_BOOL,PROP_FLAG_DONT_SAVE);
        bprop->bool_data = FALSE;

        g_ptr_array_add(props,bprop);
    } else {
        ColorProperty *cprop = 
            (ColorProperty *)make_new_prop("fill_colour",
                                           PROP_TYPE_COLOUR,
                                           PROP_FLAG_DONT_SAVE);
        cprop->color_data = *fill_colour;

        g_ptr_array_add(props,cprop);
    }

    obj->ops->set_props(obj, props);
    prop_list_free(props);
}




/* General absolute length conversion Visio -> Dia */
static double da(double a, VDXDocument* theDoc)
{
    return vdx_Point_Scale*a;
}

/* Visio x -> Dia x */
static double dx(double x, VDXDocument* theDoc)
{
    return vdx_Point_Scale*x + vdx_Page_Width*theDoc->Page;
}

/* Visio y -> Dia y */
static double dy(double y, VDXDocument* theDoc)
{
    return vdx_Y_Offset + vdx_Y_Flip*vdx_Point_Scale*y;
}

static void
plot_geom(const struct vdx_Geom *Geom, const struct vdx_XForm *XForm, 
          const struct vdx_Fill *Fill,
          VDXDocument* theDoc, DiagramData *dia)
{
    DiaObject *newobj = NULL;
    GPtrArray *props;
    GSList *item = Geom->children;
    struct vdx_MoveTo *MoveTo;
    struct vdx_LineTo *LineTo;
    struct vdx_Ellipse *Ellipse;
    struct vdx_EllipticalArcTo *EllipticalArcTo;
    Point *points;

    /* Start with individual objects */
    /* Later, we move to polylines */
    /* Translate from Visio-space to Dia-space before plotting */

    Point offset = {0, 0};
    Point current = {0, 0};

    g_debug("plot_geom");

    if (XForm)
    {
        offset.x = XForm->PinX;
        offset.y = XForm->PinY;
    }

    while(item)
    {
        switch(((struct vdx_MoveTo *)(item->data))->type)
        {
        case vdx_types_MoveTo:
            MoveTo = (struct vdx_MoveTo*)(item->data);
            g_debug("MoveTo");
            current.x = MoveTo->X;
            current.y = MoveTo->Y;
            break;

        case vdx_types_LineTo:
            points = calloc(sizeof(Point), 2);
            LineTo = (struct vdx_LineTo*)(item->data);
            g_debug("LineTo");
            points[0].x = dx(current.x + offset.x, theDoc);
            points[0].y = dy(current.y + offset.y, theDoc);
            points[1].x = dx(LineTo->X + offset.x, theDoc);
            points[1].y = dy(LineTo->Y + offset.y, theDoc);

            newobj = create_standard_polyline(2, points, NULL, NULL, dia);
            props = prop_list_from_descs(vdx_simple_prop_descs_line,
                                         pdtpp_true);
            newobj->ops->set_props(newobj, props);
            prop_list_free(props);
            layer_add_object(dia->active_layer, newobj);

            free(points);
            current.x = LineTo->X;
            current.y = LineTo->Y;
            break;

        case vdx_types_Ellipse:
            Ellipse = (struct vdx_Ellipse*)(item->data);
            g_debug("Ellipse");
            /* Visio Ellipse is centred, Dia is bottom left */
            newobj = 
                create_standard_ellipse(
                    dx(current.x + offset.x + Ellipse->A/2, theDoc), 
                    dy(current.y + offset.y + Ellipse->D/2, theDoc), 
                    da(Ellipse->A, theDoc), da(Ellipse->D, theDoc), 
                    dia);
            props = prop_list_from_descs(vdx_simple_prop_descs_line,
                                         pdtpp_true);
            newobj->ops->set_props(newobj, props);
            prop_list_free(props);

            if (!Geom->NoFill && Fill)
            {
                vdx_simple_properties(newobj, 0, 0, 0, 0, &Fill->FillForegnd, 
                                      theDoc);
            }

            layer_add_object(dia->active_layer, newobj);
            break;

        case vdx_types_EllipticalArcTo:
            g_debug("EllipticalArcTo");
            break;

        default:
            g_debug("default");
        }
        item = item->next;
    }
}

static void
plot_text(const char *Text, const struct vdx_XForm *XForm, 
          const struct vdx_Char *Char, VDXDocument* theDoc, DiagramData *dia)
{
    DiaObject *newobj;
    GPtrArray *props;
    TextProperty *tprop;
    struct vdx_FontEntry FontEntry;

    g_debug("plot_text");
    newobj = create_standard_text(dx(XForm->PinX + XForm->LocPinX/2, theDoc), 
                                  dy(XForm->PinY + XForm->LocPinY/2, theDoc), 
                                  dia);
    props = prop_list_from_descs(vdx_text_descs,pdtpp_true);
    tprop = g_ptr_array_index(props,0);
    tprop->text_data = g_strdup(Text);
    /* tprop->attr.alignment = sub_type; */
    tprop->attr.position.x = dx(XForm->PinX + XForm->LocPinX/2, theDoc);
    tprop->attr.position.y = dy(XForm->PinY + XForm->LocPinY/2, theDoc);
    if (theDoc->Fonts && Char->Font < theDoc->Fonts->len)
    {
        FontEntry = 
            g_array_index(theDoc->Fonts, struct vdx_FontEntry, Char->Font);
    }
    else { FontEntry.Name = "sans"; }
    tprop->attr.font = dia_font_new_from_legacy_name(FontEntry.Name);
    tprop->attr.height = Char->Size*vdx_Font_Size_Conversion; 
    /* tprop->attr.color = fig_color(color); */
    newobj->ops->set_props(newobj, props);
    prop_list_free(props);
    layer_add_object(dia->active_layer, newobj);
}


static void
vdx_add_shape(xmlDocPtr doc, xmlNodePtr Shape, VDXDocument* theDoc,
              DiagramData *dia)
{
    /* All decoding is done in Visio-space */
    xmlNodePtr Element = Shape->xmlChildrenNode;
    xmlAttrPtr attr = Shape->properties;
    struct vdx_Geom Geom = {0, 0 };
    char *Text = NULL;
    struct vdx_XForm XForm = {0, 0 };
    struct vdx_Shape theShape = {0, 0 };
    struct vdx_Char Char = {0, 0 };
    struct vdx_Fill Fill = {0, 0 };
    struct vdx_Fill *Fillptr = 0;
    struct vdx_Char *Charptr = 0;
    struct vdx_Line *Lineptr = 0;

    g_debug("vdx_add_shape");

    /* Get the props */
    while(attr) {
        if (!strcmp((char *)attr->name, "LineStyle")) {
            theShape.LineStyle = atoi((char *)attr->children->content);
        }
        if (!strcmp((char *)attr->name, "FillStyle")) {
            theShape.FillStyle = atoi((char *)attr->children->content);
            Fillptr = 
                (struct vdx_Fill *)get_style_child(vdx_types_Fill, 
                                                   theShape.FillStyle,
                                                   theDoc);
        }
        if (!strcmp((char *)attr->name, "TextStyle")) {
            theShape.TextStyle = atoi((char *)attr->children->content);
        }
        if (!strcmp((char *)attr->name, "Type")) {
            theShape.Type = (char *)attr->children->content;
        }
        attr = attr->next;
    }

    /* Ignore Guide and Group */
    if (strcmp(theShape.Type, "Shape")) 
    {
        g_debug("Ignoring Type = %s", theShape.Type);
        return;
    }

    /* Examine the children */
    while(Element)
    {
        if (xmlIsBlankNode(Element)) { 
            Element = Element->next;
            continue;
        }
        g_debug("Got element %s", Element->name);
        if (!strcmp((char *)Element->name, "Text"))
        {
            xmlNodePtr Child = Element->xmlChildrenNode;
            while (Child)
            {
                if (xmlNodeIsText(Child)) { Text = (char *)Child->content; }
                Child = Child->next;
            } 
            g_debug("Got Text '%s'", Text);
        }
        if (!strcmp((char *)Element->name, "XForm"))
        {
            xmlNodePtr Child = Element->xmlChildrenNode;
            while (Child)
            {
                if (xmlIsBlankNode(Child)) { Child = Child->next; continue; }
                if (!strcmp((char *)Child->name, "PinX")) {
                    XForm.PinX = atof((char *)Child->children->content);
                }
                if (!strcmp((char *)Child->name, "PinY")) {
                    XForm.PinY = atof((char *)Child->children->content);
                }
                if (!strcmp((char *)Child->name, "LocPinX")) {
                    XForm.LocPinX = atof((char *)Child->children->content);
                }
                if (!strcmp((char *)Child->name, "LocPinY")) {
                    XForm.LocPinY = atof((char *)Child->children->content);
                }
                Child = Child->next;
            }
        }
        if (!strcmp((char *)Element->name, "Char"))
        {
            xmlNodePtr Child = Element->xmlChildrenNode;
            while (Child)
            {
                if (xmlIsBlankNode(Child)) { Child = Child->next; continue; }
                if (!strcmp((char *)Child->name, "Size")) {
                    Char.Size = 
                        atof((char *)Child->children->content);
                }
                if (!strcmp((char *)Child->name, "Font")) {
                    Char.Font = 
                        atoi((char *)Child->children->content);
                }
                Child = Child->next;
            }
        }

        if (!strcmp((char *)Element->name, "Fill"))
        {
            xmlNodePtr Child = Element->xmlChildrenNode;
            Fillptr = &Fill;
            while (Child)
            {
                if (xmlIsBlankNode(Child)) { Child = Child->next; continue; }
                if (!strcmp((char *)Child->name, "FillForegnd")) {
                    Fill.FillForegnd = 
                        vdx_get_color((char *)Child->children->content, 
                                      theDoc);
                }
                Child = Child->next;
            }
        }

        if (!strcmp((char *)Element->name, "Geom"))
        {
            int IX = 0;
            xmlNodePtr Child = Element->xmlChildrenNode;
            Geom.children = NULL;
            while (Child)
            {
                if (xmlIsBlankNode(Child)) { Child = Child->next; continue; }
                g_debug("Got Geom element %s", Child->name);
                if (!strcmp((char *)Child->name, "NoFill")) {
                    Geom.NoFill = atoi((char *)Child->children->content);
                }
                if (!strcmp((char *)Child->name, "NoLine")) {
                    Geom.NoLine = atoi((char *)Child->children->content);
                }
                if (!strcmp((char *)Child->name, "NoShow")) {
                    Geom.NoShow = atoi((char *)Child->children->content);
                }
                if (!strcmp((char *)Child->name, "NoSnap")) {
                    Geom.NoSnap = atoi((char *)Child->children->content);
                }
                /* Ellipse, EllipticalArcTo, InfiniteLine, LineTo, MoveTo */

                if (!strcmp((char *)Child->name, "MoveTo")) {
                    struct vdx_MoveTo *MoveTo = g_new0(struct vdx_MoveTo, 1);
                    xmlNodePtr Param = Child->xmlChildrenNode;
                    while(Param)
                    {
                        if (xmlIsBlankNode(Param)) 
                        { Param = Param->next; continue; }
                        if (!strcmp((char *)Param->name, "X")) {
                            MoveTo->X = atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "Y")) {
                            MoveTo->Y = atof((char *)Param->children->content);
                        } 
                        Param = Param->next;
                    }
                    MoveTo->type = vdx_types_MoveTo;
                    Geom.children = g_slist_append(Geom.children, MoveTo);
                }

                if (!strcmp((char *)Child->name, "LineTo")) {
                    struct vdx_LineTo *LineTo = g_new0(struct vdx_LineTo, 1);
                    xmlNodePtr Param = Child->xmlChildrenNode;
                    while(Param)
                    {
                        if (xmlIsBlankNode(Param)) 
                        { Param = Param->next; continue; }
                        if (!strcmp((char *)Param->name, "Del")) {
                            LineTo->Del = 
                                atoi((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "X")) {
                            LineTo->X = 
                                atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "Y")) {
                            LineTo->Y = 
                                atof((char *)Param->children->content);
                        } 
                        Param = Param->next;
                    }
                    LineTo->type = vdx_types_LineTo;
                    Geom.children = g_slist_append(Geom.children, LineTo);
                }

                if (!strcmp((char *)Child->name, "Ellipse")) {
                    struct vdx_Ellipse *Ellipse = 
                        g_new0(struct vdx_Ellipse, 1);
                    xmlNodePtr Param = Child->xmlChildrenNode;
                    while(Param)
                    {
                        if (xmlIsBlankNode(Param)) 
                        { Param = Param->next; continue; }
                        if (!strcmp((char *)Param->name, "A")) {
                            Ellipse->A =
                                atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "D")) {
                            Ellipse->D =
                                atof((char *)Param->children->content);
                        }
                        Param = Param->next;
                    }
                    Ellipse->type = vdx_types_Ellipse;
                    Geom.children = g_slist_append(Geom.children, Ellipse);
                }

                if (!strcmp((char *)Child->name, "EllipticalArcTo")) {
                    struct vdx_EllipticalArcTo *EllipticalArcTo = 
                        g_new0(struct vdx_EllipticalArcTo, 1);
                    xmlNodePtr Param = Child->xmlChildrenNode;
                    while(Param)
                    {
                        if (xmlIsBlankNode(Param)) 
                        { Param = Param->next; continue; }
                        if (!strcmp((char *)Param->name, "A")) {
                            EllipticalArcTo->A = 
                                atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "B")) {
                            EllipticalArcTo->B = 
                                atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "C")) {
                            EllipticalArcTo->C = 
                                atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "D")) {
                            EllipticalArcTo->D = 
                                atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "X")) {
                            EllipticalArcTo->X = 
                                atof((char *)Param->children->content);
                        }
                        if (!strcmp((char *)Param->name, "Y")) {
                            EllipticalArcTo->Y = 
                                atof((char *)Param->children->content);
                        } 
                        Param = Param->next;
                    }
                    EllipticalArcTo->type = vdx_types_EllipticalArcTo;
                    Geom.children = g_slist_append(Geom.children, 
                                                   EllipticalArcTo);
                }
                Child = Child->next;
                IX++;
            }
        }

        /* Connection, Control, Event, Fill, Hyperlink, LayerMem, Layout,
           Line, Misc, Para, Protection, Shapes, TextBlock, TextXForm,
           User, XForm1D */

        Element = Element->next;
    }

    if (Text)
    {
        plot_text(Text, &XForm, &Char, theDoc, dia);
    }    
    else 
    {
        plot_geom(&Geom, &XForm, Fillptr, theDoc, dia);
    }

    if(Geom.children)
    {
        GSList *p = Geom.children;
        while(p)
        {
            g_free(p->data);
            p = p->next;
        }
        g_slist_free(Geom.children);
    }
    
}

static void 
vdx_get_pages(xmlDocPtr doc, xmlNodePtr cur, VDXDocument* theDoc,
              DiagramData *dia)
{
    xmlNodePtr Page = cur->xmlChildrenNode;
    xmlNodePtr Shapes;
    g_debug("vdx_get_pages");

    /* Only do page 1 */
    
    while(Page)
    {
        if (xmlIsBlankNode(Page)) { Page = Page->next; continue; }

        Shapes = Page->xmlChildrenNode;
        
        while(Shapes)
        {
            if (xmlIsBlankNode(Shapes)) { 
                Shapes = Shapes->next;
                continue;
            }
            g_debug("Page element: %s", Shapes->name);
            if (strcmp((char *)Shapes->name, "Shapes")) {
                /* Ignore non-shapes */
                Shapes = Shapes->next;
                continue;
            } else {
                xmlNodePtr Shape = Shapes->xmlChildrenNode;
                while (Shape)
                {
                    if (xmlIsBlankNode(Shape)) { 
                        Shape = Shape->next;
                        continue;
                    }
                    vdx_add_shape(doc, Shape, theDoc, dia);
                    Shape = Shape->next;
                }
            }
            Shapes = Shapes->next;
        }
        theDoc->Page++;
        Page = Page->next;
    }
}

static void 
vdx_free(VDXDocument *theDoc)
{
    int i;
    struct vdx_StyleSheet theSheet;
    GSList *p;

    g_debug("vdx_free");
    g_array_free(theDoc->Colors, TRUE);
    g_array_free(theDoc->Fonts, TRUE);
    
    for (i=0; i<theDoc->StyleSheets->len; i++) {
        theSheet = g_array_index(theDoc->StyleSheets, 
                                 struct vdx_StyleSheet, i);
        p = theSheet.children;
        while(p)
        {
            g_free(p->data);
            p = p->next;
        }
        g_slist_free(theSheet.children);
    }
    g_array_free(theDoc->StyleSheets, TRUE);
}


/* imports the given  file, returns TRUE if successful */
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
    root = doc->xmlRootNode;
    while (root && (root->type != XML_ELEMENT_NODE))
    {
        g_debug("Skipping node '%s'", root->name);
        root = root->next;
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
    
    cur = root->xmlChildrenNode;
    while(cur)
    {
        if (xmlIsBlankNode(cur)) { cur = cur->next; continue; }
        s = (const char *)cur->name;
        g_debug("Top level node '%s'", s);
        if (!strcmp(s, "Colors")) vdx_get_colors(doc, cur, theDoc);
        if (!strcmp(s, "FaceNames")) 
            vdx_get_facenames(doc, cur, theDoc);
        if (!strcmp(s, "Fonts")) 
            vdx_get_fonts(doc, cur, theDoc);
        if (!strcmp(s, "Masters")) 
            vdx_get_masters(doc, cur, theDoc);
        if (!strcmp(s, "StyleSheets")) 
            vdx_get_stylesheets(doc, cur, theDoc);
        if (!strcmp(s, "Pages")) 
            vdx_get_pages(doc, cur, theDoc, dia);
        if (!theDoc->ok) break;
        cur = cur->next;
    }

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
