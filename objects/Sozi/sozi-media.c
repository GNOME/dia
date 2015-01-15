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

/* xpath for searching in the svg doc */
#include <libxml/xpath.h>

/* dia stuff */
#include "object.h"
#include "diarenderer.h"
#include "diasvgrenderer.h"
#include "properties.h"

/* sozi stuff */
#include "pixmaps/sozi-media.xpm"
#include "sozi-object.h"

/******************************************************************************
 *
 *****************************************************************************/

typedef enum
{
    MEDIA_TYPE_UNDEFINED ,
    MEDIA_TYPE_VIDEO_MP4 ,
    MEDIA_TYPE_VIDEO_WEBM,
    MEDIA_TYPE_VIDEO_OGG ,
    MEDIA_TYPE_AUDIO_MPEG,
    MEDIA_TYPE_AUDIO_OGG ,
    MEDIA_TYPE_AUDIO_WAV ,
} MediaType;

/******************************************************************************
 *
 *****************************************************************************/

typedef struct _SoziMedia
{
    /* sozi object inheritance */

    SoziObject sozi_object;

    /* sozi media specific stuff */

    MediaType type;
    gchar *   file;

    int start_frame;
    int stop_frame;
    
} SoziMedia;

/******************************************************************************
 * Dia object type operations
 *****************************************************************************/

static DiaObject * sozi_media_create(Point *startpoint,
                                     void *user_data,
                                     Handle **handle1,
                                     Handle **handle2);
static DiaObject * sozi_media_load(ObjectNode obj_node, int version, DiaContext *ctx);

static ObjectTypeOps sozi_media_type_ops =
{
    (CreateFunc)        sozi_media_create,            /* create */
    (LoadFunc)          sozi_media_load,              /* load (== object_load_using_properties ?) */
    (SaveFunc)          object_save_using_properties, /* save  */
    (GetDefaultsFunc)   NULL,
    (ApplyDefaultsFunc) NULL,
};

DiaObjectType sozi_media_type =
{
    "Sozi - Media",           /* name */
    0,                        /* version */
    sozi_media_xpm,           /* pixmap */
    &sozi_media_type_ops,     /* ops */
    NULL,                     /* pixmap_file */
    0                         /* default_user_data */
};

/******************************************************************************
 * Dia object operations
 *****************************************************************************/

static void              sozi_media_destroy(SoziMedia *sozi_media);

static void              sozi_media_draw(SoziMedia *sozi_media, DiaRenderer *renderer);

static real              sozi_media_distance_from(SoziMedia *sozi_media, Point *point);

static void              sozi_media_select(SoziMedia *sozi_media, Point *clicked_point,
                                           DiaRenderer *interactive_renderer);

static ObjectChange*     sozi_media_move(SoziMedia *sozi_media, Point *to);

static ObjectChange*     sozi_media_move_handle(SoziMedia *sozi_media, Handle *handle,
                                                Point *to, ConnectionPoint *cp,
                                                HandleMoveReason reason, ModifierKeys modifiers);

static PropDescription * sozi_media_describe_props(SoziMedia *sozi_media);

static void              sozi_media_get_props(SoziMedia *sozi_media, GPtrArray *props);

static void              sozi_media_set_props(SoziMedia *sozi_media, GPtrArray *props);


static ObjectOps sozi_media_ops =
{
  (DestroyFunc)               sozi_media_destroy,
  (DrawFunc)                  sozi_media_draw,
  (DistanceFunc)              sozi_media_distance_from,
  (SelectFunc)                sozi_media_select,
  (CopyFunc)                  object_copy_using_properties,
  (MoveFunc)                  sozi_media_move,
  (MoveHandleFunc)            sozi_media_move_handle,
  (GetPropertiesFunc)         object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)            0,
  (DescribePropsFunc)         sozi_media_describe_props,
  (GetPropsFunc)              sozi_media_get_props,
  (SetPropsFunc)              sozi_media_set_props,
  (TextEditFunc)              0,
  (ApplyPropertiesListFunc)   object_apply_props,
};

/******************************************************************************
 * Functions for managing Sozi media
 *****************************************************************************/

static void
sozi_media_update(SoziMedia *sozi_media)
{
    char legend[32];
    int nb_char;

    /* update legend */
    nb_char = g_snprintf(legend, sizeof(legend), "%s", sozi_media->file);
    if(sizeof(legend) < nb_char)
    {
       legend[sizeof(legend) - 1] = 0;
       legend[sizeof(legend) - 2] = '.';
       legend[sizeof(legend) - 3] = '.';
       legend[sizeof(legend) - 4] = '.';
    }
    text_set_string(sozi_media->sozi_object.legend.text, legend);
}

