/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 *
 * vdx-xml.c: Visio XML import filter for dia
 * Copyright (C) 2006-2007 Ian Redfern
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

/* Generated Wed Jan 24 17:00:55 2007 */
/* From: All.vdx animation_tests.vdx Arrows-2.vdx Arrow & Text samples.vdx BasicShapes.vdx basic_tests.vdx Beispiel 1.vdx Beispiel 2.vdx Beispiel 3.vdx cable loom EL axis.vdx Circle1.vdx Circle2.vdx circle with angles.vdx curve_tests.vdx Drawing2.vdx Electrical system SatMax.vdx Embedded-Pics-1.vdx emf_dump_test2.orig.vdx emf_dump_test2.vdx Entreprise_etat_desire.vdx IMU-DD Ver2.vdx ISAD_page1.vdx ISAD_page2.vdx Line1.vdx Line2.vdx Line3.vdx Line4.vdx Line5.vdx Line6.vdx LombardiWireframe.vdx London-Citibank-Network-detail-02-15-2006.vdx London-Citibank-Network Detail-11-07-2005.vdx London-Citibank-racks-11-04-2005.vdx London-colo-move.vdx London-Colo-Network-detail-11-01-2005.vdx London-Colo-Racks-11-03-2005.vdx Network DiagramV2.vdx pattern_tests.vdx Processflow.vdx Rectangle1.vdx Rectangle2.vdx Rectangle3.vdx Rectangle4.vdx render-test.vdx sample1.vdx Sample2.vdx samp_vdx.vdx Satmax RF path.vdx seq_test.vdx Servo block diagram V2.vdx Servo block diagram V3.vdx Servo block diagram.vdx Sigma-function.vdx SmithWireframe.vdx states.vdx Text1.vdx Text2.vdx Text3.vdx text_tests.vdx Tracking Array -  Level.vdx Tracking Array -  Phase.vdx Wayzata-WAN-Detail.vdx Wayzata-WAN-Overview.vdx WDS Cabling.vdx */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <glib.h>
#include <string.h>
#include <libxml/tree.h>
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
#include "dia-layer.h"

/** Parses an XML object into internal representation
 * @param cur the current XML node
 * @param theDoc the current document (with its colour table)
 * @param p a pointer to the storage to use, or NULL to allocate some
 * @param ctx the context for error/warning messages
 * @return an internal representation, or NULL if unknown
 */
