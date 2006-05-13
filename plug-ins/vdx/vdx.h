#ifndef VISIO_H
#define VISIO_H

struct VDXDocument
{
    GArray *Colors;
    GArray *FaceNames;
    GArray *Fonts;
    GArray *Masters;
    GArray *StyleSheets;
    gboolean ok;
    unsigned int Page;
};

typedef struct VDXDocument VDXDocument;

static const double vdx_Font_Size_Conversion = 1.5;
static const double vdx_Y_Offset = 24.0;
static const double vdx_Y_Flip = -1.0;
static const double vdx_Point_Scale = 1.0;
static const double vdx_Page_Width = 15.0;

#endif
