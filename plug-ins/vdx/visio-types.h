/* Generated Mon Mar 20 19:45:09 2006 */
/* From: All.vdx Circle1.vdx Circle2.vdx Line1.vdx Line2.vdx Line3.vdx Line4.vdx Line5.vdx Line6.vdx Rectangle1.vdx Rectangle2.vdx Rectangle3.vdx Rectangle4.vdx Text1.vdx Text2.vdx Text3.vdx samp_vdx.vdx */


struct vdx_Act
{
    GSList *children;
    char type;
    gboolean Action; /* F=DOCMD(1312),RUNADDON(&quot;CS&quot;),SETF(GetRef(User.ShowFooter),NOT(User.ShowFooter)) */
    gboolean BeginGroup; /* F=No Formula =0 */
    unsigned int ButtonFace; /* F=No Formula = */
    gboolean Checked; /* F=No Formula =0 */
    gboolean Disabled; /* F=No Formula =0 */
    unsigned int ID; /* */
    gboolean Invisible; /* F=No Formula =0 */
    char * Menu; /* F=IF(User.ShowFooter,&quot;Hide &amp;footer&quot;,&quot;Show &amp;footer&quot;) =%P&amp;roperties,Co&amp;lor Schemes...,Hide &amp;footer */
    char * NameU; /* =LocalRow0 */
    gboolean ReadOnly; /* F=No Formula =0 */
    unsigned int SortKey; /* F=No Formula = */
    unsigned int TagName; /* F=No Formula = */
};

struct vdx_Char
{
    GSList *children;
    char type;
    unsigned int AsianFont; /* F=Inh */
    gboolean Case; /* F=0,Inh =0 */
    Color Color; /* F=0,4,HSL(0,0,240),HSL(150,240,96),Inh */
    float ColorTrans; /* F=0%,Inh */
    unsigned int ComplexScriptFont; /* F=Inh */
    int ComplexScriptSize; /* F=Inh =-1 */
    gboolean DblUnderline; /* F=FALSE,Inh */
    gboolean DoubleStrikethrough; /* F=Inh =0 */
    unsigned int Font; /* F=0,Inh */
    float FontScale; /* F=100%,Inh */
    gboolean Highlight; /* F=Inh =0 */
    unsigned int IX; /* */
    unsigned int LangID; /* F=Inh =1033 */
    float Letterspace; /* F=0DT,Inh */
    gboolean Locale; /* F=0,Inh =0 */
    unsigned int LocalizeFont; /* F=Inh */
    gboolean Overline; /* F=FALSE,Inh */
    gboolean Perpendicular; /* F=FALSE,Inh */
    gboolean Pos; /* F=0,Inh =0 */
    gboolean RTLText; /* F=Inh =0 */
    float Size; /* F=0.125DT,0.16666666666667DT,Inh */
    gboolean Strikethru; /* F=FALSE,Inh */
    unsigned int Style; /* F=0,Inh */
    gboolean UseVertical; /* F=Inh =0 */
};

struct vdx_ColorEntry
{
    GSList *children;
    char type;
    unsigned int IX; /* */
    char * RGB; /* =#000000,#000080,#0000FF,#008000,#008080,#00FF00,#00FFFF,#1A1A1A,#333333,#4D4D4D,#666666,#800000,#800080,#808000,#808080,#9A9A9A,#B3B3B3,#C0C0C0,#CDCDCD,#E6E6E6,#FF0000,#FF00FF,#FFFF00,#FFFFFF */
};

struct vdx_Colors
{
    GSList *children;
    char type;
    unsigned int ColorEntry; /* = */
};

struct vdx_Connect
{
    GSList *children;
    char type;
    char * FromCell; /* =BeginX,EndX */
    unsigned int FromPart; /* =12,9 */
    unsigned int FromSheet; /* =13,14,20,21,22,23,24,25,26,27,28,29,30,31 */
    char * ToCell; /* =Connections.X2,Connections.X3,Connections.X4,PinX */
    unsigned int ToPart; /* =101,102,103,3 */
    unsigned int ToSheet; /* =10,11,12,13,14,15,16,17,18,19 */
};

struct vdx_Connection
{
    GSList *children;
    char type;
    gboolean AutoGen; /* F=No Formula =0 */
    gboolean DirX; /* F=No Formula =0 */
    gboolean DirY; /* F=No Formula =0 */
    unsigned int IX; /* */
    unsigned int Prompt; /* F=No Formula = */
    gboolean Type; /* F=No Formula =0 */
    float X; /* F=0.5*Width,Geometry1.A3,Geometry1.A5,Inh,Width,Width*0.5 */
    float Y; /* F=0.5*Height,Height,Height*0,Height*0.49999999999999,Height*0.5,Height*1,Inh */
};

struct vdx_Connects
{
    GSList *children;
    char type;
    unsigned int Connect; /* = */
};

struct vdx_Control
{
    GSList *children;
    char type;
    gboolean CanGlue; /* F=Inh =0 */
    unsigned int ID; /* */
    char * NameU; /* =TextPosition */
    char * Prompt; /* F=Inh =Move to change feature size.,Reposition Text */
    float X; /* F=0.125IN*User.AntiScale,Width*0.00090144230769251,Width*0.26278409090909,Width*0.26988636363636 */
    unsigned int XCon; /* F=IF(OR(STRSAME(SHAPETEXT(TheText),&quot;&quot;),HideText),5,0),Inh =0,5,7 */
    float XDyn; /* F=Controls.TextPosition,Inh */
    float Y; /* F=GUARD(0),Height*-0.0023148148148147,Height*0,Height*0.16279069767442 */
    unsigned int YCon; /* F=Inh =0,6 */
    float YDyn; /* F=Controls.Row_1.Y,Controls.TextPosition.Y,Inh */
};

struct vdx_CustomProp
{
    GSList *children;
    char type;
    char * Name; /* =_TemplateID */
    char * PropType; /* =String */
};

struct vdx_CustomProps
{
    GSList *children;
    char type;
    char * CustomProp; /* =TC010483951033 */
};

struct vdx_DocProps
{
    GSList *children;
    char type;
    gboolean AddMarkup; /* =0 */
    unsigned int DocLangID; /* =1033 */
    gboolean LockPreview; /* =0 */
    unsigned int OutputFormat; /* =2 */
    gboolean PreviewQuality; /* F=No Formula =0 */
    gboolean PreviewScope; /* F=No Formula =0 */
    gboolean ViewMarkup; /* =0 */
};