/******************************************************************************
 * Functions for managing the Sozi object life managment (init, copy, kill)
 *****************************************************************************/

static void
sozi_media_init(SoziMedia *sozi_media)
{
    sozi_media->type = MEDIA_TYPE_UNDEFINED;
    if(sozi_media->file == NULL) {
        sozi_media->file = g_strdup("");
    }
}

static void
sozi_media_kill(SoziMedia *sozi_media)
{
    if(sozi_media->file != NULL) {
        g_free(sozi_media->file);
    }
}

/******************************************************************************
 * Functions that implement the Dia fops
 *****************************************************************************/

static DiaObject *
sozi_media_create(Point *startpoint,
                  void *user_data,
                  Handle **handle1,
                  Handle **handle2)
{
    SoziMedia *sozi_media;

    sozi_media = g_new0(SoziMedia, 1);

    /* init dia object */
    sozi_media->sozi_object.dia_object.type = &sozi_media_type;
    sozi_media->sozi_object.dia_object.ops = &sozi_media_ops;

    /* init sozi object : that is also the place where the dia object
     * geometry (position, handles, ...) will be initialized
     */
    sozi_object_init(&sozi_media->sozi_object, startpoint);

    /* init sozi media */
    sozi_media_init(sozi_media);

    /* update sozi object geometry */
    sozi_object_update(&sozi_media->sozi_object);

    *handle1 = NULL; /* not connectable ... */
    g_assert (sozi_media->sozi_object.dia_object.handles[2]->id == HANDLE_RESIZE_SE);
    *handle2 = sozi_media->sozi_object.dia_object.handles[2]; /* ... but resizable */

    return &sozi_media->sozi_object.dia_object;
}

/******************************************************************************
 *
 *****************************************************************************/
static void
sozi_media_destroy(SoziMedia *sozi_media)
{
    /* kill sozi media */
    sozi_media_kill(sozi_media);

    /* kill sozi object */
    sozi_object_kill(&sozi_media->sozi_object);

    /* do NOT free sozi_media, caller will take care about that */
}

/******************************************************************************
 *
 *****************************************************************************/
static void
sozi_media_draw_svg(SoziMedia *sozi_media, DiaSvgRenderer *svg_renderer)
{
    xmlNs *sozi_name_space;
    xmlNodePtr root;
    xmlNodePtr node;
    xmlNodePtr rect;
    /* buffers for storing attributes */
    gchar * type;
    gchar * src;
    gchar * start_frame;
    gchar * stop_frame;

    /* format attributes */

    switch(sozi_media->type) {
    case MEDIA_TYPE_VIDEO_MP4 : type = g_strdup("video/mp4" ); break;
    case MEDIA_TYPE_VIDEO_WEBM: type = g_strdup("video/webm"); break;
    case MEDIA_TYPE_VIDEO_OGG : type = g_strdup("video/ogg" ); break;
    case MEDIA_TYPE_AUDIO_MPEG: type = g_strdup("audio/mpeg"); break;
    case MEDIA_TYPE_AUDIO_OGG : type = g_strdup("audio/ogg" ); break;
    case MEDIA_TYPE_AUDIO_WAV : type = g_strdup("audio/wav" ); break;
    default: return;
    }
    src = g_strdup(sozi_media->file);
    start_frame = g_strdup_printf("%d", sozi_media->start_frame);
    stop_frame = g_strdup_printf("%d", sozi_media->stop_frame);

    /* draw the sozi base object and get a reference to the sozi
       namespace and document root */

    sozi_object_draw_svg(&sozi_media->sozi_object, svg_renderer, 0, &sozi_name_space, &root, &rect);

    assert(sozi_name_space != NULL);

    /* add a "custom" media for sozi */

    switch(sozi_media->type) {
    case MEDIA_TYPE_VIDEO_MP4 :
    case MEDIA_TYPE_VIDEO_WEBM:
    case MEDIA_TYPE_VIDEO_OGG :
        node = xmlNewChild(rect, sozi_name_space, (const xmlChar *)"video", NULL);
        break;
    case MEDIA_TYPE_AUDIO_MPEG:
    case MEDIA_TYPE_AUDIO_OGG :
    case MEDIA_TYPE_AUDIO_WAV :
        node = xmlNewChild(rect, sozi_name_space, (const xmlChar *)"audio", NULL);
        break;
    default: node = 0;
    }

    xmlSetProp(node, (const xmlChar *)"sozi:type"        , (const xmlChar *)type);
    xmlSetProp(node, (const xmlChar *)"sozi:src"         , (const xmlChar *)src);
    xmlSetProp(node, (const xmlChar *)"sozi:start-frame" , (const xmlChar *)start_frame);
    xmlSetProp(node, (const xmlChar *)"sozi-stop-frame"  , (const xmlChar *)stop_frame);

    /* free resources */
    g_free(type);
    g_free(src);
    g_free(start_frame);
    g_free(stop_frame);
}