void *
vdx_read_object(xmlNodePtr cur, VDXDocument *theDoc, void *p, DiaContext *ctx)
{
    xmlNodePtr child;
    xmlAttrPtr attr;

    if (!strcmp((char *)cur->name, "Act")) {
        struct vdx_Act *s;
        if (p) { s = (struct vdx_Act *)(p); }
        else { s = g_new0(struct vdx_Act, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Act;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Align")) {
        struct vdx_Align *s;
        if (p) { s = (struct vdx_Align *)(p); }
        else { s = g_new0(struct vdx_Align, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Align;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "ArcTo")) {
        struct vdx_ArcTo *s;
        if (p) { s = (struct vdx_ArcTo *)(p); }
        else { s = g_new0(struct vdx_ArcTo, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_ArcTo;
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
            if (!strcmp((char *)child->name, "A"))
            { if (child->children && child->children->content)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Char")) {
        struct vdx_Char *s;
        if (p) { s = (struct vdx_Char *)(p); }
        else { s = g_new0(struct vdx_Char, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Char;
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
                s->Color = vdx_parse_color((char *)child->children->content, theDoc, ctx); }
            else if (!strcmp((char *)child->name, "ColorTrans"))
            { if (child->children && child->children->content)
                s->ColorTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ComplexScriptFont"))
            { if (child->children && child->children->content)
                s->ComplexScriptFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ComplexScriptSize"))
            { if (child->children && child->children->content)
                s->ComplexScriptSize = atof((char *)child->children->content); }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "ColorEntry")) {
        struct vdx_ColorEntry *s;
        if (p) { s = (struct vdx_ColorEntry *)(p); }
        else { s = g_new0(struct vdx_ColorEntry, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_ColorEntry;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Colors")) {
        struct vdx_Colors *s;
        if (p) { s = (struct vdx_Colors *)(p); }
        else { s = g_new0(struct vdx_Colors, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Colors;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "ColorEntry"))
            { if (child->children && child->children->content)
                s->ColorEntry = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Connect")) {
        struct vdx_Connect *s;
        if (p) { s = (struct vdx_Connect *)(p); }
        else { s = g_new0(struct vdx_Connect, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Connect;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FromCell") &&
                     attr->children && attr->children->content)
                s->FromCell = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "FromPart") &&
                     attr->children && attr->children->content)
            {    s->FromPart = atoi((char *)attr->children->content);
                s->FromPart_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "FromSheet") &&
                     attr->children && attr->children->content)
            {    s->FromSheet = atoi((char *)attr->children->content);
                s->FromSheet_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ToCell") &&
                     attr->children && attr->children->content)
                s->ToCell = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ToPart") &&
                     attr->children && attr->children->content)
            {    s->ToPart = atoi((char *)attr->children->content);
                s->ToPart_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ToSheet") &&
                     attr->children && attr->children->content)
            {    s->ToSheet = atoi((char *)attr->children->content);
                s->ToSheet_exists = TRUE; }
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Connection")) {
        struct vdx_Connection *s;
        if (p) { s = (struct vdx_Connection *)(p); }
        else { s = g_new0(struct vdx_Connection, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Connection;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Connects")) {
        struct vdx_Connects *s;
        if (p) { s = (struct vdx_Connects *)(p); }
        else { s = g_new0(struct vdx_Connects, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Connects;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Connect"))
            { if (child->children && child->children->content)
                s->Connect = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Control")) {
        struct vdx_Control *s;
        if (p) { s = (struct vdx_Control *)(p); }
        else { s = g_new0(struct vdx_Control, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Control;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "CustomProp")) {
        struct vdx_CustomProp *s;
        if (p) { s = (struct vdx_CustomProp *)(p); }
        else { s = g_new0(struct vdx_CustomProp, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_CustomProp;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "CustomProps")) {
        struct vdx_CustomProps *s;
        if (p) { s = (struct vdx_CustomProps *)(p); }
        else { s = g_new0(struct vdx_CustomProps, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_CustomProps;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "CustomProp"))
            { if (child->children && child->children->content)
                s->CustomProp = (char *)child->children->content; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocProps")) {
        struct vdx_DocProps *s;
        if (p) { s = (struct vdx_DocProps *)(p); }
        else { s = g_new0(struct vdx_DocProps, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_DocProps;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocumentProperties")) {
        struct vdx_DocumentProperties *s;
        if (p) { s = (struct vdx_DocumentProperties *)(p); }
        else { s = g_new0(struct vdx_DocumentProperties, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_DocumentProperties;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AlternateNames"))
            { if (child->children && child->children->content)
                s->AlternateNames = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "BuildNumberCreated"))
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocumentSettings")) {
        struct vdx_DocumentSettings *s;
        if (p) { s = (struct vdx_DocumentSettings *)(p); }
        else { s = g_new0(struct vdx_DocumentSettings, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_DocumentSettings;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "DefaultFillStyle") &&
                     attr->children && attr->children->content)
            {    s->DefaultFillStyle = atoi((char *)attr->children->content);
                s->DefaultFillStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "DefaultGuideStyle") &&
                     attr->children && attr->children->content)
            {    s->DefaultGuideStyle = atoi((char *)attr->children->content);
                s->DefaultGuideStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "DefaultLineStyle") &&
                     attr->children && attr->children->content)
            {    s->DefaultLineStyle = atoi((char *)attr->children->content);
                s->DefaultLineStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "DefaultTextStyle") &&
                     attr->children && attr->children->content)
            {    s->DefaultTextStyle = atoi((char *)attr->children->content);
                s->DefaultTextStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "TopPage") &&
                     attr->children && attr->children->content)
            {    s->TopPage = atoi((char *)attr->children->content);
                s->TopPage_exists = TRUE; }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "DocumentSheet")) {
        struct vdx_DocumentSheet *s;
        if (p) { s = (struct vdx_DocumentSheet *)(p); }
        else { s = g_new0(struct vdx_DocumentSheet, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_DocumentSheet;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
            {    s->FillStyle = atoi((char *)attr->children->content);
                s->FillStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
            {    s->LineStyle = atoi((char *)attr->children->content);
                s->LineStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
            {    s->TextStyle = atoi((char *)attr->children->content);
                s->TextStyle_exists = TRUE; }
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Ellipse")) {
        struct vdx_Ellipse *s;
        if (p) { s = (struct vdx_Ellipse *)(p); }
        else { s = g_new0(struct vdx_Ellipse, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Ellipse;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EllipticalArcTo")) {
        struct vdx_EllipticalArcTo *s;
        if (p) { s = (struct vdx_EllipticalArcTo *)(p); }
        else { s = g_new0(struct vdx_EllipticalArcTo, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_EllipticalArcTo;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Event")) {
        struct vdx_Event *s;
        if (p) { s = (struct vdx_Event *)(p); }
        else { s = g_new0(struct vdx_Event, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Event;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EventItem")) {
        struct vdx_EventItem *s;
        if (p) { s = (struct vdx_EventItem *)(p); }
        else { s = g_new0(struct vdx_EventItem, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_EventItem;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Action") &&
                     attr->children && attr->children->content)
                s->Action = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Enabled") &&
                     attr->children && attr->children->content)
                s->Enabled = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "EventCode") &&
                     attr->children && attr->children->content)
            {    s->EventCode = atoi((char *)attr->children->content);
                s->EventCode_exists = TRUE; }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "EventList")) {
        struct vdx_EventList *s;
        if (p) { s = (struct vdx_EventList *)(p); }
        else { s = g_new0(struct vdx_EventList, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_EventList;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "EventItem"))
            { if (child->children && child->children->content)
                s->EventItem = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "FaceName")) {
        struct vdx_FaceName *s;
        if (p) { s = (struct vdx_FaceName *)(p); }
        else { s = g_new0(struct vdx_FaceName, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_FaceName;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "CharSets") &&
                     attr->children && attr->children->content)
                s->CharSets = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Flags") &&
                     attr->children && attr->children->content)
            {    s->Flags = atoi((char *)attr->children->content);
                s->Flags_exists = TRUE; }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "FaceNames")) {
        struct vdx_FaceNames *s;
        if (p) { s = (struct vdx_FaceNames *)(p); }
        else { s = g_new0(struct vdx_FaceNames, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_FaceNames;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "FaceName"))
            { if (child->children && child->children->content)
                s->FaceName = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Field")) {
        struct vdx_Field *s;
        if (p) { s = (struct vdx_Field *)(p); }
        else { s = g_new0(struct vdx_Field, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Field;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Fill")) {
        struct vdx_Fill *s;
        if (p) { s = (struct vdx_Fill *)(p); }
        else { s = g_new0(struct vdx_Fill, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Fill;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "FillBkgnd"))
            { if (child->children && child->children->content)
                s->FillBkgnd = vdx_parse_color((char *)child->children->content, theDoc, ctx); }
            else if (!strcmp((char *)child->name, "FillBkgndTrans"))
            { if (child->children && child->children->content)
                s->FillBkgndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FillForegnd"))
            { if (child->children && child->children->content)
                s->FillForegnd = vdx_parse_color((char *)child->children->content, theDoc, ctx); }
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
                s->ShdwForegnd = vdx_parse_color((char *)child->children->content, theDoc, ctx); }
            else if (!strcmp((char *)child->name, "ShdwForegndTrans"))
            { if (child->children && child->children->content)
                s->ShdwForegndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwPattern"))
            { if (child->children && child->children->content)
                s->ShdwPattern = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "FontEntry")) {
        struct vdx_FontEntry *s;
        if (p) { s = (struct vdx_FontEntry *)(p); }
        else { s = g_new0(struct vdx_FontEntry, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_FontEntry;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Attributes") &&
                     attr->children && attr->children->content)
            {    s->Attributes = atoi((char *)attr->children->content);
                s->Attributes_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "CharSet") &&
                     attr->children && attr->children->content)
            {    s->CharSet = atoi((char *)attr->children->content);
                s->CharSet_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "PitchAndFamily") &&
                     attr->children && attr->children->content)
            {    s->PitchAndFamily = atoi((char *)attr->children->content);
                s->PitchAndFamily_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Unicode") &&
                     attr->children && attr->children->content)
                s->Unicode = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Weight") &&
                     attr->children && attr->children->content)
                s->Weight = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Fonts")) {
        struct vdx_Fonts *s;
        if (p) { s = (struct vdx_Fonts *)(p); }
        else { s = g_new0(struct vdx_Fonts, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Fonts;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "FontEntry"))
            { if (child->children && child->children->content)
                s->FontEntry = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Foreign")) {
        struct vdx_Foreign *s;
        if (p) { s = (struct vdx_Foreign *)(p); }
        else { s = g_new0(struct vdx_Foreign, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Foreign;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "ForeignData")) {
        struct vdx_ForeignData *s;
        if (p) { s = (struct vdx_ForeignData *)(p); }
        else { s = g_new0(struct vdx_ForeignData, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_ForeignData;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "CompressionLevel") &&
                     attr->children && attr->children->content)
                s->CompressionLevel = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "CompressionType") &&
                     attr->children && attr->children->content)
                s->CompressionType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ExtentX") &&
                     attr->children && attr->children->content)
            {    s->ExtentX = atoi((char *)attr->children->content);
                s->ExtentX_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ExtentY") &&
                     attr->children && attr->children->content)
            {    s->ExtentY = atoi((char *)attr->children->content);
                s->ExtentY_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ForeignType") &&
                     attr->children && attr->children->content)
                s->ForeignType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "MappingMode") &&
                     attr->children && attr->children->content)
            {    s->MappingMode = atoi((char *)attr->children->content);
                s->MappingMode_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ObjectHeight") &&
                     attr->children && attr->children->content)
                s->ObjectHeight = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ObjectType") &&
                     attr->children && attr->children->content)
            {    s->ObjectType = atoi((char *)attr->children->content);
                s->ObjectType_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ObjectWidth") &&
                     attr->children && attr->children->content)
                s->ObjectWidth = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ShowAsIcon") &&
                     attr->children && attr->children->content)
                s->ShowAsIcon = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Geom")) {
        struct vdx_Geom *s;
        if (p) { s = (struct vdx_Geom *)(p); }
        else { s = g_new0(struct vdx_Geom, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Geom;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Group")) {
        struct vdx_Group *s;
        if (p) { s = (struct vdx_Group *)(p); }
        else { s = g_new0(struct vdx_Group, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Group;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "HeaderFooter")) {
        struct vdx_HeaderFooter *s;
        if (p) { s = (struct vdx_HeaderFooter *)(p); }
        else { s = g_new0(struct vdx_HeaderFooter, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_HeaderFooter;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "HeaderFooterFont")) {
        struct vdx_HeaderFooterFont *s;
        if (p) { s = (struct vdx_HeaderFooterFont *)(p); }
        else { s = g_new0(struct vdx_HeaderFooterFont, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_HeaderFooterFont;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "CharSet") &&
                     attr->children && attr->children->content)
                s->CharSet = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ClipPrecision") &&
                     attr->children && attr->children->content)
            {    s->ClipPrecision = atoi((char *)attr->children->content);
                s->ClipPrecision_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Escapement") &&
                     attr->children && attr->children->content)
                s->Escapement = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "FaceName") &&
                     attr->children && attr->children->content)
                s->FaceName = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Height") &&
                     attr->children && attr->children->content)
            {    s->Height = atoi((char *)attr->children->content);
                s->Height_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Italic") &&
                     attr->children && attr->children->content)
                s->Italic = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Orientation") &&
                     attr->children && attr->children->content)
                s->Orientation = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "OutPrecision") &&
                     attr->children && attr->children->content)
            {    s->OutPrecision = atoi((char *)attr->children->content);
                s->OutPrecision_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "PitchAndFamily") &&
                     attr->children && attr->children->content)
            {    s->PitchAndFamily = atoi((char *)attr->children->content);
                s->PitchAndFamily_exists = TRUE; }
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
            {    s->Weight = atoi((char *)attr->children->content);
                s->Weight_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Width") &&
                     attr->children && attr->children->content)
                s->Width = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Help")) {
        struct vdx_Help *s;
        if (p) { s = (struct vdx_Help *)(p); }
        else { s = g_new0(struct vdx_Help, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Help;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Hyperlink")) {
        struct vdx_Hyperlink *s;
        if (p) { s = (struct vdx_Hyperlink *)(p); }
        else { s = g_new0(struct vdx_Hyperlink, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Hyperlink;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Icon")) {
        struct vdx_Icon *s;
        if (p) { s = (struct vdx_Icon *)(p); }
        else { s = g_new0(struct vdx_Icon, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Icon;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Image")) {
        struct vdx_Image *s;
        if (p) { s = (struct vdx_Image *)(p); }
        else { s = g_new0(struct vdx_Image, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Image;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "InfiniteLine")) {
        struct vdx_InfiniteLine *s;
        if (p) { s = (struct vdx_InfiniteLine *)(p); }
        else { s = g_new0(struct vdx_InfiniteLine, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_InfiniteLine;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Layer")) {
        struct vdx_Layer *s;
        if (p) { s = (struct vdx_Layer *)(p); }
        else { s = g_new0(struct vdx_Layer, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Layer;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "LayerMem")) {
        struct vdx_LayerMem *s;
        if (p) { s = (struct vdx_LayerMem *)(p); }
        else { s = g_new0(struct vdx_LayerMem, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_LayerMem;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "LayerMember"))
            { if (child->children && child->children->content)
                s->LayerMember = (char *)child->children->content; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Layout")) {
        struct vdx_Layout *s;
        if (p) { s = (struct vdx_Layout *)(p); }
        else { s = g_new0(struct vdx_Layout, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Layout;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Line")) {
        struct vdx_Line *s;
        if (p) { s = (struct vdx_Line *)(p); }
        else { s = g_new0(struct vdx_Line, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Line;
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
                s->LineColor = vdx_parse_color((char *)child->children->content, theDoc, ctx); }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "LineTo")) {
        struct vdx_LineTo *s;
        if (p) { s = (struct vdx_LineTo *)(p); }
        else { s = g_new0(struct vdx_LineTo, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_LineTo;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Master")) {
        struct vdx_Master *s;
        if (p) { s = (struct vdx_Master *)(p); }
        else { s = g_new0(struct vdx_Master, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Master;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "AlignName") &&
                     attr->children && attr->children->content)
            {    s->AlignName = atoi((char *)attr->children->content);
                s->AlignName_exists = TRUE; }
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
            {    s->IconSize = atoi((char *)attr->children->content);
                s->IconSize_exists = TRUE; }
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
            {    s->PatternFlags = atoi((char *)attr->children->content);
                s->PatternFlags_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Prompt") &&
                     attr->children && attr->children->content)
                s->Prompt = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UniqueID") &&
                     attr->children && attr->children->content)
                s->UniqueID = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Misc")) {
        struct vdx_Misc *s;
        if (p) { s = (struct vdx_Misc *)(p); }
        else { s = g_new0(struct vdx_Misc, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Misc;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "MoveTo")) {
        struct vdx_MoveTo *s;
        if (p) { s = (struct vdx_MoveTo *)(p); }
        else { s = g_new0(struct vdx_MoveTo, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_MoveTo;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "NURBSTo")) {
        struct vdx_NURBSTo *s;
        if (p) { s = (struct vdx_NURBSTo *)(p); }
        else { s = g_new0(struct vdx_NURBSTo, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_NURBSTo;
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
            else if (!strcmp((char *)child->name, "E"))
            { if (child->children && child->children->content)
                s->E = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children && child->children->content)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children && child->children->content)
                s->Y = atof((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Page")) {
        struct vdx_Page *s;
        if (p) { s = (struct vdx_Page *)(p); }
        else { s = g_new0(struct vdx_Page, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Page;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "BackPage") &&
                     attr->children && attr->children->content)
            {    s->BackPage = atoi((char *)attr->children->content);
                s->BackPage_exists = TRUE; }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PageLayout")) {
        struct vdx_PageLayout *s;
        if (p) { s = (struct vdx_PageLayout *)(p); }
        else { s = g_new0(struct vdx_PageLayout, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_PageLayout;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PageProps")) {
        struct vdx_PageProps *s;
        if (p) { s = (struct vdx_PageProps *)(p); }
        else { s = g_new0(struct vdx_PageProps, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_PageProps;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PageSheet")) {
        struct vdx_PageSheet *s;
        if (p) { s = (struct vdx_PageSheet *)(p); }
        else { s = g_new0(struct vdx_PageSheet, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_PageSheet;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
            {    s->FillStyle = atoi((char *)attr->children->content);
                s->FillStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
            {    s->LineStyle = atoi((char *)attr->children->content);
                s->LineStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
            {    s->TextStyle = atoi((char *)attr->children->content);
                s->TextStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "UniqueID") &&
                     attr->children && attr->children->content)
                s->UniqueID = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Para")) {
        struct vdx_Para *s;
        if (p) { s = (struct vdx_Para *)(p); }
        else { s = g_new0(struct vdx_Para, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Para;
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
                s->BulletFontSize = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "BulletStr"))
            { if (child->children && child->children->content)
                s->BulletStr = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Flags"))
            { if (child->children && child->children->content)
                s->Flags = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HorzAlign"))
            { if (child->children && child->children->content)
                s->HorzAlign = atoi((char *)child->children->content); }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PolylineTo")) {
        struct vdx_PolylineTo *s;
        if (p) { s = (struct vdx_PolylineTo *)(p); }
        else { s = g_new0(struct vdx_PolylineTo, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_PolylineTo;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PreviewPicture")) {
        struct vdx_PreviewPicture *s;
        if (p) { s = (struct vdx_PreviewPicture *)(p); }
        else { s = g_new0(struct vdx_PreviewPicture, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_PreviewPicture;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Size") &&
                     attr->children && attr->children->content)
            {    s->Size = atoi((char *)attr->children->content);
                s->Size_exists = TRUE; }
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PrintProps")) {
        struct vdx_PrintProps *s;
        if (p) { s = (struct vdx_PrintProps *)(p); }
        else { s = g_new0(struct vdx_PrintProps, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_PrintProps;
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
                s->ScaleX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ScaleY"))
            { if (child->children && child->children->content)
                s->ScaleY = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "PrintSetup")) {
        struct vdx_PrintSetup *s;
        if (p) { s = (struct vdx_PrintSetup *)(p); }
        else { s = g_new0(struct vdx_PrintSetup, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_PrintSetup;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Prop")) {
        struct vdx_Prop *s;
        if (p) { s = (struct vdx_Prop *)(p); }
        else { s = g_new0(struct vdx_Prop, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Prop;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Protection")) {
        struct vdx_Protection *s;
        if (p) { s = (struct vdx_Protection *)(p); }
        else { s = g_new0(struct vdx_Protection, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Protection;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "RulerGrid")) {
        struct vdx_RulerGrid *s;
        if (p) { s = (struct vdx_RulerGrid *)(p); }
        else { s = g_new0(struct vdx_RulerGrid, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_RulerGrid;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Scratch")) {
        struct vdx_Scratch *s;
        if (p) { s = (struct vdx_Scratch *)(p); }
        else { s = g_new0(struct vdx_Scratch, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Scratch;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Shape")) {
        struct vdx_Shape *s;
        if (p) { s = (struct vdx_Shape *)(p); }
        else { s = g_new0(struct vdx_Shape, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Shape;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "Del") &&
                     attr->children && attr->children->content)
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
            {    s->FillStyle = atoi((char *)attr->children->content);
                s->FillStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
            {    s->LineStyle = atoi((char *)attr->children->content);
                s->LineStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Master") &&
                     attr->children && attr->children->content)
            {    s->Master = atoi((char *)attr->children->content);
                s->Master_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "MasterShape") &&
                     attr->children && attr->children->content)
            {    s->MasterShape = atoi((char *)attr->children->content);
                s->MasterShape_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
            {    s->TextStyle = atoi((char *)attr->children->content);
                s->TextStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Type") &&
                     attr->children && attr->children->content)
                s->Type = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UniqueID") &&
                     attr->children && attr->children->content)
                s->UniqueID = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Data1"))
            { if (child->children && child->children->content)
                s->Data1 = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Data2"))
            { if (child->children && child->children->content)
                s->Data2 = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Data3"))
            { if (child->children && child->children->content)
                s->Data3 = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Shapes")) {
        struct vdx_Shapes *s;
        if (p) { s = (struct vdx_Shapes *)(p); }
        else { s = g_new0(struct vdx_Shapes, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Shapes;
        for (attr = cur->properties; attr; attr = attr->next) {
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "SplineKnot")) {
        struct vdx_SplineKnot *s;
        if (p) { s = (struct vdx_SplineKnot *)(p); }
        else { s = g_new0(struct vdx_SplineKnot, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_SplineKnot;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "SplineStart")) {
        struct vdx_SplineStart *s;
        if (p) { s = (struct vdx_SplineStart *)(p); }
        else { s = g_new0(struct vdx_SplineStart, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_SplineStart;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "StyleProp")) {
        struct vdx_StyleProp *s;
        if (p) { s = (struct vdx_StyleProp *)(p); }
        else { s = g_new0(struct vdx_StyleProp, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_StyleProp;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "StyleSheet")) {
        struct vdx_StyleSheet *s;
        if (p) { s = (struct vdx_StyleSheet *)(p); }
        else { s = g_new0(struct vdx_StyleSheet, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_StyleSheet;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "FillStyle") &&
                     attr->children && attr->children->content)
            {    s->FillStyle = atoi((char *)attr->children->content);
                s->FillStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle") &&
                     attr->children && attr->children->content)
            {    s->LineStyle = atoi((char *)attr->children->content);
                s->LineStyle_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU") &&
                     attr->children && attr->children->content)
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle") &&
                     attr->children && attr->children->content)
            {    s->TextStyle = atoi((char *)attr->children->content);
                s->TextStyle_exists = TRUE; }
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Tab")) {
        struct vdx_Tab *s;
        if (p) { s = (struct vdx_Tab *)(p); }
        else { s = g_new0(struct vdx_Tab, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Tab;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Tabs")) {
        struct vdx_Tabs *s;
        if (p) { s = (struct vdx_Tabs *)(p); }
        else { s = g_new0(struct vdx_Tabs, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Tabs;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Text")) {
        struct vdx_Text *s;
        if (p) { s = (struct vdx_Text *)(p); }
        else { s = g_new0(struct vdx_Text, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Text;
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
            else if (!strcmp((char *)child->name, "fld"))
            { if (child->children && child->children->content)
                s->fld = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "pp"))
            { if (child->children && child->children->content)
                s->pp = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "tp"))
            { if (child->children && child->children->content)
                s->tp = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "TextBlock")) {
        struct vdx_TextBlock *s;
        if (p) { s = (struct vdx_TextBlock *)(p); }
        else { s = g_new0(struct vdx_TextBlock, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_TextBlock;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "TextXForm")) {
        struct vdx_TextXForm *s;
        if (p) { s = (struct vdx_TextXForm *)(p); }
        else { s = g_new0(struct vdx_TextXForm, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_TextXForm;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "User")) {
        struct vdx_User *s;
        if (p) { s = (struct vdx_User *)(p); }
        else { s = g_new0(struct vdx_User, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_User;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ID") &&
                     attr->children && attr->children->content)
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name") &&
                     attr->children && attr->children->content)
                s->Name = (char *)attr->children->content;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "VisioDocument")) {
        struct vdx_VisioDocument *s;
        if (p) { s = (struct vdx_VisioDocument *)(p); }
        else { s = g_new0(struct vdx_VisioDocument, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_VisioDocument;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "DocLangID") &&
                     attr->children && attr->children->content)
            {    s->DocLangID = atoi((char *)attr->children->content);
                s->DocLangID_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "buildnum") &&
                     attr->children && attr->children->content)
            {    s->buildnum = atoi((char *)attr->children->content);
                s->buildnum_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "key") &&
                     attr->children && attr->children->content)
                s->key = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "metric") &&
                     attr->children && attr->children->content)
                s->metric = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "start") &&
                     attr->children && attr->children->content)
            {    s->start = atoi((char *)attr->children->content);
                s->start_exists = TRUE; }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Window")) {
        struct vdx_Window *s;
        if (p) { s = (struct vdx_Window *)(p); }
        else { s = g_new0(struct vdx_Window, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Window;
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
            {    s->Page = atoi((char *)attr->children->content);
                s->Page_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ParentWindow") &&
                     attr->children && attr->children->content)
            {    s->ParentWindow = atoi((char *)attr->children->content);
                s->ParentWindow_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ReadOnly") &&
                     attr->children && attr->children->content)
                s->ReadOnly = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Sheet") &&
                     attr->children && attr->children->content)
            {    s->Sheet = atoi((char *)attr->children->content);
                s->Sheet_exists = TRUE; }
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
            {    s->WindowHeight = atoi((char *)attr->children->content);
                s->WindowHeight_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "WindowLeft") &&
                     attr->children && attr->children->content)
            {    s->WindowLeft = atoi((char *)attr->children->content);
                s->WindowLeft_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "WindowState") &&
                     attr->children && attr->children->content)
            {    s->WindowState = atoi((char *)attr->children->content);
                s->WindowState_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "WindowTop") &&
                     attr->children && attr->children->content)
            {    s->WindowTop = atoi((char *)attr->children->content);
                s->WindowTop_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "WindowType") &&
                     attr->children && attr->children->content)
                s->WindowType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "WindowWidth") &&
                     attr->children && attr->children->content)
            {    s->WindowWidth = atoi((char *)attr->children->content);
                s->WindowWidth_exists = TRUE; }
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "Windows")) {
        struct vdx_Windows *s;
        if (p) { s = (struct vdx_Windows *)(p); }
        else { s = g_new0(struct vdx_Windows, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_Windows;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "ClientHeight") &&
                     attr->children && attr->children->content)
            {    s->ClientHeight = atoi((char *)attr->children->content);
                s->ClientHeight_exists = TRUE; }
            else if (!strcmp((char *)attr->name, "ClientWidth") &&
                     attr->children && attr->children->content)
            {    s->ClientWidth = atoi((char *)attr->children->content);
                s->ClientWidth_exists = TRUE; }
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Window"))
            { if (child->children && child->children->content)
                s->Window = atoi((char *)child->children->content); }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "XForm")) {
        struct vdx_XForm *s;
        if (p) { s = (struct vdx_XForm *)(p); }
        else { s = g_new0(struct vdx_XForm, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_XForm;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "XForm1D")) {
        struct vdx_XForm1D *s;
        if (p) { s = (struct vdx_XForm1D *)(p); }
        else { s = g_new0(struct vdx_XForm1D, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_XForm1D;
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
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "cp")) {
        struct vdx_cp *s;
        if (p) { s = (struct vdx_cp *)(p); }
        else { s = g_new0(struct vdx_cp, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_cp;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "fld")) {
        struct vdx_fld *s;
        if (p) { s = (struct vdx_fld *)(p); }
        else { s = g_new0(struct vdx_fld, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_fld;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "pp")) {
        struct vdx_pp *s;
        if (p) { s = (struct vdx_pp *)(p); }
        else { s = g_new0(struct vdx_pp, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_pp;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "tp")) {
        struct vdx_tp *s;
        if (p) { s = (struct vdx_tp *)(p); }
        else { s = g_new0(struct vdx_tp, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_tp;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX") &&
                     attr->children && attr->children->content)
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            else s->any.children =
                     g_slist_append(s->any.children,
                                    vdx_read_object(child, theDoc, 0, ctx));
        }
        return s;
    }

    if (!strcmp((char *)cur->name, "text")) {
        struct vdx_text *s;
        if (p) { s = (struct vdx_text *)(p); }
        else { s = g_new0(struct vdx_text, 1); }
        s->any.children = 0;
        s->any.type = vdx_types_text;
        s->text = (char *)cur->content;
        return s;
    }

    dia_context_add_message(ctx, _("Can't decode object %s"), (char*)cur->name);
    return 0;
}

/** Writes an object in internal representation as XML
 * @param file the output file
 * @param depth tag depth for pretty-printing
 * @param p a pointer to the object
 */

void
vdx_write_object(FILE *file, unsigned int depth, const void *p)
{
    const struct vdx_any *Any = (const struct vdx_any*)p;
    const GSList *child = Any->children;

    const struct vdx_Act *Act;
    const struct vdx_Align *Align;
    const struct vdx_ArcTo *ArcTo;
    const struct vdx_Char *Char;
    const struct vdx_ColorEntry *ColorEntry;
    const struct vdx_Colors *Colors;
    const struct vdx_Connect *Connect;
    const struct vdx_Connection *vdxConnection;
    const struct vdx_Connects *Connects;
    const struct vdx_Control *Control;
    const struct vdx_CustomProp *CustomProp;
    const struct vdx_CustomProps *CustomProps;
    const struct vdx_DocProps *DocProps;
    const struct vdx_DocumentProperties *DocumentProperties;
    const struct vdx_DocumentSettings *DocumentSettings;
    const struct vdx_DocumentSheet *DocumentSheet;
    const struct vdx_Ellipse *Ellipse;
    const struct vdx_EllipticalArcTo *EllipticalArcTo;
    const struct vdx_Event *Event;
    const struct vdx_EventItem *EventItem;
    const struct vdx_EventList *EventList;
    const struct vdx_FaceName *FaceName;
    const struct vdx_FaceNames *FaceNames;
    const struct vdx_Field *Field;
    const struct vdx_Fill *Fill;
    const struct vdx_FontEntry *FontEntry;
    const struct vdx_Fonts *Fonts;
    const struct vdx_Foreign *Foreign;
    const struct vdx_ForeignData *ForeignData;
    const struct vdx_Geom *Geom;
    const struct vdx_Group *vdxGroup;
    const struct vdx_HeaderFooter *HeaderFooter;
    const struct vdx_HeaderFooterFont *HeaderFooterFont;
    const struct vdx_Help *Help;
    const struct vdx_Hyperlink *Hyperlink;
    const struct vdx_Icon *Icon;
    const struct vdx_Image *Image;
    const struct vdx_InfiniteLine *InfiniteLine;
    const struct vdx_Layer *Layer;
    const struct vdx_LayerMem *LayerMem;
    const struct vdx_Layout *Layout;
    const struct vdx_Line *Line;
    const struct vdx_LineTo *LineTo;
    const struct vdx_Master *Master;
    const struct vdx_Misc *Misc;
    const struct vdx_MoveTo *MoveTo;
    const struct vdx_NURBSTo *NURBSTo;
    const struct vdx_Page *Page;
    const struct vdx_PageLayout *PageLayout;
    const struct vdx_PageProps *PageProps;
    const struct vdx_PageSheet *PageSheet;
    const struct vdx_Para *Para;
    const struct vdx_PolylineTo *PolylineTo;
    const struct vdx_PreviewPicture *PreviewPicture;
    const struct vdx_PrintProps *PrintProps;
    const struct vdx_PrintSetup *PrintSetup;
    const struct vdx_Prop *Prop;
    const struct vdx_Protection *Protection;
    const struct vdx_RulerGrid *RulerGrid;
    const struct vdx_Scratch *Scratch;
    const struct vdx_Shape *Shape;
    const struct vdx_Shapes *vdxShapes;
    const struct vdx_SplineKnot *SplineKnot;
    const struct vdx_SplineStart *SplineStart;
    const struct vdx_StyleProp *StyleProp;
    const struct vdx_StyleSheet *StyleSheet;
    const struct vdx_Tab *Tab;
    const struct vdx_Tabs *Tabs;
    const struct vdx_Text *vdxText;
    const struct vdx_TextBlock *TextBlock;
    const struct vdx_TextXForm *TextXForm;
    const struct vdx_User *User;
    const struct vdx_VisioDocument *VisioDocument;
    const struct vdx_Window *Window;
    const struct vdx_Windows *Windows;
    const struct vdx_XForm *XForm;
    const struct vdx_XForm1D *XForm1D;
    const struct vdx_cp *cp;
    const struct vdx_fld *fld;
    const struct vdx_pp *pp;
    const struct vdx_tp *tp;
    const struct vdx_text *text;
    char *pad = (char *)g_alloca(2*depth+1);
    unsigned int i;
    for (i=0; i<2*depth; i++) { pad[i] = ' '; }
    pad[2*depth] = 0;

    switch (Any->type)
    {
    case vdx_types_Act:
        Act = (const struct vdx_Act *)(p);
        fprintf(file, "%s<Act ID='%u' IX='%u'", pad, Act->ID, Act->IX);
        if (Act->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(Act->Name));
        if (Act->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(Act->NameU));
        fprintf(file, ">\n");
        fprintf(file, "%s  <Action>%f</Action>\n", pad,
                Act->Action);
        fprintf(file, "%s  <BeginGroup>%u</BeginGroup>\n", pad,
                Act->BeginGroup);
        fprintf(file, "%s  <ButtonFace>%u</ButtonFace>\n", pad,
                Act->ButtonFace);
        fprintf(file, "%s  <Checked>%u</Checked>\n", pad,
                Act->Checked);
        fprintf(file, "%s  <Disabled>%u</Disabled>\n", pad,
                Act->Disabled);
        fprintf(file, "%s  <Invisible>%u</Invisible>\n", pad,
                Act->Invisible);
        fprintf(file, "%s  <Menu>%s</Menu>\n", pad,
                vdx_convert_xml_string(Act->Menu));
        fprintf(file, "%s  <ReadOnly>%u</ReadOnly>\n", pad,
                Act->ReadOnly);
        fprintf(file, "%s  <SortKey>%u</SortKey>\n", pad,
                Act->SortKey);
        fprintf(file, "%s  <TagName>%u</TagName>\n", pad,
                Act->TagName);
        break;

    case vdx_types_Align:
        Align = (const struct vdx_Align *)(p);
        fprintf(file, "%s<Align>\n", pad);
        fprintf(file, "%s  <AlignBottom>%f</AlignBottom>\n", pad,
                Align->AlignBottom);
        fprintf(file, "%s  <AlignCenter>%f</AlignCenter>\n", pad,
                Align->AlignCenter);
        fprintf(file, "%s  <AlignLeft>%u</AlignLeft>\n", pad,
                Align->AlignLeft);
        fprintf(file, "%s  <AlignMiddle>%u</AlignMiddle>\n", pad,
                Align->AlignMiddle);
        fprintf(file, "%s  <AlignRight>%u</AlignRight>\n", pad,
                Align->AlignRight);
        fprintf(file, "%s  <AlignTop>%f</AlignTop>\n", pad,
                Align->AlignTop);
        break;

    case vdx_types_ArcTo:
        ArcTo = (const struct vdx_ArcTo *)(p);
        fprintf(file, "%s<ArcTo IX='%u'", pad, ArcTo->IX);
        if (ArcTo->Del)
            fprintf(file, " Del='%u'",
                    ArcTo->Del);
        fprintf(file, ">\n");
        fprintf(file, "%s  <A>%f</A>\n", pad,
                ArcTo->A);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                ArcTo->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                ArcTo->Y);
        break;

    case vdx_types_Char:
        Char = (const struct vdx_Char *)(p);
        fprintf(file, "%s<Char IX='%u'", pad, Char->IX);
        if (Char->Del)
            fprintf(file, " Del='%u'",
                    Char->Del);
        fprintf(file, ">\n");
	if (Char->AsianFont)
          fprintf(file, "%s  <AsianFont>%u</AsianFont>\n", pad,
                  Char->AsianFont);
	if (Char->Case)
          fprintf(file, "%s  <Case>%u</Case>\n", pad,
                  Char->Case);
        fprintf(file, "%s  <Color>%s</Color>\n", pad,
                vdx_string_color(Char->Color));
	if (Char->ColorTrans)
          fprintf(file, "%s  <ColorTrans>%f</ColorTrans>\n", pad,
                  Char->ColorTrans);
	if (Char->ComplexScriptFont)
          fprintf(file, "%s  <ComplexScriptFont>%u</ComplexScriptFont>\n", pad,
                  Char->ComplexScriptFont);
	if (Char->ComplexScriptSize)
          fprintf(file, "%s  <ComplexScriptSize>%f</ComplexScriptSize>\n", pad,
                  Char->ComplexScriptSize);
	if (Char->DblUnderline)
          fprintf(file, "%s  <DblUnderline>%u</DblUnderline>\n", pad,
                  Char->DblUnderline);
	if (Char->DoubleStrikethrough)
          fprintf(file, "%s  <DoubleStrikethrough>%u</DoubleStrikethrough>\n", pad,
                  Char->DoubleStrikethrough);
	/* although stated as such, not optional! */
        fprintf(file, "%s  <Font>%u</Font>\n", pad,
                Char->Font);
	if (Char->FontScale)
          fprintf(file, "%s  <FontScale>%f</FontScale>\n", pad,
                  Char->FontScale);
	if (Char->Highlight)
          fprintf(file, "%s  <Highlight>%u</Highlight>\n", pad,
                  Char->Highlight);
	if (Char->LangID)
          fprintf(file, "%s  <LangID>%u</LangID>\n", pad,
                  Char->LangID);
	if (Char->Letterspace)
          fprintf(file, "%s  <Letterspace>%f</Letterspace>\n", pad,
                  Char->Letterspace);
	if (Char->Locale)
          fprintf(file, "%s  <Locale>%u</Locale>\n", pad,
                  Char->Locale);
	if (Char->LocalizeFont)
          fprintf(file, "%s  <LocalizeFont>%u</LocalizeFont>\n", pad,
                  Char->LocalizeFont);
	if (Char->Overline)
          fprintf(file, "%s  <Overline>%u</Overline>\n", pad,
                  Char->Overline);
	if (Char->Perpendicular)
          fprintf(file, "%s  <Perpendicular>%u</Perpendicular>\n", pad,
                  Char->Perpendicular);
	if (Char->Pos)
          fprintf(file, "%s  <Pos>%u</Pos>\n", pad,
                  Char->Pos);
	if (Char->RTLText)
          fprintf(file, "%s  <RTLText>%u</RTLText>\n", pad,
                  Char->RTLText);
	if (Char->Size)
          fprintf(file, "%s  <Size>%f</Size>\n", pad,
                  Char->Size);
	if (Char->Strikethru)
          fprintf(file, "%s  <Strikethru>%u</Strikethru>\n", pad,
                  Char->Strikethru);
	if (Char->Style)
          fprintf(file, "%s  <Style>%u</Style>\n", pad,
                  Char->Style);
	if (Char->UseVertical)
          fprintf(file, "%s  <UseVertical>%u</UseVertical>\n", pad,
                  Char->UseVertical);
        break;

    case vdx_types_ColorEntry:
        ColorEntry = (const struct vdx_ColorEntry *)(p);
        fprintf(file, "%s<ColorEntry IX='%u' RGB='%s'", pad, ColorEntry->IX, vdx_convert_xml_string(ColorEntry->RGB));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Colors:
        Colors = (const struct vdx_Colors *)(p);
        fprintf(file, "%s<Colors>\n", pad);
        fprintf(file, "%s  <ColorEntry>%u</ColorEntry>\n", pad,
                Colors->ColorEntry);
        break;

    case vdx_types_Connect:
        Connect = (const struct vdx_Connect *)(p);
        fprintf(file, "%s<Connect FromCell='%s' ToCell='%s'", pad, vdx_convert_xml_string(Connect->FromCell), vdx_convert_xml_string(Connect->ToCell));
        if (Connect->FromPart_exists)
            fprintf(file, " FromPart='%u'",
                    Connect->FromPart);
        if (Connect->FromSheet_exists)
            fprintf(file, " FromSheet='%u'",
                    Connect->FromSheet);
        if (Connect->ToPart_exists)
            fprintf(file, " ToPart='%u'",
                    Connect->ToPart);
        if (Connect->ToSheet_exists)
            fprintf(file, " ToSheet='%u'",
                    Connect->ToSheet);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Connection:
        vdxConnection = (const struct vdx_Connection *)(p);
        fprintf(file, "%s<Connection ID='%u' IX='%u'", pad, vdxConnection->ID, vdxConnection->IX);
        if (vdxConnection->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(vdxConnection->NameU));
        fprintf(file, ">\n");
        fprintf(file, "%s  <AutoGen>%u</AutoGen>\n", pad,
                vdxConnection->AutoGen);
        fprintf(file, "%s  <DirX>%f</DirX>\n", pad,
                vdxConnection->DirX);
        fprintf(file, "%s  <DirY>%f</DirY>\n", pad,
                vdxConnection->DirY);
        fprintf(file, "%s  <Prompt>%s</Prompt>\n", pad,
                vdx_convert_xml_string(vdxConnection->Prompt));
        fprintf(file, "%s  <Type>%u</Type>\n", pad,
                vdxConnection->Type);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                vdxConnection->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                vdxConnection->Y);
        break;

    case vdx_types_Connects:
        Connects = (const struct vdx_Connects *)(p);
        fprintf(file, "%s<Connects>\n", pad);
        fprintf(file, "%s  <Connect>%u</Connect>\n", pad,
                Connects->Connect);
        break;

    case vdx_types_Control:
        Control = (const struct vdx_Control *)(p);
        fprintf(file, "%s<Control ID='%u' IX='%u'", pad, Control->ID, Control->IX);
        if (Control->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(Control->NameU));
        fprintf(file, ">\n");
        fprintf(file, "%s  <CanGlue>%u</CanGlue>\n", pad,
                Control->CanGlue);
        fprintf(file, "%s  <Prompt>%s</Prompt>\n", pad,
                vdx_convert_xml_string(Control->Prompt));
        fprintf(file, "%s  <X>%f</X>\n", pad,
                Control->X);
        fprintf(file, "%s  <XCon>%f</XCon>\n", pad,
                Control->XCon);
        fprintf(file, "%s  <XDyn>%f</XDyn>\n", pad,
                Control->XDyn);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                Control->Y);
        fprintf(file, "%s  <YCon>%f</YCon>\n", pad,
                Control->YCon);
        fprintf(file, "%s  <YDyn>%f</YDyn>\n", pad,
                Control->YDyn);
        break;

    case vdx_types_CustomProp:
        CustomProp = (const struct vdx_CustomProp *)(p);
        fprintf(file, "%s<CustomProp PropType='%s'", pad, vdx_convert_xml_string(CustomProp->PropType));
        if (CustomProp->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(CustomProp->Name));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_CustomProps:
        CustomProps = (const struct vdx_CustomProps *)(p);
        fprintf(file, "%s<CustomProps>\n", pad);
        fprintf(file, "%s  <CustomProp>%s</CustomProp>\n", pad,
                vdx_convert_xml_string(CustomProps->CustomProp));
        break;

    case vdx_types_DocProps:
        DocProps = (const struct vdx_DocProps *)(p);
        fprintf(file, "%s<DocProps>\n", pad);
        fprintf(file, "%s  <AddMarkup>%u</AddMarkup>\n", pad,
                DocProps->AddMarkup);
        fprintf(file, "%s  <DocLangID>%u</DocLangID>\n", pad,
                DocProps->DocLangID);
        fprintf(file, "%s  <LockPreview>%u</LockPreview>\n", pad,
                DocProps->LockPreview);
        fprintf(file, "%s  <OutputFormat>%u</OutputFormat>\n", pad,
                DocProps->OutputFormat);
        fprintf(file, "%s  <PreviewQuality>%u</PreviewQuality>\n", pad,
                DocProps->PreviewQuality);
        fprintf(file, "%s  <PreviewScope>%u</PreviewScope>\n", pad,
                DocProps->PreviewScope);
        fprintf(file, "%s  <ViewMarkup>%u</ViewMarkup>\n", pad,
                DocProps->ViewMarkup);
        break;

    case vdx_types_DocumentProperties:
        DocumentProperties = (const struct vdx_DocumentProperties *)(p);
        fprintf(file, "%s<DocumentProperties>\n", pad);
        fprintf(file, "%s  <AlternateNames>%s</AlternateNames>\n", pad,
                vdx_convert_xml_string(DocumentProperties->AlternateNames));
        fprintf(file, "%s  <BuildNumberCreated>%d)</BuildNumberCreated>\n", pad,
                DocumentProperties->BuildNumberCreated);
        fprintf(file, "%s  <BuildNumberEdited>%u</BuildNumberEdited>\n", pad,
                DocumentProperties->BuildNumberEdited);
        fprintf(file, "%s  <Company>%s</Company>\n", pad,
                vdx_convert_xml_string(DocumentProperties->Company));
        fprintf(file, "%s  <Creator>%s</Creator>\n", pad,
                vdx_convert_xml_string(DocumentProperties->Creator));
        fprintf(file, "%s  <Desc>%s</Desc>\n", pad,
                vdx_convert_xml_string(DocumentProperties->Desc));
        fprintf(file, "%s  <Subject>%s</Subject>\n", pad,
                vdx_convert_xml_string(DocumentProperties->Subject));
        fprintf(file, "%s  <Template>%s</Template>\n", pad,
                vdx_convert_xml_string(DocumentProperties->Template));
        fprintf(file, "%s  <TimeCreated>%s</TimeCreated>\n", pad,
                vdx_convert_xml_string(DocumentProperties->TimeCreated));
        fprintf(file, "%s  <TimeEdited>%s</TimeEdited>\n", pad,
                vdx_convert_xml_string(DocumentProperties->TimeEdited));
        fprintf(file, "%s  <TimePrinted>%s</TimePrinted>\n", pad,
                vdx_convert_xml_string(DocumentProperties->TimePrinted));
        fprintf(file, "%s  <TimeSaved>%s</TimeSaved>\n", pad,
                vdx_convert_xml_string(DocumentProperties->TimeSaved));
        fprintf(file, "%s  <Title>%s</Title>\n", pad,
                vdx_convert_xml_string(DocumentProperties->Title));
        break;

    case vdx_types_DocumentSettings:
        DocumentSettings = (const struct vdx_DocumentSettings *)(p);
        fprintf(file, "%s<DocumentSettings", pad);
        if (DocumentSettings->DefaultFillStyle_exists)
            fprintf(file, " DefaultFillStyle='%u'",
                    DocumentSettings->DefaultFillStyle);
        if (DocumentSettings->DefaultGuideStyle_exists)
            fprintf(file, " DefaultGuideStyle='%u'",
                    DocumentSettings->DefaultGuideStyle);
        if (DocumentSettings->DefaultLineStyle_exists)
            fprintf(file, " DefaultLineStyle='%u'",
                    DocumentSettings->DefaultLineStyle);
        if (DocumentSettings->DefaultTextStyle_exists)
            fprintf(file, " DefaultTextStyle='%u'",
                    DocumentSettings->DefaultTextStyle);
        if (DocumentSettings->TopPage_exists)
            fprintf(file, " TopPage='%u'",
                    DocumentSettings->TopPage);
        fprintf(file, ">\n");
        fprintf(file, "%s  <DynamicGridEnabled>%u</DynamicGridEnabled>\n", pad,
                DocumentSettings->DynamicGridEnabled);
        fprintf(file, "%s  <GlueSettings>%u</GlueSettings>\n", pad,
                DocumentSettings->GlueSettings);
        fprintf(file, "%s  <ProtectBkgnds>%u</ProtectBkgnds>\n", pad,
                DocumentSettings->ProtectBkgnds);
        fprintf(file, "%s  <ProtectMasters>%u</ProtectMasters>\n", pad,
                DocumentSettings->ProtectMasters);
        fprintf(file, "%s  <ProtectShapes>%u</ProtectShapes>\n", pad,
                DocumentSettings->ProtectShapes);
        fprintf(file, "%s  <ProtectStyles>%u</ProtectStyles>\n", pad,
                DocumentSettings->ProtectStyles);
        fprintf(file, "%s  <SnapExtensions>%u</SnapExtensions>\n", pad,
                DocumentSettings->SnapExtensions);
        fprintf(file, "%s  <SnapSettings>%u</SnapSettings>\n", pad,
                DocumentSettings->SnapSettings);
        break;

    case vdx_types_DocumentSheet:
        DocumentSheet = (const struct vdx_DocumentSheet *)(p);
        fprintf(file, "%s<DocumentSheet", pad);
        if (DocumentSheet->FillStyle_exists)
            fprintf(file, " FillStyle='%u'",
                    DocumentSheet->FillStyle);
        if (DocumentSheet->LineStyle_exists)
            fprintf(file, " LineStyle='%u'",
                    DocumentSheet->LineStyle);
        if (DocumentSheet->TextStyle_exists)
            fprintf(file, " TextStyle='%u'",
                    DocumentSheet->TextStyle);
        if (DocumentSheet->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(DocumentSheet->Name));
        if (DocumentSheet->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(DocumentSheet->NameU));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Ellipse:
        Ellipse = (const struct vdx_Ellipse *)(p);
        fprintf(file, "%s<Ellipse IX='%u'>\n", pad, Ellipse->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                Ellipse->A);
        fprintf(file, "%s  <B>%f</B>\n", pad,
                Ellipse->B);
        fprintf(file, "%s  <C>%f</C>\n", pad,
                Ellipse->C);
        fprintf(file, "%s  <D>%f</D>\n", pad,
                Ellipse->D);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                Ellipse->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                Ellipse->Y);
        break;

    case vdx_types_EllipticalArcTo:
        EllipticalArcTo = (const struct vdx_EllipticalArcTo *)(p);
        fprintf(file, "%s<EllipticalArcTo IX='%u'>\n", pad, EllipticalArcTo->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                EllipticalArcTo->A);
        fprintf(file, "%s  <B>%f</B>\n", pad,
                EllipticalArcTo->B);
        fprintf(file, "%s  <C>%f</C>\n", pad,
                EllipticalArcTo->C);
        fprintf(file, "%s  <D>%f</D>\n", pad,
                EllipticalArcTo->D);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                EllipticalArcTo->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                EllipticalArcTo->Y);
        break;

    case vdx_types_Event:
        Event = (const struct vdx_Event *)(p);
        fprintf(file, "%s<Event>\n", pad);
        fprintf(file, "%s  <EventDblClick>%u</EventDblClick>\n", pad,
                Event->EventDblClick);
        fprintf(file, "%s  <EventDrop>%f</EventDrop>\n", pad,
                Event->EventDrop);
        fprintf(file, "%s  <EventXFMod>%u</EventXFMod>\n", pad,
                Event->EventXFMod);
        fprintf(file, "%s  <TheData>%u</TheData>\n", pad,
                Event->TheData);
        fprintf(file, "%s  <TheText>%u</TheText>\n", pad,
                Event->TheText);
        break;

    case vdx_types_EventItem:
        EventItem = (const struct vdx_EventItem *)(p);
        fprintf(file, "%s<EventItem Action='%u' Enabled='%u' ID='%u' Target='%s' TargetArgs='%s'", pad, EventItem->Action, EventItem->Enabled, EventItem->ID, vdx_convert_xml_string(EventItem->Target), vdx_convert_xml_string(EventItem->TargetArgs));
        if (EventItem->EventCode_exists)
            fprintf(file, " EventCode='%u'",
                    EventItem->EventCode);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_EventList:
        EventList = (const struct vdx_EventList *)(p);
        fprintf(file, "%s<EventList>\n", pad);
        fprintf(file, "%s  <EventItem>%u</EventItem>\n", pad,
                EventList->EventItem);
        break;

    case vdx_types_FaceName:
        FaceName = (const struct vdx_FaceName *)(p);
        fprintf(file, "%s<FaceName CharSets='%s' ID='%u' Panos='%s' UnicodeRanges='%s'", pad, vdx_convert_xml_string(FaceName->CharSets), FaceName->ID, vdx_convert_xml_string(FaceName->Panos), vdx_convert_xml_string(FaceName->UnicodeRanges));
        if (FaceName->Flags_exists)
            fprintf(file, " Flags='%u'",
                    FaceName->Flags);
        if (FaceName->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(FaceName->Name));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_FaceNames:
        FaceNames = (const struct vdx_FaceNames *)(p);
        fprintf(file, "%s<FaceNames>\n", pad);
        fprintf(file, "%s  <FaceName>%u</FaceName>\n", pad,
                FaceNames->FaceName);
        break;

    case vdx_types_Field:
        Field = (const struct vdx_Field *)(p);
        fprintf(file, "%s<Field IX='%u'", pad, Field->IX);
        if (Field->Del)
            fprintf(file, " Del='%u'",
                    Field->Del);
        fprintf(file, ">\n");
        fprintf(file, "%s  <Calendar>%u</Calendar>\n", pad,
                Field->Calendar);
        fprintf(file, "%s  <EditMode>%u</EditMode>\n", pad,
                Field->EditMode);
        fprintf(file, "%s  <Format>%f</Format>\n", pad,
                Field->Format);
        fprintf(file, "%s  <ObjectKind>%u</ObjectKind>\n", pad,
                Field->ObjectKind);
        fprintf(file, "%s  <Type>%u</Type>\n", pad,
                Field->Type);
        fprintf(file, "%s  <UICat>%u</UICat>\n", pad,
                Field->UICat);
        fprintf(file, "%s  <UICod>%u</UICod>\n", pad,
                Field->UICod);
        fprintf(file, "%s  <UIFmt>%u</UIFmt>\n", pad,
                Field->UIFmt);
        fprintf(file, "%s  <Value>%f</Value>\n", pad,
                Field->Value);
        break;

    case vdx_types_Fill:
        Fill = (const struct vdx_Fill *)(p);
        fprintf(file, "%s<Fill>\n", pad);
        fprintf(file, "%s  <FillBkgnd>%s</FillBkgnd>\n", pad,
                vdx_string_color(Fill->FillBkgnd));
	if (Fill->FillBkgndTrans)
          fprintf(file, "%s  <FillBkgndTrans>%f</FillBkgndTrans>\n", pad,
                  Fill->FillBkgndTrans);
        fprintf(file, "%s  <FillForegnd>%s</FillForegnd>\n", pad,
                vdx_string_color(Fill->FillForegnd));
	if (Fill->FillForegndTrans)
          fprintf(file, "%s  <FillForegndTrans>%f</FillForegndTrans>\n", pad,
                  Fill->FillForegndTrans);
	if (Fill->FillPattern)
          fprintf(file, "%s  <FillPattern>%u</FillPattern>\n", pad,
                  Fill->FillPattern);
	if (Fill->ShapeShdwObliqueAngle)
          fprintf(file, "%s  <ShapeShdwObliqueAngle>%f</ShapeShdwObliqueAngle>\n", pad,
                  Fill->ShapeShdwObliqueAngle);
	if (Fill->ShapeShdwOffsetX)
          fprintf(file, "%s  <ShapeShdwOffsetX>%f</ShapeShdwOffsetX>\n", pad,
                  Fill->ShapeShdwOffsetX);
	if (Fill->ShapeShdwOffsetY)
          fprintf(file, "%s  <ShapeShdwOffsetY>%f</ShapeShdwOffsetY>\n", pad,
                  Fill->ShapeShdwOffsetY);
	if (Fill->ShapeShdwScaleFactor)
          fprintf(file, "%s  <ShapeShdwScaleFactor>%f</ShapeShdwScaleFactor>\n", pad,
                  Fill->ShapeShdwScaleFactor);
	if (Fill->ShapeShdwType)
          fprintf(file, "%s  <ShapeShdwType>%u</ShapeShdwType>\n", pad,
                  Fill->ShapeShdwType);
	if (Fill->ShdwBkgnd)
          fprintf(file, "%s  <ShdwBkgnd>%u</ShdwBkgnd>\n", pad,
                  Fill->ShdwBkgnd);
	if (Fill->ShdwBkgndTrans)
          fprintf(file, "%s  <ShdwBkgndTrans>%f</ShdwBkgndTrans>\n", pad,
                  Fill->ShdwBkgndTrans);
        fprintf(file, "%s  <ShdwForegnd>%s</ShdwForegnd>\n", pad,
                vdx_string_color(Fill->ShdwForegnd));
	if (Fill->ShdwForegndTrans)
          fprintf(file, "%s  <ShdwForegndTrans>%f</ShdwForegndTrans>\n", pad,
                  Fill->ShdwForegndTrans);
	if (Fill->ShdwPattern)
          fprintf(file, "%s  <ShdwPattern>%u</ShdwPattern>\n", pad,
                  Fill->ShdwPattern);
        break;

    case vdx_types_FontEntry:
        FontEntry = (const struct vdx_FontEntry *)(p);
        fprintf(file, "%s<FontEntry ID='%u' Unicode='%u' Weight='%u'", pad, FontEntry->ID, FontEntry->Unicode, FontEntry->Weight);
        if (FontEntry->Attributes_exists)
            fprintf(file, " Attributes='%u'",
                    FontEntry->Attributes);
        if (FontEntry->CharSet_exists)
            fprintf(file, " CharSet='%u'",
                    FontEntry->CharSet);
        if (FontEntry->PitchAndFamily_exists)
            fprintf(file, " PitchAndFamily='%u'",
                    FontEntry->PitchAndFamily);
        if (FontEntry->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(FontEntry->Name));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Fonts:
        Fonts = (const struct vdx_Fonts *)(p);
        fprintf(file, "%s<Fonts>\n", pad);
        fprintf(file, "%s  <FontEntry>%u</FontEntry>\n", pad,
                Fonts->FontEntry);
        break;

    case vdx_types_Foreign:
        Foreign = (const struct vdx_Foreign *)(p);
        fprintf(file, "%s<Foreign>\n", pad);
        fprintf(file, "%s  <ImgHeight>%f</ImgHeight>\n", pad,
                Foreign->ImgHeight);
        fprintf(file, "%s  <ImgOffsetX>%f</ImgOffsetX>\n", pad,
                Foreign->ImgOffsetX);
        fprintf(file, "%s  <ImgOffsetY>%f</ImgOffsetY>\n", pad,
                Foreign->ImgOffsetY);
        fprintf(file, "%s  <ImgWidth>%f</ImgWidth>\n", pad,
                Foreign->ImgWidth);
        break;

    case vdx_types_ForeignData:
        ForeignData = (const struct vdx_ForeignData *)(p);
#if 0
        fprintf(file, "%s<ForeignData CompressionLevel='%f' CompressionType='%s' ForeignType='%s' ObjectHeight='%f' ObjectWidth='%f' ShowAsIcon='%u'",
		pad, ForeignData->CompressionLevel, vdx_convert_xml_string(ForeignData->CompressionType), vdx_convert_xml_string(ForeignData->ForeignType),
		ForeignData->ObjectHeight, ForeignData->ObjectWidth, ForeignData->ShowAsIcon);
#else
	/* avoid writing optional values which are almost certainly meaningless */
        fprintf(file, "%s<ForeignData CompressionType='%s' ForeignType='%s' ",
		pad, vdx_convert_xml_string(ForeignData->CompressionType), vdx_convert_xml_string(ForeignData->ForeignType));
#endif
        if (ForeignData->ExtentX_exists)
            fprintf(file, " ExtentX='%u'",
                    ForeignData->ExtentX);
        if (ForeignData->ExtentY_exists)
            fprintf(file, " ExtentY='%u'",
                    ForeignData->ExtentY);
        if (ForeignData->MappingMode_exists)
            fprintf(file, " MappingMode='%u'",
                    ForeignData->MappingMode);
        if (ForeignData->ObjectType_exists)
            fprintf(file, " ObjectType='%u'",
                    ForeignData->ObjectType);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Geom:
        Geom = (const struct vdx_Geom *)(p);
        fprintf(file, "%s<Geom IX='%u'>\n", pad, Geom->IX);
        fprintf(file, "%s  <NoFill>%u</NoFill>\n", pad,
                Geom->NoFill);
        fprintf(file, "%s  <NoLine>%u</NoLine>\n", pad,
                Geom->NoLine);
        fprintf(file, "%s  <NoShow>%u</NoShow>\n", pad,
                Geom->NoShow);
        fprintf(file, "%s  <NoSnap>%u</NoSnap>\n", pad,
                Geom->NoSnap);
        break;

    case vdx_types_Group:
        vdxGroup = (const struct vdx_Group *)(p);
        fprintf(file, "%s<Group>\n", pad);
        fprintf(file, "%s  <DisplayMode>%u</DisplayMode>\n", pad,
                vdxGroup->DisplayMode);
        fprintf(file, "%s  <DontMoveChildren>%u</DontMoveChildren>\n", pad,
                vdxGroup->DontMoveChildren);
        fprintf(file, "%s  <IsDropTarget>%u</IsDropTarget>\n", pad,
                vdxGroup->IsDropTarget);
        fprintf(file, "%s  <IsSnapTarget>%u</IsSnapTarget>\n", pad,
                vdxGroup->IsSnapTarget);
        fprintf(file, "%s  <IsTextEditTarget>%u</IsTextEditTarget>\n", pad,
                vdxGroup->IsTextEditTarget);
        fprintf(file, "%s  <SelectMode>%u</SelectMode>\n", pad,
                vdxGroup->SelectMode);
        break;

    case vdx_types_HeaderFooter:
        HeaderFooter = (const struct vdx_HeaderFooter *)(p);
        fprintf(file, "%s<HeaderFooter HeaderFooterColor='%s'>\n", pad, vdx_convert_xml_string(HeaderFooter->HeaderFooterColor));
        fprintf(file, "%s  <FooterLeft>%s</FooterLeft>\n", pad,
                vdx_convert_xml_string(HeaderFooter->FooterLeft));
        fprintf(file, "%s  <FooterMargin>%f</FooterMargin>\n", pad,
                HeaderFooter->FooterMargin);
        fprintf(file, "%s  <HeaderFooterFont>%u</HeaderFooterFont>\n", pad,
                HeaderFooter->HeaderFooterFont);
        fprintf(file, "%s  <HeaderLeft>%s</HeaderLeft>\n", pad,
                vdx_convert_xml_string(HeaderFooter->HeaderLeft));
        fprintf(file, "%s  <HeaderMargin>%f</HeaderMargin>\n", pad,
                HeaderFooter->HeaderMargin);
        fprintf(file, "%s  <HeaderRight>%s</HeaderRight>\n", pad,
                vdx_convert_xml_string(HeaderFooter->HeaderRight));
        break;

    case vdx_types_HeaderFooterFont:
        HeaderFooterFont = (const struct vdx_HeaderFooterFont *)(p);
        fprintf(file, "%s<HeaderFooterFont CharSet='%u' Escapement='%u' FaceName='%s' Italic='%u' Orientation='%u' Quality='%u' StrikeOut='%u' Underline='%u' Width='%u'", pad, HeaderFooterFont->CharSet, HeaderFooterFont->Escapement, vdx_convert_xml_string(HeaderFooterFont->FaceName), HeaderFooterFont->Italic, HeaderFooterFont->Orientation, HeaderFooterFont->Quality, HeaderFooterFont->StrikeOut, HeaderFooterFont->Underline, HeaderFooterFont->Width);
        if (HeaderFooterFont->ClipPrecision_exists)
            fprintf(file, " ClipPrecision='%u'",
                    HeaderFooterFont->ClipPrecision);
        if (HeaderFooterFont->Height_exists)
            fprintf(file, " Height='%d)'",
                    HeaderFooterFont->Height);
        if (HeaderFooterFont->OutPrecision_exists)
            fprintf(file, " OutPrecision='%u'",
                    HeaderFooterFont->OutPrecision);
        if (HeaderFooterFont->PitchAndFamily_exists)
            fprintf(file, " PitchAndFamily='%u'",
                    HeaderFooterFont->PitchAndFamily);
        if (HeaderFooterFont->Weight_exists)
            fprintf(file, " Weight='%u'",
                    HeaderFooterFont->Weight);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Help:
        Help = (const struct vdx_Help *)(p);
        fprintf(file, "%s<Help>\n", pad);
        fprintf(file, "%s  <Copyright>%s</Copyright>\n", pad,
                vdx_convert_xml_string(Help->Copyright));
        fprintf(file, "%s  <HelpTopic>%s</HelpTopic>\n", pad,
                vdx_convert_xml_string(Help->HelpTopic));
        break;

    case vdx_types_Hyperlink:
        Hyperlink = (const struct vdx_Hyperlink *)(p);
        fprintf(file, "%s<Hyperlink ID='%u'", pad, Hyperlink->ID);
        if (Hyperlink->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(Hyperlink->NameU));
        fprintf(file, ">\n");
        fprintf(file, "%s  <Address>%s</Address>\n", pad,
                vdx_convert_xml_string(Hyperlink->Address));
        fprintf(file, "%s  <Default>%u</Default>\n", pad,
                Hyperlink->Default);
        fprintf(file, "%s  <Description>%s</Description>\n", pad,
                vdx_convert_xml_string(Hyperlink->Description));
        fprintf(file, "%s  <ExtraInfo>%s</ExtraInfo>\n", pad,
                vdx_convert_xml_string(Hyperlink->ExtraInfo));
        fprintf(file, "%s  <Frame>%u</Frame>\n", pad,
                Hyperlink->Frame);
        fprintf(file, "%s  <Invisible>%u</Invisible>\n", pad,
                Hyperlink->Invisible);
        fprintf(file, "%s  <NewWindow>%u</NewWindow>\n", pad,
                Hyperlink->NewWindow);
        fprintf(file, "%s  <SortKey>%u</SortKey>\n", pad,
                Hyperlink->SortKey);
        fprintf(file, "%s  <SubAddress>%s</SubAddress>\n", pad,
                vdx_convert_xml_string(Hyperlink->SubAddress));
        break;

    case vdx_types_Icon:
        Icon = (const struct vdx_Icon *)(p);
        fprintf(file, "%s<Icon IX='%u'", pad, Icon->IX);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Image:
        Image = (const struct vdx_Image *)(p);
        fprintf(file, "%s<Image>\n", pad);
        fprintf(file, "%s  <Blur>%f</Blur>\n", pad,
                Image->Blur);
        fprintf(file, "%s  <Brightness>%f</Brightness>\n", pad,
                Image->Brightness);
        fprintf(file, "%s  <Contrast>%f</Contrast>\n", pad,
                Image->Contrast);
        fprintf(file, "%s  <Denoise>%f</Denoise>\n", pad,
                Image->Denoise);
        fprintf(file, "%s  <Gamma>%u</Gamma>\n", pad,
                Image->Gamma);
        fprintf(file, "%s  <Sharpen>%f</Sharpen>\n", pad,
                Image->Sharpen);
        fprintf(file, "%s  <Transparency>%f</Transparency>\n", pad,
                Image->Transparency);
        break;

    case vdx_types_InfiniteLine:
        InfiniteLine = (const struct vdx_InfiniteLine *)(p);
        fprintf(file, "%s<InfiniteLine IX='%u'>\n", pad, InfiniteLine->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                InfiniteLine->A);
        fprintf(file, "%s  <B>%f</B>\n", pad,
                InfiniteLine->B);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                InfiniteLine->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                InfiniteLine->Y);
        break;

    case vdx_types_Layer:
        Layer = (const struct vdx_Layer *)(p);
        fprintf(file, "%s<Layer IX='%u'>\n", pad, Layer->IX);
        fprintf(file, "%s  <Active>%u</Active>\n", pad,
                Layer->Active);
        fprintf(file, "%s  <Color>%u</Color>\n", pad,
                Layer->Color);
        fprintf(file, "%s  <ColorTrans>%f</ColorTrans>\n", pad,
                Layer->ColorTrans);
        fprintf(file, "%s  <Glue>%u</Glue>\n", pad,
                Layer->Glue);
        fprintf(file, "%s  <Lock>%u</Lock>\n", pad,
                Layer->Lock);
        fprintf(file, "%s  <Name>%s</Name>\n", pad,
                vdx_convert_xml_string(Layer->Name));
        fprintf(file, "%s  <NameUniv>%s</NameUniv>\n", pad,
                vdx_convert_xml_string(Layer->NameUniv));
        fprintf(file, "%s  <Print>%u</Print>\n", pad,
                Layer->Print);
        fprintf(file, "%s  <Snap>%u</Snap>\n", pad,
                Layer->Snap);
        fprintf(file, "%s  <Status>%u</Status>\n", pad,
                Layer->Status);
        fprintf(file, "%s  <Visible>%u</Visible>\n", pad,
                Layer->Visible);
        break;

    case vdx_types_LayerMem:
        LayerMem = (const struct vdx_LayerMem *)(p);
        fprintf(file, "%s<LayerMem>\n", pad);
        fprintf(file, "%s  <LayerMember>%s</LayerMember>\n", pad,
                vdx_convert_xml_string(LayerMem->LayerMember));
        break;

    case vdx_types_Layout:
        Layout = (const struct vdx_Layout *)(p);
        fprintf(file, "%s<Layout>\n", pad);
        fprintf(file, "%s  <ConFixedCode>%u</ConFixedCode>\n", pad,
                Layout->ConFixedCode);
        fprintf(file, "%s  <ConLineJumpCode>%u</ConLineJumpCode>\n", pad,
                Layout->ConLineJumpCode);
        fprintf(file, "%s  <ConLineJumpDirX>%u</ConLineJumpDirX>\n", pad,
                Layout->ConLineJumpDirX);
        fprintf(file, "%s  <ConLineJumpDirY>%u</ConLineJumpDirY>\n", pad,
                Layout->ConLineJumpDirY);
        fprintf(file, "%s  <ConLineJumpStyle>%u</ConLineJumpStyle>\n", pad,
                Layout->ConLineJumpStyle);
        fprintf(file, "%s  <ConLineRouteExt>%u</ConLineRouteExt>\n", pad,
                Layout->ConLineRouteExt);
        fprintf(file, "%s  <ShapeFixedCode>%u</ShapeFixedCode>\n", pad,
                Layout->ShapeFixedCode);
        fprintf(file, "%s  <ShapePermeablePlace>%u</ShapePermeablePlace>\n", pad,
                Layout->ShapePermeablePlace);
        fprintf(file, "%s  <ShapePermeableX>%u</ShapePermeableX>\n", pad,
                Layout->ShapePermeableX);
        fprintf(file, "%s  <ShapePermeableY>%u</ShapePermeableY>\n", pad,
                Layout->ShapePermeableY);
        fprintf(file, "%s  <ShapePlaceFlip>%u</ShapePlaceFlip>\n", pad,
                Layout->ShapePlaceFlip);
        fprintf(file, "%s  <ShapePlowCode>%u</ShapePlowCode>\n", pad,
                Layout->ShapePlowCode);
        fprintf(file, "%s  <ShapeRouteStyle>%u</ShapeRouteStyle>\n", pad,
                Layout->ShapeRouteStyle);
        fprintf(file, "%s  <ShapeSplit>%u</ShapeSplit>\n", pad,
                Layout->ShapeSplit);
        fprintf(file, "%s  <ShapeSplittable>%u</ShapeSplittable>\n", pad,
                Layout->ShapeSplittable);
        break;

    case vdx_types_Line:
        Line = (const struct vdx_Line *)(p);
        fprintf(file, "%s<Line>\n", pad);
	if (Line->BeginArrow)
          fprintf(file, "%s  <BeginArrow>%u</BeginArrow>\n", pad,
                  Line->BeginArrow);
	if (Line->BeginArrowSize)
          fprintf(file, "%s  <BeginArrowSize>%u</BeginArrowSize>\n", pad,
                  Line->BeginArrowSize);
	if (Line->EndArrow)
          fprintf(file, "%s  <EndArrow>%u</EndArrow>\n", pad,
                  Line->EndArrow);
	if (Line->EndArrowSize)
          fprintf(file, "%s  <EndArrowSize>%u</EndArrowSize>\n", pad,
                  Line->EndArrowSize);
	if (Line->LineCap)
          fprintf(file, "%s  <LineCap>%u</LineCap>\n", pad,
                  Line->LineCap);
        fprintf(file, "%s  <LineColor>%s</LineColor>\n", pad,
                vdx_string_color(Line->LineColor));
	if (Line->LineColorTrans)
          fprintf(file, "%s  <LineColorTrans>%f</LineColorTrans>\n", pad,
                  Line->LineColorTrans);
	if (Line->LinePattern)
          fprintf(file, "%s  <LinePattern>%u</LinePattern>\n", pad,
                  Line->LinePattern);
	if (Line->LineWeight)
          fprintf(file, "%s  <LineWeight>%f</LineWeight>\n", pad,
                  Line->LineWeight);
	if (Line->Rounding)
          fprintf(file, "%s  <Rounding>%f</Rounding>\n", pad,
                  Line->Rounding);
        break;

    case vdx_types_LineTo:
        LineTo = (const struct vdx_LineTo *)(p);
        fprintf(file, "%s<LineTo IX='%u'", pad, LineTo->IX);
        if (LineTo->Del)
            fprintf(file, " Del='%u'",
                    LineTo->Del);
        fprintf(file, ">\n");
        fprintf(file, "%s  <X>%f</X>\n", pad,
                LineTo->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                LineTo->Y);
        break;

    case vdx_types_Master:
        Master = (const struct vdx_Master *)(p);
        fprintf(file, "%s<Master BaseID='%s' Hidden='%u' ID='%u' IconUpdate='%u' MatchByName='%u' Prompt='%s'", pad, vdx_convert_xml_string(Master->BaseID), Master->Hidden, Master->ID, Master->IconUpdate, Master->MatchByName, vdx_convert_xml_string(Master->Prompt));
        if (Master->AlignName_exists)
            fprintf(file, " AlignName='%u'",
                    Master->AlignName);
        if (Master->IconSize_exists)
            fprintf(file, " IconSize='%u'",
                    Master->IconSize);
        if (Master->PatternFlags_exists)
            fprintf(file, " PatternFlags='%u'",
                    Master->PatternFlags);
        if (Master->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(Master->Name));
        if (Master->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(Master->NameU));
        if (Master->UniqueID)
            fprintf(file, " UniqueID='%s'",
                    vdx_convert_xml_string(Master->UniqueID));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Misc:
        Misc = (const struct vdx_Misc *)(p);
        fprintf(file, "%s<Misc>\n", pad);
        fprintf(file, "%s  <BegTrigger>%u</BegTrigger>\n", pad,
                Misc->BegTrigger);
        fprintf(file, "%s  <Calendar>%u</Calendar>\n", pad,
                Misc->Calendar);
        fprintf(file, "%s  <Comment>%s</Comment>\n", pad,
                vdx_convert_xml_string(Misc->Comment));
        fprintf(file, "%s  <DropOnPageScale>%u</DropOnPageScale>\n", pad,
                Misc->DropOnPageScale);
        fprintf(file, "%s  <DynFeedback>%u</DynFeedback>\n", pad,
                Misc->DynFeedback);
        fprintf(file, "%s  <EndTrigger>%u</EndTrigger>\n", pad,
                Misc->EndTrigger);
        fprintf(file, "%s  <GlueType>%u</GlueType>\n", pad,
                Misc->GlueType);
        fprintf(file, "%s  <HideText>%u</HideText>\n", pad,
                Misc->HideText);
        fprintf(file, "%s  <IsDropSource>%u</IsDropSource>\n", pad,
                Misc->IsDropSource);
        fprintf(file, "%s  <LangID>%u</LangID>\n", pad,
                Misc->LangID);
        fprintf(file, "%s  <LocalizeMerge>%u</LocalizeMerge>\n", pad,
                Misc->LocalizeMerge);
        fprintf(file, "%s  <NoAlignBox>%u</NoAlignBox>\n", pad,
                Misc->NoAlignBox);
        fprintf(file, "%s  <NoCtlHandles>%u</NoCtlHandles>\n", pad,
                Misc->NoCtlHandles);
        fprintf(file, "%s  <NoLiveDynamics>%u</NoLiveDynamics>\n", pad,
                Misc->NoLiveDynamics);
        fprintf(file, "%s  <NoObjHandles>%u</NoObjHandles>\n", pad,
                Misc->NoObjHandles);
        fprintf(file, "%s  <NonPrinting>%u</NonPrinting>\n", pad,
                Misc->NonPrinting);
        fprintf(file, "%s  <ObjType>%u</ObjType>\n", pad,
                Misc->ObjType);
        fprintf(file, "%s  <ShapeKeywords>%s</ShapeKeywords>\n", pad,
                vdx_convert_xml_string(Misc->ShapeKeywords));
        fprintf(file, "%s  <UpdateAlignBox>%u</UpdateAlignBox>\n", pad,
                Misc->UpdateAlignBox);
        fprintf(file, "%s  <WalkPreference>%u</WalkPreference>\n", pad,
                Misc->WalkPreference);
        break;

    case vdx_types_MoveTo:
        MoveTo = (const struct vdx_MoveTo *)(p);
        fprintf(file, "%s<MoveTo IX='%u'>\n", pad, MoveTo->IX);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                MoveTo->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                MoveTo->Y);
        break;

    case vdx_types_NURBSTo:
        NURBSTo = (const struct vdx_NURBSTo *)(p);
        fprintf(file, "%s<NURBSTo IX='%u'>\n", pad, NURBSTo->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                NURBSTo->A);
        fprintf(file, "%s  <B>%f</B>\n", pad,
                NURBSTo->B);
        fprintf(file, "%s  <C>%f</C>\n", pad,
                NURBSTo->C);
        fprintf(file, "%s  <D>%f</D>\n", pad,
                NURBSTo->D);
        fprintf(file, "%s  <E>%s</E>\n", pad,
                vdx_convert_xml_string(NURBSTo->E));
        fprintf(file, "%s  <X>%f</X>\n", pad,
                NURBSTo->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                NURBSTo->Y);
        break;

    case vdx_types_Page:
        Page = (const struct vdx_Page *)(p);
        fprintf(file, "%s<Page Background='%u' ID='%u' ViewCenterX='%f' ViewCenterY='%f' ViewScale='%f'", pad, Page->Background, Page->ID, Page->ViewCenterX, Page->ViewCenterY, Page->ViewScale);
        if (Page->BackPage_exists)
            fprintf(file, " BackPage='%u'",
                    Page->BackPage);
        if (Page->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(Page->Name));
        if (Page->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(Page->NameU));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_PageLayout:
        PageLayout = (const struct vdx_PageLayout *)(p);
        fprintf(file, "%s<PageLayout>\n", pad);
        fprintf(file, "%s  <AvenueSizeX>%f</AvenueSizeX>\n", pad,
                PageLayout->AvenueSizeX);
        fprintf(file, "%s  <AvenueSizeY>%f</AvenueSizeY>\n", pad,
                PageLayout->AvenueSizeY);
        fprintf(file, "%s  <BlockSizeX>%f</BlockSizeX>\n", pad,
                PageLayout->BlockSizeX);
        fprintf(file, "%s  <BlockSizeY>%f</BlockSizeY>\n", pad,
                PageLayout->BlockSizeY);
        fprintf(file, "%s  <CtrlAsInput>%u</CtrlAsInput>\n", pad,
                PageLayout->CtrlAsInput);
        fprintf(file, "%s  <DynamicsOff>%u</DynamicsOff>\n", pad,
                PageLayout->DynamicsOff);
        fprintf(file, "%s  <EnableGrid>%u</EnableGrid>\n", pad,
                PageLayout->EnableGrid);
        fprintf(file, "%s  <LineAdjustFrom>%u</LineAdjustFrom>\n", pad,
                PageLayout->LineAdjustFrom);
        fprintf(file, "%s  <LineAdjustTo>%u</LineAdjustTo>\n", pad,
                PageLayout->LineAdjustTo);
        fprintf(file, "%s  <LineJumpCode>%u</LineJumpCode>\n", pad,
                PageLayout->LineJumpCode);
        fprintf(file, "%s  <LineJumpFactorX>%f</LineJumpFactorX>\n", pad,
                PageLayout->LineJumpFactorX);
        fprintf(file, "%s  <LineJumpFactorY>%f</LineJumpFactorY>\n", pad,
                PageLayout->LineJumpFactorY);
        fprintf(file, "%s  <LineJumpStyle>%u</LineJumpStyle>\n", pad,
                PageLayout->LineJumpStyle);
        fprintf(file, "%s  <LineRouteExt>%u</LineRouteExt>\n", pad,
                PageLayout->LineRouteExt);
        fprintf(file, "%s  <LineToLineX>%f</LineToLineX>\n", pad,
                PageLayout->LineToLineX);
        fprintf(file, "%s  <LineToLineY>%f</LineToLineY>\n", pad,
                PageLayout->LineToLineY);
        fprintf(file, "%s  <LineToNodeX>%f</LineToNodeX>\n", pad,
                PageLayout->LineToNodeX);
        fprintf(file, "%s  <LineToNodeY>%f</LineToNodeY>\n", pad,
                PageLayout->LineToNodeY);
        fprintf(file, "%s  <PageLineJumpDirX>%u</PageLineJumpDirX>\n", pad,
                PageLayout->PageLineJumpDirX);
        fprintf(file, "%s  <PageLineJumpDirY>%u</PageLineJumpDirY>\n", pad,
                PageLayout->PageLineJumpDirY);
        fprintf(file, "%s  <PageShapeSplit>%u</PageShapeSplit>\n", pad,
                PageLayout->PageShapeSplit);
        fprintf(file, "%s  <PlaceDepth>%u</PlaceDepth>\n", pad,
                PageLayout->PlaceDepth);
        fprintf(file, "%s  <PlaceFlip>%u</PlaceFlip>\n", pad,
                PageLayout->PlaceFlip);
        fprintf(file, "%s  <PlaceStyle>%u</PlaceStyle>\n", pad,
                PageLayout->PlaceStyle);
        fprintf(file, "%s  <PlowCode>%u</PlowCode>\n", pad,
                PageLayout->PlowCode);
        fprintf(file, "%s  <ResizePage>%u</ResizePage>\n", pad,
                PageLayout->ResizePage);
        fprintf(file, "%s  <RouteStyle>%u</RouteStyle>\n", pad,
                PageLayout->RouteStyle);
        break;

    case vdx_types_PageProps:
        PageProps = (const struct vdx_PageProps *)(p);
        fprintf(file, "%s<PageProps>\n", pad);
        fprintf(file, "%s  <DrawingScale>%f</DrawingScale>\n", pad,
                PageProps->DrawingScale);
        fprintf(file, "%s  <DrawingScaleType>%u</DrawingScaleType>\n", pad,
                PageProps->DrawingScaleType);
        fprintf(file, "%s  <DrawingSizeType>%u</DrawingSizeType>\n", pad,
                PageProps->DrawingSizeType);
        fprintf(file, "%s  <InhibitSnap>%u</InhibitSnap>\n", pad,
                PageProps->InhibitSnap);
        fprintf(file, "%s  <PageHeight>%f</PageHeight>\n", pad,
                PageProps->PageHeight);
        fprintf(file, "%s  <PageScale>%f</PageScale>\n", pad,
                PageProps->PageScale);
        fprintf(file, "%s  <PageWidth>%f</PageWidth>\n", pad,
                PageProps->PageWidth);
        fprintf(file, "%s  <ShdwObliqueAngle>%f</ShdwObliqueAngle>\n", pad,
                PageProps->ShdwObliqueAngle);
        fprintf(file, "%s  <ShdwOffsetX>%f</ShdwOffsetX>\n", pad,
                PageProps->ShdwOffsetX);
        fprintf(file, "%s  <ShdwOffsetY>%f</ShdwOffsetY>\n", pad,
                PageProps->ShdwOffsetY);
        fprintf(file, "%s  <ShdwScaleFactor>%f</ShdwScaleFactor>\n", pad,
                PageProps->ShdwScaleFactor);
        fprintf(file, "%s  <ShdwType>%u</ShdwType>\n", pad,
                PageProps->ShdwType);
        fprintf(file, "%s  <UIVisibility>%u</UIVisibility>\n", pad,
                PageProps->UIVisibility);
        break;

    case vdx_types_PageSheet:
        PageSheet = (const struct vdx_PageSheet *)(p);
        fprintf(file, "%s<PageSheet", pad);
        if (PageSheet->FillStyle_exists)
            fprintf(file, " FillStyle='%u'",
                    PageSheet->FillStyle);
        if (PageSheet->LineStyle_exists)
            fprintf(file, " LineStyle='%u'",
                    PageSheet->LineStyle);
        if (PageSheet->TextStyle_exists)
            fprintf(file, " TextStyle='%u'",
                    PageSheet->TextStyle);
        if (PageSheet->UniqueID)
            fprintf(file, " UniqueID='%s'",
                    vdx_convert_xml_string(PageSheet->UniqueID));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Para:
        Para = (const struct vdx_Para *)(p);
        fprintf(file, "%s<Para IX='%u'>\n", pad, Para->IX);
	if (Para->Bullet)
          fprintf(file, "%s  <Bullet>%u</Bullet>\n", pad,
                  Para->Bullet);
	if (Para->BulletFont)
          fprintf(file, "%s  <BulletFont>%u</BulletFont>\n", pad,
                  Para->BulletFont);
	if (Para->BulletFontSize)
          fprintf(file, "%s  <BulletFontSize>%s</BulletFontSize>\n", pad,
                  vdx_convert_xml_string(Para->BulletFontSize));
	if (Para->BulletStr)
          fprintf(file, "%s  <BulletStr>%s</BulletStr>\n", pad,
                  vdx_convert_xml_string(Para->BulletStr));
	if (Para->Flags)
          fprintf(file, "%s  <Flags>%u</Flags>\n", pad,
                  Para->Flags);
	/* just to be sure */
        fprintf(file, "%s  <HorzAlign>%u</HorzAlign>\n", pad,
                Para->HorzAlign);
	if (Para->IndFirst)
          fprintf(file, "%s  <IndFirst>%f</IndFirst>\n", pad,
                  Para->IndFirst);
	if (Para->IndLeft)
          fprintf(file, "%s  <IndLeft>%f</IndLeft>\n", pad,
                  Para->IndLeft);
	if (Para->IndRight)
          fprintf(file, "%s  <IndRight>%f</IndRight>\n", pad,
                  Para->IndRight);
	if (Para->LocalizeBulletFont)
          fprintf(file, "%s  <LocalizeBulletFont>%u</LocalizeBulletFont>\n", pad,
                  Para->LocalizeBulletFont);
	if (Para->SpAfter)
          fprintf(file, "%s  <SpAfter>%f</SpAfter>\n", pad,
                  Para->SpAfter);
	if (Para->SpBefore)
          fprintf(file, "%s  <SpBefore>%f</SpBefore>\n", pad,
                  Para->SpBefore);
	if (Para->SpLine)
          fprintf(file, "%s  <SpLine>%f</SpLine>\n", pad,
                Para->SpLine);
	if (Para->TextPosAfterBullet)
          fprintf(file, "%s  <TextPosAfterBullet>%u</TextPosAfterBullet>\n", pad,
                Para->TextPosAfterBullet);
        break;

    case vdx_types_PolylineTo:
        PolylineTo = (const struct vdx_PolylineTo *)(p);
        fprintf(file, "%s<PolylineTo IX='%u'>\n", pad, PolylineTo->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                PolylineTo->A);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                PolylineTo->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                PolylineTo->Y);
        break;

    case vdx_types_PreviewPicture:
        PreviewPicture = (const struct vdx_PreviewPicture *)(p);
        fprintf(file, "%s<PreviewPicture", pad);
        if (PreviewPicture->Size_exists)
            fprintf(file, " Size='%u'",
                    PreviewPicture->Size);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_PrintProps:
        PrintProps = (const struct vdx_PrintProps *)(p);
        fprintf(file, "%s<PrintProps>\n", pad);
        fprintf(file, "%s  <CenterX>%u</CenterX>\n", pad,
                PrintProps->CenterX);
        fprintf(file, "%s  <CenterY>%u</CenterY>\n", pad,
                PrintProps->CenterY);
        fprintf(file, "%s  <OnPage>%u</OnPage>\n", pad,
                PrintProps->OnPage);
        fprintf(file, "%s  <PageBottomMargin>%f</PageBottomMargin>\n", pad,
                PrintProps->PageBottomMargin);
        fprintf(file, "%s  <PageLeftMargin>%f</PageLeftMargin>\n", pad,
                PrintProps->PageLeftMargin);
        fprintf(file, "%s  <PageRightMargin>%f</PageRightMargin>\n", pad,
                PrintProps->PageRightMargin);
        fprintf(file, "%s  <PageTopMargin>%f</PageTopMargin>\n", pad,
                PrintProps->PageTopMargin);
        fprintf(file, "%s  <PagesX>%u</PagesX>\n", pad,
                PrintProps->PagesX);
        fprintf(file, "%s  <PagesY>%u</PagesY>\n", pad,
                PrintProps->PagesY);
        fprintf(file, "%s  <PaperKind>%u</PaperKind>\n", pad,
                PrintProps->PaperKind);
        fprintf(file, "%s  <PaperSource>%u</PaperSource>\n", pad,
                PrintProps->PaperSource);
        fprintf(file, "%s  <PrintGrid>%u</PrintGrid>\n", pad,
                PrintProps->PrintGrid);
        fprintf(file, "%s  <PrintPageOrientation>%u</PrintPageOrientation>\n", pad,
                PrintProps->PrintPageOrientation);
        fprintf(file, "%s  <ScaleX>%f</ScaleX>\n", pad,
                PrintProps->ScaleX);
        fprintf(file, "%s  <ScaleY>%u</ScaleY>\n", pad,
                PrintProps->ScaleY);
        break;

    case vdx_types_PrintSetup:
        PrintSetup = (const struct vdx_PrintSetup *)(p);
        fprintf(file, "%s<PrintSetup>\n", pad);
        fprintf(file, "%s  <PageBottomMargin>%f</PageBottomMargin>\n", pad,
                PrintSetup->PageBottomMargin);
        fprintf(file, "%s  <PageLeftMargin>%f</PageLeftMargin>\n", pad,
                PrintSetup->PageLeftMargin);
        fprintf(file, "%s  <PageRightMargin>%f</PageRightMargin>\n", pad,
                PrintSetup->PageRightMargin);
        fprintf(file, "%s  <PageTopMargin>%f</PageTopMargin>\n", pad,
                PrintSetup->PageTopMargin);
        fprintf(file, "%s  <PaperSize>%u</PaperSize>\n", pad,
                PrintSetup->PaperSize);
        fprintf(file, "%s  <PrintCenteredH>%u</PrintCenteredH>\n", pad,
                PrintSetup->PrintCenteredH);
        fprintf(file, "%s  <PrintCenteredV>%u</PrintCenteredV>\n", pad,
                PrintSetup->PrintCenteredV);
        fprintf(file, "%s  <PrintFitOnPages>%u</PrintFitOnPages>\n", pad,
                PrintSetup->PrintFitOnPages);
        fprintf(file, "%s  <PrintLandscape>%u</PrintLandscape>\n", pad,
                PrintSetup->PrintLandscape);
        fprintf(file, "%s  <PrintPagesAcross>%u</PrintPagesAcross>\n", pad,
                PrintSetup->PrintPagesAcross);
        fprintf(file, "%s  <PrintPagesDown>%u</PrintPagesDown>\n", pad,
                PrintSetup->PrintPagesDown);
        fprintf(file, "%s  <PrintScale>%u</PrintScale>\n", pad,
                PrintSetup->PrintScale);
        break;

    case vdx_types_Prop:
        Prop = (const struct vdx_Prop *)(p);
        fprintf(file, "%s<Prop ID='%u'", pad, Prop->ID);
        if (Prop->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(Prop->Name));
        if (Prop->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(Prop->NameU));
        fprintf(file, ">\n");
        fprintf(file, "%s  <Calendar>%u</Calendar>\n", pad,
                Prop->Calendar);
        fprintf(file, "%s  <Format>%s</Format>\n", pad,
                vdx_convert_xml_string(Prop->Format));
        fprintf(file, "%s  <Invisible>%u</Invisible>\n", pad,
                Prop->Invisible);
        fprintf(file, "%s  <Label>%s</Label>\n", pad,
                vdx_convert_xml_string(Prop->Label));
        fprintf(file, "%s  <LangID>%u</LangID>\n", pad,
                Prop->LangID);
        fprintf(file, "%s  <Prompt>%s</Prompt>\n", pad,
                vdx_convert_xml_string(Prop->Prompt));
        fprintf(file, "%s  <SortKey>%s</SortKey>\n", pad,
                vdx_convert_xml_string(Prop->SortKey));
        fprintf(file, "%s  <Type>%u</Type>\n", pad,
                Prop->Type);
        fprintf(file, "%s  <Value>%f</Value>\n", pad,
                Prop->Value);
        fprintf(file, "%s  <Verify>%u</Verify>\n", pad,
                Prop->Verify);
        break;

    case vdx_types_Protection:
        Protection = (const struct vdx_Protection *)(p);
        fprintf(file, "%s<Protection>\n", pad);
        fprintf(file, "%s  <LockAspect>%u</LockAspect>\n", pad,
                Protection->LockAspect);
        fprintf(file, "%s  <LockBegin>%u</LockBegin>\n", pad,
                Protection->LockBegin);
        fprintf(file, "%s  <LockCalcWH>%u</LockCalcWH>\n", pad,
                Protection->LockCalcWH);
        fprintf(file, "%s  <LockCrop>%u</LockCrop>\n", pad,
                Protection->LockCrop);
        fprintf(file, "%s  <LockCustProp>%u</LockCustProp>\n", pad,
                Protection->LockCustProp);
        fprintf(file, "%s  <LockDelete>%u</LockDelete>\n", pad,
                Protection->LockDelete);
        fprintf(file, "%s  <LockEnd>%u</LockEnd>\n", pad,
                Protection->LockEnd);
        fprintf(file, "%s  <LockFormat>%u</LockFormat>\n", pad,
                Protection->LockFormat);
        fprintf(file, "%s  <LockGroup>%u</LockGroup>\n", pad,
                Protection->LockGroup);
        fprintf(file, "%s  <LockHeight>%u</LockHeight>\n", pad,
                Protection->LockHeight);
        fprintf(file, "%s  <LockMoveX>%u</LockMoveX>\n", pad,
                Protection->LockMoveX);
        fprintf(file, "%s  <LockMoveY>%u</LockMoveY>\n", pad,
                Protection->LockMoveY);
        fprintf(file, "%s  <LockRotate>%u</LockRotate>\n", pad,
                Protection->LockRotate);
        fprintf(file, "%s  <LockSelect>%u</LockSelect>\n", pad,
                Protection->LockSelect);
        fprintf(file, "%s  <LockTextEdit>%u</LockTextEdit>\n", pad,
                Protection->LockTextEdit);
        fprintf(file, "%s  <LockVtxEdit>%u</LockVtxEdit>\n", pad,
                Protection->LockVtxEdit);
        fprintf(file, "%s  <LockWidth>%u</LockWidth>\n", pad,
                Protection->LockWidth);
        break;

    case vdx_types_RulerGrid:
        RulerGrid = (const struct vdx_RulerGrid *)(p);
        fprintf(file, "%s<RulerGrid>\n", pad);
        fprintf(file, "%s  <XGridDensity>%u</XGridDensity>\n", pad,
                RulerGrid->XGridDensity);
        fprintf(file, "%s  <XGridOrigin>%f</XGridOrigin>\n", pad,
                RulerGrid->XGridOrigin);
        fprintf(file, "%s  <XGridSpacing>%f</XGridSpacing>\n", pad,
                RulerGrid->XGridSpacing);
        fprintf(file, "%s  <XRulerDensity>%u</XRulerDensity>\n", pad,
                RulerGrid->XRulerDensity);
        fprintf(file, "%s  <XRulerOrigin>%f</XRulerOrigin>\n", pad,
                RulerGrid->XRulerOrigin);
        fprintf(file, "%s  <YGridDensity>%u</YGridDensity>\n", pad,
                RulerGrid->YGridDensity);
        fprintf(file, "%s  <YGridOrigin>%f</YGridOrigin>\n", pad,
                RulerGrid->YGridOrigin);
        fprintf(file, "%s  <YGridSpacing>%f</YGridSpacing>\n", pad,
                RulerGrid->YGridSpacing);
        fprintf(file, "%s  <YRulerDensity>%u</YRulerDensity>\n", pad,
                RulerGrid->YRulerDensity);
        fprintf(file, "%s  <YRulerOrigin>%f</YRulerOrigin>\n", pad,
                RulerGrid->YRulerOrigin);
        break;

    case vdx_types_Scratch:
        Scratch = (const struct vdx_Scratch *)(p);
        fprintf(file, "%s<Scratch IX='%u'>\n", pad, Scratch->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                Scratch->A);
        fprintf(file, "%s  <B>%f</B>\n", pad,
                Scratch->B);
        fprintf(file, "%s  <C>%f</C>\n", pad,
                Scratch->C);
        fprintf(file, "%s  <D>%f</D>\n", pad,
                Scratch->D);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                Scratch->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                Scratch->Y);
        break;

    case vdx_types_Shape:
        Shape = (const struct vdx_Shape *)(p);
        fprintf(file, "%s<Shape ID='%u' Type='%s'", pad, Shape->ID, vdx_convert_xml_string(Shape->Type));
        if (Shape->FillStyle_exists)
            fprintf(file, " FillStyle='%u'",
                    Shape->FillStyle);
        if (Shape->LineStyle_exists)
            fprintf(file, " LineStyle='%u'",
                    Shape->LineStyle);
        if (Shape->Master_exists)
            fprintf(file, " Master='%u'",
                    Shape->Master);
        if (Shape->MasterShape_exists)
            fprintf(file, " MasterShape='%u'",
                    Shape->MasterShape);
        if (Shape->TextStyle_exists)
            fprintf(file, " TextStyle='%u'",
                    Shape->TextStyle);
        if (Shape->Del)
            fprintf(file, " Del='%u'",
                    Shape->Del);
        if (Shape->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(Shape->Name));
        if (Shape->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(Shape->NameU));
        if (Shape->UniqueID)
            fprintf(file, " UniqueID='%s'",
                    vdx_convert_xml_string(Shape->UniqueID));
        fprintf(file, ">\n");
        fprintf(file, "%s  <Data1>%u</Data1>\n", pad,
                Shape->Data1);
        fprintf(file, "%s  <Data2>%u</Data2>\n", pad,
                Shape->Data2);
        fprintf(file, "%s  <Data3>%u</Data3>\n", pad,
                Shape->Data3);
        break;

    case vdx_types_Shapes:
        vdxShapes = (const struct vdx_Shapes *)(p);
        fprintf(file, "%s<Shapes", pad);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_SplineKnot:
        SplineKnot = (const struct vdx_SplineKnot *)(p);
        fprintf(file, "%s<SplineKnot IX='%u'>\n", pad, SplineKnot->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                SplineKnot->A);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                SplineKnot->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                SplineKnot->Y);
        break;

    case vdx_types_SplineStart:
        SplineStart = (const struct vdx_SplineStart *)(p);
        fprintf(file, "%s<SplineStart IX='%u'>\n", pad, SplineStart->IX);
        fprintf(file, "%s  <A>%f</A>\n", pad,
                SplineStart->A);
        fprintf(file, "%s  <B>%f</B>\n", pad,
                SplineStart->B);
        fprintf(file, "%s  <C>%f</C>\n", pad,
                SplineStart->C);
        fprintf(file, "%s  <D>%f</D>\n", pad,
                SplineStart->D);
        fprintf(file, "%s  <X>%f</X>\n", pad,
                SplineStart->X);
        fprintf(file, "%s  <Y>%f</Y>\n", pad,
                SplineStart->Y);
        break;

    case vdx_types_StyleProp:
        StyleProp = (const struct vdx_StyleProp *)(p);
        fprintf(file, "%s<StyleProp>\n", pad);
        fprintf(file, "%s  <EnableFillProps>%u</EnableFillProps>\n", pad,
                StyleProp->EnableFillProps);
        fprintf(file, "%s  <EnableLineProps>%u</EnableLineProps>\n", pad,
                StyleProp->EnableLineProps);
        fprintf(file, "%s  <EnableTextProps>%u</EnableTextProps>\n", pad,
                StyleProp->EnableTextProps);
        fprintf(file, "%s  <HideForApply>%u</HideForApply>\n", pad,
                StyleProp->HideForApply);
        break;

    case vdx_types_StyleSheet:
        StyleSheet = (const struct vdx_StyleSheet *)(p);
        fprintf(file, "%s<StyleSheet ID='%u'", pad, StyleSheet->ID);
        if (StyleSheet->FillStyle_exists)
            fprintf(file, " FillStyle='%u'",
                    StyleSheet->FillStyle);
        if (StyleSheet->LineStyle_exists)
            fprintf(file, " LineStyle='%u'",
                    StyleSheet->LineStyle);
        if (StyleSheet->TextStyle_exists)
            fprintf(file, " TextStyle='%u'",
                    StyleSheet->TextStyle);
        if (StyleSheet->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(StyleSheet->Name));
        if (StyleSheet->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(StyleSheet->NameU));
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Tab:
        Tab = (const struct vdx_Tab *)(p);
        fprintf(file, "%s<Tab IX='%u'>\n", pad, Tab->IX);
        fprintf(file, "%s  <Alignment>%u</Alignment>\n", pad,
                Tab->Alignment);
        fprintf(file, "%s  <Position>%f</Position>\n", pad,
                Tab->Position);
        break;

    case vdx_types_Tabs:
        Tabs = (const struct vdx_Tabs *)(p);
        fprintf(file, "%s<Tabs IX='%u'", pad, Tabs->IX);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_Text:
        vdxText = (const struct vdx_Text *)(p);
        fprintf(file, "%s<Text>", pad);
        *pad = 0;
        break;

    case vdx_types_TextBlock:
        TextBlock = (const struct vdx_TextBlock *)(p);
        fprintf(file, "%s<TextBlock>\n", pad);
        fprintf(file, "%s  <BottomMargin>%f</BottomMargin>\n", pad,
                TextBlock->BottomMargin);
        fprintf(file, "%s  <DefaultTabStop>%f</DefaultTabStop>\n", pad,
                TextBlock->DefaultTabStop);
        fprintf(file, "%s  <LeftMargin>%f</LeftMargin>\n", pad,
                TextBlock->LeftMargin);
        fprintf(file, "%s  <RightMargin>%f</RightMargin>\n", pad,
                TextBlock->RightMargin);
        fprintf(file, "%s  <TextBkgnd>%u</TextBkgnd>\n", pad,
                TextBlock->TextBkgnd);
        fprintf(file, "%s  <TextBkgndTrans>%f</TextBkgndTrans>\n", pad,
                TextBlock->TextBkgndTrans);
        fprintf(file, "%s  <TextDirection>%u</TextDirection>\n", pad,
                TextBlock->TextDirection);
        fprintf(file, "%s  <TopMargin>%f</TopMargin>\n", pad,
                TextBlock->TopMargin);
        fprintf(file, "%s  <VerticalAlign>%u</VerticalAlign>\n", pad,
                TextBlock->VerticalAlign);
        break;

    case vdx_types_TextXForm:
        TextXForm = (const struct vdx_TextXForm *)(p);
        fprintf(file, "%s<TextXForm>\n", pad);
	if (TextXForm->TxtAngle)
          fprintf(file, "%s  <TxtAngle>%f</TxtAngle>\n", pad,
                  TextXForm->TxtAngle);
	if (TextXForm->TxtHeight)
          fprintf(file, "%s  <TxtHeight>%f</TxtHeight>\n", pad,
                  TextXForm->TxtHeight);
	if (TextXForm->TxtLocPinX)
          fprintf(file, "%s  <TxtLocPinX>%f</TxtLocPinX>\n", pad,
                  TextXForm->TxtLocPinX);
	if (TextXForm->TxtLocPinY)
          fprintf(file, "%s  <TxtLocPinY>%f</TxtLocPinY>\n", pad,
                  TextXForm->TxtLocPinY);
	if (TextXForm->TxtPinX)
          fprintf(file, "%s  <TxtPinX>%f</TxtPinX>\n", pad,
                  TextXForm->TxtPinX);
	if (TextXForm->TxtPinY)
          fprintf(file, "%s  <TxtPinY>%f</TxtPinY>\n", pad,
                  TextXForm->TxtPinY);
	if (TextXForm->TxtWidth)
          fprintf(file, "%s  <TxtWidth>%f</TxtWidth>\n", pad,
                  TextXForm->TxtWidth);
        break;

    case vdx_types_User:
        User = (const struct vdx_User *)(p);
        fprintf(file, "%s<User ID='%u'", pad, User->ID);
        if (User->Name)
            fprintf(file, " Name='%s'",
                    vdx_convert_xml_string(User->Name));
        if (User->NameU)
            fprintf(file, " NameU='%s'",
                    vdx_convert_xml_string(User->NameU));
        fprintf(file, ">\n");
        fprintf(file, "%s  <Prompt>%s</Prompt>\n", pad,
                vdx_convert_xml_string(User->Prompt));
        fprintf(file, "%s  <Value>%f</Value>\n", pad,
                User->Value);
        break;

    case vdx_types_VisioDocument:
        VisioDocument = (const struct vdx_VisioDocument *)(p);
        fprintf(file, "%s<VisioDocument key='%s' metric='%u' version='%s' xmlns='%s'", pad, vdx_convert_xml_string(VisioDocument->key), VisioDocument->metric, vdx_convert_xml_string(VisioDocument->version), vdx_convert_xml_string(VisioDocument->xmlns));
        if (VisioDocument->DocLangID_exists)
            fprintf(file, " DocLangID='%u'",
                    VisioDocument->DocLangID);
        if (VisioDocument->buildnum_exists)
            fprintf(file, " buildnum='%u'",
                    VisioDocument->buildnum);
        if (VisioDocument->start_exists)
            fprintf(file, " start='%u'",
                    VisioDocument->start);
        fprintf(file, ">\n");
        fprintf(file, "%s  <EventList>%u</EventList>\n", pad,
                VisioDocument->EventList);
        fprintf(file, "%s  <Masters>%u</Masters>\n", pad,
                VisioDocument->Masters);
        break;

    case vdx_types_Window:
        Window = (const struct vdx_Window *)(p);
        fprintf(file, "%s<Window ContainerType='%s' Document='%s' ID='%u' ReadOnly='%u' ViewCenterX='%f' ViewCenterY='%f' ViewScale='%f' WindowType='%s'", pad, vdx_convert_xml_string(Window->ContainerType), vdx_convert_xml_string(Window->Document), Window->ID, Window->ReadOnly, Window->ViewCenterX, Window->ViewCenterY, Window->ViewScale, vdx_convert_xml_string(Window->WindowType));
        if (Window->Page_exists)
            fprintf(file, " Page='%u'",
                    Window->Page);
        if (Window->ParentWindow_exists)
            fprintf(file, " ParentWindow='%u'",
                    Window->ParentWindow);
        if (Window->Sheet_exists)
            fprintf(file, " Sheet='%u'",
                    Window->Sheet);
        if (Window->WindowHeight_exists)
            fprintf(file, " WindowHeight='%u'",
                    Window->WindowHeight);
        if (Window->WindowLeft_exists)
            fprintf(file, " WindowLeft='%d)'",
                    Window->WindowLeft);
        if (Window->WindowState_exists)
            fprintf(file, " WindowState='%u'",
                    Window->WindowState);
        if (Window->WindowTop_exists)
            fprintf(file, " WindowTop='%d)'",
                    Window->WindowTop);
        if (Window->WindowWidth_exists)
            fprintf(file, " WindowWidth='%u'",
                    Window->WindowWidth);
        fprintf(file, ">\n");
        fprintf(file, "%s  <DynamicGridEnabled>%u</DynamicGridEnabled>\n", pad,
                Window->DynamicGridEnabled);
        fprintf(file, "%s  <GlueSettings>%u</GlueSettings>\n", pad,
                Window->GlueSettings);
        fprintf(file, "%s  <ShowConnectionPoints>%u</ShowConnectionPoints>\n", pad,
                Window->ShowConnectionPoints);
        fprintf(file, "%s  <ShowGrid>%u</ShowGrid>\n", pad,
                Window->ShowGrid);
        fprintf(file, "%s  <ShowGuides>%u</ShowGuides>\n", pad,
                Window->ShowGuides);
        fprintf(file, "%s  <ShowPageBreaks>%u</ShowPageBreaks>\n", pad,
                Window->ShowPageBreaks);
        fprintf(file, "%s  <ShowRulers>%u</ShowRulers>\n", pad,
                Window->ShowRulers);
        fprintf(file, "%s  <SnapExtensions>%u</SnapExtensions>\n", pad,
                Window->SnapExtensions);
        fprintf(file, "%s  <SnapSettings>%u</SnapSettings>\n", pad,
                Window->SnapSettings);
        fprintf(file, "%s  <StencilGroup>%u</StencilGroup>\n", pad,
                Window->StencilGroup);
        fprintf(file, "%s  <StencilGroupPos>%u</StencilGroupPos>\n", pad,
                Window->StencilGroupPos);
        fprintf(file, "%s  <TabSplitterPos>%f</TabSplitterPos>\n", pad,
                Window->TabSplitterPos);
        break;

    case vdx_types_Windows:
        Windows = (const struct vdx_Windows *)(p);
        fprintf(file, "%s<Windows", pad);
        if (Windows->ClientHeight_exists)
            fprintf(file, " ClientHeight='%u'",
                    Windows->ClientHeight);
        if (Windows->ClientWidth_exists)
            fprintf(file, " ClientWidth='%u'",
                    Windows->ClientWidth);
        fprintf(file, ">\n");
        fprintf(file, "%s  <Window>%u</Window>\n", pad,
                Windows->Window);
        break;

    case vdx_types_XForm:
        XForm = (const struct vdx_XForm *)(p);
        fprintf(file, "%s<XForm>\n", pad);
	if (XForm->Angle)
          fprintf(file, "%s  <Angle>%f</Angle>\n", pad,
                  XForm->Angle);
	if (XForm->FlipX)
          fprintf(file, "%s  <FlipX>%u</FlipX>\n", pad,
                  XForm->FlipX);
	if (XForm->FlipY)
          fprintf(file, "%s  <FlipY>%u</FlipY>\n", pad,
                  XForm->FlipY);
	if (XForm->Height)
          fprintf(file, "%s  <Height>%f</Height>\n", pad,
                  XForm->Height);
	if (XForm->LocPinX)
          fprintf(file, "%s  <LocPinX>%f</LocPinX>\n", pad,
                  XForm->LocPinX);
	if (XForm->LocPinY)
          fprintf(file, "%s  <LocPinY>%f</LocPinY>\n", pad,
                  XForm->LocPinY);
	if (XForm->PinX)
          fprintf(file, "%s  <PinX>%f</PinX>\n", pad,
                  XForm->PinX);
	if (XForm->PinY)
          fprintf(file, "%s  <PinY>%f</PinY>\n", pad,
                  XForm->PinY);
	if (XForm->ResizeMode)
          fprintf(file, "%s  <ResizeMode>%u</ResizeMode>\n", pad,
                  XForm->ResizeMode);
	if (XForm->Width)
          fprintf(file, "%s  <Width>%f</Width>\n", pad,
                  XForm->Width);
        break;

    case vdx_types_XForm1D:
        XForm1D = (const struct vdx_XForm1D *)(p);
        fprintf(file, "%s<XForm1D>\n", pad);
        fprintf(file, "%s  <BeginX>%f</BeginX>\n", pad,
                XForm1D->BeginX);
        fprintf(file, "%s  <BeginY>%f</BeginY>\n", pad,
                XForm1D->BeginY);
        fprintf(file, "%s  <EndX>%f</EndX>\n", pad,
                XForm1D->EndX);
        fprintf(file, "%s  <EndY>%f</EndY>\n", pad,
                XForm1D->EndY);
        break;

    case vdx_types_cp:
        cp = (const struct vdx_cp *)(p);
        fprintf(file, "<cp IX='%u'/>", cp->IX);
        if (!child)
            return;
        break;

    case vdx_types_fld:
        fld = (const struct vdx_fld *)(p);
        fprintf(file, "%s<fld IX='%u'", pad, fld->IX);
        if (!child)
        {
            fprintf(file, "/>\n");
            return;
        }
        fprintf(file, ">\n");
        break;

    case vdx_types_pp:
        pp = (const struct vdx_pp *)(p);
        fprintf(file, "<pp IX='%u'/>", pp->IX);
        if (!child)
            return;
        break;

    case vdx_types_tp:
        tp = (const struct vdx_tp *)(p);
        fprintf(file, "<tp IX='%u'/>", tp->IX);
        if (!child)
            return;
        break;

    case vdx_types_text:
        text = (const struct vdx_text *)(p);
        fputs(vdx_convert_xml_string(text->text), file);
        break;

    default:
         g_warning("Can't write object %u", Any->type);
    }
    while(child)
    {
        vdx_write_object(file, depth+1, child->data);
        child = child->next;
    }
    /* LibreOffice Draw 4.2.4.2 does not like </ForeignData> with padding, it gives: General input/output error. */
    if (Any->type == vdx_types_ForeignData)
        fprintf(file, "</%s>\n", vdx_Types[(int)Any->type]);
    else if (Any->type != vdx_types_text)
        fprintf(file, "%s</%s>\n", pad, vdx_Types[(int)Any->type]);
}