struct vdx_DocumentProperties
{
    GSList *children;
    char type;
    unsigned int BuildNumberCreated; /* =671351309,738200627 */
    unsigned int BuildNumberEdited; /* =671351309,738200720 */
    char * Company; /* =Microsoft Corporation */
    char * Creator; /* =Microsoft Corporation */
    char * Template; /* =C:\Program Files\Microsoft Office\Visio11\1033\BASFLO_U.VST */
    char * TimeCreated; /* =2003-07-28T11:32:31,2003-12-11T11:37:52 */
    char * TimeEdited; /* =2003-10-24T16:57:00,2003-12-11T11:39:33,2003-12-11T11:40:20,2003-12-11T11:41:31,2003-12-11T11:42:48,2003-12-11T11:43:50,2003-12-11T11:44:51,2003-12-11T11:46:42,2003-12-11T11:54:00,2003-12-11T11:57:24,2003-12-11T11:58:26,2003-12-11T11:59:27,2003-12-11T12:00:07,2003-12-11T12:01:03,2003-12-11T12:02:01,2003-12-11T12:04:52,2003-12-11T12:08:01 */
    char * TimePrinted; /* =2003-07-28T11:32:31,2003-12-11T11:37:52 */
    char * TimeSaved; /* =2003-10-24T16:57:23,2003-12-11T11:39:40,2003-12-11T11:40:45,2003-12-11T11:41:46,2003-12-11T11:43:03,2003-12-11T11:44:08,2003-12-11T11:45:07,2003-12-11T11:46:58,2003-12-11T11:56:50,2003-12-11T11:57:48,2003-12-11T11:58:45,2003-12-11T11:59:42,2003-12-11T12:00:22,2003-12-11T12:01:23,2003-12-11T12:02:21,2003-12-11T12:05:11,2003-12-11T12:08:21 */
};

struct vdx_DocumentSettings
{
    GSList *children;
    char type;
    unsigned int DefaultFillStyle; /* */
    unsigned int DefaultGuideStyle; /* */
    unsigned int DefaultLineStyle; /* */
    unsigned int DefaultTextStyle; /* */
    gboolean DynamicGridEnabled; /* =0,1 */
    unsigned int GlueSettings; /* =9 */
    gboolean ProtectBkgnds; /* =0 */
    gboolean ProtectMasters; /* =0 */
    gboolean ProtectShapes; /* =0 */
    gboolean ProtectStyles; /* =0 */
    unsigned int SnapExtensions; /* =34 */
    unsigned int SnapSettings; /* =295,65831 */
    gboolean TopPage; /* =0 */
};

struct vdx_DocumentSheet
{
    GSList *children;
    char type;
    unsigned int FillStyle; /* */
    unsigned int LineStyle; /* */
    char * Name; /* =TheDoc */
    char * NameU; /* =TheDoc */
    unsigned int TextStyle; /* */
};

struct vdx_Ellipse
{
    GSList *children;
    char type;
    float A; /* F=Width*1 */
    float B; /* F=Height*0.5 */
    float C; /* F=Width*0.5 */
    float D; /* F=Height*1 */
    unsigned int IX; /* */
    float X; /* F=Width*0.5 */
    float Y; /* F=Height*0.5 */
};

struct vdx_EllipticalArcTo
{
    GSList *children;
    char type;
    float A; /* F=Inh,Width,Width*0.0485,Width*0.048503597491669,Width*0.19949695423237,Width*0.1995,Width*0.3229,Width*0.32290495932947,Width*0.45319261647222,Width*0.4532,Width*0.5468,Width*0.54680738352777,Width*0.67709504067052,Width*0.6771,Width*0.8005,Width*0.80050304576763,Width*0.95149640250833,Width*0.9515 */
    float B; /* F=Height*0.00018283632885901,Height*0.0002,Height*0.060887869711828,Height*0.0609,Height*0.3544,Height*0.35443789112062,Height*0.39595481358611,Height*0.396,Height*0.49999999999999,Height*0.604,Height*0.60404518641389,Height*0.64556210887939,Height*0.6456,Height*0.9391,Height*0.93911213028817,Height*0.9998,Height*0.99981716367115,Inh */
    float C; /* F=Inh,_ELLIPSE_THETA(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height),_ELLIPSE_THETA(0.0175,4.0709,2.1703,0.521,Width,Height),_ELLIPSE_THETA(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height),_ELLIPSE_THETA(0.1763,4.6667,2.1703,0.521,Width,Height) */
    float D; /* F=2*Geometry1.X1/Height,Geometry1.D3,Inh,_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C5),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C5),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C5),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C5) */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Geometry1.X2,Inh,Width*0,Width*0.00057808450186472,Width*0.19678663930461,Width*0.20811090826019,Width*0.7918890917398,Width*0.7919,Width*0.8032,Width*0.80321336069539,Width*0.9994,Width*0.99942191549813,Width*0.99999999999999 */
    float Y; /* F=Geometry1.Y1,Geometry1.Y4,Height*0,Height*0.12195045039423,Height*0.122,Height*0.3324,Height*0.33243986480781,Height*0.3611,Height*0.36113315618117,Height*0.63886684381884,Height*0.6389,Height*0.6675601351922,Height*0.6676,Height*0.878,Height*0.87804954960577,Height*1,Inh */
};

struct vdx_Event
{
    GSList *children;
    char type;
    gboolean EventDblClick; /* F=Inh,No Formula,OPENTEXTWIN(),_OpenTextWin() =0 */
    float EventDrop; /* F=No Formula,SETF(GetRef(Width),ThePage!PageWidth-4*User.PageMargin)+SETF(GetRef(Height),ThePage!PageHeight-4*User.PageMargin)+SETF(GetRef(PinX),ThePage!PageWidth*0.5)+SETF(GetRef(PinY),ThePage!PageHeight*0.5)+SETF(&quot;EventDrop&quot;,0) */
    gboolean EventXFMod; /* F=No Formula =0 */
    gboolean TheData; /* F=No Formula =0 */
    gboolean TheText; /* F=No Formula =0 */
};

struct vdx_EventItem
{
    GSList *children;
    char type;
    gboolean Action; /* =1 */
    gboolean Enabled; /* =1 */
    gboolean EventCode; /* =1 */
    unsigned int ID; /* */
    char * Target; /* =PPT */
    unsigned int TargetArgs; /* = */
};

struct vdx_EventList
{
    GSList *children;
    char type;
    unsigned int EventItem; /* = */
};