/******************************************************************************
 *
 *****************************************************************************/

#define DIA_IS_INTERACTIVE_RENDERER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_INTERACTIVE_RENDERER))

static void
sozi_media_draw(SoziMedia *sozi_media, DiaRenderer *renderer)
{

    if (DIA_IS_SVG_RENDERER(renderer)) {
        sozi_media_draw_svg(sozi_media, DIA_SVG_RENDERER (renderer));
    }
    else if(DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer)) {
        DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);

        renderer_ops->set_linewidth(renderer, SOZI_OBJECT_LINE_WIDTH);
        renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);
        renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
        renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID, 0);

        renderer_ops->draw_polygon(renderer, sozi_media->sozi_object.corners, 4, NULL, &color_black);

        if(sozi_media->sozi_object.legend.disp == TRUE) {
            text_draw(sozi_media->sozi_object.legend.text, renderer);
        }
    }
}

/******************************************************************************
 *
 *****************************************************************************/

static real
sozi_media_distance_from(SoziMedia *sozi_media, Point *point)
{
    return distance_polygon_point(sozi_media->sozi_object.corners, 4, SOZI_OBJECT_LINE_WIDTH, point);
}

/******************************************************************************
 *
 *****************************************************************************/
static void
sozi_media_select(SoziMedia *sozi_media, Point *clicked_point,
                  DiaRenderer *interactive_renderer)
{
    /* this callback will be called the first time the object is
       placed on the sheet, it allows to initialize the object */
    sozi_media_update(sozi_media);
    sozi_object_update(&sozi_media->sozi_object);
}

/******************************************************************************
 *
 *****************************************************************************/

static ObjectChange*
sozi_media_move(SoziMedia *sozi_media, Point *to)
{
    return sozi_object_move(&sozi_media->sozi_object, to);
}

/******************************************************************************
 *
 *****************************************************************************/

static ObjectChange*
sozi_media_move_handle(SoziMedia *sozi_media, Handle *handle,
                       Point *to, ConnectionPoint *cp,
                       HandleMoveReason reason, ModifierKeys modifiers)
{
    return sozi_object_move_handle(&sozi_media->sozi_object, handle,
                            to, cp, reason, modifiers);
}

/******************************************************************************
 *
 *****************************************************************************/
static DiaObject *
sozi_media_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
    return object_load_using_properties(&sozi_media_type, obj_node, version, ctx);
}

/******************************************************************************
 *
 *****************************************************************************/

static PropEnumData prop_aspect_data[] =
{
    { N_("Free")  , ASPECT_FREE   },
    { N_("Fixed") , ASPECT_FIXED  },
    { NULL, 0 }
};

static PropEnumData prop_media_type_data[] =
{
    { N_("undefined")  , MEDIA_TYPE_UNDEFINED  },
    { N_("video/mp4")  , MEDIA_TYPE_VIDEO_MP4  },
    { N_("video/webm") , MEDIA_TYPE_VIDEO_WEBM },
    { N_("video/ogg")  , MEDIA_TYPE_VIDEO_OGG  },
    { N_("audio/mpeg") , MEDIA_TYPE_AUDIO_MPEG },
    { N_("audio/ogg")  , MEDIA_TYPE_AUDIO_OGG  },
    { N_("audio/wav")  , MEDIA_TYPE_AUDIO_WAV  },
    { NULL, 0 }
};

