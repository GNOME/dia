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

/* standard libs*/
#include <assert.h>
#include <math.h>

/* xpath for searching in the svg doc (xmlDocGetRootElement) */
#include <libxml/xpath.h>

/* dia stuff */
#include "diarenderer.h"
#include "message.h"

/* sozi stuff */
#include "sozi-player.h"
#include "sozi-object.h"

/******************************************************************************
 * unit square
 *****************************************************************************/
static const Handle default_handles[4] =
{
    {HANDLE_RESIZE_NW, HANDLE_MAJOR_CONTROL, { 0, 0}, HANDLE_NONCONNECTABLE, NULL},
    {HANDLE_RESIZE_SW, HANDLE_MAJOR_CONTROL, { 0, 1}, HANDLE_NONCONNECTABLE, NULL},
    {HANDLE_RESIZE_SE, HANDLE_MAJOR_CONTROL, { 1, 1}, HANDLE_NONCONNECTABLE, NULL},
    {HANDLE_RESIZE_NE, HANDLE_MAJOR_CONTROL, { 1, 0}, HANDLE_NONCONNECTABLE, NULL},
};

/******************************************************************************
 * FUNCTIONS FOR MANAGING ACCESS TO THE SVG SOZI PLAYER ENGINE
 *****************************************************************************/

/**
 * Test the presence of the sozi player in an svg document.
 * @param[in] doc the svg document
 * @param[in] sozi_elem the sozi player identification string that will
 * probably be "//script[@id='sozi-script']"
 * @return 1 if the sozi player has been found, 0 if not, -1 if errors 
 * happened.
 */
static int
sozi_player_is_present(xmlDocPtr doc, const xmlChar *sozi_elem)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;
    int is_present;

    context = xmlXPathNewContext(doc);
    if (context == NULL) {
        g_warning("sozi-object : error in xmlXPathNewContext\n");
        return -1;
    }

    result = xmlXPathEvalExpression(sozi_elem, context);
    xmlXPathFreeContext(context);
    if (result == NULL) {
        g_warning("sozi-object : error in xmlXPathEvalExpression\n");
        return -1;
    }

    is_present = xmlXPathNodeSetIsEmpty(result->nodesetval)?0:1;

    xmlXPathFreeObject(result);

    return is_present;
}

/**
 * Retreive the sozi player.
 * @param[out] sozi_version_ptr sozi version placeholder
 * @param[out] sozi_js_ptr sozi player placeholder
 * @param[out] sozi_extras_media_js_ptr sozi player media extension placeholder
 * @param[out] sozi_css_ptr sozi stylesheet placeholder
 */
static void
sozi_player_get(gchar **sozi_version_ptr, gchar **sozi_js_ptr, gchar **sozi_extras_media_js_ptr, gchar **sozi_css_ptr)
{
#if defined(SOZI_PATH)
    /* try to get sozi engine from SOZI_PATH */
    struct
    {
        const gchar * filename;
        gchar **      data;
    } external_sozi[] =
          {
              { SOZI_PATH"/sozi.version"         , sozi_version_ptr         },
              { SOZI_PATH"/sozi.js"              , sozi_js_ptr              },
              { SOZI_PATH"/sozi_extras_media.js" , sozi_extras_media_js_ptr },
              { SOZI_PATH"/sozi.css"             , sozi_css_ptr             }
          };
    static const unsigned external_sozi_cnt = sizeof(external_sozi)/sizeof(external_sozi[0]);
    unsigned i;

    for (i = 0; i < external_sozi_cnt; i++) {
        GError * err = 0;
        if (!g_file_get_contents(external_sozi[i].filename,
                                 external_sozi[i].data,
                                 NULL,
                                 &err)) {
            message_error("sozi-object : %s\nWill use builtin player", err->message);
            g_error_free (err);
            while(i--) {
                g_free (*external_sozi[i].data);
            }
            break;
        }
    }

    if (i == external_sozi_cnt) {
        /* that's ok, don't need to go further */
        return;
    }
#endif /* defined(SOZI_PATH) */

    /* if SOZI_PATH isn't defined, fallback to the player that comes with dia sources */
    *sozi_version_ptr         = g_strdup((const gchar *)sozi_version);
    *sozi_js_ptr              = g_strdup((const gchar *)sozi_min_js);
    *sozi_extras_media_js_ptr = g_strdup((const gchar *)sozi_extras_media_min_js);
    *sozi_css_ptr             = g_strdup((const gchar *)sozi_min_css);
}