struct vdx_FaceName
{
    GSList *children;
    char type;
    char * CharSets; /* =-2147483648 0,0 0,1048577 0,1073742335 -65536,1073873055 -539557888,1074266271 -539557888,1614742015 -65536,262145 0,536871071 0,536936959 539492352 */
    unsigned int Flags; /* =261,325,327,357,421 */
    unsigned int ID; /* */
    char * Name; /* =Arial,Arial Unicode MS,Cordia New,Dhenu,Dotum,Estrangelo Edessa,Gautami,Gulim,Latha,MS Farsi,MS PGothic,Mangal,PMingLiU,Raavi,Sendnya,Shruti,SimSun,Sylfaen,Symbol,Times New Roman,Tunga,Vrinda,Wingdings */
    char * Panos; /* =0 0 4 0 0 0 0 0 0 0,1 10 5 2 5 3 6 3 3 3,2 0 4 0 0 0 0 0 0 0,2 0 5 0 0 0 0 0 0 0,2 1 6 0 3 1 1 1 1 1,2 11 6 0 0 1 1 1 1 1,2 11 6 0 7 2 5 8 2 4,2 11 6 4 2 2 2 2 2 4,2 2 3 0 0 0 0 0 0 0,2 2 6 3 5 4 5 2 3 4,3 8 6 0 0 0 0 0 0 0,5 0 0 0 0 0 0 0 0 0,5 5 1 2 1 7 6 2 5 7 */
    char * UnicodeRanges; /* =-1 -369098753 63 0,-1342176593 1775729915 48 0,-1610612033 1757936891 16 0,-2147459008 0 128 0,0 0 0 0,1048576 0 0 0,131072 0 0 0,16804487 -2147483648 8 0,2097152 0 0 0,262144 0 0 0,3 135135232 0 0,3 137232384 22 0,31367 -2147483648 8 0,32768 0 0 0,4194304 0 0 0,67110535 0 0 0 */
};

struct vdx_FaceNames
{
    GSList *children;
    char type;
    unsigned int FaceName; /* = */
};

struct vdx_Field
{
    GSList *children;
    char type;
    gboolean Calendar; /* F=No Formula =0 */
    gboolean Del; /* =1 */
    gboolean EditMode; /* F=Inh =0 */
    float Format; /* F=FIELDPICTURE(0),FIELDPICTURE(201),Inh */
    unsigned int IX; /* */
    gboolean ObjectKind; /* F=No Formula =0 */
    unsigned int Type; /* F=Inh =0,5 */
    unsigned int UICat; /* F=Inh =1,5 */
    unsigned int UICod; /* F=Inh =3,4 */
    unsigned int UIFmt; /* F=Inh =0,201 */
    float Value; /* F=DOCLASTSAVE(),Inh,PAGENUMBER() */
};

struct vdx_Fill
{
    GSList *children;
    char type;
    Color FillBkgnd; /* F=0,HSL(0,0,240),HSL(149,240,185),HSL(149,240,210),HSL(149,240,215),HSL(149,240,225),HSL(150,240,216),HSL(160,0,235),HSL(67,140,190),HSL(67,144,215),HSL(67,145,190),HSL(67,152,222),Inh */
    float FillBkgndTrans; /* F=0%,Inh */
    Color FillForegnd; /* F=1,HSL(0,0,240),HSL(145,240,235),HSL(149,240,226),HSL(150,240,96),HSL(35,240,168),HSL(65,123,221),HSL(67,141,213),HSL(67,143,186),HSL(67,144,215),HSL(67,144,225),HSL(67,144,235),HSL(67,148,188),HSL(68,141,226),Inh */
    float FillForegndTrans; /* F=0%,Inh */
    unsigned int FillPattern; /* F=0,1,GUARD(0),Inh */
    float ShapeShdwObliqueAngle; /* F=Inh */
    float ShapeShdwOffsetX; /* F=Inh */
    float ShapeShdwOffsetY; /* F=Inh */
    float ShapeShdwScaleFactor; /* F=Inh */
    gboolean ShapeShdwType; /* F=Inh =0 */
    unsigned int ShdwBkgnd; /* F=1,Inh */
    float ShdwBkgndTrans; /* F=0%,Inh */
    Color ShdwForegnd; /* F=0,HSL(67,141,113),Inh */
    float ShdwForegndTrans; /* F=0%,Inh */
    unsigned int ShdwPattern; /* F=0,Inh */
};

struct vdx_FontEntry
{
    GSList *children;
    char type;
    unsigned int Attributes; /* =23040,4096 */
    unsigned int CharSet; /* =0,2 */
    unsigned int ID; /* */
    char * Name; /* =Arial,Monotype Sorts,Symbol,Times New Roman,Wingdings */
    unsigned int PitchAndFamily; /* =18,2,32 */
    gboolean Unicode; /* =0 */
    gboolean Weight; /* =0 */
};

struct vdx_Fonts
{
    GSList *children;
    char type;
    unsigned int FontEntry; /* = */
};

struct vdx_Geom
{
    GSList *children;
    char type;
    unsigned int IX; /* */
    gboolean NoFill; /* F=Inh =0,1 */
    gboolean NoLine; /* F=Inh,No Formula =0 */
    gboolean NoShow; /* F=Inh,NOT(Sheet.5!User.ShowFooter),No Formula =0 */
    gboolean NoSnap; /* F=No Formula =0 */
};

struct vdx_Group
{
    GSList *children;
    char type;
    unsigned int DisplayMode; /* F=2,Inh =2 */
    gboolean DontMoveChildren; /* F=FALSE */
    gboolean IsDropTarget; /* F=FALSE,Inh */
    gboolean IsSnapTarget; /* F=TRUE */
    gboolean IsTextEditTarget; /* F=TRUE */
    gboolean SelectMode; /* F=1,Inh =1 */
};

struct vdx_Help
{
    GSList *children;
    char type;
    char * Copyright; /* =,&#xe000;,Copyright (c) 2003 Microsoft Corporation.  All rights reserved.,Copyright 2001 Microsoft Corporation.  All rights reserved. */
    char * HelpTopic; /* =,&#xe000;,Vis_SE.chm!#20000,Vis_SE.chm!#49271,Vis_SFB.chm!#50032,Vis_SFB.chm!#50033,Vis_SFB.chm!#50049 */
};

struct vdx_Hyperlink
{
    GSList *children;
    char type;
    unsigned int Address; /* = */
    gboolean Default; /* =0 */
    unsigned int Description; /* = */
    unsigned int ExtraInfo; /* = */
    unsigned int Frame; /* = */
    unsigned int ID; /* */
    gboolean Invisible; /* =0 */
    char * NameU; /* =Row_2 */
    gboolean NewWindow; /* =0 */
    unsigned int SortKey; /* = */
    char * SubAddress; /* =Detail */
};

