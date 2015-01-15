/* -*- c-basic-offset:4; -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * Sozi objects support
 * Copyright (C) 2015 Paul Chavent
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
#ifndef SOZI_OBJECT_H
#define SOZI_OBJECT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* dia stuff */
#include "object.h"
#include "diasvgrenderer.h"
#include "text.h"

/**
 * Common properties defines
 */
#define SOZI_OBJECT_LINE_WIDTH 0.01

/**
 * A Sozi object is represented with a box with
 * free or fixed aspect ratio.
 */
typedef enum
{
    ASPECT_FREE,
    ASPECT_FIXED
} AspectType;

/**
 * Manage Sozi objects placement.
 *
 * This object hold the base geometry that manage
 * other Sozi objects (frame,media, ...) placement.
 *
 * Could inherit struct _Element instead of DiaObject ?
 *
 * \extends _DiaObject
 *
 */
typedef struct _SoziObject
{
    /* dia object inheritance */

    DiaObject  dia_object;

    /* geometry of the object */

    Point      center;
    real       width;
    real       height;
    int        angle;  /* in degree , positive from x axis to y axis */
    AspectType aspect;
    gboolean   scale_from_center;

    /* geometry of the object : internal stuff */

    real       cos_angle;
    real       sin_angle;

    real       m[6]; /* transformation matrix of the unit square */

    Point      corners[4]; /* corners of the object */

    /* interactive rendering appearence */

    struct
    {
      gboolean       disp;
      Text *         text;
      TextAttributes attrs;
    } legend;

} SoziObject;

/**
 * Initialize an already *allocated* object.
 * @param[in] sozi_object pointer to object
 * @param[in] center initial position of the object
 */
void sozi_object_init(SoziObject *sozi_object, Point *center);

/**
 * Cleanup the object.
 * @param[in] sozi_object pointer to object
 */
void sozi_object_kill(SoziObject *sozi_object);

/**
 * Update the object.
 * @param[in] sozi_object pointer to object
 */
void sozi_object_update(SoziObject *sozi_object);

/**
 * Helper function for the dia's DrawFunc
 * @param[in] sozi_object pointer to object
 * @param[in] svg_renderer
 * @param[in] refif string describing the refid of the object
 * @param[out] p_sozi_name_space a pointer to the sozi namespace pointer
 * @param[out] p_root a pointer to the root element
 * @param[out] p_rect a pointer to the sozi rect element
 */
void sozi_object_draw_svg(SoziObject *sozi_object, DiaSvgRenderer *svg_renderer, gchar * refid, xmlNs **p_sozi_name_space, xmlNodePtr * p_root, xmlNodePtr * p_rect);

/**
 * Helper function for the dia's MoveFunc
 * @param[in] sozi_object pointer to object
 * @param[in] to destination point pointer
 * @see lib/object.h
 */
ObjectChange* sozi_object_move(SoziObject *sozi_object, Point *to);

/**
 * Helper function for the dia's MoveHandleFunc
 * @param[in] sozi_object pointer to object
 * @see lib/object.h
 */
ObjectChange* sozi_object_move_handle(SoziObject *sozi_object, Handle *handle,
                                      Point *to, ConnectionPoint *cp,
                                      HandleMoveReason reason, ModifierKeys modifiers);

#endif /* SOZI_OBJECT_H */
