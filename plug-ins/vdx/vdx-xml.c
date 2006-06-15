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

/* Generated Sun May 21 23:11:05 2006 */
/* From: All.vdx Circle1.vdx Circle2.vdx Line1.vdx Line2.vdx Line3.vdx Line4.vdx Line5.vdx Line6.vdx Rectangle1.vdx Rectangle2.vdx Rectangle3.vdx Rectangle4.vdx Text1.vdx Text2.vdx Text3.vdx samp_vdx.vdx Entreprise_etat_desire.vdx animation_tests.vdx basic_tests.vdx curve_tests.vdx pattern_tests.vdx seq_test.vdx text_tests.vdx */


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
            if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Action"))
            { if (child->children)
                s->Action = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BeginGroup"))
            { if (child->children)
                s->BeginGroup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ButtonFace"))
            { if (child->children)
                s->ButtonFace = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Checked"))
            { if (child->children)
                s->Checked = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Disabled"))
            { if (child->children)
                s->Disabled = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Invisible"))
            { if (child->children)
                s->Invisible = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Menu"))
            { if (child->children)
                s->Menu = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "ReadOnly"))
            { if (child->children)
                s->ReadOnly = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SortKey"))
            { if (child->children)
                s->SortKey = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TagName"))
            { if (child->children)
                s->TagName = atoi((char *)child->children->content); }
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
                s->Y = atof((char *)child->children->content); }
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AsianFont"))
            { if (child->children)
                s->AsianFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Case"))
            { if (child->children)
                s->Case = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Color"))
            { if (child->children)
                s->Color = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "ColorTrans"))
            { if (child->children)
                s->ColorTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ComplexScriptFont"))
            { if (child->children)
                s->ComplexScriptFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ComplexScriptSize"))
            { if (child->children)
                s->ComplexScriptSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DblUnderline"))
            { if (child->children)
                s->DblUnderline = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DoubleStrikethrough"))
            { if (child->children)
                s->DoubleStrikethrough = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Font"))
            { if (child->children)
                s->Font = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FontScale"))
            { if (child->children)
                s->FontScale = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Highlight"))
            { if (child->children)
                s->Highlight = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LangID"))
            { if (child->children)
                s->LangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Letterspace"))
            { if (child->children)
                s->Letterspace = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Locale"))
            { if (child->children)
                s->Locale = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocalizeFont"))
            { if (child->children)
                s->LocalizeFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Overline"))
            { if (child->children)
                s->Overline = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Perpendicular"))
            { if (child->children)
                s->Perpendicular = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Pos"))
            { if (child->children)
                s->Pos = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "RTLText"))
            { if (child->children)
                s->RTLText = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Size"))
            { if (child->children)
                s->Size = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Strikethru"))
            { if (child->children)
                s->Strikethru = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Style"))
            { if (child->children)
                s->Style = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UseVertical"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "RGB"))
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
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "FromCell"))
                s->FromCell = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "FromPart"))
                s->FromPart = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "FromSheet"))
                s->FromSheet = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ToCell"))
                s->ToCell = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ToPart"))
                s->ToPart = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ToSheet"))
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
            if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "AutoGen"))
            { if (child->children)
                s->AutoGen = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DirX"))
            { if (child->children)
                s->DirX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DirY"))
            { if (child->children)
                s->DirY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children)
                s->Prompt = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Type"))
            { if (child->children)
                s->Type = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "CanGlue"))
            { if (child->children)
                s->CanGlue = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children)
                s->Prompt = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XCon"))
            { if (child->children)
                s->XCon = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XDyn"))
            { if (child->children)
                s->XDyn = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
                s->Y = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YCon"))
            { if (child->children)
                s->YCon = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YDyn"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "PropType"))
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
            { if (child->children)
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
            { if (child->children)
                s->AddMarkup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DocLangID"))
            { if (child->children)
                s->DocLangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockPreview"))
            { if (child->children)
                s->LockPreview = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "OutputFormat"))
            { if (child->children)
                s->OutputFormat = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PreviewQuality"))
            { if (child->children)
                s->PreviewQuality = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PreviewScope"))
            { if (child->children)
                s->PreviewScope = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ViewMarkup"))
            { if (child->children)
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
            { if (child->children)
                s->BuildNumberCreated = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BuildNumberEdited"))
            { if (child->children)
                s->BuildNumberEdited = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Company"))
            { if (child->children)
                s->Company = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Creator"))
            { if (child->children)
                s->Creator = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Desc"))
            { if (child->children)
                s->Desc = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Template"))
            { if (child->children)
                s->Template = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimeCreated"))
            { if (child->children)
                s->TimeCreated = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimeEdited"))
            { if (child->children)
                s->TimeEdited = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimePrinted"))
            { if (child->children)
                s->TimePrinted = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "TimeSaved"))
            { if (child->children)
                s->TimeSaved = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Title"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "DefaultFillStyle"))
                s->DefaultFillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "DefaultGuideStyle"))
                s->DefaultGuideStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "DefaultLineStyle"))
                s->DefaultLineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "DefaultTextStyle"))
                s->DefaultTextStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "TopPage"))
                s->TopPage = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "DynamicGridEnabled"))
            { if (child->children)
                s->DynamicGridEnabled = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "GlueSettings"))
            { if (child->children)
                s->GlueSettings = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectBkgnds"))
            { if (child->children)
                s->ProtectBkgnds = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectMasters"))
            { if (child->children)
                s->ProtectMasters = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectShapes"))
            { if (child->children)
                s->ProtectShapes = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ProtectStyles"))
            { if (child->children)
                s->ProtectStyles = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapExtensions"))
            { if (child->children)
                s->SnapExtensions = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapSettings"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "FillStyle"))
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle"))
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle"))
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children)
                s->B = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children)
                s->C = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children)
                s->D = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children)
                s->B = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children)
                s->C = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children)
                s->D = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
                s->Y = atof((char *)child->children->content); }
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
            { if (child->children)
                s->EventDblClick = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EventDrop"))
            { if (child->children)
                s->EventDrop = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EventXFMod"))
            { if (child->children)
                s->EventXFMod = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TheData"))
            { if (child->children)
                s->TheData = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TheText"))
            { if (child->children)
                s->TheText = atoi((char *)child->children->content); }
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
            if (!strcmp((char *)attr->name, "Action"))
                s->Action = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Enabled"))
                s->Enabled = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "EventCode"))
                s->EventCode = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Target"))
                s->Target = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TargetArgs"))
                s->TargetArgs = atoi((char *)attr->children->content);
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
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "CharSets"))
                s->CharSets = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Flags"))
                s->Flags = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Panos"))
                s->Panos = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UnicodeRanges"))
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
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "Del"))
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Calendar"))
            { if (child->children)
                s->Calendar = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EditMode"))
            { if (child->children)
                s->EditMode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Format"))
            { if (child->children)
                s->Format = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ObjectKind"))
            { if (child->children)
                s->ObjectKind = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Type"))
            { if (child->children)
                s->Type = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UICat"))
            { if (child->children)
                s->UICat = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UICod"))
            { if (child->children)
                s->UICod = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UIFmt"))
            { if (child->children)
                s->UIFmt = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Value"))
            { if (child->children)
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
            { if (child->children)
                s->FillBkgnd = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "FillBkgndTrans"))
            { if (child->children)
                s->FillBkgndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FillForegnd"))
            { if (child->children)
                s->FillForegnd = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "FillForegndTrans"))
            { if (child->children)
                s->FillForegndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FillPattern"))
            { if (child->children)
                s->FillPattern = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwObliqueAngle"))
            { if (child->children)
                s->ShapeShdwObliqueAngle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwOffsetX"))
            { if (child->children)
                s->ShapeShdwOffsetX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwOffsetY"))
            { if (child->children)
                s->ShapeShdwOffsetY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwScaleFactor"))
            { if (child->children)
                s->ShapeShdwScaleFactor = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeShdwType"))
            { if (child->children)
                s->ShapeShdwType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwBkgnd"))
            { if (child->children)
                s->ShdwBkgnd = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwBkgndTrans"))
            { if (child->children)
                s->ShdwBkgndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwForegnd"))
            { if (child->children)
                s->ShdwForegnd = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "ShdwForegndTrans"))
            { if (child->children)
                s->ShdwForegndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwPattern"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "Attributes"))
                s->Attributes = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "CharSet"))
                s->CharSet = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "PitchAndFamily"))
                s->PitchAndFamily = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Unicode"))
                s->Unicode = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Weight"))
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
            { if (child->children)
                s->FontEntry = atoi((char *)child->children->content); }
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "NoFill"))
            { if (child->children)
                s->NoFill = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoLine"))
            { if (child->children)
                s->NoLine = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoShow"))
            { if (child->children)
                s->NoShow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoSnap"))
            { if (child->children)
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
            { if (child->children)
                s->DisplayMode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DontMoveChildren"))
            { if (child->children)
                s->DontMoveChildren = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsDropTarget"))
            { if (child->children)
                s->IsDropTarget = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsSnapTarget"))
            { if (child->children)
                s->IsSnapTarget = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsTextEditTarget"))
            { if (child->children)
                s->IsTextEditTarget = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SelectMode"))
            { if (child->children)
                s->SelectMode = atoi((char *)child->children->content); }
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
            { if (child->children)
                s->Copyright = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "HelpTopic"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Address"))
            { if (child->children)
                s->Address = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Default"))
            { if (child->children)
                s->Default = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Description"))
            { if (child->children)
                s->Description = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "ExtraInfo"))
            { if (child->children)
                s->ExtraInfo = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Frame"))
            { if (child->children)
                s->Frame = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Invisible"))
            { if (child->children)
                s->Invisible = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NewWindow"))
            { if (child->children)
                s->NewWindow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SortKey"))
            { if (child->children)
                s->SortKey = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SubAddress"))
            { if (child->children)
                s->SubAddress = (char *)child->children->content; }
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
            { if (child->children)
                s->Blur = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Brightness"))
            { if (child->children)
                s->Brightness = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Contrast"))
            { if (child->children)
                s->Contrast = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Denoise"))
            { if (child->children)
                s->Denoise = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Gamma"))
            { if (child->children)
                s->Gamma = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Sharpen"))
            { if (child->children)
                s->Sharpen = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Transparency"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children)
                s->B = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Active"))
            { if (child->children)
                s->Active = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Color"))
            { if (child->children)
                s->Color = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ColorTrans"))
            { if (child->children)
                s->ColorTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Glue"))
            { if (child->children)
                s->Glue = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Lock"))
            { if (child->children)
                s->Lock = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Name"))
            { if (child->children)
                s->Name = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "NameUniv"))
            { if (child->children)
                s->NameUniv = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Print"))
            { if (child->children)
                s->Print = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Snap"))
            { if (child->children)
                s->Snap = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Status"))
            { if (child->children)
                s->Status = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Visible"))
            { if (child->children)
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
            { if (child->children)
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
            { if (child->children)
                s->ConFixedCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpCode"))
            { if (child->children)
                s->ConLineJumpCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpDirX"))
            { if (child->children)
                s->ConLineJumpDirX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpDirY"))
            { if (child->children)
                s->ConLineJumpDirY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineJumpStyle"))
            { if (child->children)
                s->ConLineJumpStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ConLineRouteExt"))
            { if (child->children)
                s->ConLineRouteExt = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeFixedCode"))
            { if (child->children)
                s->ShapeFixedCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePermeablePlace"))
            { if (child->children)
                s->ShapePermeablePlace = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePermeableX"))
            { if (child->children)
                s->ShapePermeableX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePermeableY"))
            { if (child->children)
                s->ShapePermeableY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePlaceFlip"))
            { if (child->children)
                s->ShapePlaceFlip = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapePlowCode"))
            { if (child->children)
                s->ShapePlowCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeRouteStyle"))
            { if (child->children)
                s->ShapeRouteStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeSplit"))
            { if (child->children)
                s->ShapeSplit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeSplittable"))
            { if (child->children)
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
            { if (child->children)
                s->BeginArrow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BeginArrowSize"))
            { if (child->children)
                s->BeginArrowSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndArrow"))
            { if (child->children)
                s->EndArrow = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndArrowSize"))
            { if (child->children)
                s->EndArrowSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineCap"))
            { if (child->children)
                s->LineCap = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineColor"))
            { if (child->children)
                s->LineColor = vdx_parse_color((char *)child->children->content, theDoc); }
            else if (!strcmp((char *)child->name, "LineColorTrans"))
            { if (child->children)
                s->LineColorTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LinePattern"))
            { if (child->children)
                s->LinePattern = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineWeight"))
            { if (child->children)
                s->LineWeight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Rounding"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "Del"))
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "AlignName"))
                s->AlignName = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "BaseID"))
                s->BaseID = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Hidden"))
                s->Hidden = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IconSize"))
                s->IconSize = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "IconUpdate"))
                s->IconUpdate = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "MatchByName"))
                s->MatchByName = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "PatternFlags"))
                s->PatternFlags = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Prompt"))
                s->Prompt = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UniqueID"))
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
            { if (child->children)
                s->BegTrigger = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Calendar"))
            { if (child->children)
                s->Calendar = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Comment"))
            { if (child->children)
                s->Comment = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "DropOnPageScale"))
            { if (child->children)
                s->DropOnPageScale = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DynFeedback"))
            { if (child->children)
                s->DynFeedback = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndTrigger"))
            { if (child->children)
                s->EndTrigger = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "GlueType"))
            { if (child->children)
                s->GlueType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HideText"))
            { if (child->children)
                s->HideText = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IsDropSource"))
            { if (child->children)
                s->IsDropSource = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LangID"))
            { if (child->children)
                s->LangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocalizeMerge"))
            { if (child->children)
                s->LocalizeMerge = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoAlignBox"))
            { if (child->children)
                s->NoAlignBox = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoCtlHandles"))
            { if (child->children)
                s->NoCtlHandles = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoLiveDynamics"))
            { if (child->children)
                s->NoLiveDynamics = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NoObjHandles"))
            { if (child->children)
                s->NoObjHandles = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "NonPrinting"))
            { if (child->children)
                s->NonPrinting = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ObjType"))
            { if (child->children)
                s->ObjType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShapeKeywords"))
            { if (child->children)
                s->ShapeKeywords = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "UpdateAlignBox"))
            { if (child->children)
                s->UpdateAlignBox = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "WalkPreference"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children)
                s->B = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children)
                s->C = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children)
                s->D = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "E"))
            { if (child->children)
                s->E = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "Err"))
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
            if (!strcmp((char *)attr->name, "BackPage"))
                s->BackPage = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Background"))
                s->Background = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ViewCenterX"))
                s->ViewCenterX = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewCenterY"))
                s->ViewCenterY = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewScale"))
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
            { if (child->children)
                s->AvenueSizeX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "AvenueSizeY"))
            { if (child->children)
                s->AvenueSizeY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BlockSizeX"))
            { if (child->children)
                s->BlockSizeX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BlockSizeY"))
            { if (child->children)
                s->BlockSizeY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "CtrlAsInput"))
            { if (child->children)
                s->CtrlAsInput = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DynamicsOff"))
            { if (child->children)
                s->DynamicsOff = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EnableGrid"))
            { if (child->children)
                s->EnableGrid = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineAdjustFrom"))
            { if (child->children)
                s->LineAdjustFrom = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineAdjustTo"))
            { if (child->children)
                s->LineAdjustTo = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpCode"))
            { if (child->children)
                s->LineJumpCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpFactorX"))
            { if (child->children)
                s->LineJumpFactorX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpFactorY"))
            { if (child->children)
                s->LineJumpFactorY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineJumpStyle"))
            { if (child->children)
                s->LineJumpStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineRouteExt"))
            { if (child->children)
                s->LineRouteExt = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToLineX"))
            { if (child->children)
                s->LineToLineX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToLineY"))
            { if (child->children)
                s->LineToLineY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToNodeX"))
            { if (child->children)
                s->LineToNodeX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LineToNodeY"))
            { if (child->children)
                s->LineToNodeY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLineJumpDirX"))
            { if (child->children)
                s->PageLineJumpDirX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLineJumpDirY"))
            { if (child->children)
                s->PageLineJumpDirY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageShapeSplit"))
            { if (child->children)
                s->PageShapeSplit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlaceDepth"))
            { if (child->children)
                s->PlaceDepth = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlaceFlip"))
            { if (child->children)
                s->PlaceFlip = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlaceStyle"))
            { if (child->children)
                s->PlaceStyle = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PlowCode"))
            { if (child->children)
                s->PlowCode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ResizePage"))
            { if (child->children)
                s->ResizePage = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "RouteStyle"))
            { if (child->children)
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
            { if (child->children)
                s->DrawingScale = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DrawingScaleType"))
            { if (child->children)
                s->DrawingScaleType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DrawingSizeType"))
            { if (child->children)
                s->DrawingSizeType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "InhibitSnap"))
            { if (child->children)
                s->InhibitSnap = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageHeight"))
            { if (child->children)
                s->PageHeight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageScale"))
            { if (child->children)
                s->PageScale = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageWidth"))
            { if (child->children)
                s->PageWidth = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwObliqueAngle"))
            { if (child->children)
                s->ShdwObliqueAngle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwOffsetX"))
            { if (child->children)
                s->ShdwOffsetX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwOffsetY"))
            { if (child->children)
                s->ShdwOffsetY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwScaleFactor"))
            { if (child->children)
                s->ShdwScaleFactor = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShdwType"))
            { if (child->children)
                s->ShdwType = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "UIVisibility"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "FillStyle"))
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle"))
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "TextStyle"))
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

    if (!strcmp((char *)cur->name, "Para")) {
        struct vdx_Para *s;
        if (p) { s = (struct vdx_Para *)(p); }
        else { s = g_new0(struct vdx_Para, 1); }
        s->children = 0;
        s->type = vdx_types_Para;
        for (attr = cur->properties; attr; attr = attr->next) {
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Bullet"))
            { if (child->children)
                s->Bullet = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BulletFont"))
            { if (child->children)
                s->BulletFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BulletFontSize"))
            { if (child->children)
                s->BulletFontSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BulletStr"))
            { if (child->children)
                s->BulletStr = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Flags"))
            { if (child->children)
                s->Flags = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HorzAlign"))
            { if (child->children)
                s->HorzAlign = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IndFirst"))
            { if (child->children)
                s->IndFirst = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IndLeft"))
            { if (child->children)
                s->IndLeft = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "IndRight"))
            { if (child->children)
                s->IndRight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocalizeBulletFont"))
            { if (child->children)
                s->LocalizeBulletFont = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SpAfter"))
            { if (child->children)
                s->SpAfter = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SpBefore"))
            { if (child->children)
                s->SpBefore = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SpLine"))
            { if (child->children)
                s->SpLine = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextPosAfterBullet"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "Size"))
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
            { if (child->children)
                s->CenterX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "CenterY"))
            { if (child->children)
                s->CenterY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "OnPage"))
            { if (child->children)
                s->OnPage = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageBottomMargin"))
            { if (child->children)
                s->PageBottomMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLeftMargin"))
            { if (child->children)
                s->PageLeftMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageRightMargin"))
            { if (child->children)
                s->PageRightMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageTopMargin"))
            { if (child->children)
                s->PageTopMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PagesX"))
            { if (child->children)
                s->PagesX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PagesY"))
            { if (child->children)
                s->PagesY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PaperKind"))
            { if (child->children)
                s->PaperKind = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PaperSource"))
            { if (child->children)
                s->PaperSource = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintGrid"))
            { if (child->children)
                s->PrintGrid = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintPageOrientation"))
            { if (child->children)
                s->PrintPageOrientation = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ScaleX"))
            { if (child->children)
                s->ScaleX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ScaleY"))
            { if (child->children)
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
            { if (child->children)
                s->PageBottomMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageLeftMargin"))
            { if (child->children)
                s->PageLeftMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageRightMargin"))
            { if (child->children)
                s->PageRightMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PageTopMargin"))
            { if (child->children)
                s->PageTopMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PaperSize"))
            { if (child->children)
                s->PaperSize = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintCenteredH"))
            { if (child->children)
                s->PrintCenteredH = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintCenteredV"))
            { if (child->children)
                s->PrintCenteredV = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintFitOnPages"))
            { if (child->children)
                s->PrintFitOnPages = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintLandscape"))
            { if (child->children)
                s->PrintLandscape = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintPagesAcross"))
            { if (child->children)
                s->PrintPagesAcross = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintPagesDown"))
            { if (child->children)
                s->PrintPagesDown = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PrintScale"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Calendar"))
            { if (child->children)
                s->Calendar = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Format"))
            { if (child->children)
                s->Format = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Invisible"))
            { if (child->children)
                s->Invisible = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Label"))
            { if (child->children)
                s->Label = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "LangID"))
            { if (child->children)
                s->LangID = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children)
                s->Prompt = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "SortKey"))
            { if (child->children)
                s->SortKey = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Type"))
            { if (child->children)
                s->Type = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Value"))
            { if (child->children)
                s->Value = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Verify"))
            { if (child->children)
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
            { if (child->children)
                s->LockAspect = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockBegin"))
            { if (child->children)
                s->LockBegin = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockCalcWH"))
            { if (child->children)
                s->LockCalcWH = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockCrop"))
            { if (child->children)
                s->LockCrop = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockCustProp"))
            { if (child->children)
                s->LockCustProp = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockDelete"))
            { if (child->children)
                s->LockDelete = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockEnd"))
            { if (child->children)
                s->LockEnd = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockFormat"))
            { if (child->children)
                s->LockFormat = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockGroup"))
            { if (child->children)
                s->LockGroup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockHeight"))
            { if (child->children)
                s->LockHeight = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockMoveX"))
            { if (child->children)
                s->LockMoveX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockMoveY"))
            { if (child->children)
                s->LockMoveY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockRotate"))
            { if (child->children)
                s->LockRotate = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockSelect"))
            { if (child->children)
                s->LockSelect = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockTextEdit"))
            { if (child->children)
                s->LockTextEdit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockVtxEdit"))
            { if (child->children)
                s->LockVtxEdit = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LockWidth"))
            { if (child->children)
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
            { if (child->children)
                s->XGridDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XGridOrigin"))
            { if (child->children)
                s->XGridOrigin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XGridSpacing"))
            { if (child->children)
                s->XGridSpacing = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XRulerDensity"))
            { if (child->children)
                s->XRulerDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "XRulerOrigin"))
            { if (child->children)
                s->XRulerOrigin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YGridDensity"))
            { if (child->children)
                s->YGridDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YGridOrigin"))
            { if (child->children)
                s->YGridOrigin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YGridSpacing"))
            { if (child->children)
                s->YGridSpacing = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YRulerDensity"))
            { if (child->children)
                s->YRulerDensity = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "YRulerOrigin"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children)
                s->B = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children)
                s->C = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children)
                s->D = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "Del"))
                s->Del = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "FillStyle"))
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle"))
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Master"))
                s->Master = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "MasterShape"))
                s->MasterShape = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle"))
                s->TextStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Type"))
                s->Type = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "UniqueID"))
                s->UniqueID = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Field"))
            { if (child->children)
                s->Field = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Tabs"))
            { if (child->children)
                s->Tabs = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "cp"))
            { if (child->children)
                s->cp = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "fld"))
            { if (child->children)
                s->fld = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "pp"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
                s->IX = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "A"))
            { if (child->children)
                s->A = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "B"))
            { if (child->children)
                s->B = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "C"))
            { if (child->children)
                s->C = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "D"))
            { if (child->children)
                s->D = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "X"))
            { if (child->children)
                s->X = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Y"))
            { if (child->children)
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
            { if (child->children)
                s->EnableFillProps = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EnableLineProps"))
            { if (child->children)
                s->EnableLineProps = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EnableTextProps"))
            { if (child->children)
                s->EnableTextProps = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "HideForApply"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "FillStyle"))
                s->FillStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "LineStyle"))
                s->LineStyle = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Name"))
                s->Name = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "TextStyle"))
                s->TextStyle = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Tabs"))
            { if (child->children)
                s->Tabs = atoi((char *)child->children->content); }
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
            if (!strcmp((char *)attr->name, "IX"))
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
            if (!strcmp((char *)attr->name, "IX"))
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
            { if (child->children)
                s->BottomMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "DefaultTabStop"))
            { if (child->children)
                s->DefaultTabStop = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LeftMargin"))
            { if (child->children)
                s->LeftMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "RightMargin"))
            { if (child->children)
                s->RightMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextBkgnd"))
            { if (child->children)
                s->TextBkgnd = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextBkgndTrans"))
            { if (child->children)
                s->TextBkgndTrans = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TextDirection"))
            { if (child->children)
                s->TextDirection = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TopMargin"))
            { if (child->children)
                s->TopMargin = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "VerticalAlign"))
            { if (child->children)
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
            { if (child->children)
                s->TxtAngle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtHeight"))
            { if (child->children)
                s->TxtHeight = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtLocPinX"))
            { if (child->children)
                s->TxtLocPinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtLocPinY"))
            { if (child->children)
                s->TxtLocPinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtPinX"))
            { if (child->children)
                s->TxtPinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtPinY"))
            { if (child->children)
                s->TxtPinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TxtWidth"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "NameU"))
                s->NameU = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Prompt"))
            { if (child->children)
                s->Prompt = (char *)child->children->content; }
            else if (!strcmp((char *)child->name, "Value"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "DocLangID"))
                s->DocLangID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "buildnum"))
                s->buildnum = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "key"))
                s->key = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "metric"))
                s->metric = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "start"))
                s->start = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "version"))
                s->version = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "xmlns"))
                s->xmlns = (char *)attr->children->content;
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "EventList"))
            { if (child->children)
                s->EventList = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Masters"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "ContainerType"))
                s->ContainerType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "Document"))
                s->Document = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "ID"))
                s->ID = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "Page"))
                s->Page = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ParentWindow"))
                s->ParentWindow = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewCenterX"))
                s->ViewCenterX = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewCenterY"))
                s->ViewCenterY = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ViewScale"))
                s->ViewScale = atof((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowHeight"))
                s->WindowHeight = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowLeft"))
                s->WindowLeft = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowState"))
                s->WindowState = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowTop"))
                s->WindowTop = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "WindowType"))
                s->WindowType = (char *)attr->children->content;
            else if (!strcmp((char *)attr->name, "WindowWidth"))
                s->WindowWidth = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "DynamicGridEnabled"))
            { if (child->children)
                s->DynamicGridEnabled = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "GlueSettings"))
            { if (child->children)
                s->GlueSettings = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowConnectionPoints"))
            { if (child->children)
                s->ShowConnectionPoints = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowGrid"))
            { if (child->children)
                s->ShowGrid = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowGuides"))
            { if (child->children)
                s->ShowGuides = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowPageBreaks"))
            { if (child->children)
                s->ShowPageBreaks = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ShowRulers"))
            { if (child->children)
                s->ShowRulers = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapExtensions"))
            { if (child->children)
                s->SnapExtensions = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "SnapSettings"))
            { if (child->children)
                s->SnapSettings = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "StencilGroup"))
            { if (child->children)
                s->StencilGroup = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "StencilGroupPos"))
            { if (child->children)
                s->StencilGroupPos = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "TabSplitterPos"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "ClientHeight"))
                s->ClientHeight = atoi((char *)attr->children->content);
            else if (!strcmp((char *)attr->name, "ClientWidth"))
                s->ClientWidth = atoi((char *)attr->children->content);
        }
        for (child = cur->xmlChildrenNode; child; child = child->next) {
            if (xmlIsBlankNode(child)) { continue; }
            if (!strcmp((char *)child->name, "Window"))
            { if (child->children)
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
            { if (child->children)
                s->Angle = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FlipX"))
            { if (child->children)
                s->FlipX = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "FlipY"))
            { if (child->children)
                s->FlipY = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Height"))
            { if (child->children)
                s->Height = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocPinX"))
            { if (child->children)
                s->LocPinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "LocPinY"))
            { if (child->children)
                s->LocPinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PinX"))
            { if (child->children)
                s->PinX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "PinY"))
            { if (child->children)
                s->PinY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "ResizeMode"))
            { if (child->children)
                s->ResizeMode = atoi((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "Width"))
            { if (child->children)
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
            { if (child->children)
                s->BeginX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "BeginY"))
            { if (child->children)
                s->BeginY = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndX"))
            { if (child->children)
                s->EndX = atof((char *)child->children->content); }
            else if (!strcmp((char *)child->name, "EndY"))
            { if (child->children)
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
            if (!strcmp((char *)attr->name, "IX"))
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
            if (!strcmp((char *)attr->name, "IX"))
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
            if (!strcmp((char *)attr->name, "IX"))
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
            if (!strcmp((char *)attr->name, "IX"))
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