struct vdx_Image
{
    GSList *children;
    char type;
    float Blur; /* F=0% */
    float Brightness; /* F=50% */
    float Contrast; /* F=50% */
    float Denoise; /* F=0% */
    gboolean Gamma; /* F=1 =1 */
    float Sharpen; /* F=0% */
    float Transparency; /* F=0% */
};

struct vdx_InfiniteLine
{
    GSList *children;
    char type;
    float A; /* F=If(Width&gt;0,Width,0.039370078740157DL) */
    float B; /* F=Height*0.5 */
    unsigned int IX; /* */
    float X; /* F=Width*0 */
    float Y; /* F=Height*0.5 */
};

struct vdx_Layer
{
    GSList *children;
    char type;
    gboolean Active; /* =0 */
    unsigned int Color; /* =255 */
    gboolean ColorTrans; /* F=No Formula =0 */
    gboolean Glue; /* =1 */
    unsigned int IX; /* */
    gboolean Lock; /* =0 */
    char * Name; /* =Connector,Flowchart */
    char * NameUniv; /* =Connector,Flowchart */
    gboolean Print; /* =1 */
    gboolean Snap; /* =1 */
    gboolean Status; /* =0 */
    gboolean Visible; /* =1 */
};

struct vdx_LayerMem
{
    GSList *children;
    char type;
    char * LayerMember; /* =,&#xe000;,0,1 */
};

struct vdx_Layout
{
    GSList *children;
    char type;
    unsigned int ConFixedCode; /* F=0,Inh =0,6 */
    gboolean ConLineJumpCode; /* F=0,Inh =0 */
    gboolean ConLineJumpDirX; /* F=0,Inh =0 */
    gboolean ConLineJumpDirY; /* F=0,Inh =0 */
    unsigned int ConLineJumpStyle; /* F=0,Inh */
    gboolean ConLineRouteExt; /* F=0,Inh =0 */
    gboolean ShapeFixedCode; /* F=0,Inh =0 */
    gboolean ShapePermeablePlace; /* F=FALSE,Inh,TRUE */
    gboolean ShapePermeableX; /* F=FALSE,Inh,TRUE */
    gboolean ShapePermeableY; /* F=FALSE,Inh,TRUE */
    gboolean ShapePlaceFlip; /* F=0,Inh =0 */
    gboolean ShapePlowCode; /* F=0,Inh =0 */
    unsigned int ShapeRouteStyle; /* F=0,Inh */
    gboolean ShapeSplit; /* F=Inh =0,1 */
    gboolean ShapeSplittable; /* F=Inh =0,1 */
};

struct vdx_Line
{
    GSList *children;
    char type;
    unsigned int BeginArrow; /* F=0,Inh =0,13 */
    unsigned int BeginArrowSize; /* F=2,Inh =1,2 */
    unsigned int EndArrow; /* F=0,Inh =0,1,4 */
    unsigned int EndArrowSize; /* F=2,Inh =1,2 */
    gboolean LineCap; /* F=0,Inh =0 */
    Color LineColor; /* F=0,4,HSL(0,0,240),HSL(150,240,220),HSL(150,240,229),HSL(150,240,96),Inh */
    float LineColorTrans; /* F=0%,Inh */
    unsigned int LinePattern; /* F=0,1,23,Inh */
    float LineWeight; /* F=0.01DT,0PT,Inh */
    float Rounding; /* F=0DL,Inh */
};

struct vdx_LineTo
{
    GSList *children;
    char type;
    gboolean Del; /* =1 */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Inh,Width*0,Width*0.5,Width*1,Width-Geometry1.X1 */
    float Y; /* F=Geometry1.Y1,Height,Height*0,Height*0.5,Height*1,Inh */
};

struct vdx_Master
{
    GSList *children;
    char type;
    unsigned int AlignName; /* =2 */
    char * BaseID; /* ={15A50647-7503-42A7-8B21-76CD97299D15},{2260FB21-E7DE-4F61-949E-679548813B45},{7813AA6D-FF4D-4795-B39A-7FF08597892B},{7C295579-2F17-4D19-956C-9049B19A00E3},{F7290A45-E3AD-11D2-AE4F-006008C9F5A9} */
    gboolean Hidden; /* =0 */
    unsigned int ID; /* */
    gboolean IconSize; /* =1 */
    gboolean IconUpdate; /* =0 */
    gboolean MatchByName; /* =0,1 */
    char * Name; /* =Border retro,Decision,Dynamic connector,Process,Terminator */
    char * NameU; /* =Border retro,Decision,Dynamic connector,Process,Terminator */
    gboolean PatternFlags; /* =0 */
    char * Prompt; /* =Drag onto the page to add a page-sized border. Drag handles to resize.,Drag onto the page to indicate the beginning or end of a program flow.,Drag the shape onto the drawing page.,This connector automatically routes between the shapes it connects. */
    char * UniqueID; /* ={002A9108-0000-0000-8E40-00608CF305B2},{1FBEA284-0000-0000-8E40-00608CF305B2},{1FBEA37E-0002-0000-8E40-00608CF305B2},{1FBEB428-0012-0000-8E40-00608CF305B2},{1FEF70B8-0006-0000-8E40-00608CF305B2} */
};