static PropDescription sozi_media_props[] =
{
    OBJECT_COMMON_PROPERTIES,

    PROP_STD_NOTEBOOK_BEGIN,

    PROP_NOTEBOOK_PAGE("geometry",0,N_("Geometry")),
    { "x"                , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Center x")         , NULL, NULL },
    { "y"                , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Center y")         , NULL, NULL },
    { "width"            , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Width")            , NULL, NULL },
    { "height"           , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Height")           , NULL, NULL },
    { "angle"            , PROP_TYPE_INT  , PROP_FLAG_VISIBLE, N_("Angle (deg)")      , NULL, NULL },
    { "aspect"           , PROP_TYPE_ENUM , PROP_FLAG_VISIBLE, N_("Aspect ratio")     , NULL, prop_aspect_data },
    { "scale_from_center", PROP_TYPE_BOOL , PROP_FLAG_VISIBLE, N_("Scale from center"), NULL, NULL },

    PROP_NOTEBOOK_PAGE("legend",0,N_("Legend")),
    { "legend_disp"      , PROP_TYPE_BOOL , PROP_FLAG_VISIBLE, N_("Legend is visible"), NULL, NULL},
    { "legend"           , PROP_TYPE_TEXT , 0                , NULL                   , NULL, NULL},
    PROP_STD_TEXT_ALIGNMENT,
    PROP_STD_TEXT_FONT,
    PROP_STD_TEXT_HEIGHT,
    PROP_STD_TEXT_COLOUR,

    PROP_NOTEBOOK_PAGE("sozi",0,N_("Sozi")),
    { "media_type" , PROP_TYPE_ENUM , PROP_FLAG_VISIBLE, N_("Media type") , NULL, prop_media_type_data},
    { "media_file" , PROP_TYPE_FILE , PROP_FLAG_VISIBLE, N_("Media file") , NULL, NULL },
    { "start_frame", PROP_TYPE_INT  , PROP_FLAG_VISIBLE, N_("Start frame"), NULL, NULL },
    { "stop_frame" , PROP_TYPE_INT  , PROP_FLAG_VISIBLE, N_("Stop frame") , NULL, NULL },

    PROP_STD_NOTEBOOK_END,

    PROP_DESC_END
};

static PropOffset sozi_media_offsets[] =
{
    OBJECT_COMMON_PROPERTIES_OFFSETS,

    PROP_OFFSET_STD_NOTEBOOK_BEGIN,

    PROP_OFFSET_NOTEBOOK_PAGE("geometry"),
    { "x"                     , PROP_TYPE_REAL          , offsetof(SoziMedia, sozi_object.center.x)          },
    { "y"                     , PROP_TYPE_REAL          , offsetof(SoziMedia, sozi_object.center.y)          },
    { "width"                 , PROP_TYPE_REAL          , offsetof(SoziMedia, sozi_object.width)             },
    { "height"                , PROP_TYPE_REAL          , offsetof(SoziMedia, sozi_object.height)            },
    { "angle"                 , PROP_TYPE_INT           , offsetof(SoziMedia, sozi_object.angle)             },
    { "aspect"                , PROP_TYPE_ENUM          , offsetof(SoziMedia, sozi_object.aspect)            },
    { "scale_from_center"     , PROP_TYPE_BOOL          , offsetof(SoziMedia, sozi_object.scale_from_center) },

    PROP_OFFSET_NOTEBOOK_PAGE("legend"),
    { "legend_disp"           , PROP_TYPE_BOOL          , offsetof(SoziMedia, sozi_object.legend.disp)             },
    { "legend"                , PROP_TYPE_TEXT          , offsetof(SoziMedia, sozi_object.legend.text)             },
    { "text_alignment"        , PROP_TYPE_ENUM          , offsetof(SoziMedia, sozi_object.legend.attrs.alignment)  },
    { "text_font"             , PROP_TYPE_FONT          , offsetof(SoziMedia, sozi_object.legend.attrs.font)       },
    { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(SoziMedia, sozi_object.legend.attrs.height)     },
    { "text_colour"           , PROP_TYPE_COLOUR        , offsetof(SoziMedia, sozi_object.legend.attrs.color)      },

    PROP_OFFSET_NOTEBOOK_PAGE("sozi"),
    { "media_type"            , PROP_TYPE_ENUM          , offsetof(SoziMedia, type)                                },
    { "media_file"            , PROP_TYPE_FILE          , offsetof(SoziMedia, file)                                },
    { "start_frame"           , PROP_TYPE_INT           , offsetof(SoziMedia, start_frame)                         },
    { "stop_frame"            , PROP_TYPE_INT           , offsetof(SoziMedia, stop_frame)                          },

    PROP_OFFSET_STD_NOTEBOOK_END,

    {NULL}
};

static PropDescription *
sozi_media_describe_props(SoziMedia *sozi_media)
{
    if (sozi_media_props[0].quark == 0)
        prop_desc_list_calculate_quarks(sozi_media_props);
    return sozi_media_props;
}

static void
sozi_media_get_props(SoziMedia *sozi_media, GPtrArray *props)
{
    text_get_attributes(sozi_media->sozi_object.legend.text, &sozi_media->sozi_object.legend.attrs);
    object_get_props_from_offsets(&sozi_media->sozi_object.dia_object, sozi_media_offsets, props);
}

static void
sozi_media_set_props(SoziMedia *sozi_media, GPtrArray *props)
{
    object_set_props_from_offsets(&sozi_media->sozi_object.dia_object, sozi_media_offsets, props);
    apply_textattr_properties(props, sozi_media->sozi_object.legend.text, "legend" , &sozi_media->sozi_object.legend.attrs);
    sozi_media_update(sozi_media);
    sozi_object_update(&sozi_media->sozi_object);
}