/******************************************************************************
 * FUNCTIONS FOR MANAGING THE SOZI OBJECT INIT AND UPDATE
 *****************************************************************************/

void
sozi_object_init(SoziObject *sozi_object, Point *center)
{
    DiaObject *dia_object;
    int i;

    dia_object = &sozi_object->dia_object;

    dia_object->position = *center;

    /* dia_object->bounding_box will be set in sozi_object_update */

    dia_object->num_handles = 4;
    if (dia_object->handles == NULL) {
        dia_object->handles = g_new0(Handle *, 4);
    }

    for (i = 0; i < 4; i++) {
        if (dia_object->handles[i] == NULL) {
            dia_object->handles[i] = g_new0(Handle, 1);
        }
        *dia_object->handles[i] = default_handles[i];
    }

    dia_object->num_connections = 1;
    if (dia_object->connections == NULL) {
        dia_object->connections = g_new0(ConnectionPoint *, 1);
    }

    if (dia_object->connections[0] == NULL) {
        dia_object->connections[0] = g_new0(ConnectionPoint, 1);
    }
    dia_object->connections[0]->object = dia_object;
    dia_object->connections[0]->directions = DIR_ALL;

    /* dia_object->bounding_box will be set in sozi_object_update */

    sozi_object->center = *center;
    sozi_object->width = 4;
    sozi_object->height = 3;
    sozi_object->angle = 0;
    sozi_object->aspect = ASPECT_FIXED;
    sozi_object->scale_from_center = FALSE;

    /* sozi_object->cos_angle will be set in sozi_object_update */
    /* sozi_object->sin_angle will be set in sozi_object_update */
    /* sozi_object->m         will be set in sozi_object_update */

    /* sozi_object->corners are set in sozi_object_update */

    sozi_object->legend.disp = TRUE;
    sozi_object->legend.text = new_text_default(center, &color_black, ALIGN_LEFT);
    text_get_attributes(sozi_object->legend.text, &sozi_object->legend.attrs);
}

void
sozi_object_kill(SoziObject *sozi_object)
{
    int i;

    text_destroy(sozi_object->legend.text);

    object_unconnect_all(&sozi_object->dia_object);

    for (i = 0; i < 1; i++) {
        if (sozi_object->dia_object.connections[i] != NULL) {
            g_free(sozi_object->dia_object.connections[i]);
        }
    }

    for (i = 0; i < 4; i++) {
        if (sozi_object->dia_object.handles[i] != NULL) {
            g_free(sozi_object->dia_object.handles[i]);
        }
    }

    if (sozi_object->dia_object.connections) {
        g_free(sozi_object->dia_object.connections);
        sozi_object->dia_object.connections = NULL;
    }

    if (sozi_object->dia_object.handles) {
        g_free(sozi_object->dia_object.handles);
        sozi_object->dia_object.handles = NULL;
    }

    if (sozi_object->dia_object.meta) {
        g_hash_table_destroy (sozi_object->dia_object.meta);
        sozi_object->dia_object.meta = NULL;
    }
}