struct vdx_Misc
{
    GSList *children;
    char type;
    unsigned int BegTrigger; /* F=No Formula,_XFTRIGGER(Decision!EventXFMod),_XFTRIGGER(Decision.15!EventXFMod),_XFTRIGGER(Decision.17!EventXFMod),_XFTRIGGER(Decision.19!EventXFMod),_XFTRIGGER(Process!EventXFMod),_XFTRIGGER(Process.11!EventXFMod),_XFTRIGGER(Process.14!EventXFMod),_XFTRIGGER(Process.16!EventXFMod),_XFTRIGGER(Process.18!EventXFMod) =0,2 */
    gboolean Calendar; /* F=Inh =0 */
    char * Comment; /* F=Inh =,&#xe000; */
    gboolean DropOnPageScale; /* F=Inh =1 */
    unsigned int DynFeedback; /* F=0,Inh =0,2 */
    unsigned int EndTrigger; /* F=No Formula,_XFTRIGGER(Decision!EventXFMod),_XFTRIGGER(Decision.15!EventXFMod),_XFTRIGGER(Decision.17!EventXFMod),_XFTRIGGER(Decision.19!EventXFMod),_XFTRIGGER(Process!EventXFMod),_XFTRIGGER(Process.11!EventXFMod),_XFTRIGGER(Process.12!EventXFMod),_XFTRIGGER(Process.14!EventXFMod),_XFTRIGGER(Process.16!EventXFMod),_XFTRIGGER(Process.18!EventXFMod),_XFTRIGGER(Terminator!EventXFMod),_XFTRIGGER(Terminator.13!EventXFMod) =0,2 */
    unsigned int GlueType; /* F=0,Inh =0,2 */
    gboolean HideText; /* F=FALSE,Inh,NOT(Sheet.5!User.ShowFooter) */
    gboolean IsDropSource; /* F=FALSE,Inh */
    unsigned int LangID; /* F=Inh =1033 */
    gboolean LocalizeMerge; /* F=Inh =0 */
    gboolean NoAlignBox; /* F=FALSE,Inh */
    gboolean NoCtlHandles; /* F=FALSE,Inh */
    gboolean NoLiveDynamics; /* F=FALSE,Inh */
    gboolean NoObjHandles; /* F=FALSE,Inh */
    gboolean NonPrinting; /* F=FALSE,Inh,TRUE */
    unsigned int ObjType; /* F=0,Inh =0,1,12,2 */
    char * ShapeKeywords; /* F=Inh =,Decision,Yes/No,flowchart,basic,information,flow,data,process,joiner,association,business,process,Six,6,Sigma,ISO,9000,Process,function,task,Basic,Flowchart,flow,data,process,joiner,association,business,process,Six,6,Sigma,ISO,9000,Terminator,stop,begin,end,program,flow,Basic,Flowchart,information,data,process,joiner,association,business,process,Six,6,Sigma,ISO,9000,border,edge,margin,boundary,frame,retro */
    gboolean UpdateAlignBox; /* F=FALSE,Inh */
    gboolean WalkPreference; /* F=0,Inh =0 */
};

struct vdx_MoveTo
{
    GSList *children;
    char type;
    unsigned int IX; /* */
    float X; /* F=Inh,MIN(Height/2,Width/4),Width*0,Width*0.20811090826019 */
    float Y; /* F=Height*0,Height*0.5,Height*0.63886684381884,Height*0.6389,Height*1,Inh */
};

struct vdx_Page
{
    GSList *children;
    char type;
    unsigned int ID; /* */
    char * Name; /* =Detail,Overview */
    char * NameU; /* =Detail,Overview,Page-1 */
    float ViewCenterX; /* =4.241935483871,4.25,5.8425196850394,6.2677165354331 */
    float ViewCenterY; /* =3.7244094488189,4.1338582677165,5.494623655914,5.5078125 */
    float ViewScale; /* =-1,0.66145833333333 */
};

struct vdx_PageLayout
{
    GSList *children;
    char type;
    float AvenueSizeX; /* F=0.29527559055118DL */
    float AvenueSizeY; /* F=0.29527559055118DL */
    float BlockSizeX; /* F=0.19685039370079DL */
    float BlockSizeY; /* F=0.19685039370079DL */
    gboolean CtrlAsInput; /* F=FALSE,Inh */
    gboolean DynamicsOff; /* F=FALSE,Inh */
    gboolean EnableGrid; /* F=FALSE,Inh */
    gboolean LineAdjustFrom; /* F=0,Inh =0 */
    gboolean LineAdjustTo; /* F=0,Inh =0 */
    gboolean LineJumpCode; /* F=1,Inh =1 */
    float LineJumpFactorX; /* F=0.66666666666667,Inh */
    float LineJumpFactorY; /* F=0.66666666666667,Inh */
    unsigned int LineJumpStyle; /* F=0,Inh */
    gboolean LineRouteExt; /* F=0,Inh =0 */
    float LineToLineX; /* F=0.098425196850394DL,Inh */
    float LineToLineY; /* F=0.098425196850394DL,Inh */
    float LineToNodeX; /* F=0.098425196850394DL,Inh */
    float LineToNodeY; /* F=0.098425196850394DL,Inh */
    gboolean PageLineJumpDirX; /* F=0,Inh =0 */
    gboolean PageLineJumpDirY; /* F=0,Inh =0 */
    gboolean PageShapeSplit; /* =0,1 */
    gboolean PlaceDepth; /* F=0,Inh =0,1 */
    gboolean PlaceFlip; /* F=0,Inh =0 */
    unsigned int PlaceStyle; /* F=0,Inh */
    gboolean PlowCode; /* F=0,Inh =0 */
    gboolean ResizePage; /* F=FALSE,Inh */
    unsigned int RouteStyle; /* F=0 */
};

struct vdx_PageProps
{
    GSList *children;
    char type;
    float DrawingScale; /* F=No Formula */
    gboolean DrawingScaleType; /* F=No Formula =0 */
    unsigned int DrawingSizeType; /* F=No Formula =0,2,4 */
    gboolean InhibitSnap; /* F=No Formula =0 */
    float PageHeight; /* F=No Formula */
    float PageScale; /* F=No Formula */
    float PageWidth; /* F=No Formula */
    float ShdwObliqueAngle; /* F=No Formula */
    float ShdwOffsetX; /* F=No Formula */
    float ShdwOffsetY; /* F=-0.11811023622047DP,-0.125IN,No Formula */
    float ShdwScaleFactor; /* F=No Formula */
    gboolean ShdwType; /* F=No Formula =0 */
    gboolean UIVisibility; /* F=No Formula =0 */
};

struct vdx_PageSheet
{
    GSList *children;
    char type;
    unsigned int FillStyle; /* */
    unsigned int LineStyle; /* */
    unsigned int TextStyle; /* */
};

struct vdx_Para
{
    GSList *children;
    char type;
    gboolean Bullet; /* F=0,Inh =0 */
    unsigned int BulletFont; /* F=Inh */
    int BulletFontSize; /* F=Inh =-1 */
    char * BulletStr; /* F=Inh =,&#xe000; */
    gboolean Flags; /* F=Inh =0 */
    unsigned int HorzAlign; /* F=0,1,Inh =0,1,2 */
    unsigned int IX; /* */
    float IndFirst; /* F=0DP,Inh */
    float IndLeft; /* F=0DP,Inh */
    float IndRight; /* F=0DP,Inh */
    unsigned int LocalizeBulletFont; /* F=Inh */
    float SpAfter; /* F=0DT,Inh */
    float SpBefore; /* F=0DT,Inh */
    float SpLine; /* F=-120%,Inh */
    gboolean TextPosAfterBullet; /* F=Inh =0 */
};

