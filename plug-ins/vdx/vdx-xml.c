/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 *
 * vdx-xml.c: Visio XML import filter for dia
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

/* Generated Sun Jun 25 08:08:55 2006 */
/* From: All.vdx animation_tests.vdx BasicShapes.vdx basic_tests.vdx Beispiel 1.vdx Beispiel 2.vdx Beispiel 3.vdx Circle1.vdx Circle2.vdx curve_tests.vdx Drawing2.vdx emf_dump_test2.orig.vdx emf_dump_test2.vdx Entreprise_etat_desire.vdx Line1.vdx Line2.vdx Line3.vdx Line4.vdx Line5.vdx Line6.vdx LombardiWireframe.vdx pattern_tests.vdx Rectangle1.vdx Rectangle2.vdx Rectangle3.vdx Rectangle4.vdx sample1.vdx Sample2.vdx samp_vdx.vdx seq_test.vdx SmithWireframe.vdx states.vdx Text1.vdx Text2.vdx Text3.vdx text_tests.vdx */


#include <glib.h>
#include <string.h>
#include <libxml/tree.h>
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

/** Parses an XML object into internal representation
 * @param cur the current XML node
 * @param theDoc the current document (with its colour table)
 * @param p a pointer to the storage to use, or NULL to allocate some
 * @returns An internal representation, or NULL if unknown
 */