void
sozi_object_update(SoziObject *sozi_object)
{
    DiaObject *dia_object;
    int i;
    Point legend_pos;
    Rectangle legend_bb;

    dia_object = &sozi_object->dia_object;

    dia_object->position = sozi_object->center;

    dia_object->bounding_box.left   =  G_MAXFLOAT;
    dia_object->bounding_box.top    =  G_MAXFLOAT;
    dia_object->bounding_box.right  = -G_MAXFLOAT;
    dia_object->bounding_box.bottom = -G_MAXFLOAT;

    /* angle */
    if (sozi_object->angle < -180) {
        sozi_object->angle += 360.0;
    }
    if (180 < sozi_object->angle) {
        sozi_object->angle -= 360;
    }
    sozi_object->cos_angle = cos(sozi_object->angle * M_PI / 180.0);
    sozi_object->sin_angle = sin(sozi_object->angle * M_PI / 180.0);

    /*
     *
     * translate(center.x,center.y) . rotate(angle) . scale(width,height) . translate(-0.5,-0.5)
     *
     * m
     * =
     * | 1 0 x | . | cos(a) -sin(a) 0 | . | w 0 0 | . | 1 0 -0.5 |
     * | 0 1 y |   | sin(a)  cos(a) 0 |   | 0 h 0 |   | 0 1 -0.5 |
     * | 0 0 1 |   | 0       0      1 |   | 0 0 1 |   | 0 0  1   |
     * =
     * |  w*cos(a) -h*sin(a) x-0.5*w*cos(a)+0.5*h*sin(a) |
     * |  w*sin(a)  h*cos(a) y-0.5*w*sin(a)-0.5*h*cos(a) |
     * |  0         0        1                           |
     */
    sozi_object->m[0] =  sozi_object->width  * sozi_object->cos_angle ;
    sozi_object->m[1] = -sozi_object->height * sozi_object->sin_angle ;
    sozi_object->m[2] =  sozi_object->center.x - 0.5 * sozi_object->width * sozi_object->cos_angle + 0.5 * sozi_object->height * sozi_object->sin_angle;
    sozi_object->m[3] =  sozi_object->width  * sozi_object->sin_angle ;
    sozi_object->m[4] =  sozi_object->height * sozi_object->cos_angle ;
    sozi_object->m[5] =  sozi_object->center.y - 0.5 * sozi_object->width * sozi_object->sin_angle - 0.5 * sozi_object->height * sozi_object->cos_angle;

    for (i = 0; i < 4; i++) {

        sozi_object->corners[i].x = sozi_object->m[0] * default_handles[i].pos.x + sozi_object->m[1] * default_handles[i].pos.y + sozi_object->m[2];
        sozi_object->corners[i].y = sozi_object->m[3] * default_handles[i].pos.x + sozi_object->m[4] * default_handles[i].pos.y + sozi_object->m[5];

        dia_object->handles[i]->pos = sozi_object->corners[i];

        if (sozi_object->corners[i].x < dia_object->bounding_box.left) {
            dia_object->bounding_box.left = sozi_object->corners[i].x - SOZI_OBJECT_LINE_WIDTH;
        }
        if (dia_object->bounding_box.right < sozi_object->corners[i].x) {
            dia_object->bounding_box.right = sozi_object->corners[i].x + SOZI_OBJECT_LINE_WIDTH;
        }
        if (sozi_object->corners[i].y < dia_object->bounding_box.top) {
            dia_object->bounding_box.top = sozi_object->corners[i].y - SOZI_OBJECT_LINE_WIDTH;
        }
        if (dia_object->bounding_box.bottom < sozi_object->corners[i].y) {
            dia_object->bounding_box.bottom = sozi_object->corners[i].y + SOZI_OBJECT_LINE_WIDTH;
        }
    }

    dia_object->connections[0]->pos = sozi_object->corners[0];

    /* setup legend appearence */
    legend_pos = sozi_object->corners[0];
    legend_pos.y += text_get_ascent(sozi_object->legend.text);
    text_set_position(sozi_object->legend.text, &legend_pos);

    /* final bounding / enclosing box with legend */
    text_calc_boundingbox(sozi_object->legend.text, &legend_bb);
    rectangle_union(&dia_object->bounding_box, &legend_bb);
}

/******************************************************************************
 * FUNCTIONS FOR MANAGING THE SOZI OBJECT RENDERING
 *****************************************************************************/