struct vdx_PreviewPicture
{
    GSList *children;
    char type;
    unsigned int Size; /* =1056,1064,1088,1124,1168,1192,1352,1372,21048,3168,4976,752 */
};

struct vdx_PrintProps
{
    GSList *children;
    char type;
    gboolean CenterX; /* =0 */
    gboolean CenterY; /* =0 */
    gboolean OnPage; /* =0 */
    float PageBottomMargin; /* =0.25 */
    float PageLeftMargin; /* =0.25 */
    float PageRightMargin; /* =0.25 */
    float PageTopMargin; /* =0.25 */
    gboolean PagesX; /* =1 */
    gboolean PagesY; /* =1 */
    gboolean PaperKind; /* =0,1 */
    unsigned int PaperSource; /* =7 */
    gboolean PrintGrid; /* =0 */
    unsigned int PrintPageOrientation; /* =0,1,2 */
    gboolean ScaleX; /* =1 */
    gboolean ScaleY; /* =1 */
};

struct vdx_PrintSetup
{
    GSList *children;
    char type;
    float PageBottomMargin; /* =0.25 */
    float PageLeftMargin; /* =0.25 */
    float PageRightMargin; /* =0.25 */
    float PageTopMargin; /* =0.25 */
    unsigned int PaperSize; /* =9 */
    gboolean PrintCenteredH; /* =0 */
    gboolean PrintCenteredV; /* =0 */
    gboolean PrintFitOnPages; /* =0 */
    gboolean PrintLandscape; /* =1 */
    gboolean PrintPagesAcross; /* =1 */
    gboolean PrintPagesDown; /* =1 */
    gboolean PrintScale; /* =1 */
};

struct vdx_Prop
{
    GSList *children;
    char type;
    gboolean Calendar; /* F=No Formula =0 */
    char * Format; /* F=No Formula =,@ */
    unsigned int ID; /* */
    gboolean Invisible; /* F=No Formula =0 */
    char * Label; /* =Cost,Duration,Resources */
    unsigned int LangID; /* =1033 */
    char * NameU; /* =Cost,Duration,Resources */
    unsigned int Prompt; /* F=No Formula = */
    unsigned int SortKey; /* F=No Formula = */
    unsigned int Type; /* F=No Formula =0,2,7 */
    gboolean Value; /* F=No Formula =0 */
    gboolean Verify; /* F=No Formula =0 */
};

struct vdx_Protection
{
    GSList *children;
    char type;
    gboolean LockAspect; /* F=0,Inh =0,1 */
    gboolean LockBegin; /* F=0,Inh =0 */
    gboolean LockCalcWH; /* F=0,Inh =0,1 */
    gboolean LockCrop; /* F=0,Inh =0 */
    gboolean LockCustProp; /* F=Inh =0 */
    gboolean LockDelete; /* F=0,Inh,NOT(ISERROR(Sheet.5!Width)) =0,1 */
    gboolean LockEnd; /* F=0,Inh =0 */
    gboolean LockFormat; /* F=0,Inh =0 */
    gboolean LockGroup; /* F=0,Inh =0,1 */
    gboolean LockHeight; /* F=0,Inh =0,1 */
    gboolean LockMoveX; /* F=0,Inh =0,1 */
    gboolean LockMoveY; /* F=0,Inh =0,1 */
    gboolean LockRotate; /* F=0,GUARD(0),Inh =0 */
    gboolean LockSelect; /* F=0,Inh =0 */
    gboolean LockTextEdit; /* F=0,Inh =0,1 */
    gboolean LockVtxEdit; /* F=0,Inh =0 */
    gboolean LockWidth; /* F=0,Inh =0,1 */
};

struct vdx_RulerGrid
{
    GSList *children;
    char type;
    unsigned int XGridDensity; /* F=8,Inh =8 */
    float XGridOrigin; /* F=0DL,Inh */
    float XGridSpacing; /* F=0DL,Inh */
    unsigned int XRulerDensity; /* F=32,Inh =32 */
    float XRulerOrigin; /* F=0DL,Inh */
    unsigned int YGridDensity; /* F=8,Inh =8 */
    float YGridOrigin; /* F=0DL,Inh */
    float YGridSpacing; /* F=0DL,Inh */
    unsigned int YRulerDensity; /* F=32,Inh =32 */
    float YRulerOrigin; /* F=0DL,Inh */
};

struct vdx_Shape
{
    GSList *children;
    char type;
    unsigned int Field; /* = */
    unsigned int FillStyle; /* */
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    unsigned int Master; /* =0,2,3,4,5 */
    unsigned int MasterShape; /* =10,11,12,13,6,7,8,9 */
    char * NameU; /* =Border retro,Decision,Decision.15,Decision.17,Decision.19,Dynamic connector,Process,Process.11,Process.12,Process.14,Process.16,Process.18,Terminator */
    unsigned int Tabs; /* = */
    char * Text; /* =Example Text Here */
    unsigned int TextStyle; /* */
    char * Type; /* =Group,Guide,Shape */
    char * UniqueID; /* ={52A11EA5-F639-4EC9-BFFD-8D528CEA5BAC},{6AC00A6B-4D15-4815-9EB9-5FBE6A399A1D},{B73F33F1-A74B-447A-926B-F0997F1B61F1},{EC370BC5-A8D6-421A-A2F7-8765FE718385} */
    unsigned int cp; /* = */
    char * fld; /* =1,Friday, October 24, 2003 */
};

struct vdx_StyleProp
{
    GSList *children;
    char type;
    gboolean EnableFillProps; /* F=1 =0,1 */
    gboolean EnableLineProps; /* F=1 =0,1 */
    gboolean EnableTextProps; /* F=1 =0,1 */
    gboolean HideForApply; /* F=FALSE */
};

struct vdx_StyleSheet
{
    GSList *children;
    char type;
    unsigned int FillStyle; /* */
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    char * Name; /* =Border,Border graduated,Connector,Flow Connector Text,Flow Normal,Guide,Hairline,No Style,None,Normal,Text Only,Visio 00,Visio 01,Visio 02,Visio 03,Visio 10,Visio 11,Visio 12,Visio 13,Visio 20,Visio 21,Visio 22,Visio 23,Visio 50,Visio 51,Visio 52,Visio 53,Visio 70,Visio 80,Visio 90 */
    char * NameU; /* =Border,Border graduated,Connector,Flow Connector Text,Flow Normal,Guide,Hairline,No Style,None,Normal,Text Only,Visio 00,Visio 01,Visio 02,Visio 03,Visio 10,Visio 11,Visio 12,Visio 13,Visio 20,Visio 21,Visio 22,Visio 23,Visio 50,Visio 51,Visio 52,Visio 53,Visio 70,Visio 80,Visio 90 */
    unsigned int Tabs; /* = */
    unsigned int TextStyle; /* */
};