void *
vdx_read_object(xmlNodePtr cur, VDXDocument *theDoc, void *p)
{
    xmlNodePtr child;
    xmlAttrPtr attr;
    g_debug(" XML Decoding %s", cur->name);

    if (!strcmp((char *)cur->name, "Act")) {
        struct vdx_Act *s;
        if (p) { s = (struct vdx_Act *)(p); }
        else { s = g_new0(struct vdx_Act, 1); }
        s->children = 0;
        s->type = vdx_types_Act;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Action"))
            { if (child->children && child->children->content)
                s->Action = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BeginGroup"))
            { if (child->children && child->children->content)
                s->BeginGroup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ButtonFace"))
            { if (child->children && child->children->content)
                s->ButtonFace = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Checked"))
            { if (child->children && child->children->content)
                s->Checked = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Disabled"))
            { if (child->children && child->children->content)
                s->Disabled = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Invisible"))
            { if (child->children && child->children->content)
                s->Invisible = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Menu"))
            { if (child->children && child->children->content)
                s->Menu = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "ReadOnly"))
            { if (child->children && child->children->content)
                s->ReadOnly = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SortKey"))
            { if (child->children && child->children->content)
                s->SortKey = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TagName"))
            { if (child->children && child->children->content)
                s->TagName = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Align")) {
        struct vdx_Align *s;
        if (p) { s = (struct vdx_Align *)(p); }
        else { s = g_new0(struct vdx_Align, 1); }
        s->children = 0;
        s->type = vdx_types_Align;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AlignBottom"))
            { if (child->children && child->children->content)
                s->AlignBottom = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "AlignCenter"))
            { if (child->children && child->children->content)
                s->AlignCenter = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "AlignLeft"))
            { if (child->children && child->children->content)
                s->AlignLeft = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "AlignMiddle"))
            { if (child->children && child->children->content)
                s->AlignMiddle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "AlignRight"))
            { if (child->children && child->children->content)
                s->AlignRight = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "AlignTop"))
            { if (child->children && child->children->content)
                s->AlignTop = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "ArcTo")) {
        struct vdx_ArcTo *s;
        if (p) { s = (struct vdx_ArcTo *)(p); }
        else { s = g_new0(struct vdx_ArcTo, 1); }
        s->children = 0;
        s->type = vdx_types_ArcTo;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "BegTrigger")) {
        struct vdx_BegTrigger *s;
        if (p) { s = (struct vdx_BegTrigger *)(p); }
        else { s = g_new0(struct vdx_BegTrigger, 1); }
        s->children = 0;
        s->type = vdx_types_BegTrigger;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "BeginX")) {
        struct vdx_BeginX *s;
        if (p) { s = (struct vdx_BeginX *)(p); }
        else { s = g_new0(struct vdx_BeginX, 1); }
        s->children = 0;
        s->type = vdx_types_BeginX;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "BeginY")) {
        struct vdx_BeginY *s;
        if (p) { s = (struct vdx_BeginY *)(p); }
        else { s = g_new0(struct vdx_BeginY, 1); }
        s->children = 0;
        s->type = vdx_types_BeginY;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Char")) {
        struct vdx_Char *s;
        if (p) { s = (struct vdx_Char *)(p); }
        else { s = g_new0(struct vdx_Char, 1); }
        s->children = 0;
        s->type = vdx_types_Char;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Del") &&
                     attr->children && attr->children->content)
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AsianFont"))
            { if (child->children && child->children->content)
                s->AsianFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Case"))
            { if (child->children && child->children->content)
                s->Case = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Color"))
            { if (child->children && child->children->content)
                s->Color = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "ColorTrans"))
            { if (child->children && child->children->content)
                s->ColorTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ComplexScriptFont"))
            { if (child->children && child->children->content)
                s->ComplexScriptFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ComplexScriptSize"))
            { if (child->children && child->children->content)
                s->ComplexScriptSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DblUnderline"))
            { if (child->children && child->children->content)
                s->DblUnderline = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DoubleStrikethrough"))
            { if (child->children && child->children->content)
                s->DoubleStrikethrough = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Font"))
            { if (child->children && child->children->content)
                s->Font = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FontScale"))
            { if (child->children && child->children->content)
                s->FontScale = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Highlight"))
            { if (child->children && child->children->content)
                s->Highlight = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LangID"))
            { if (child->children && child->children->content)
                s->LangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Letterspace"))
            { if (child->children && child->children->content)
                s->Letterspace = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Locale"))
            { if (child->children && child->children->content)
                s->Locale = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocalizeFont"))
            { if (child->children && child->children->content)
                s->LocalizeFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Overline"))
            { if (child->children && child->children->content)
                s->Overline = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Perpendicular"))
            { if (child->children && child->children->content)
                s->Perpendicular = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Pos"))
            { if (child->children && child->children->content)
                s->Pos = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "RTLText"))
            { if (child->children && child->children->content)
                s->RTLText = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Size"))
            { if (child->children && child->children->content)
                s->Size = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Strikethru"))
            { if (child->children && child->children->content)
                s->Strikethru = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Style"))
            { if (child->children && child->children->content)
                s->Style = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UseVertical"))
            { if (child->children && child->children->content)
                s->UseVertical = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "ColorEntry")) {
        struct vdx_ColorEntry *s;
        if (p) { s = (struct vdx_ColorEntry *)(p); }
        else { s = g_new0(struct vdx_ColorEntry, 1); }
        s->children = 0;
        s->type = vdx_types_ColorEntry;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "RGB") &&
                     attr->children && attr->children->content)
                s->RGB = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Colors")) {
        struct vdx_Colors *s;
        if (p) { s = (struct vdx_Colors *)(p); }
        else { s = g_new0(struct vdx_Colors, 1); }
        s->children = 0;
        s->type = vdx_types_Colors;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "ColorEntry"))
            { if (child->children && child->children->content)
                s->ColorEntry = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Connect")) {
        struct vdx_Connect *s;
        if (p) { s = (struct vdx_Connect *)(p); }
        else { s = g_new0(struct vdx_Connect, 1); }
        s->children = 0;
        s->type = vdx_types_Connect;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FromCell") &&
                     attr->children && attr->children->content)
                s->FromCell = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "FromPart") &&
                     attr->children && attr->children->content)
                s->FromPart = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "FromSheet") &&
                     attr->children && attr->children->content)
                s->FromSheet = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ToCell") &&
                     attr->children && attr->children->content)
                s->ToCell = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ToPart") &&
                     attr->children && attr->children->content)
                s->ToPart = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ToSheet") &&
                     attr->children && attr->children->content)
                s->ToSheet = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Connection")) {
        struct vdx_Connection *s;
        if (p) { s = (struct vdx_Connection *)(p); }
        else { s = g_new0(struct vdx_Connection, 1); }
        s->children = 0;
        s->type = vdx_types_Connection;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AutoGen"))
            { if (child->children && child->children->content)
                s->AutoGen = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DirX"))
            { if (child->children && child->children->content)
                s->DirX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DirY"))
            { if (child->children && child->children->content)
                s->DirY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children && child->children->content)
                s->Prompt = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Type"))
            { if (child->children && child->children->content)
                s->Type = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Connects")) {
        struct vdx_Connects *s;
        if (p) { s = (struct vdx_Connects *)(p); }
        else { s = g_new0(struct vdx_Connects, 1); }
        s->children = 0;
        s->type = vdx_types_Connects;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Connect"))
            { if (child->children && child->children->content)
                s->Connect = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Control")) {
        struct vdx_Control *s;
        if (p) { s = (struct vdx_Control *)(p); }
        else { s = g_new0(struct vdx_Control, 1); }
        s->children = 0;
        s->type = vdx_types_Control;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "CanGlue"))
            { if (child->children && child->children->content)
                s->CanGlue = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children && child->children->content)
                s->Prompt = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XCon"))
            { if (child->children && child->children->content)
                s->XCon = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XDyn"))
            { if (child->children && child->children->content)
                s->XDyn = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YCon"))
            { if (child->children && child->children->content)
                s->YCon = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YDyn"))
            { if (child->children && child->children->content)
                s->YDyn = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "CustomProp")) {
        struct vdx_CustomProp *s;
        if (p) { s = (struct vdx_CustomProp *)(p); }
        else { s = g_new0(struct vdx_CustomProp, 1); }
        s->children = 0;
        s->type = vdx_types_CustomProp;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "PropType") &&
                     attr->children && attr->children->content)
                s->PropType = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "CustomProps")) {
        struct vdx_CustomProps *s;
        if (p) { s = (struct vdx_CustomProps *)(p); }
        else { s = g_new0(struct vdx_CustomProps, 1); }
        s->children = 0;
        s->type = vdx_types_CustomProps;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "CustomProp"))
            { if (child->children && child->children->content)
                s->CustomProp = (char *)child->children->content; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocProps")) {
        struct vdx_DocProps *s;
        if (p) { s = (struct vdx_DocProps *)(p); }
        else { s = g_new0(struct vdx_DocProps, 1); }
        s->children = 0;
        s->type = vdx_types_DocProps;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AddMarkup"))
            { if (child->children && child->children->content)
                s->AddMarkup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DocLangID"))
            { if (child->children && child->children->content)
                s->DocLangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockPreview"))
            { if (child->children && child->children->content)
                s->LockPreview = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "OutputFormat"))
            { if (child->children && child->children->content)
                s->OutputFormat = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PreviewQuality"))
            { if (child->children && child->children->content)
                s->PreviewQuality = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PreviewScope"))
            { if (child->children && child->children->content)
                s->PreviewScope = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ViewMarkup"))
            { if (child->children && child->children->content)
                s->ViewMarkup = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocumentProperties")) {
        struct vdx_DocumentProperties *s;
        if (p) { s = (struct vdx_DocumentProperties *)(p); }
        else { s = g_new0(struct vdx_DocumentProperties, 1); }
        s->children = 0;
        s->type = vdx_types_DocumentProperties;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "BuildNumberCreated"))
            { if (child->children && child->children->content)
                s->BuildNumberCreated = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BuildNumberEdited"))
            { if (child->children && child->children->content)
                s->BuildNumberEdited = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Company"))
            { if (child->children && child->children->content)
                s->Company = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Creator"))
            { if (child->children && child->children->content)
                s->Creator = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Desc"))
            { if (child->children && child->children->content)
                s->Desc = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Subject"))
            { if (child->children && child->children->content)
                s->Subject = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Template"))
            { if (child->children && child->children->content)
                s->Template = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimeCreated"))
            { if (child->children && child->children->content)
                s->TimeCreated = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimeEdited"))
            { if (child->children && child->children->content)
                s->TimeEdited = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimePrinted"))
            { if (child->children && child->children->content)
                s->TimePrinted = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimeSaved"))
            { if (child->children && child->children->content)
                s->TimeSaved = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Title"))
            { if (child->children && child->children->content)
                s->Title = (char *)child->children->content; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocumentSettings")) {
        struct vdx_DocumentSettings *s;
        if (p) { s = (struct vdx_DocumentSettings *)(p); }
        else { s = g_new0(struct vdx_DocumentSettings, 1); }
        s->children = 0;
        s->type = vdx_types_DocumentSettings;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "DefaultFillStyle") &&
                     attr->children && attr->children->content)
                s->DefaultFillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "DefaultGuideStyle") &&
                     attr->children && attr->children->content)
                s->DefaultGuideStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "DefaultLineStyle") &&
                     attr->children && attr->children->content)
                s->DefaultLineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "DefaultTextStyle") &&
                     attr->children && attr->children->content)
                s->DefaultTextStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "TopPage") &&
                     attr->children && attr->children->content)
                s->TopPage = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "DynamicGridEnabled"))
            { if (child->children && child->children->content)
                s->DynamicGridEnabled = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "GlueSettings"))
            { if (child->children && child->children->content)
                s->GlueSettings = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectBkgnds"))
            { if (child->children && child->children->content)
                s->ProtectBkgnds = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectMasters"))
            { if (child->children && child->children->content)
                s->ProtectMasters = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectShapes"))
            { if (child->children && child->children->content)
                s->ProtectShapes = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectStyles"))
            { if (child->children && child->children->content)
                s->ProtectStyles = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapExtensions"))
            { if (child->children && child->children->content)
                s->SnapExtensions = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapSettings"))
            { if (child->children && child->children->content)
                s->SnapSettings = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocumentSheet")) {
        struct vdx_DocumentSheet *s;
        if (p) { s = (struct vdx_DocumentSheet *)(p); }
        else { s = g_new0(struct vdx_DocumentSheet, 1); }
        s->children = 0;
        s->type = vdx_types_DocumentSheet;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
                s->TextStyle = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Ellipse")) {
        struct vdx_Ellipse *s;
        if (p) { s = (struct vdx_Ellipse *)(p); }
        else { s = g_new0(struct vdx_Ellipse, 1); }
        s->children = 0;
        s->type = vdx_types_Ellipse;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children && child->children->content)
                s->B = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children && child->children->content)
                s->C = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children && child->children->content)
                s->D = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EllipticalArcTo")) {
        struct vdx_EllipticalArcTo *s;
        if (p) { s = (struct vdx_EllipticalArcTo *)(p); }
        else { s = g_new0(struct vdx_EllipticalArcTo, 1); }
        s->children = 0;
        s->type = vdx_types_EllipticalArcTo;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children && child->children->content)
                s->B = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children && child->children->content)
                s->C = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children && child->children->content)
                s->D = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EndX")) {
        struct vdx_EndX *s;
        if (p) { s = (struct vdx_EndX *)(p); }
        else { s = g_new0(struct vdx_EndX, 1); }
        s->children = 0;
        s->type = vdx_types_EndX;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EndY")) {
        struct vdx_EndY *s;
        if (p) { s = (struct vdx_EndY *)(p); }
        else { s = g_new0(struct vdx_EndY, 1); }
        s->children = 0;
        s->type = vdx_types_EndY;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Event")) {
        struct vdx_Event *s;
        if (p) { s = (struct vdx_Event *)(p); }
        else { s = g_new0(struct vdx_Event, 1); }
        s->children = 0;
        s->type = vdx_types_Event;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "EventDblClick"))
            { if (child->children && child->children->content)
                s->EventDblClick = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EventDrop"))
            { if (child->children && child->children->content)
                s->EventDrop = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EventXFMod"))
            { if (child->children && child->children->content)
                s->EventXFMod = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TheData"))
            { if (child->children && child->children->content)
                s->TheData = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TheText"))
            { if (child->children && child->children->content)
                s->TheText = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EventDblClick")) {
        struct vdx_EventDblClick *s;
        if (p) { s = (struct vdx_EventDblClick *)(p); }
        else { s = g_new0(struct vdx_EventDblClick, 1); }
        s->children = 0;
        s->type = vdx_types_EventDblClick;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EventItem")) {
        struct vdx_EventItem *s;
        if (p) { s = (struct vdx_EventItem *)(p); }
        else { s = g_new0(struct vdx_EventItem, 1); }
        s->children = 0;
        s->type = vdx_types_EventItem;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Action") &&
                     attr->children && attr->children->content)
                s->Action = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Enabled") &&
                     attr->children && attr->children->content)
                s->Enabled = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "EventCode") &&
                     attr->children && attr->children->content)
                s->EventCode = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Target") &&
                     attr->children && attr->children->content)
                s->Target = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TargetArgs") &&
                     attr->children && attr->children->content)
                s->TargetArgs = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EventList")) {
        struct vdx_EventList *s;
        if (p) { s = (struct vdx_EventList *)(p); }
        else { s = g_new0(struct vdx_EventList, 1); }
        s->children = 0;
        s->type = vdx_types_EventList;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "EventItem"))
            { if (child->children && child->children->content)
                s->EventItem = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "FaceName")) {
        struct vdx_FaceName *s;
        if (p) { s = (struct vdx_FaceName *)(p); }
        else { s = g_new0(struct vdx_FaceName, 1); }
        s->children = 0;
        s->type = vdx_types_FaceName;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "CharSets") &&
                     attr->children && attr->children->content)
                s->CharSets = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Flags") &&
                     attr->children && attr->children->content)
                s->Flags = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Panos") &&
                     attr->children && attr->children->content)
                s->Panos = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UnicodeRanges") &&
                     attr->children && attr->children->content)
                s->UnicodeRanges = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "FaceNames")) {
        struct vdx_FaceNames *s;
        if (p) { s = (struct vdx_FaceNames *)(p); }
        else { s = g_new0(struct vdx_FaceNames, 1); }
        s->children = 0;
        s->type = vdx_types_FaceNames;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "FaceName"))
            { if (child->children && child->children->content)
                s->FaceName = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Field")) {
        struct vdx_Field *s;
        if (p) { s = (struct vdx_Field *)(p); }
        else { s = g_new0(struct vdx_Field, 1); }
        s->children = 0;
        s->type = vdx_types_Field;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Del") &&
                     attr->children && attr->children->content)
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Calendar"))
            { if (child->children && child->children->content)
                s->Calendar = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EditMode"))
            { if (child->children && child->children->content)
                s->EditMode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Format"))
            { if (child->children && child->children->content)
                s->Format = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ObjectKind"))
            { if (child->children && child->children->content)
                s->ObjectKind = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Type"))
            { if (child->children && child->children->content)
                s->Type = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UICat"))
            { if (child->children && child->children->content)
                s->UICat = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UICod"))
            { if (child->children && child->children->content)
                s->UICod = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UIFmt"))
            { if (child->children && child->children->content)
                s->UIFmt = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Value"))
            { if (child->children && child->children->content)
                s->Value = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Fill")) {
        struct vdx_Fill *s;
        if (p) { s = (struct vdx_Fill *)(p); }
        else { s = g_new0(struct vdx_Fill, 1); }
        s->children = 0;
        s->type = vdx_types_Fill;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "FillBkgnd"))
            { if (child->children && child->children->content)
                s->FillBkgnd = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "FillBkgndTrans"))
            { if (child->children && child->children->content)
                s->FillBkgndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FillForegnd"))
            { if (child->children && child->children->content)
                s->FillForegnd = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "FillForegndTrans"))
            { if (child->children && child->children->content)
                s->FillForegndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FillPattern"))
            { if (child->children && child->children->content)
                s->FillPattern = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwObliqueAngle"))
            { if (child->children && child->children->content)
                s->ShapeShdwObliqueAngle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwOffsetX"))
            { if (child->children && child->children->content)
                s->ShapeShdwOffsetX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwOffsetY"))
            { if (child->children && child->children->content)
                s->ShapeShdwOffsetY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwScaleFactor"))
            { if (child->children && child->children->content)
                s->ShapeShdwScaleFactor = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwType"))
            { if (child->children && child->children->content)
                s->ShapeShdwType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwBkgnd"))
            { if (child->children && child->children->content)
                s->ShdwBkgnd = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwBkgndTrans"))
            { if (child->children && child->children->content)
                s->ShdwBkgndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwForegnd"))
            { if (child->children && child->children->content)
                s->ShdwForegnd = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "ShdwForegndTrans"))
            { if (child->children && child->children->content)
                s->ShdwForegndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwPattern"))
            { if (child->children && child->children->content)
                s->ShdwPattern = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "FontEntry")) {
        struct vdx_FontEntry *s;
        if (p) { s = (struct vdx_FontEntry *)(p); }
        else { s = g_new0(struct vdx_FontEntry, 1); }
        s->children = 0;
        s->type = vdx_types_FontEntry;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Attributes") &&
                     attr->children && attr->children->content)
                s->Attributes = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "CharSet") &&
                     attr->children && attr->children->content)
                s->CharSet = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "PitchAndFamily") &&
                     attr->children && attr->children->content)
                s->PitchAndFamily = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Unicode") &&
                     attr->children && attr->children->content)
                s->Unicode = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Weight") &&
                     attr->children && attr->children->content)
                s->Weight = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Fonts")) {
        struct vdx_Fonts *s;
        if (p) { s = (struct vdx_Fonts *)(p); }
        else { s = g_new0(struct vdx_Fonts, 1); }
        s->children = 0;
        s->type = vdx_types_Fonts;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "FontEntry"))
            { if (child->children && child->children->content)
                s->FontEntry = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Foreign")) {
        struct vdx_Foreign *s;
        if (p) { s = (struct vdx_Foreign *)(p); }
        else { s = g_new0(struct vdx_Foreign, 1); }
        s->children = 0;
        s->type = vdx_types_Foreign;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "ImgHeight"))
            { if (child->children && child->children->content)
                s->ImgHeight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ImgOffsetX"))
            { if (child->children && child->children->content)
                s->ImgOffsetX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ImgOffsetY"))
            { if (child->children && child->children->content)
                s->ImgOffsetY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ImgWidth"))
            { if (child->children && child->children->content)
                s->ImgWidth = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "ForeignData")) {
        struct vdx_ForeignData *s;
        if (p) { s = (struct vdx_ForeignData *)(p); }
        else { s = g_new0(struct vdx_ForeignData, 1); }
        s->children = 0;
        s->type = vdx_types_ForeignData;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ExtentX") &&
                     attr->children && attr->children->content)
                s->ExtentX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ExtentY") &&
                     attr->children && attr->children->content)
                s->ExtentY = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ForeignType") &&
                     attr->children && attr->children->content)
                s->ForeignType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "MappingMode") &&
                     attr->children && attr->children->content)
                s->MappingMode = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ObjectHeight") &&
                     attr->children && attr->children->content)
                s->ObjectHeight = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ObjectType") &&
                     attr->children && attr->children->content)
                s->ObjectType = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ObjectWidth") &&
                     attr->children && attr->children->content)
                s->ObjectWidth = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ShowAsIcon") &&
                     attr->children && attr->children->content)
                s->ShowAsIcon = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Geom")) {
        struct vdx_Geom *s;
        if (p) { s = (struct vdx_Geom *)(p); }
        else { s = g_new0(struct vdx_Geom, 1); }
        s->children = 0;
        s->type = vdx_types_Geom;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "NoFill"))
            { if (child->children && child->children->content)
                s->NoFill = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoLine"))
            { if (child->children && child->children->content)
                s->NoLine = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoShow"))
            { if (child->children && child->children->content)
                s->NoShow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoSnap"))
            { if (child->children && child->children->content)
                s->NoSnap = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Group")) {
        struct vdx_Group *s;
        if (p) { s = (struct vdx_Group *)(p); }
        else { s = g_new0(struct vdx_Group, 1); }
        s->children = 0;
        s->type = vdx_types_Group;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "DisplayMode"))
            { if (child->children && child->children->content)
                s->DisplayMode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DontMoveChildren"))
            { if (child->children && child->children->content)
                s->DontMoveChildren = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsDropTarget"))
            { if (child->children && child->children->content)
                s->IsDropTarget = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsSnapTarget"))
            { if (child->children && child->children->content)
                s->IsSnapTarget = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsTextEditTarget"))
            { if (child->children && child->children->content)
                s->IsTextEditTarget = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SelectMode"))
            { if (child->children && child->children->content)
                s->SelectMode = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "HeaderFooter")) {
        struct vdx_HeaderFooter *s;
        if (p) { s = (struct vdx_HeaderFooter *)(p); }
        else { s = g_new0(struct vdx_HeaderFooter, 1); }
        s->children = 0;
        s->type = vdx_types_HeaderFooter;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "HeaderFooterColor") &&
                     attr->children && attr->children->content)
                s->HeaderFooterColor = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "FooterLeft"))
            { if (child->children && child->children->content)
                s->FooterLeft = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "FooterMargin"))
            { if (child->children && child->children->content)
                s->FooterMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HeaderFooterFont"))
            { if (child->children && child->children->content)
                s->HeaderFooterFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HeaderLeft"))
            { if (child->children && child->children->content)
                s->HeaderLeft = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "HeaderMargin"))
            { if (child->children && child->children->content)
                s->HeaderMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HeaderRight"))
            { if (child->children && child->children->content)
                s->HeaderRight = (char *)child->children->content; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "HeaderFooterFont")) {
        struct vdx_HeaderFooterFont *s;
        if (p) { s = (struct vdx_HeaderFooterFont *)(p); }
        else { s = g_new0(struct vdx_HeaderFooterFont, 1); }
        s->children = 0;
        s->type = vdx_types_HeaderFooterFont;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "CharSet") &&
                     attr->children && attr->children->content)
                s->CharSet = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ClipPrecision") &&
                     attr->children && attr->children->content)
                s->ClipPrecision = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Escapement") &&
                     attr->children && attr->children->content)
                s->Escapement = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "FaceName") &&
                     attr->children && attr->children->content)
                s->FaceName = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Height") &&
                     attr->children && attr->children->content)
                s->Height = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Italic") &&
                     attr->children && attr->children->content)
                s->Italic = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Orientation") &&
                     attr->children && attr->children->content)
                s->Orientation = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "OutPrecision") &&
                     attr->children && attr->children->content)
                s->OutPrecision = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "PitchAndFamily") &&
                     attr->children && attr->children->content)
                s->PitchAndFamily = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Quality") &&
                     attr->children && attr->children->content)
                s->Quality = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "StrikeOut") &&
                     attr->children && attr->children->content)
                s->StrikeOut = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Underline") &&
                     attr->children && attr->children->content)
                s->Underline = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Weight") &&
                     attr->children && attr->children->content)
                s->Weight = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Width") &&
                     attr->children && attr->children->content)
                s->Width = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Help")) {
        struct vdx_Help *s;
        if (p) { s = (struct vdx_Help *)(p); }
        else { s = g_new0(struct vdx_Help, 1); }
        s->children = 0;
        s->type = vdx_types_Help;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Copyright"))
            { if (child->children && child->children->content)
                s->Copyright = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "HelpTopic"))
            { if (child->children && child->children->content)
                s->HelpTopic = (char *)child->children->content; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Hyperlink")) {
        struct vdx_Hyperlink *s;
        if (p) { s = (struct vdx_Hyperlink *)(p); }
        else { s = g_new0(struct vdx_Hyperlink, 1); }
        s->children = 0;
        s->type = vdx_types_Hyperlink;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Address"))
            { if (child->children && child->children->content)
                s->Address = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Default"))
            { if (child->children && child->children->content)
                s->Default = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Description"))
            { if (child->children && child->children->content)
                s->Description = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "ExtraInfo"))
            { if (child->children && child->children->content)
                s->ExtraInfo = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Frame"))
            { if (child->children && child->children->content)
                s->Frame = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Invisible"))
            { if (child->children && child->children->content)
                s->Invisible = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NewWindow"))
            { if (child->children && child->children->content)
                s->NewWindow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SortKey"))
            { if (child->children && child->children->content)
                s->SortKey = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SubAddress"))
            { if (child->children && child->children->content)
                s->SubAddress = (char *)child->children->content; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Icon")) {
        struct vdx_Icon *s;
        if (p) { s = (struct vdx_Icon *)(p); }
        else { s = g_new0(struct vdx_Icon, 1); }
        s->children = 0;
        s->type = vdx_types_Icon;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Image")) {
        struct vdx_Image *s;
        if (p) { s = (struct vdx_Image *)(p); }
        else { s = g_new0(struct vdx_Image, 1); }
        s->children = 0;
        s->type = vdx_types_Image;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Blur"))
            { if (child->children && child->children->content)
                s->Blur = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Brightness"))
            { if (child->children && child->children->content)
                s->Brightness = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Contrast"))
            { if (child->children && child->children->content)
                s->Contrast = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Denoise"))
            { if (child->children && child->children->content)
                s->Denoise = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Gamma"))
            { if (child->children && child->children->content)
                s->Gamma = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Sharpen"))
            { if (child->children && child->children->content)
                s->Sharpen = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Transparency"))
            { if (child->children && child->children->content)
                s->Transparency = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "InfiniteLine")) {
        struct vdx_InfiniteLine *s;
        if (p) { s = (struct vdx_InfiniteLine *)(p); }
        else { s = g_new0(struct vdx_InfiniteLine, 1); }
        s->children = 0;
        s->type = vdx_types_InfiniteLine;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children && child->children->content)
                s->B = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Layer")) {
        struct vdx_Layer *s;
        if (p) { s = (struct vdx_Layer *)(p); }
        else { s = g_new0(struct vdx_Layer, 1); }
        s->children = 0;
        s->type = vdx_types_Layer;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Active"))
            { if (child->children && child->children->content)
                s->Active = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Color"))
            { if (child->children && child->children->content)
                s->Color = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ColorTrans"))
            { if (child->children && child->children->content)
                s->ColorTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Glue"))
            { if (child->children && child->children->content)
                s->Glue = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Lock"))
            { if (child->children && child->children->content)
                s->Lock = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Name"))
            { if (child->children && child->children->content)
                s->Name = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "NameUniv"))
            { if (child->children && child->children->content)
                s->NameUniv = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Print"))
            { if (child->children && child->children->content)
                s->Print = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Snap"))
            { if (child->children && child->children->content)
                s->Snap = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Status"))
            { if (child->children && child->children->content)
                s->Status = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Visible"))
            { if (child->children && child->children->content)
                s->Visible = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "LayerMem")) {
        struct vdx_LayerMem *s;
        if (p) { s = (struct vdx_LayerMem *)(p); }
        else { s = g_new0(struct vdx_LayerMem, 1); }
        s->children = 0;
        s->type = vdx_types_LayerMem;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "LayerMember"))
            { if (child->children && child->children->content)
                s->LayerMember = (char *)child->children->content; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Layout")) {
        struct vdx_Layout *s;
        if (p) { s = (struct vdx_Layout *)(p); }
        else { s = g_new0(struct vdx_Layout, 1); }
        s->children = 0;
        s->type = vdx_types_Layout;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "ConFixedCode"))
            { if (child->children && child->children->content)
                s->ConFixedCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpCode"))
            { if (child->children && child->children->content)
                s->ConLineJumpCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpDirX"))
            { if (child->children && child->children->content)
                s->ConLineJumpDirX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpDirY"))
            { if (child->children && child->children->content)
                s->ConLineJumpDirY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpStyle"))
            { if (child->children && child->children->content)
                s->ConLineJumpStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineRouteExt"))
            { if (child->children && child->children->content)
                s->ConLineRouteExt = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeFixedCode"))
            { if (child->children && child->children->content)
                s->ShapeFixedCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePermeablePlace"))
            { if (child->children && child->children->content)
                s->ShapePermeablePlace = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePermeableX"))
            { if (child->children && child->children->content)
                s->ShapePermeableX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePermeableY"))
            { if (child->children && child->children->content)
                s->ShapePermeableY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePlaceFlip"))
            { if (child->children && child->children->content)
                s->ShapePlaceFlip = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePlowCode"))
            { if (child->children && child->children->content)
                s->ShapePlowCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeRouteStyle"))
            { if (child->children && child->children->content)
                s->ShapeRouteStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeSplit"))
            { if (child->children && child->children->content)
                s->ShapeSplit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeSplittable"))
            { if (child->children && child->children->content)
                s->ShapeSplittable = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Line")) {
        struct vdx_Line *s;
        if (p) { s = (struct vdx_Line *)(p); }
        else { s = g_new0(struct vdx_Line, 1); }
        s->children = 0;
        s->type = vdx_types_Line;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "BeginArrow"))
            { if (child->children && child->children->content)
                s->BeginArrow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BeginArrowSize"))
            { if (child->children && child->children->content)
                s->BeginArrowSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndArrow"))
            { if (child->children && child->children->content)
                s->EndArrow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndArrowSize"))
            { if (child->children && child->children->content)
                s->EndArrowSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineCap"))
            { if (child->children && child->children->content)
                s->LineCap = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineColor"))
            { if (child->children && child->children->content)
                s->LineColor = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "LineColorTrans"))
            { if (child->children && child->children->content)
                s->LineColorTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LinePattern"))
            { if (child->children && child->children->content)
                s->LinePattern = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineWeight"))
            { if (child->children && child->children->content)
                s->LineWeight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Rounding"))
            { if (child->children && child->children->content)
                s->Rounding = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "LineTo")) {
        struct vdx_LineTo *s;
        if (p) { s = (struct vdx_LineTo *)(p); }
        else { s = g_new0(struct vdx_LineTo, 1); }
        s->children = 0;
        s->type = vdx_types_LineTo;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Del") &&
                     attr->children && attr->children->content)
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Master")) {
        struct vdx_Master *s;
        if (p) { s = (struct vdx_Master *)(p); }
        else { s = g_new0(struct vdx_Master, 1); }
        s->children = 0;
        s->type = vdx_types_Master;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "AlignName") &&
                     attr->children && attr->children->content)
                s->AlignName = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "BaseID") &&
                     attr->children && attr->children->content)
                s->BaseID = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Hidden") &&
                     attr->children && attr->children->content)
                s->Hidden = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IconSize") &&
                     attr->children && attr->children->content)
                s->IconSize = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IconUpdate") &&
                     attr->children && attr->children->content)
                s->IconUpdate = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "MatchByName") &&
                     attr->children && attr->children->content)
                s->MatchByName = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "PatternFlags") &&
                     attr->children && attr->children->content)
                s->PatternFlags = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Prompt") &&
                     attr->children && attr->children->content)
                s->Prompt = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UniqueID") &&
                     attr->children && attr->children->content)
                s->UniqueID = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Menu")) {
        struct vdx_Menu *s;
        if (p) { s = (struct vdx_Menu *)(p); }
        else { s = g_new0(struct vdx_Menu, 1); }
        s->children = 0;
        s->type = vdx_types_Menu;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Misc")) {
        struct vdx_Misc *s;
        if (p) { s = (struct vdx_Misc *)(p); }
        else { s = g_new0(struct vdx_Misc, 1); }
        s->children = 0;
        s->type = vdx_types_Misc;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "BegTrigger"))
            { if (child->children && child->children->content)
                s->BegTrigger = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Calendar"))
            { if (child->children && child->children->content)
                s->Calendar = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Comment"))
            { if (child->children && child->children->content)
                s->Comment = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "DropOnPageScale"))
            { if (child->children && child->children->content)
                s->DropOnPageScale = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DynFeedback"))
            { if (child->children && child->children->content)
                s->DynFeedback = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndTrigger"))
            { if (child->children && child->children->content)
                s->EndTrigger = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "GlueType"))
            { if (child->children && child->children->content)
                s->GlueType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HideText"))
            { if (child->children && child->children->content)
                s->HideText = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsDropSource"))
            { if (child->children && child->children->content)
                s->IsDropSource = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LangID"))
            { if (child->children && child->children->content)
                s->LangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocalizeMerge"))
            { if (child->children && child->children->content)
                s->LocalizeMerge = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoAlignBox"))
            { if (child->children && child->children->content)
                s->NoAlignBox = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoCtlHandles"))
            { if (child->children && child->children->content)
                s->NoCtlHandles = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoLiveDynamics"))
            { if (child->children && child->children->content)
                s->NoLiveDynamics = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoObjHandles"))
            { if (child->children && child->children->content)
                s->NoObjHandles = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NonPrinting"))
            { if (child->children && child->children->content)
                s->NonPrinting = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ObjType"))
            { if (child->children && child->children->content)
                s->ObjType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeKeywords"))
            { if (child->children && child->children->content)
                s->ShapeKeywords = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "UpdateAlignBox"))
            { if (child->children && child->children->content)
                s->UpdateAlignBox = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "WalkPreference"))
            { if (child->children && child->children->content)
                s->WalkPreference = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "MoveTo")) {
        struct vdx_MoveTo *s;
        if (p) { s = (struct vdx_MoveTo *)(p); }
        else { s = g_new0(struct vdx_MoveTo, 1); }
        s->children = 0;
        s->type = vdx_types_MoveTo;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "NURBSTo")) {
        struct vdx_NURBSTo *s;
        if (p) { s = (struct vdx_NURBSTo *)(p); }
        else { s = g_new0(struct vdx_NURBSTo, 1); }
        s->children = 0;
        s->type = vdx_types_NURBSTo;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children && child->children->content)
                s->B = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children && child->children->content)
                s->C = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children && child->children->content)
                s->D = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "E"))
            { if (child->children && child->children->content)
                s->E = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "NameUniv")) {
        struct vdx_NameUniv *s;
        if (p) { s = (struct vdx_NameUniv *)(p); }
        else { s = g_new0(struct vdx_NameUniv, 1); }
        s->children = 0;
        s->type = vdx_types_NameUniv;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Err") &&
                     attr->children && attr->children->content)
                s->Err = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Page")) {
        struct vdx_Page *s;
        if (p) { s = (struct vdx_Page *)(p); }
        else { s = g_new0(struct vdx_Page, 1); }
        s->children = 0;
        s->type = vdx_types_Page;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "BackPage") &&
                     attr->children && attr->children->content)
                s->BackPage = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Background") &&
                     attr->children && attr->children->content)
                s->Background = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ViewCenterX") &&
                     attr->children && attr->children->content)
                s->ViewCenterX = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewCenterY") &&
                     attr->children && attr->children->content)
                s->ViewCenterY = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewScale") &&
                     attr->children && attr->children->content)
                s->ViewScale = atof((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PageLayout")) {
        struct vdx_PageLayout *s;
        if (p) { s = (struct vdx_PageLayout *)(p); }
        else { s = g_new0(struct vdx_PageLayout, 1); }
        s->children = 0;
        s->type = vdx_types_PageLayout;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AvenueSizeX"))
            { if (child->children && child->children->content)
                s->AvenueSizeX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "AvenueSizeY"))
            { if (child->children && child->children->content)
                s->AvenueSizeY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BlockSizeX"))
            { if (child->children && child->children->content)
                s->BlockSizeX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BlockSizeY"))
            { if (child->children && child->children->content)
                s->BlockSizeY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "CtrlAsInput"))
            { if (child->children && child->children->content)
                s->CtrlAsInput = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DynamicsOff"))
            { if (child->children && child->children->content)
                s->DynamicsOff = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EnableGrid"))
            { if (child->children && child->children->content)
                s->EnableGrid = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineAdjustFrom"))
            { if (child->children && child->children->content)
                s->LineAdjustFrom = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineAdjustTo"))
            { if (child->children && child->children->content)
                s->LineAdjustTo = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpCode"))
            { if (child->children && child->children->content)
                s->LineJumpCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpFactorX"))
            { if (child->children && child->children->content)
                s->LineJumpFactorX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpFactorY"))
            { if (child->children && child->children->content)
                s->LineJumpFactorY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpStyle"))
            { if (child->children && child->children->content)
                s->LineJumpStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineRouteExt"))
            { if (child->children && child->children->content)
                s->LineRouteExt = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToLineX"))
            { if (child->children && child->children->content)
                s->LineToLineX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToLineY"))
            { if (child->children && child->children->content)
                s->LineToLineY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToNodeX"))
            { if (child->children && child->children->content)
                s->LineToNodeX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToNodeY"))
            { if (child->children && child->children->content)
                s->LineToNodeY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLineJumpDirX"))
            { if (child->children && child->children->content)
                s->PageLineJumpDirX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLineJumpDirY"))
            { if (child->children && child->children->content)
                s->PageLineJumpDirY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageShapeSplit"))
            { if (child->children && child->children->content)
                s->PageShapeSplit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlaceDepth"))
            { if (child->children && child->children->content)
                s->PlaceDepth = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlaceFlip"))
            { if (child->children && child->children->content)
                s->PlaceFlip = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlaceStyle"))
            { if (child->children && child->children->content)
                s->PlaceStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlowCode"))
            { if (child->children && child->children->content)
                s->PlowCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ResizePage"))
            { if (child->children && child->children->content)
                s->ResizePage = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "RouteStyle"))
            { if (child->children && child->children->content)
                s->RouteStyle = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PageProps")) {
        struct vdx_PageProps *s;
        if (p) { s = (struct vdx_PageProps *)(p); }
        else { s = g_new0(struct vdx_PageProps, 1); }
        s->children = 0;
        s->type = vdx_types_PageProps;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "DrawingScale"))
            { if (child->children && child->children->content)
                s->DrawingScale = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DrawingScaleType"))
            { if (child->children && child->children->content)
                s->DrawingScaleType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DrawingSizeType"))
            { if (child->children && child->children->content)
                s->DrawingSizeType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "InhibitSnap"))
            { if (child->children && child->children->content)
                s->InhibitSnap = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageHeight"))
            { if (child->children && child->children->content)
                s->PageHeight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageScale"))
            { if (child->children && child->children->content)
                s->PageScale = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageWidth"))
            { if (child->children && child->children->content)
                s->PageWidth = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwObliqueAngle"))
            { if (child->children && child->children->content)
                s->ShdwObliqueAngle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwOffsetX"))
            { if (child->children && child->children->content)
                s->ShdwOffsetX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwOffsetY"))
            { if (child->children && child->children->content)
                s->ShdwOffsetY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwScaleFactor"))
            { if (child->children && child->children->content)
                s->ShdwScaleFactor = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwType"))
            { if (child->children && child->children->content)
                s->ShdwType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UIVisibility"))
            { if (child->children && child->children->content)
                s->UIVisibility = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PageSheet")) {
        struct vdx_PageSheet *s;
        if (p) { s = (struct vdx_PageSheet *)(p); }
        else { s = g_new0(struct vdx_PageSheet, 1); }
        s->children = 0;
        s->type = vdx_types_PageSheet;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
                s->TextStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "UniqueID") &&
                     attr->children && attr->children->content)
                s->UniqueID = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Para")) {
        struct vdx_Para *s;
        if (p) { s = (struct vdx_Para *)(p); }
        else { s = g_new0(struct vdx_Para, 1); }
        s->children = 0;
        s->type = vdx_types_Para;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Bullet"))
            { if (child->children && child->children->content)
                s->Bullet = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BulletFont"))
            { if (child->children && child->children->content)
                s->BulletFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BulletFontSize"))
            { if (child->children && child->children->content)
                s->BulletFontSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BulletStr"))
            { if (child->children && child->children->content)
                s->BulletStr = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Flags"))
            { if (child->children && child->children->content)
                s->Flags = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HorzAlign"))
            { if (child->children && child->children->content)
                s->HorzAlign = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IndFirst"))
            { if (child->children && child->children->content)
                s->IndFirst = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IndLeft"))
            { if (child->children && child->children->content)
                s->IndLeft = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IndRight"))
            { if (child->children && child->children->content)
                s->IndRight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocalizeBulletFont"))
            { if (child->children && child->children->content)
                s->LocalizeBulletFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SpAfter"))
            { if (child->children && child->children->content)
                s->SpAfter = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SpBefore"))
            { if (child->children && child->children->content)
                s->SpBefore = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SpLine"))
            { if (child->children && child->children->content)
                s->SpLine = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextPosAfterBullet"))
            { if (child->children && child->children->content)
                s->TextPosAfterBullet = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PreviewPicture")) {
        struct vdx_PreviewPicture *s;
        if (p) { s = (struct vdx_PreviewPicture *)(p); }
        else { s = g_new0(struct vdx_PreviewPicture, 1); }
        s->children = 0;
        s->type = vdx_types_PreviewPicture;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Size") &&
                     attr->children && attr->children->content)
                s->Size = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PrintProps")) {
        struct vdx_PrintProps *s;
        if (p) { s = (struct vdx_PrintProps *)(p); }
        else { s = g_new0(struct vdx_PrintProps, 1); }
        s->children = 0;
        s->type = vdx_types_PrintProps;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "CenterX"))
            { if (child->children && child->children->content)
                s->CenterX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "CenterY"))
            { if (child->children && child->children->content)
                s->CenterY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "OnPage"))
            { if (child->children && child->children->content)
                s->OnPage = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageBottomMargin"))
            { if (child->children && child->children->content)
                s->PageBottomMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLeftMargin"))
            { if (child->children && child->children->content)
                s->PageLeftMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageRightMargin"))
            { if (child->children && child->children->content)
                s->PageRightMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageTopMargin"))
            { if (child->children && child->children->content)
                s->PageTopMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PagesX"))
            { if (child->children && child->children->content)
                s->PagesX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PagesY"))
            { if (child->children && child->children->content)
                s->PagesY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PaperKind"))
            { if (child->children && child->children->content)
                s->PaperKind = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PaperSource"))
            { if (child->children && child->children->content)
                s->PaperSource = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintGrid"))
            { if (child->children && child->children->content)
                s->PrintGrid = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintPageOrientation"))
            { if (child->children && child->children->content)
                s->PrintPageOrientation = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ScaleX"))
            { if (child->children && child->children->content)
                s->ScaleX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ScaleY"))
            { if (child->children && child->children->content)
                s->ScaleY = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PrintSetup")) {
        struct vdx_PrintSetup *s;
        if (p) { s = (struct vdx_PrintSetup *)(p); }
        else { s = g_new0(struct vdx_PrintSetup, 1); }
        s->children = 0;
        s->type = vdx_types_PrintSetup;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "PageBottomMargin"))
            { if (child->children && child->children->content)
                s->PageBottomMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLeftMargin"))
            { if (child->children && child->children->content)
                s->PageLeftMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageRightMargin"))
            { if (child->children && child->children->content)
                s->PageRightMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageTopMargin"))
            { if (child->children && child->children->content)
                s->PageTopMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PaperSize"))
            { if (child->children && child->children->content)
                s->PaperSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintCenteredH"))
            { if (child->children && child->children->content)
                s->PrintCenteredH = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintCenteredV"))
            { if (child->children && child->children->content)
                s->PrintCenteredV = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintFitOnPages"))
            { if (child->children && child->children->content)
                s->PrintFitOnPages = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintLandscape"))
            { if (child->children && child->children->content)
                s->PrintLandscape = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintPagesAcross"))
            { if (child->children && child->children->content)
                s->PrintPagesAcross = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintPagesDown"))
            { if (child->children && child->children->content)
                s->PrintPagesDown = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintScale"))
            { if (child->children && child->children->content)
                s->PrintScale = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Prop")) {
        struct vdx_Prop *s;
        if (p) { s = (struct vdx_Prop *)(p); }
        else { s = g_new0(struct vdx_Prop, 1); }
        s->children = 0;
        s->type = vdx_types_Prop;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Calendar"))
            { if (child->children && child->children->content)
                s->Calendar = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Format"))
            { if (child->children && child->children->content)
                s->Format = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Invisible"))
            { if (child->children && child->children->content)
                s->Invisible = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Label"))
            { if (child->children && child->children->content)
                s->Label = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "LangID"))
            { if (child->children && child->children->content)
                s->LangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children && child->children->content)
                s->Prompt = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "SortKey"))
            { if (child->children && child->children->content)
                s->SortKey = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Type"))
            { if (child->children && child->children->content)
                s->Type = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Value"))
            { if (child->children && child->children->content)
                s->Value = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Verify"))
            { if (child->children && child->children->content)
                s->Verify = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Protection")) {
        struct vdx_Protection *s;
        if (p) { s = (struct vdx_Protection *)(p); }
        else { s = g_new0(struct vdx_Protection, 1); }
        s->children = 0;
        s->type = vdx_types_Protection;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "LockAspect"))
            { if (child->children && child->children->content)
                s->LockAspect = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockBegin"))
            { if (child->children && child->children->content)
                s->LockBegin = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockCalcWH"))
            { if (child->children && child->children->content)
                s->LockCalcWH = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockCrop"))
            { if (child->children && child->children->content)
                s->LockCrop = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockCustProp"))
            { if (child->children && child->children->content)
                s->LockCustProp = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockDelete"))
            { if (child->children && child->children->content)
                s->LockDelete = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockEnd"))
            { if (child->children && child->children->content)
                s->LockEnd = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockFormat"))
            { if (child->children && child->children->content)
                s->LockFormat = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockGroup"))
            { if (child->children && child->children->content)
                s->LockGroup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockHeight"))
            { if (child->children && child->children->content)
                s->LockHeight = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockMoveX"))
            { if (child->children && child->children->content)
                s->LockMoveX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockMoveY"))
            { if (child->children && child->children->content)
                s->LockMoveY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockRotate"))
            { if (child->children && child->children->content)
                s->LockRotate = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockSelect"))
            { if (child->children && child->children->content)
                s->LockSelect = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockTextEdit"))
            { if (child->children && child->children->content)
                s->LockTextEdit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockVtxEdit"))
            { if (child->children && child->children->content)
                s->LockVtxEdit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockWidth"))
            { if (child->children && child->children->content)
                s->LockWidth = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "RulerGrid")) {
        struct vdx_RulerGrid *s;
        if (p) { s = (struct vdx_RulerGrid *)(p); }
        else { s = g_new0(struct vdx_RulerGrid, 1); }
        s->children = 0;
        s->type = vdx_types_RulerGrid;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "XGridDensity"))
            { if (child->children && child->children->content)
                s->XGridDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XGridOrigin"))
            { if (child->children && child->children->content)
                s->XGridOrigin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XGridSpacing"))
            { if (child->children && child->children->content)
                s->XGridSpacing = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XRulerDensity"))
            { if (child->children && child->children->content)
                s->XRulerDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XRulerOrigin"))
            { if (child->children && child->children->content)
                s->XRulerOrigin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YGridDensity"))
            { if (child->children && child->children->content)
                s->YGridDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YGridOrigin"))
            { if (child->children && child->children->content)
                s->YGridOrigin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YGridSpacing"))
            { if (child->children && child->children->content)
                s->YGridSpacing = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YRulerDensity"))
            { if (child->children && child->children->content)
                s->YRulerDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YRulerOrigin"))
            { if (child->children && child->children->content)
                s->YRulerOrigin = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Scratch")) {
        struct vdx_Scratch *s;
        if (p) { s = (struct vdx_Scratch *)(p); }
        else { s = g_new0(struct vdx_Scratch, 1); }
        s->children = 0;
        s->type = vdx_types_Scratch;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children && child->children->content)
                s->B = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children && child->children->content)
                s->C = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children && child->children->content)
                s->D = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Shape")) {
        struct vdx_Shape *s;
        if (p) { s = (struct vdx_Shape *)(p); }
        else { s = g_new0(struct vdx_Shape, 1); }
        s->children = 0;
        s->type = vdx_types_Shape;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Del") &&
                     attr->children && attr->children->content)
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Master") &&
                     attr->children && attr->children->content)
                s->Master = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "MasterShape") &&
                     attr->children && attr->children->content)
                s->MasterShape = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
                s->TextStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Type") &&
                     attr->children && attr->children->content)
                s->Type = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UniqueID") &&
                     attr->children && attr->children->content)
                s->UniqueID = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Char"))
            { if (child->children && child->children->content)
                s->Char = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Field"))
            { if (child->children && child->children->content)
                s->Field = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "cp"))
            { if (child->children && child->children->content)
                s->cp = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "fld"))
            { if (child->children && child->children->content)
                s->fld = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "pp"))
            { if (child->children && child->children->content)
                s->pp = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Shapes")) {
        struct vdx_Shapes *s;
        if (p) { s = (struct vdx_Shapes *)(p); }
        else { s = g_new0(struct vdx_Shapes, 1); }
        s->children = 0;
        s->type = vdx_types_Shapes;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "SplineKnot")) {
        struct vdx_SplineKnot *s;
        if (p) { s = (struct vdx_SplineKnot *)(p); }
        else { s = g_new0(struct vdx_SplineKnot, 1); }
        s->children = 0;
        s->type = vdx_types_SplineKnot;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "SplineStart")) {
        struct vdx_SplineStart *s;
        if (p) { s = (struct vdx_SplineStart *)(p); }
        else { s = g_new0(struct vdx_SplineStart, 1); }
        s->children = 0;
        s->type = vdx_types_SplineStart;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children && child->children->content)
                s->B = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children && child->children->content)
                s->C = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children && child->children->content)
                s->D = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "StyleProp")) {
        struct vdx_StyleProp *s;
        if (p) { s = (struct vdx_StyleProp *)(p); }
        else { s = g_new0(struct vdx_StyleProp, 1); }
        s->children = 0;
        s->type = vdx_types_StyleProp;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "EnableFillProps"))
            { if (child->children && child->children->content)
                s->EnableFillProps = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EnableLineProps"))
            { if (child->children && child->children->content)
                s->EnableLineProps = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EnableTextProps"))
            { if (child->children && child->children->content)
                s->EnableTextProps = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HideForApply"))
            { if (child->children && child->children->content)
                s->HideForApply = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "StyleSheet")) {
        struct vdx_StyleSheet *s;
        if (p) { s = (struct vdx_StyleSheet *)(p); }
        else { s = g_new0(struct vdx_StyleSheet, 1); }
        s->children = 0;
        s->type = vdx_types_StyleSheet;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
                s->TextStyle = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Tab")) {
        struct vdx_Tab *s;
        if (p) { s = (struct vdx_Tab *)(p); }
        else { s = g_new0(struct vdx_Tab, 1); }
        s->children = 0;
        s->type = vdx_types_Tab;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Alignment"))
            { if (child->children && child->children->content)
                s->Alignment = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Position"))
            { if (child->children && child->children->content)
                s->Position = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Tabs")) {
        struct vdx_Tabs *s;
        if (p) { s = (struct vdx_Tabs *)(p); }
        else { s = g_new0(struct vdx_Tabs, 1); }
        s->children = 0;
        s->type = vdx_types_Tabs;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Text")) {
        struct vdx_Text *s;
        if (p) { s = (struct vdx_Text *)(p); }
        else { s = g_new0(struct vdx_Text, 1); }
        s->children = 0;
        s->type = vdx_types_Text;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "cp"))
            { if (child->children && child->children->content)
                s->cp = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "TextBlock")) {
        struct vdx_TextBlock *s;
        if (p) { s = (struct vdx_TextBlock *)(p); }
        else { s = g_new0(struct vdx_TextBlock, 1); }
        s->children = 0;
        s->type = vdx_types_TextBlock;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "BottomMargin"))
            { if (child->children && child->children->content)
                s->BottomMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DefaultTabStop"))
            { if (child->children && child->children->content)
                s->DefaultTabStop = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LeftMargin"))
            { if (child->children && child->children->content)
                s->LeftMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "RightMargin"))
            { if (child->children && child->children->content)
                s->RightMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextBkgnd"))
            { if (child->children && child->children->content)
                s->TextBkgnd = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextBkgndTrans"))
            { if (child->children && child->children->content)
                s->TextBkgndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextDirection"))
            { if (child->children && child->children->content)
                s->TextDirection = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TopMargin"))
            { if (child->children && child->children->content)
                s->TopMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "VerticalAlign"))
            { if (child->children && child->children->content)
                s->VerticalAlign = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "TextXForm")) {
        struct vdx_TextXForm *s;
        if (p) { s = (struct vdx_TextXForm *)(p); }
        else { s = g_new0(struct vdx_TextXForm, 1); }
        s->children = 0;
        s->type = vdx_types_TextXForm;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "TxtAngle"))
            { if (child->children && child->children->content)
                s->TxtAngle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtHeight"))
            { if (child->children && child->children->content)
                s->TxtHeight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtLocPinX"))
            { if (child->children && child->children->content)
                s->TxtLocPinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtLocPinY"))
            { if (child->children && child->children->content)
                s->TxtLocPinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtPinX"))
            { if (child->children && child->children->content)
                s->TxtPinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtPinY"))
            { if (child->children && child->children->content)
                s->TxtPinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtWidth"))
            { if (child->children && child->children->content)
                s->TxtWidth = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "User")) {
        struct vdx_User *s;
        if (p) { s = (struct vdx_User *)(p); }
        else { s = g_new0(struct vdx_User, 1); }
        s->children = 0;
        s->type = vdx_types_User;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children && child->children->content)
                s->Prompt = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Value"))
            { if (child->children && child->children->content)
                s->Value = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "VisioDocument")) {
        struct vdx_VisioDocument *s;
        if (p) { s = (struct vdx_VisioDocument *)(p); }
        else { s = g_new0(struct vdx_VisioDocument, 1); }
        s->children = 0;
        s->type = vdx_types_VisioDocument;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "DocLangID") &&
                     attr->children && attr->children->content)
                s->DocLangID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "buildnum") &&
                     attr->children && attr->children->content)
                s->buildnum = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "key") &&
                     attr->children && attr->children->content)
                s->key = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "metric") &&
                     attr->children && attr->children->content)
                s->metric = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "start") &&
                     attr->children && attr->children->content)
                s->start = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "version") &&
                     attr->children && attr->children->content)
                s->version = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "xmlns") &&
                     attr->children && attr->children->content)
                s->xmlns = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "EventList"))
            { if (child->children && child->children->content)
                s->EventList = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Masters"))
            { if (child->children && child->children->content)
                s->Masters = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Window")) {
        struct vdx_Window *s;
        if (p) { s = (struct vdx_Window *)(p); }
        else { s = g_new0(struct vdx_Window, 1); }
        s->children = 0;
        s->type = vdx_types_Window;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ContainerType") &&
                     attr->children && attr->children->content)
                s->ContainerType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Document") &&
                     attr->children && attr->children->content)
                s->Document = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Page") &&
                     attr->children && attr->children->content)
                s->Page = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ParentWindow") &&
                     attr->children && attr->children->content)
                s->ParentWindow = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Sheet") &&
                     attr->children && attr->children->content)
                s->Sheet = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewCenterX") &&
                     attr->children && attr->children->content)
                s->ViewCenterX = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewCenterY") &&
                     attr->children && attr->children->content)
                s->ViewCenterY = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewScale") &&
                     attr->children && attr->children->content)
                s->ViewScale = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowHeight") &&
                     attr->children && attr->children->content)
                s->WindowHeight = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowLeft") &&
                     attr->children && attr->children->content)
                s->WindowLeft = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowState") &&
                     attr->children && attr->children->content)
                s->WindowState = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowTop") &&
                     attr->children && attr->children->content)
                s->WindowTop = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowType") &&
                     attr->children && attr->children->content)
                s->WindowType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "WindowWidth") &&
                     attr->children && attr->children->content)
                s->WindowWidth = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "DynamicGridEnabled"))
            { if (child->children && child->children->content)
                s->DynamicGridEnabled = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "GlueSettings"))
            { if (child->children && child->children->content)
                s->GlueSettings = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowConnectionPoints"))
            { if (child->children && child->children->content)
                s->ShowConnectionPoints = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowGrid"))
            { if (child->children && child->children->content)
                s->ShowGrid = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowGuides"))
            { if (child->children && child->children->content)
                s->ShowGuides = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowPageBreaks"))
            { if (child->children && child->children->content)
                s->ShowPageBreaks = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowRulers"))
            { if (child->children && child->children->content)
                s->ShowRulers = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapExtensions"))
            { if (child->children && child->children->content)
                s->SnapExtensions = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapSettings"))
            { if (child->children && child->children->content)
                s->SnapSettings = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "StencilGroup"))
            { if (child->children && child->children->content)
                s->StencilGroup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "StencilGroupPos"))
            { if (child->children && child->children->content)
                s->StencilGroupPos = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TabSplitterPos"))
            { if (child->children && child->children->content)
                s->TabSplitterPos = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Windows")) {
        struct vdx_Windows *s;
        if (p) { s = (struct vdx_Windows *)(p); }
        else { s = g_new0(struct vdx_Windows, 1); }
        s->children = 0;
        s->type = vdx_types_Windows;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ClientHeight") &&
                     attr->children && attr->children->content)
                s->ClientHeight = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ClientWidth") &&
                     attr->children && attr->children->content)
                s->ClientWidth = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Window"))
            { if (child->children && child->children->content)
                s->Window = atoi((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "XForm")) {
        struct vdx_XForm *s;
        if (p) { s = (struct vdx_XForm *)(p); }
        else { s = g_new0(struct vdx_XForm, 1); }
        s->children = 0;
        s->type = vdx_types_XForm;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Angle"))
            { if (child->children && child->children->content)
                s->Angle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FlipX"))
            { if (child->children && child->children->content)
                s->FlipX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FlipY"))
            { if (child->children && child->children->content)
                s->FlipY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Height"))
            { if (child->children && child->children->content)
                s->Height = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocPinX"))
            { if (child->children && child->children->content)
                s->LocPinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocPinY"))
            { if (child->children && child->children->content)
                s->LocPinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PinX"))
            { if (child->children && child->children->content)
                s->PinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PinY"))
            { if (child->children && child->children->content)
                s->PinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ResizeMode"))
            { if (child->children && child->children->content)
                s->ResizeMode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Width"))
            { if (child->children && child->children->content)
                s->Width = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "XForm1D")) {
        struct vdx_XForm1D *s;
        if (p) { s = (struct vdx_XForm1D *)(p); }
        else { s = g_new0(struct vdx_XForm1D, 1); }
        s->children = 0;
        s->type = vdx_types_XForm1D;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "BeginX"))
            { if (child->children && child->children->content)
                s->BeginX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BeginY"))
            { if (child->children && child->children->content)
                s->BeginY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndX"))
            { if (child->children && child->children->content)
                s->EndX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndY"))
            { if (child->children && child->children->content)
                s->EndY = atof((char *)child->children->content); }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "cp")) {
        struct vdx_cp *s;
        if (p) { s = (struct vdx_cp *)(p); }
        else { s = g_new0(struct vdx_cp, 1); }
        s->children = 0;
        s->type = vdx_types_cp;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "fld")) {
        struct vdx_fld *s;
        if (p) { s = (struct vdx_fld *)(p); }
        else { s = g_new0(struct vdx_fld, 1); }
        s->children = 0;
        s->type = vdx_types_fld;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "pp")) {
        struct vdx_pp *s;
        if (p) { s = (struct vdx_pp *)(p); }
        else { s = g_new0(struct vdx_pp, 1); }
        s->children = 0;
        s->type = vdx_types_pp;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "tp")) {
        struct vdx_tp *s;
        if (p) { s = (struct vdx_tp *)(p); }
        else { s = g_new0(struct vdx_tp, 1); }
        s->children = 0;
        s->type = vdx_types_tp;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->children =
                     g_slist_append(s->children,
                                    vdx_read_object(child, theDoc, 0));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "text")) {
        struct vdx_text *s;
        if (p) { s = (struct vdx_text *)(p); }
        else { s = g_new0(struct vdx_text, 1); }
        s->children = 0;
        s->type = vdx_types_text;
        s->text = (char *)cur->content;
        return s;
    }

    message_error(_("Can't decode object %s"), (char*)cur->name);
    return 0;
}