void
sozi_object_draw_svg(SoziObject *sozi_object, DiaSvgRenderer *svg_renderer, gchar * refid, xmlNs **p_sozi_name_space, xmlNodePtr * p_root, xmlNodePtr * p_rect)
{
    static xmlNs * sozi_name_space = NULL;
    int player_is_present;
    xmlNodePtr root;
    xmlNodePtr node;
    /* for managing sozi player implantation */
    gchar * sozi_version;
    gchar * sozi_js;
    gchar * sozi_extras_media_js;
    gchar * sozi_css;
    xmlChar * escaped;
    /* buffers for storing attributes */
    gchar * x;
    gchar * y;
    gchar * width;
    gchar * height;
    gchar * transform;
    gchar * style;
    gchar dtostr_buf[6][G_ASCII_DTOSTR_BUF_SIZE];
#   define dtostr(n,d) g_ascii_formatd(dtostr_buf[(n)], sizeof(dtostr_buf[(n)]), "%g", (d))

    root = xmlDocGetRootElement(svg_renderer->doc);

    /* check that the sozi namespace, scripts and style are present */

    player_is_present = sozi_player_is_present(svg_renderer->doc, (const xmlChar*) "//script[@id='sozi-script']");
    if (player_is_present < 0) {
        return;
    }
    else if (!player_is_present) {

        sozi_player_get(&sozi_version, &sozi_js, &sozi_extras_media_js, &sozi_css);

        sozi_name_space = xmlNewNs(root, (const xmlChar *)"http://sozi.baierouge.fr", (const xmlChar *)"sozi");

        node = xmlNewChild(root, NULL, (const xmlChar *)"script", NULL);
        xmlSetProp(node, (const xmlChar *)"id", (const xmlChar *)"sozi-script");
        xmlSetProp(node, (const xmlChar *)"sozi:version", (const xmlChar *)sozi_version);
        escaped = xmlEncodeEntitiesReentrant(svg_renderer->doc, (const xmlChar *)sozi_js);
        xmlNodeSetContent(node, escaped);
        xmlFree(escaped);

        node = xmlNewChild(root, NULL, (const xmlChar *)"script", NULL);
        xmlSetProp(node, (const xmlChar *)"id", (const xmlChar *)"sozi-extras-media-script");
        xmlSetProp(node, (const xmlChar *)"sozi:version", (const xmlChar *)sozi_version);
        escaped = xmlEncodeEntitiesReentrant(svg_renderer->doc, (const xmlChar *)sozi_extras_media_js);
        xmlNodeSetContent(node, escaped);
        xmlFree(escaped);

        node = xmlNewChild(root, NULL, (const xmlChar *)"style", NULL);
        xmlSetProp(node, (const xmlChar *)"id", (const xmlChar *)"sozi-style");
        xmlSetProp(node, (const xmlChar *)"sozi:version", (const xmlChar *)sozi_version);
        escaped = xmlEncodeEntitiesReentrant(svg_renderer->doc, (const xmlChar *)sozi_css);
        xmlNodeSetContent(node, escaped);
        xmlFree(escaped);

        g_free(sozi_version);
        g_free(sozi_js);
        g_free(sozi_css);
    }

    assert(sozi_name_space != NULL);

    /* format attributes */

#if 0 /* pure transformation */

    x         = g_strdup_printf("0");
    y         = g_strdup_printf("0");
    /* FIXME : see Sozi/player/js/display.js
     * exports.CameraState::setAtElement :
     * the properties of the objects are based
     * on the geometrical properties of the rectangle
     * => this makes 1:1 ratio rectangles...
     */
    width     = g_strdup_printf("1");
    height    = g_strdup_printf("1");
    transform = g_strdup_printf("matrix(%s,%s,%s,%s,%s,%s)",
                                /* svg has colomn-major order, we use row-major order */
                                dtostr(0, sozi_object->m[0] * svg_renderer->scale),
                                dtostr(1, sozi_object->m[3] * svg_renderer->scale),
                                dtostr(2, sozi_object->m[1] * svg_renderer->scale),
                                dtostr(3, sozi_object->m[4] * svg_renderer->scale),
                                dtostr(4, sozi_object->m[2] * svg_renderer->scale),
                                dtostr(5, sozi_object->m[5] * svg_renderer->scale));
    /* FIXME : Scaling problem on stroke width : http://dev.w3.org/SVG/modules/vectoreffects/master/SVGVectorEffects.html */
    style     = g_strdup_printf("fill:none;stroke:#000000;stroke-width:%s",
                                dtostr(0, 0.1 / svg_renderer->scale));

#elif 0 /* decompose transformation */

    x         = g_strdup_printf("0");
    y         = g_strdup_printf("0");
    /* FIXME : see Sozi/player/js/display.js
     * exports.CameraState::setAtElement :
     * the properties of the objects are based
     * on the geometrical properties of the rectangle
     * => this makes 1:1 ratio rectangles...
     */
    width     = g_strdup_printf("1");
    height    = g_strdup_printf("1");
    transform = g_strdup_printf("scale(%s,%s),translate(%s,%s),rotate(%s),scale(%s,%s),translate(-0.5,-0.5)",
                                dtostr(0, svg_renderer->scale),
                                dtostr(1, svg_renderer->scale),
                                dtostr(2, sozi_object->center.x),
                                dtostr(3, sozi_object->center.y),
                                dtostr(4, sozi_object->angle),
                                dtostr(5, sozi_object->width),
                                dtostr(6, sozi_object->height));
    /* FIXME : Scaling problem on stroke width : http://dev.w3.org/SVG/modules/vectoreffects/master/SVGVectorEffects.html */
    style     = g_strdup_printf("fill:none;stroke:#000000;stroke-width:%s",
                                dtostr(0, 0.1 / svg_renderer->scale));

#else /* no apply scale to transformation : best compromise */

    x         = g_strdup_printf("%s", dtostr(0, sozi_object->corners[0].x * svg_renderer->scale));
    y         = g_strdup_printf("%s", dtostr(0, sozi_object->corners[0].y * svg_renderer->scale));
    width     = g_strdup_printf("%s", dtostr(0, sozi_object->width  * svg_renderer->scale));
    height    = g_strdup_printf("%s", dtostr(0, sozi_object->height * svg_renderer->scale));
    transform = g_strdup_printf("rotate(%s,%s,%s)",
                                dtostr(0, sozi_object->angle),
                                dtostr(1, sozi_object->corners[0].x * svg_renderer->scale),
                                dtostr(2, sozi_object->corners[0].y * svg_renderer->scale));

    style     = g_strdup_printf("fill:none;stroke:#000000;stroke-width:0.1");

#endif

    /* add a "classic" rectangle to the svg document */

    node = xmlNewChild(svg_renderer->root, svg_renderer->svg_name_space, (const xmlChar *)"rect", NULL);

    if (refid) {
        xmlSetProp(node, (const xmlChar *)"id"       , (xmlChar *)refid);
    }
    xmlSetProp(node, (const xmlChar *)"x"        , (xmlChar *)x);
    xmlSetProp(node, (const xmlChar *)"y"        , (xmlChar *)y);
    xmlSetProp(node, (const xmlChar *)"width"    , (xmlChar *)width);
    xmlSetProp(node, (const xmlChar *)"height"   , (xmlChar *)height);
    xmlSetProp(node, (const xmlChar *)"transform", (xmlChar *)transform);
    xmlSetProp(node, (const xmlChar *)"style"    , (xmlChar *)style);

    /* free resources */
    g_free(x);
    g_free(y);
    g_free(width);
    g_free(height);
    g_free(transform);
    g_free(style);

    /* provide results */
    *p_sozi_name_space = sozi_name_space;
    *p_root = root;
    *p_rect = node;

#   undef dtostr
}