struct vdx_Tabs
{
    GSList *children;
    char type;
    unsigned int IX; /* */
};

struct vdx_Text
{
    GSList *children;
    char type;
};

struct vdx_TextBlock
{
    GSList *children;
    char type;
    float BottomMargin; /* F=0DP,0DT,4PT,Inh */
    float DefaultTabStop; /* F=0.59055118110236DP,Inh */
    float LeftMargin; /* F=0DP,4PT,Inh */
    float RightMargin; /* F=0DP,4PT,Inh */
    unsigned int TextBkgnd; /* F=0,1+FillForegnd,Inh */
    float TextBkgndTrans; /* F=0%,Inh */
    gboolean TextDirection; /* F=0,Inh =0 */
    float TopMargin; /* F=0DP,0DT,4PT,Inh */
    unsigned int VerticalAlign; /* F=0,1,2,Inh =0,1,2 */
};

struct vdx_TextXForm
{
    GSList *children;
    char type;
    float TxtAngle; /* F=GUARD(0DEG),Inh */
    float TxtHeight; /* F=GUARD(0.25IN),GUARD(TEXTHEIGHT(TheText,TxtWidth)),Height*0.75,Height*1,Inh,TEXTHEIGHT(TheText,TxtWidth) */
    float TxtLocPinX; /* F=GUARD(TxtWidth*0.5),Inh,TxtWidth*0.5 */
    float TxtLocPinY; /* F=GUARD(TxtHeight),GUARD(TxtHeight*0.5),Inh,TxtHeight*0.5 */
    float TxtPinX; /* F=GUARD(Width*0.5),Inh,SETATREF(Controls.TextPosition),Width*0.5 */
    float TxtPinY; /* F=GUARD(-0.125IN),GUARD(Height),Height*0.5,Inh,SETATREF(Controls.TextPosition.Y) */
    float TxtWidth; /* F=GUARD(Width*1),Inh,MAX(TEXTWIDTH(TheText),5*Char.Size),Width*0.83333333333333,Width*1 */
};

struct vdx_User
{
    GSList *children;
    char type;
    unsigned int ID; /* */
    char * NameU; /* =AntiScale,Margin,PageMargin,Scale,Scheme,SchemeName,ShowFooter,visVersion */
    unsigned int Prompt; /* F=No Formula = */
    float Value; /* F=0.25IN*User.AntiScale,Controls.Row_1,IF(AND(User.Scale&gt;0.125,User.Scale&lt;8),1,User.Scale),Inh,ThePage!DrawingScale/ThePage!PageScale */
};

struct vdx_VisioDocument
{
    GSList *children;
    char type;
    unsigned int DocLangID; /* =1033 */
    unsigned int Masters; /* = */
    unsigned int buildnum; /* =3216 */
    char * key; /* =9F9D0DE06B95F5E9626C0D8F3FE058D8A9171702DF08E80D2568B3CD1299B9F0F04F6B4D530A8194F5E7CE8818E77CA90B41DFDD34D849AC7DA3D8C3E85DD922 */
    gboolean metric; /* =0 */
    unsigned int start; /* =190 */
    char * version; /* =preserve */
    char * xmlns; /* =http://schemas.microsoft.com/visio/2003/core,urn:schemas-microsoft-com:office:visio */
};

struct vdx_Window
{
    GSList *children;
    char type;
    char * ContainerType; /* =Page */
    char * Document; /* =C:\Program Files\Microsoft Office\Visio11\1033\ARROWS_U.VSS,C:\Program Files\Microsoft Office\Visio11\1033\BACKGR_U.VSS,C:\Program Files\Microsoft Office\Visio11\1033\BASFLO_U.VSS,C:\Program Files\Microsoft Office\Visio11\1033\BORDER_U.VSS */
    gboolean DynamicGridEnabled; /* =0,1 */
    unsigned int GlueSettings; /* =9 */
    unsigned int ID; /* */
    gboolean Page; /* =0 */
    gboolean ParentWindow; /* =0 */
    gboolean ShowConnectionPoints; /* =0,1 */
    gboolean ShowGrid; /* =0,1 */
    gboolean ShowGuides; /* =1 */
    gboolean ShowPageBreaks; /* =0 */
    gboolean ShowRulers; /* =1 */
    unsigned int SnapExtensions; /* =34 */
    unsigned int SnapSettings; /* =295,65831 */
    unsigned int StencilGroup; /* =10 */
    unsigned int StencilGroupPos; /* =0,1,2,3 */
    float TabSplitterPos; /* =0.33,0.5 */
    float ViewCenterX; /* =4.25,5.8425196850394,6.2677165354331 */
    float ViewCenterY; /* =3.7244094488189,4.1338582677165,5.5078125 */
    float ViewScale; /* =-1,0.66145833333333 */
    unsigned int WindowHeight; /* =652,884,928 */
    int WindowLeft; /* =-240,-4 */
    unsigned int WindowState; /* =1025,1073741824,67109889 */
    int WindowTop; /* =-23,-34,1 */
    char * WindowType; /* =Drawing,Stencil */
    unsigned int WindowWidth; /* =1032,1288,234 */
};

struct vdx_Windows
{
    GSList *children;
    char type;
    unsigned int ClientHeight; /* =625,890 */
    unsigned int ClientWidth; /* =1024,1280 */
};

struct vdx_XForm
{
    GSList *children;
    char type;
    float Angle; /* F=ATan2(EndY-BeginY,EndX-BeginX),GUARD(0DA),GUARD(0DEG),GUARD(ATAN2(EndY-BeginY,EndX-BeginX)),Inh */
    gboolean FlipX; /* F=GUARD(0),GUARD(FALSE),Inh =0 */
    gboolean FlipY; /* F=GUARD(0),GUARD(FALSE),Inh =0 */
    float Height; /* F=GUARD(0.25DL),GUARD(0IN),GUARD(EndY-BeginY),GUARD(Sheet.5!User.Margin*2),GUARD(Sheet.5!User.Margin*4),GUARD(TEXTHEIGHT(TheText,100)),Inh,ThePage!PageHeight-4*User.PageMargin */
    float LocPinX; /* F=GUARD(Width*0),GUARD(Width*0.25),GUARD(Width*0.5),GUARD(Width*0.75),GUARD(Width*1),Inh,Width*0,Width*0.5 */
    float LocPinY; /* F=GUARD(Height*0),GUARD(Height*0.5),GUARD(Height*1),Height*0,Height*0.5,Inh */
    float PinX; /* F=(BeginX+EndX)/2,GUARD((BeginX+EndX)/2),GUARD(0),GUARD(IF(Width&gt;Sheet.8!Width*0.75,Sheet.8!Width*0.25,Sheet.8!Width*0.5+(Sheet.8!Width*0.25)-(Width*0.5))),GUARD(MAX(Width*1.25,Sheet.10!PinX+Sheet.10!Width)),GUARD(MIN(Sheet.5!Width-Width*1.25,Sheet.5!Width-((Sheet.5!Width-Sheet.13!PinX)+Sheet.13!Width))),GUARD(Sheet.5!Width),GUARD(Sheet.5!Width-IF(Width&gt;Sheet.11!Width*0.75,Sheet.11!Width*0.25,Sheet.11!Width*0.5+(Sheet.11!Width*0.25)-(Width*0.5))),Inh,ThePage!PageWidth*0.5 */
    float PinY; /* F=(BeginY+EndY)/2,GUARD((BeginY+EndY)/2),GUARD(0),GUARD(0.5*Sheet.11!Height),GUARD(MAX(Height+Sheet.5!User.Margin*0.55,Sheet.13!PinY+Sheet.13!Height)),GUARD(MIN(Sheet.5!Height-(Height+Sheet.5!User.Margin),Sheet.10!PinY-Sheet.10!Height)),GUARD(Sheet.5!Height),GUARD(Sheet.5!Height-(0.5*Sheet.9!Height)),Inh,ThePage!PageHeight*0.5 */
    gboolean ResizeMode; /* F=Inh =0 */
    float Width; /* F=GUARD(0.25DL),GUARD(EndX-BeginX),GUARD(Height*4.044),GUARD(SQRT((EndX-BeginX)^2+(EndY-BeginY)^2)),GUARD(TEXTWIDTH(TheText,100)),Inh,Sqrt((EndX-BeginX)^2+(EndY-BeginY)^2),ThePage!PageWidth-4*User.PageMargin */
};

struct vdx_XForm1D
{
    GSList *children;
    char type;
    float BeginX; /* F=GUARD(Sheet.10!PinX),Inh,PAR(PNT(Decision!Connections.X2,Decision!Connections.Y2)),PAR(PNT(Decision.17!Connections.X2,Decision.17!Connections.Y2)),PAR(PNT(Decision.19!Connections.X3,Decision.19!Connections.Y3)),Sheet.5!Width*0,_WALKGLUE(BegTrigger,EndTrigger,WalkPreference) */
    float BeginY; /* F=GUARD(Sheet.13!PinY+Sheet.13!Height),GUARD(Sheet.8!PinY),Inh,PAR(PNT(Decision!Connections.X2,Decision!Connections.Y2)),PAR(PNT(Decision.17!Connections.X2,Decision.17!Connections.Y2)),PAR(PNT(Decision.19!Connections.X3,Decision.19!Connections.Y3)),_WALKGLUE(BegTrigger,EndTrigger,WalkPreference) */
    float EndX; /* F=GUARD(Sheet.13!PinX),Inh,PAR(PNT(Process!Connections.X2,Process!Connections.Y2)),PAR(PNT(Process.18!Connections.X3,Process.18!Connections.Y3)),PAR(PNT(Process.18!Connections.X4,Process.18!Connections.Y4)),Sheet.5!Width*1,_WALKGLUE(EndTrigger,BegTrigger,WalkPreference) */
    float EndY; /* F=GUARD(Sheet.13!PinY+Sheet.13!Height),GUARD(Sheet.8!PinY),Inh,PAR(PNT(Process!Connections.X2,Process!Connections.Y2)),PAR(PNT(Process.18!Connections.X3,Process.18!Connections.Y3)),PAR(PNT(Process.18!Connections.X4,Process.18!Connections.Y4)),_WALKGLUE(EndTrigger,BegTrigger,WalkPreference) */
};

struct vdx_cp
{
    GSList *children;
    char type;
    unsigned int IX; /* */
};

struct vdx_fld
{
    GSList *children;
    char type;
    unsigned int IX; /* */
};

enum {
    vdx_types_Act,
    vdx_types_Char,
    vdx_types_ColorEntry,
    vdx_types_Colors,
    vdx_types_Connect,
    vdx_types_Connection,
    vdx_types_Connects,
    vdx_types_Control,
    vdx_types_CustomProp,
    vdx_types_CustomProps,
    vdx_types_DocProps,
    vdx_types_DocumentProperties,
    vdx_types_DocumentSettings,
    vdx_types_DocumentSheet,
    vdx_types_Ellipse,
    vdx_types_EllipticalArcTo,
    vdx_types_Event,
    vdx_types_EventItem,
    vdx_types_EventList,
    vdx_types_FaceName,
    vdx_types_FaceNames,
    vdx_types_Field,
    vdx_types_Fill,
    vdx_types_FontEntry,
    vdx_types_Fonts,
    vdx_types_Geom,
    vdx_types_Group,
    vdx_types_Help,
    vdx_types_Hyperlink,
    vdx_types_Image,
    vdx_types_InfiniteLine,
    vdx_types_Layer,
    vdx_types_LayerMem,
    vdx_types_Layout,
    vdx_types_Line,
    vdx_types_LineTo,
    vdx_types_Master,
    vdx_types_Misc,
    vdx_types_MoveTo,
    vdx_types_Page,
    vdx_types_PageLayout,
    vdx_types_PageProps,
    vdx_types_PageSheet,
    vdx_types_Para,
    vdx_types_PreviewPicture,
    vdx_types_PrintProps,
    vdx_types_PrintSetup,
    vdx_types_Prop,
    vdx_types_Protection,
    vdx_types_RulerGrid,
    vdx_types_Shape,
    vdx_types_StyleProp,
    vdx_types_StyleSheet,
    vdx_types_Tabs,
    vdx_types_Text,
    vdx_types_TextBlock,
    vdx_types_TextXForm,
    vdx_types_User,
    vdx_types_VisioDocument,
    vdx_types_Window,
    vdx_types_Windows,
    vdx_types_XForm,
    vdx_types_XForm1D,
    vdx_types_cp,
    vdx_types_fld
};

extern char * vdx_Units[];
extern char * vdx_Types[];