/******************************************************************************
 * FUNCTIONS FOR MANAGING THE OBJECT GRABBING, SCALLING AND ROTATION
 *****************************************************************************/

ObjectChange*
sozi_object_move(SoziObject *sozi_object, Point *to)
{
    sozi_object->center = *to;

    /* refresh interactive rendering */
    sozi_object_update(sozi_object);

    return NULL;
}

ObjectChange*
sozi_object_move_handle(SoziObject *sozi_object, Handle *handle,
                        Point *to, ConnectionPoint *cp,
                        HandleMoveReason reason, ModifierKeys modifiers)
{
    /* fprintf(stdout, "reason %d modifiers %x\n", reason, modifiers); */

    if (modifiers & MODIFIER_SHIFT) {
        /* rotate object around its center */
        Point p1;
        Point p2;

        /* p1 is the vector from the center to the old position of the handle */
        p1 = handle->pos;
        point_sub(&p1, &sozi_object->center);
        /* p2 is the vector from the center to the new position of the handle */
        p2 = *to;
        point_sub(&p2, &sozi_object->center);

        /*
          p1 . p2 = ||p1|| ||p2|| cos(delta)
          p1 ^ p2 = ||p1|| ||p2|| sin(delta)
          tan(delta) = (p1 ^ p2) / p1 . p2
        */
        sozi_object->angle += (180.0 / M_PI * atan2(p1.x * p2.y - p1.y * p2.x, p1.x * p2.x + p1.y * p2.y));
    }
    else {
        /* scale object */
        DiaObject * dia_object;
        unsigned i;

        Point diagonal;

        real ratio;
        real width;
        real height;

        /* find the current handle */
        dia_object = &sozi_object->dia_object;
        for (i = 0; i < 4; i++) {
            if (dia_object->handles[i] == handle) {
                break;
            }
        }
        assert(i < 4);

        if (sozi_object->scale_from_center) {
            /* scale object from its center  */

            /* diagonal is the vector from the center to the new handle pos */
            diagonal = *to;
            point_sub(&diagonal, &sozi_object->center);
            /* ratio is the old ratio */
            ratio = sozi_object->width / sozi_object->height;
            /* dot product with the unit lenght vector that hold the width axe */
            width = 2 * fabs(diagonal.x * sozi_object->cos_angle +
                             diagonal.y * sozi_object->sin_angle);
            /* cross product with the unit lenght vector that hold the width axe */
            height = 2 * fabs(diagonal.x * sozi_object->sin_angle -
                              diagonal.y * sozi_object->cos_angle);
            /* compute the new size */
            if (sozi_object->aspect == ASPECT_FREE) {
                sozi_object->width  = width;
                sozi_object->height = height;
            }
            else {
                sozi_object->width  = (((height * ratio) < width)?width:(height * ratio));
                sozi_object->height = (((width / ratio) < height)?height:(width / ratio));
            }
        }
        else {
            /* scale object from oposite handle */

            /* go to the oposite handle */
            i = (i + 2) % 4;

            /* diagonal is the vector from the oposite handle to the new handle pos */
            diagonal = *to;
            point_sub(&diagonal, &dia_object->handles[i]->pos);
            /* ratio is the old ratio */
            ratio = sozi_object->width / sozi_object->height;
            /* dot product with the unit lenght vector that hold the width axe */
            width = fabs(diagonal.x * sozi_object->cos_angle +
                         diagonal.y * sozi_object->sin_angle);
            /* cross product with the unit lenght vector that hold the width axe */
            height = fabs(diagonal.x * sozi_object->sin_angle -
                          diagonal.y * sozi_object->cos_angle);
            /* compute the new size */
            if (sozi_object->aspect == ASPECT_FREE) {
                sozi_object->width  = width;
                sozi_object->height = height;
            }
            else {
                sozi_object->width  = (((height * ratio) < width)?width:(height * ratio));
                sozi_object->height = (((width / ratio) < height)?height:(width / ratio));
            }
            /* compute the new position */
            if (sozi_object->aspect == ASPECT_FREE) {
                sozi_object->center.x = 0.5 * (dia_object->handles[i]->pos.x + to->x);
                sozi_object->center.y = 0.5 * (dia_object->handles[i]->pos.y + to->y);
            }
            else {
                /* FIXME : in FIXED aspect the behavior isn't userfriendly */
                static const real coefs [4][4] =
                    {
                        { 0.5,-0.5, 0.5, 0.5},
                        { 0.5, 0.5, 0.5,-0.5},
                        {-0.5, 0.5,-0.5,-0.5},
                        {-0.5,-0.5,-0.5, 0.5},
                    };
                sozi_object->center.x = dia_object->handles[i]->pos.x +
                    coefs[i][0] * sozi_object->width  * sozi_object->cos_angle +
                    coefs[i][1] * sozi_object->height * sozi_object->sin_angle;
                sozi_object->center.y = dia_object->handles[i]->pos.y +
                    coefs[i][2] * sozi_object->width  * sozi_object->sin_angle +
                    coefs[i][3] * sozi_object->height * sozi_object->cos_angle;
            }
        }
    }

    /* refresh interactive rendering */
    sozi_object_update(sozi_object);

    return NULL;
}
