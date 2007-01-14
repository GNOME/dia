/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 *
 * visio-types.h: Visio XML import filter for dia
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

/* Generated Thu Nov 30 19:27:16 2006 */
/* From: All.vdx animation_tests.vdx Arrows-2.vdx Arrow & Text samples.vdx BasicShapes.vdx basic_tests.vdx Beispiel 1.vdx Beispiel 2.vdx Beispiel 3.vdx Circle1.vdx Circle2.vdx curve_tests.vdx Drawing2.vdx Embedded-Pics-1.vdx emf_dump_test2.orig.vdx emf_dump_test2.vdx Entreprise_etat_desire.vdx Line1.vdx Line2.vdx Line3.vdx Line4.vdx Line5.vdx Line6.vdx LombardiWireframe.vdx pattern_tests.vdx Rectangle1.vdx Rectangle2.vdx Rectangle3.vdx Rectangle4.vdx sample1.vdx Sample2.vdx samp_vdx.vdx seq_test.vdx SmithWireframe.vdx states.vdx Text1.vdx Text2.vdx Text3.vdx text_tests.vdx */


struct vdx_any
{
    GSList *children;
    char type;
};

struct vdx_Act
{
    GSList *children;
    char type;
    float Action; /* F=0,1,SETF(&quot;LockDelete&quot;,&quot;Guard(0)&quot;),SETF(&quot;LockDelete&q Unit=BOOL */
    gboolean BeginGroup; /* F=No Formula =0 */
    unsigned int ButtonFace; /* F=No Formula = */
    gboolean Checked; /* F=0",1",2",Inh,LockDelete,LockTextEdit,No Formula =0,1 */
    gboolean Disabled; /* F=Inh,NOT(User.UMLError),No Formula,Not(User.HasSubordinates) =0,1 */
    unsigned int ID; /* */
    unsigned int IX; /* */
    gboolean Invisible; /* F=No Formula =0 */
    char * Menu; /* F=0),"","_Set As Straight Line")',0),&quot;&quot;,&quot;_ Gerade Linie&quot;)", */
    char * NameU; /* =LocalRow0 */
    gboolean ReadOnly; /* F=No Formula =0 */
    unsigned int SortKey; /* F=No Formula = */
    unsigned int TagName; /* F=No Formula = */
};

struct vdx_Align
{
    GSList *children;
    char type;
    float AlignBottom; /* F=IntersectY(Sheet.1972!PinX,Sheet.1972!PinY,Sheet.1972!Angle,10.185039370079DL */
    float AlignCenter; /* F=IntersectX(Sheet.1972!PinX,Sheet.1972!PinY,Sheet.1972!Angle,10.185039370079DL */
    gboolean AlignLeft; /* F=_Marker(1) =1 */
    gboolean AlignMiddle; /* F=_Marker(1) =1 */
    gboolean AlignRight; /* F=_Marker(1) =1 */
    float AlignTop; /* F=IntersectY(Sheet.1973!PinX,Sheet.1973!PinY,Sheet.1973!Angle,14.464566929134DL */
};

struct vdx_ArcTo
{
    GSList *children;
    char type;
    float A; /* F=-Scratch.Y1,-User.Margin*(3/2),-User.Margin/2,-User.Margin/4,-Width,0.125*Wid Unit=DL,IN,MM */
    unsigned int IX; /* */
    float X; /* F=-User.Margin,0.4*Scratch.X1,0.6*Scratch.X1,Geometry1.X1,Geometry1.X2,Geometry Unit=IN,MM */
    float Y; /* F=-User.Margin*(3/2),Geometry1.Y1,Geometry1.Y4,Geometry2.Y1,Geometry2.Y4,Height Unit=IN,MM */
};

struct vdx_BegTrigger
{
    GSList *children;
    char type;
    char * Err; /* =#REF! */
};

struct vdx_BeginX
{
    GSList *children;
    char type;
    char * Err; /* =#REF! */
};

struct vdx_BeginY
{
    GSList *children;
    char type;
    char * Err; /* =#REF! */
};

struct vdx_Char
{
    GSList *children;
    char type;
    unsigned int AsianFont; /* F=Inh */
    gboolean Case; /* F=0,Inh =0 */
    Color Color; /* F=0,0,0,19))",0,1,16))",1,14,15,2,4,GUARD(IF(Sheet.5!User.active,0,19)),HSL(0,0 */
    float ColorTrans; /* F=0%,Inh,No Formula */
    unsigned int ComplexScriptFont; /* F=Inh */
    int ComplexScriptSize; /* F=Inh =-1 */
    gboolean DblUnderline; /* F=FALSE,Inh,No Formula */
    gboolean Del; /* =1 */
    gboolean DoubleStrikethrough; /* F=Inh =0 */
    unsigned int Font; /* F=0,1,2,Inh */
    float FontScale; /* F=100%,Inh */
    gboolean Highlight; /* F=Inh,No Formula =0 */
    unsigned int IX; /* */
    unsigned int LangID; /* F=Inh =1031,1033,1036,2057,3081,3084,4105 */
    float Letterspace; /* F=0DT,Inh,No Formula */
    unsigned int Locale; /* F=0,Inh =0,57 */
    unsigned int LocalizeFont; /* F=Inh */
    gboolean Overline; /* F=FALSE,Inh,No Formula */
    gboolean Perpendicular; /* F=FALSE,Inh,No Formula */
    gboolean Pos; /* F=0,Inh =0,1 */
    gboolean RTLText; /* F=Inh =0 */
    float Size; /* F=0.11111111111111DT,0.125DT,0.16666666666667DT,10PT,12PT/User.ScaleRatio,14PT/ Unit=PT */
    gboolean Strikethru; /* F=FALSE,Inh,No Formula */
    unsigned int Style; /* F=0,3,2,0)))',Inh */
    gboolean UseVertical; /* F=Inh =0 */
};

struct vdx_ColorEntry
{
    GSList *children;
    char type;
    unsigned int IX; /* */
    char * RGB; /* =#000000,#000080,#0000FF,#008000,#008080,#00FF00,#00FFFF,#1A1A1A,#333333,#4D4D4 */
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
    char * FromCell; /* =AlignBottom,AlignTop,BeginX,BeginY,Controls.X1,Controls.X2,EndX,EndY */
    unsigned int FromPart; /* =10,100,101,11,12,4,6,7,8,9 */
    gboolean FromPart_exists;
    unsigned int FromSheet; /* =100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,12,121,124,13, */
    gboolean FromSheet_exists;
    char * ToCell; /* =Angle,Connections.Bottom.X,Connections.Float.X,Connections.Row_1.X,Connections */
    unsigned int ToPart; /* =100,101,102,103,104,105,108,3,4 */
    gboolean ToPart_exists;
    unsigned int ToSheet; /* =1,10,102,107,11,114,1152,1153,1155,1157,119,12,13,131,14,15,16,17,170,178,18,1 */
    gboolean ToSheet_exists;
};

struct vdx_Connection
{
    GSList *children;
    char type;
    gboolean AutoGen; /* F=Inh,No Formula =0,1 */
    float DirX; /* F=-25.4MM,Inh,No Formula Unit=IN,MM,NUM */
    float DirY; /* F=-25.4MM,Inh,No Formula Unit=IN,MM,NUM */
    unsigned int ID; /* */
    unsigned int IX; /* */
    char * NameU; /* =Bottom,Float,Left,Right,Top */
    char * Prompt; /* F=No Formula =,&#xe000; */
    gboolean Type; /* F=Inh,No Formula =0 */
    float X; /* F=0.5*Width,2,Width*0.18,Width*0.2003))",4,Width*0.0822,Width*0.1929))",4,Width Unit=IN,MM */
    float Y; /* F=0.5*Height,2,Height*0.05,Height*0.1233))))",3,Height*0,Height*0.1197)))",3,He Unit=IN,MM */
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
    gboolean CanGlue; /* F=Inh =0,1 */
    unsigned int ID; /* */
    unsigned int IX; /* */
    char * NameU; /* =TextPosition */
    char * Prompt; /* F=Controls.Prompt,Controls.Row_1.Prompt,Inh =,&#xe000;,Adjust Frame Thickness,A */
    float X; /* F=0.125IN*User.AntiScale,0.7IN*User.AntiScale,GUARD(Connections.X1+COS(User.Beg Unit=IN,MM */
    float XCon; /* F=(Controls.X1&gt;Width/2)*2+2+5*Not(Actions.Action[2]),(Controls.X2&gt;Width/2 */
    float XDyn; /* F=Controls.Row_1,Controls.Row_2,Controls.Row_3,Controls.Row_4,Controls.TextPosi Unit=IN,MM */
    float Y; /* F=Controls.Row_1.Y,GUARD(0),GUARD(Connections.Y1+SIN(User.BeginAngle+45DEG)*0.1 Unit=IN,MM,PT */
    float YCon; /* F=(Controls.Y1&gt;Height/2)*2+2,(Controls.Y2&gt;Height/2)*2+2,4-2*Scratch.B1,In */
    float YDyn; /* F=Controls.Row_1.Y,Controls.Row_2.Y,Controls.Row_3.Y,Controls.Row_4.Y,Controls. Unit=IN,MM,PT */
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
    char * CustomProp; /* =TC010483951033,TC010492811033,TC010498511033 */
};

struct vdx_DocProps
{
    GSList *children;
    char type;
    gboolean AddMarkup; /* F=No Formula =0 */
    unsigned int DocLangID; /* =1033,2057 */
    gboolean LockPreview; /* F=No Formula =0,1 */
    unsigned int OutputFormat; /* =0,2 */
    gboolean PreviewQuality; /* F=No Formula =0 */
    gboolean PreviewScope; /* F=No Formula =0,1 */
    gboolean ViewMarkup; /* F=No Formula =0 */
};

struct vdx_DocumentProperties
{
    GSList *children;
    char type;
    int BuildNumberCreated; /* =-1,1245,2072,671351279,671351309,671353298,738200627,738200720,738203013 */
    unsigned int BuildNumberEdited; /* =671351309,671352994,671353097,671353298,671353406,738200720,738203013 */
    char * Company; /* =ARS-Solutions,Artis Group,Celox,GarrettCom Europe,Microsoft Corporation,Regule */
    char * Creator; /* = Pierre Robitaille,Administrator,Copyright (c) 2001 Microsoft Corporation.  Al */
    char * Desc; /* =$Id$ */
    char * Subject; /* =Sample file for Visio Viewer,Sample file for Visio Web Component */
    char * Template; /* =C:\Documents and Settings\Gene.GENE-LAPTOP\My Documents\aifia_wireframe_templa */
    char * TimeCreated; /* =2001-03-14T11:57:54,2002-01-24T12:57:03,2002-02-28T14:18:35,2002-03-25T13:54:0 */
    char * TimeEdited; /* =1899-12-30T00:00:00,2001-10-23T14:49:27,2002-02-28T14:20:31,2002-05-15T07:40:2 */
    char * TimePrinted; /* =2001-03-14T11:57:54,2002-02-28T14:18:35,2002-03-25T13:54:03,2002-04-17T14:27:5 */
    char * TimeSaved; /* =2001-10-23T14:49:30,2002-02-28T14:20:34,2002-05-15T07:40:45,2002-06-18T14:58:4 */
    char * Title; /* =Basic tests,Curve tests,Drawing2,Kabelpläne,LombardiWireframe,Meldelinien im  */
};

struct vdx_DocumentSettings
{
    GSList *children;
    char type;
    unsigned int DefaultFillStyle; /* */
    gboolean DefaultFillStyle_exists;
    unsigned int DefaultGuideStyle; /* */
    gboolean DefaultGuideStyle_exists;
    unsigned int DefaultLineStyle; /* */
    gboolean DefaultLineStyle_exists;
    unsigned int DefaultTextStyle; /* */
    gboolean DefaultTextStyle_exists;
    gboolean DynamicGridEnabled; /* =0,1 */
    unsigned int GlueSettings; /* =47,9 */
    gboolean ProtectBkgnds; /* =0 */
    gboolean ProtectMasters; /* =0 */
    gboolean ProtectShapes; /* =0 */
    gboolean ProtectStyles; /* =0 */
    unsigned int SnapExtensions; /* =1,34 */
    unsigned int SnapSettings; /* =295,319,33063,39,65831,65847 */
    unsigned int TopPage; /* =0,2,5,7,8 */
    gboolean TopPage_exists;
};

struct vdx_DocumentSheet
{
    GSList *children;
    char type;
    unsigned int FillStyle; /* */
    gboolean FillStyle_exists;
    unsigned int LineStyle; /* */
    gboolean LineStyle_exists;
    char * Name; /* =DasDok,TheDoc */
    char * NameU; /* =TheDoc */
    unsigned int TextStyle; /* */
    gboolean TextStyle_exists;
};

struct vdx_Ellipse
{
    GSList *children;
    char type;
    float A; /* F=Width*1 Unit=DL */
    float B; /* F=Height*0.5 Unit=DL */
    float C; /* F=Width*0.5 Unit=DL */
    float D; /* F=Height*1 Unit=DL */
    unsigned int IX; /* */
    float X; /* F=Width*0.5 */
    float Y; /* F=Height*0.5 */
};

struct vdx_EllipticalArcTo
{
    GSList *children;
    char type;
    float A; /* F=Controls.Row_1,Controls.X1,Geometry1.A2,Geometry3.A2,Inh,Width,Width*0.000357 Unit=DL,IN,MM */
    float B; /* F=2*Scratch.X1,Controls.Row_1.Y,Controls.Y1,Geometry1.B2,Geometry2.B2,Height*0, Unit=DL,IN,MM */
    float C; /* F=Inh,_ELLIPSE_THETA(-0.010584331794649,1.0252774665981,8.2610199709935,10.5,Wi Unit=DA */
    float D; /* F=2*Geometry1.X1/Height,Geometry1.D3,Inh,Width/Height*0.049509756796392,Width/H */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Geometry1.X2,Geometry2.X1,Geometry3.X1,Geometry4.X1,Geometry6.X1 Unit=IN,MM */
    float Y; /* F=Geometry1.Y1,Geometry1.Y4,Geometry2.Y1,Geometry3.Y1,Geometry4.Y1,Geometry6.Y1 Unit=IN,MM */
};

struct vdx_EndX
{
    GSList *children;
    char type;
    char * Err; /* =#REF! */
};

struct vdx_EndY
{
    GSList *children;
    char type;
    char * Err; /* =#REF! */
};

struct vdx_Event
{
    GSList *children;
    char type;
    gboolean EventDblClick; /* F=1001&quot;)",DEFAULTEVENT(),Inh,NA(),No Formula,OPENTEXTWIN(),_DefaultEvent() */
    float EventDrop; /* F=0,DOCMD(1312)+SETF(&quot;eventdrop&quot;,0),Inh,No Formula,RUNADDON(&quot;Mak */
    gboolean EventXFMod; /* F=No Formula =0 */
    gboolean TheData; /* F=No Formula =0 */
    gboolean TheText; /* F=No Formula =0 */
};

struct vdx_EventDblClick
{
    GSList *children;
    char type;
    char * Err; /* =#N/A */
};

struct vdx_EventItem
{
    GSList *children;
    char type;
    gboolean Action; /* =1 */
    gboolean Enabled; /* =1 */
    unsigned int EventCode; /* =1,2 */
    gboolean EventCode_exists;
    unsigned int ID; /* */
    char * Target; /* =AM,OrgC,OrgCWiz,PPT,UML Background Add-on */
    char * TargetArgs; /* =,/autolaunch,1,1",2,2",3,DocCreated',DocOpened' */
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
    char * CharSets; /* =-2147483648 0,0 0,1 0,1048577 0,1073742335 -65536,1073873055 -539557888,107426 */
    unsigned int Flags; /* =260,261,325,327,357,421 */
    gboolean Flags_exists;
    unsigned int ID; /* */
    char * Name; /* =,Arial,Arial Unicode MS,Book Antiqua,Cordia New,Dhenu,Dotum,Estrangelo Edessa, */
    char * Panos; /* =0 0 4 0 0 0 0 0 0 0,1 1 6 0 1 1 1 1 1 1,1 10 5 2 5 3 6 3 3 3,2 0 4 0 0 0 0 0 0 */
    char * UnicodeRanges; /* =-1 -369098753 63 0,-1342176593 1775729915 48 0,-1610612033 1757936891 16 0,-21 */
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
    float Format; /* F=FIELDPICTURE(0),FIELDPICTURE(201),FIELDPICTURE(21),FIELDPICTURE(24),FIELDPICT Unit=STR */
    unsigned int IX; /* */
    gboolean ObjectKind; /* F=No Formula =0 */
    unsigned int Type; /* F=Inh =0,2,5 */
    unsigned int UICat; /* F=Inh =0,1,2,3,5 */
    unsigned int UICod; /* F=Inh =0,1,2,255,3,4,6,8 */
    unsigned int UIFmt; /* F=Inh =0,20,201,21,255,34,37,8 */
    float Value; /* F=COMPANY(),CREATOR(),Creator(),DOCLASTSAVE(),DocCreation(),DocLastEdit(),DocLa Unit=DATE,DL,STR */
};

struct vdx_Fill
{
    GSList *children;
    char type;
    Color FillBkgnd; /* F=0,1,14,15,18,2,8,HSL(0,0,128),HSL(0,0,240),HSL(0,0,80),HSL(0,240,36),HSL(0,24 */
    float FillBkgndTrans; /* F=0%,Inh,No Formula */
    Color FillForegnd; /* F=0,0,0,20)",0,1,17)",0,10,19))",1,10,11,12,13,14,15,17,18,19,2,20,21,23,3,4,5, */
    float FillForegndTrans; /* F=0%,Inh,No Formula */
    unsigned int FillPattern; /* F=0,1,31,GUARD(0),Guard(0),Inh */
    float ShapeShdwObliqueAngle; /* F=Inh */
    float ShapeShdwOffsetX; /* F=Inh */
    float ShapeShdwOffsetY; /* F=Inh */
    float ShapeShdwScaleFactor; /* F=Inh */
    gboolean ShapeShdwType; /* F=Inh =0 */
    unsigned int ShdwBkgnd; /* F=1,Inh */
    float ShdwBkgndTrans; /* F=0%,Inh,No Formula */
    Color ShdwForegnd; /* F=0,15,8,HSL(0,0,70),HSL(0,0,74),HSL(0,0,80),HSL(0,240,60),HSL(0,240,62),HSL(12 */
    float ShdwForegndTrans; /* F=0%,Inh,No Formula */
    unsigned int ShdwPattern; /* F=0,0),1,0)',Guard(0),Inh */
};

struct vdx_FontEntry
{
    GSList *children;
    char type;
    unsigned int Attributes; /* =0,16896,19072,19140,19172,23040,23108,23140,4096,4160,4196 */
    gboolean Attributes_exists;
    unsigned int CharSet; /* =0,2 */
    gboolean CharSet_exists;
    unsigned int ID; /* */
    char * Name; /* =Arial,Arial Narrow,Monotype Sorts,Symbol,Times New Roman,Wingdings */
    unsigned int PitchAndFamily; /* =18,2,32,34 */
    gboolean PitchAndFamily_exists;
    gboolean Unicode; /* =0 */
    gboolean Weight; /* =0 */
};

struct vdx_Fonts
{
    GSList *children;
    char type;
    unsigned int FontEntry; /* = */
};

struct vdx_Foreign
{
    GSList *children;
    char type;
    float ImgHeight; /* F=Height*1 */
    float ImgOffsetX; /* F=ImgWidth*0 */
    float ImgOffsetY; /* F=ImgHeight*0 */
    float ImgWidth; /* F=Width*1 */
};

struct vdx_ForeignData
{
    GSList *children;
    char type;
    float CompressionLevel; /* =0.05 */
    char * CompressionType; /* =JPEG */
    unsigned int ExtentX; /* =2588,3368,3673,4022,6403,6773,7073 */
    gboolean ExtentX_exists;
    unsigned int ExtentY; /* =1153,1318,1333,1349,3278,4513,738 */
    gboolean ExtentY_exists;
    char * ForeignType; /* =Bitmap,EnhMetaFile,Object */
    unsigned int MappingMode; /* =8 */
    gboolean MappingMode_exists;
    float ObjectHeight; /* =2,3.5566929133858 */
    unsigned int ObjectType; /* =33280,49664 */
    gboolean ObjectType_exists;
    float ObjectWidth; /* =2,9.0783464566929 */
    gboolean ShowAsIcon; /* =0 */
};

struct vdx_Geom
{
    GSList *children;
    char type;
    unsigned int IX; /* */
    gboolean NoFill; /* F=Guard(1),Inh =0,1 */
    gboolean NoLine; /* F=Inh,No Formula =0 */
    gboolean NoShow; /* F=0)",0,User.ShapeType&lt;2))))',1)",1)',1))",1))',1,0,1)",2)",2)',2))",2,0,1)" */
    gboolean NoSnap; /* F=Inh,No Formula =0 */
};

struct vdx_Group
{
    GSList *children;
    char type;
    unsigned int DisplayMode; /* F=2,Inh =2 */
    gboolean DontMoveChildren; /* F=FALSE,Inh */
    gboolean IsDropTarget; /* F=FALSE,Inh */
    gboolean IsSnapTarget; /* F=Inh,TRUE */
    gboolean IsTextEditTarget; /* F=Inh,TRUE */
    gboolean SelectMode; /* F=1,Inh =0,1 */
};

struct vdx_HeaderFooter
{
    GSList *children;
    char type;
    char * FooterLeft; /* =&amp;f&amp;e&amp;n */
    float FooterMargin; /* Unit=MM */
    char * HeaderFooterColor; /* =#000000 */
    unsigned int HeaderFooterFont; /* */
    char * HeaderLeft; /* =KVR GL/3 */
    float HeaderMargin; /* Unit=MM */
    char * HeaderRight; /* =&amp;D */
};

struct vdx_HeaderFooterFont
{
    GSList *children;
    char type;
    gboolean CharSet; /* =0 */
    unsigned int ClipPrecision; /* =2 */
    gboolean ClipPrecision_exists;
    gboolean Escapement; /* =0 */
    char * FaceName; /* =Arial,Arial Narrow */
    int Height; /* =-11,-13 */
    gboolean Height_exists;
    gboolean Italic; /* =0 */
    gboolean Orientation; /* =0 */
    unsigned int OutPrecision; /* =3 */
    gboolean OutPrecision_exists;
    unsigned int PitchAndFamily; /* =34 */
    gboolean PitchAndFamily_exists;
    gboolean Quality; /* =1 */
    gboolean StrikeOut; /* =0 */
    gboolean Underline; /* =0 */
    unsigned int Weight; /* =400 */
    gboolean Weight_exists;
    gboolean Width; /* =0 */
};

struct vdx_Help
{
    GSList *children;
    char type;
    char * Copyright; /* F=Inh =,&#xe000;,Copyright (c) 2001 Microsoft Corporation.  All rights reserved */
    char * HelpTopic; /* F=Inh =,&#xe000;,ET.HLP!#1018,ET.HLP!#1023,ET.HLP!#1024,ET.HLP!#1026,Netzwerk.H */
};

struct vdx_Hyperlink
{
    GSList *children;
    char type;
    char * Address; /* =,http://vdxtosvg.sourceforge.net/ */
    gboolean Default; /* =0 */
    char * Description; /* =,VDXtoSVG Home Page */
    char * ExtraInfo; /* =,&#xe000; */
    unsigned int Frame; /* F=No Formula = */
    unsigned int ID; /* */
    gboolean Invisible; /* =0 */
    char * NameU; /* =Row_2,Row_21 */
    gboolean NewWindow; /* =0 */
    unsigned int SortKey; /* = */
    char * SubAddress; /* =&#xe000;,Detail */
};

struct vdx_Icon
{
    GSList *children;
    char type;
    unsigned int IX; /* */
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
    float A; /* F=IF(Width&gt;0,Width,1DL),If(Width&gt;0,Width,0.039370078740157DL) Unit=DL */
    float B; /* F=Height*0.5 Unit=DL */
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
    float ColorTrans; /* F=No Formula =0,0.5 */
    gboolean Glue; /* =0,1 */
    unsigned int IX; /* */
    gboolean Lock; /* =0 */
    char * Name; /* F=No Formula =,&#xe000;,Annotations,Background form,Bezeichnung,CE,Connector,Di */
    char * NameUniv; /* F=No Formula =,&#xe000;,Annotations,Bezeichnung,CE,Connector,Etikette,Flowchart */
    gboolean Print; /* =0,1 */
    gboolean Snap; /* =0,1 */
    unsigned int Status; /* =0,2 */
    gboolean Visible; /* =0,1 */
};

struct vdx_LayerMem
{
    GSList *children;
    char type;
    char * LayerMember; /* =,&#xe000;,0,1,2,4,5,6, */
};

struct vdx_Layout
{
    GSList *children;
    char type;
    unsigned int ConFixedCode; /* F=0,Inh =0,3,6 */
    gboolean ConLineJumpCode; /* F=0,Inh =0 */
    gboolean ConLineJumpDirX; /* F=0,Inh =0 */
    gboolean ConLineJumpDirY; /* F=0,Inh =0 */
    unsigned int ConLineJumpStyle; /* F=0,Inh */
    unsigned int ConLineRouteExt; /* F=0,Inh,No Formula =0,2 */
    unsigned int ShapeFixedCode; /* F=0,Inh =0,3 */
    gboolean ShapePermeablePlace; /* F=FALSE,Inh,TRUE */
    gboolean ShapePermeableX; /* F=FALSE,Inh,TRUE */
    gboolean ShapePermeableY; /* F=FALSE,Inh,TRUE */
    gboolean ShapePlaceFlip; /* F=0,Inh,No Formula =0 */
    gboolean ShapePlowCode; /* F=0,Inh =0,1 */
    unsigned int ShapeRouteStyle; /* F=0,Inh */
    gboolean ShapeSplit; /* F=Inh =0,1 */
    gboolean ShapeSplittable; /* F=Inh =0,1 */
};

struct vdx_Line
{
    GSList *children;
    char type;
    unsigned int BeginArrow; /* F=0,3,41,4)))',4,GUARD(0),Inh,USE(&quot;Composite&quot;) =0,1,10,11,13,14,15,25 */
    unsigned int BeginArrowSize; /* F=1,2,IF(User.UMLError,0,2),Inh =0,1,2,4 */
    unsigned int EndArrow; /* F=0,3,41,4)))',4,GUARD(0),Inh =0,1,10,11,12,13,14,15,16,4,5,9 */
    unsigned int EndArrowSize; /* F=1,2,IF(User.UMLError,0,2),Inh =0,1,2,4 */
    gboolean LineCap; /* F=0,1,Inh =0,1 */
    Color LineColor; /* F=0,1,14,15,2,3,4,HSL(0,0,0),HSL(0,0,240),HSL(0,0,60),HSL(0,0,66),HSL(0,240,85) */
    float LineColorTrans; /* F=0%,Inh,No Formula */
    unsigned int LinePattern; /* F=0,1,1,0,1)',2,23,3,4,3,1)))',9,Inh */
    float LineWeight; /* F=0.0033333333333333DT,0.01DT,0.03DT,0.12PT,0.24PT,0.254MM,0.5MM,0PT,IF(Sheet.5 Unit=MM,PT */
    float Rounding; /* F=0DL,4),0.25IN,0)',4,3*User.Margin,0))',Inh Unit=IN,MM */
};

struct vdx_LineTo
{
    GSList *children;
    char type;
    gboolean Del; /* =1 */
    unsigned int IX; /* */
    float X; /* F=(Width+Scratch.Y1)/2,-Scratch.A1,-Width*0.2,0,(Width*(9/10)),User.DividerY)', Unit=IN,IN_F,MM,PT */
    float Y; /* F=(Height-Scratch.X1)/2,-2*User.Margin,-User.Margin,0,User.DividerY,(Height*(9/ Unit=IN,MM,PT */
};

struct vdx_Master
{
    GSList *children;
    char type;
    unsigned int AlignName; /* =2 */
    gboolean AlignName_exists;
    char * BaseID; /* ={0022467D-0015-F00D-0000-000000000000},{002A9508-0000-F00D-0000-000000000000}, */
    gboolean Hidden; /* =0,1 */
    unsigned int ID; /* */
    unsigned int IconSize; /* =1,4 */
    gboolean IconSize_exists;
    gboolean IconUpdate; /* =0,1 */
    gboolean MatchByName; /* =0,1 */
    char * Name; /* =45 degree single,Additional tabs,Arbeitsfluß-  schleife 1,Arbeitsgang/ Prüfu */
    char * NameU; /* =  12 Pkt.   Text,  6 Pkt.   Text,2 Linien,45 degree double,45 degree single,Ad */
    unsigned int PatternFlags; /* =0,1026,2 */
    gboolean PatternFlags_exists;
    char * Prompt; /* =,A Windows 95 dialog box or form shape.,Any processing function.,Arbeitsflußs */
    char * UniqueID; /* ={0008892C-0000-0000-8E40-00608CF305B2},{0008892C-0002-0000-8E40-00608CF305B2}, */
};

struct vdx_Menu
{
    GSList *children;
    char type;
    char * Err; /* =#N/A */
};

struct vdx_Misc
{
    GSList *children;
    char type;
    unsigned int BegTrigger; /* F=Inh,No Formula,_XFTRIGGER(Class.11!EventXFMod),_XFTRIGGER(Cloud.2!EventXFMod) */
    gboolean Calendar; /* F=Inh =0 */
    char * Comment; /* F=Inh =,&#xe000;, */
    gboolean DropOnPageScale; /* F=Inh =1 */
    unsigned int DynFeedback; /* F=0,Inh =0,2 */
    unsigned int EndTrigger; /* F=Inh,No Formula,_XFTRIGGER(Class!EventXFMod),_XFTRIGGER(Cloud.4!EventXFMod),_X */
    unsigned int GlueType; /* F=0,Inh =0,2,3 */
    gboolean HideText; /* F=FALSE,GUARD((BITAND(Sheet.5!User.UMLSuppressOption,1))),GUARD((BITAND(Sheet.5 */
    gboolean IsDropSource; /* F=FALSE,Inh */
    unsigned int LangID; /* F=Inh =1033,1036,2057 */
    gboolean LocalizeMerge; /* F=Inh =0 */
    gboolean NoAlignBox; /* F=FALSE,GUARD(TRUE),Inh */
    gboolean NoCtlHandles; /* F=FALSE,GUARD(TRUE),Inh */
    gboolean NoLiveDynamics; /* F=FALSE,Inh */
    gboolean NoObjHandles; /* F=FALSE,GUARD(TRUE),Inh */
    gboolean NonPrinting; /* F=FALSE,GUARD(TRUE),Inh,TRUE */
    unsigned int ObjType; /* F=0,Inh =0,1,12,2,4,8,9 */
    char * ShapeKeywords; /* F=Inh =,Background,backdrop,scenery,watermark,behind,cosmic,Class,UML,state,str */
    gboolean UpdateAlignBox; /* F=FALSE,GUARD(FALSE),Inh */
    unsigned int WalkPreference; /* F=0,Inh =0,2,3 */
};

struct vdx_MoveTo
{
    GSList *children;
    char type;
    unsigned int IX; /* */
    float X; /* F=(Width-Scratch.Y1)/2,-Scratch.A1,-Sheet.9!User.OFFSET/10,-Sheet.9!User.OFFSET Unit=IN,IN_F,MM,PT */
    float Y; /* F=(Height+Scratch.X1)/2,-2*User.Margin,-Height*0.2,-Height*0.25,-Scratch.A1,-Us Unit=IN,MM,PT */
};

struct vdx_NURBSTo
{
    GSList *children;
    char type;
    float A; /* =1.222604887830752,13.370618992314,4.246853645432,5.7985449361335,8.58321900194 */
    float B; /* =1 */
    float C; /* =0 */
    float D; /* =1 */
    char * E; /* F=NURBS(12.641456682609,3,0,0,0.077695244759418,0.67702102041097,0,1,0.41485578 Unit=NURBS */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Width*1 */
    float Y; /* F=Geometry1.Y1,Height*0,Height*0.39287809665782,Height*1 */
};

struct vdx_NameUniv
{
    GSList *children;
    char type;
    char * Err; /* =#N/A */
};

struct vdx_Page
{
    GSList *children;
    char type;
    unsigned int BackPage; /* =10,4,5,6,8 */
    gboolean BackPage_exists;
    gboolean Background; /* =1 */
    unsigned int ID; /* */
    char * Name; /* =ARP Sequence,About This Template,Background-Main,Background-Screen 1,Backgroun */
    char * NameU; /* =ATM Sequence,About This Template,Background-Screen 1,Background-Screen 2,Blatt */
    float ViewCenterX; /* =0,0.64955357142857,1.0133333333333,1.7838541666667,12.316272965879,2.199803149 */
    float ViewCenterY; /* =0,0.093333333333333,0.62723214285714,10.03280839895,10.767716535433,11.5625,14 */
    float ViewScale; /* =-1,0.66145833333333,1,1.5408921933086,2,3.0817610062893,3.6877828054299,4.4186 */
};

struct vdx_PageLayout
{
    GSList *children;
    char type;
    float AvenueSizeX; /* F=0.29527559055118DL,0.375DL,Inh Unit=IN,MM */
    float AvenueSizeY; /* F=0.29527559055118DL,0.375DL,Inh Unit=IN,MM */
    float BlockSizeX; /* F=0.19685039370079DL,0.25DL,Inh Unit=IN,MM */
    float BlockSizeY; /* F=0.19685039370079DL,0.25DL,Inh Unit=IN,MM */
    gboolean CtrlAsInput; /* F=FALSE,Inh */
    gboolean DynamicsOff; /* F=FALSE,Inh */
    gboolean EnableGrid; /* F=FALSE,Inh */
    unsigned int LineAdjustFrom; /* F=0,Inh =0,2,3 */
    unsigned int LineAdjustTo; /* F=0,Inh =0,2,3 */
    gboolean LineJumpCode; /* F=1,Inh =1 */
    float LineJumpFactorX; /* F=0.66666666666667,Inh */
    float LineJumpFactorY; /* F=0.66666666666667,Inh */
    unsigned int LineJumpStyle; /* F=0,Inh */
    gboolean LineRouteExt; /* F=0,Inh,No Formula =0 */
    float LineToLineX; /* F=0.098425196850394DL,0.125DL,Inh Unit=MM */
    float LineToLineY; /* F=0.098425196850394DL,0.125DL,Inh Unit=MM */
    float LineToNodeX; /* F=0.098425196850394DL,0.125DL,Inh Unit=MM */
    float LineToNodeY; /* F=0.098425196850394DL,0.125DL,Inh Unit=MM */
    gboolean PageLineJumpDirX; /* F=0,Inh =0 */
    gboolean PageLineJumpDirY; /* F=0,Inh =0 */
    gboolean PageShapeSplit; /* F=Inh =0,1 */
    unsigned int PlaceDepth; /* F=0,Inh =0,1,2,3 */
    gboolean PlaceFlip; /* F=0,Inh,No Formula =0 */
    unsigned int PlaceStyle; /* F=0,Inh */
    gboolean PlowCode; /* F=0,Inh =0,1 */
    gboolean ResizePage; /* F=FALSE,Inh */
    unsigned int RouteStyle; /* F=0,Inh */
};

struct vdx_PageProps
{
    GSList *children;
    char type;
    float DrawingScale; /* F=No Formula Unit=IN,IN_F,MM */
    unsigned int DrawingScaleType; /* F=No Formula =0,3 */
    unsigned int DrawingSizeType; /* F=No Formula =0,1,2,3,4,5,6 */
    gboolean InhibitSnap; /* F=No Formula =0 */
    float PageHeight; /* F=No Formula Unit=CM,DL,IN,IN_F,MM,NUM */
    float PageScale; /* F=No Formula Unit=CM,IN,IN_F,MM */
    float PageWidth; /* F=No Formula Unit=CM,DL,IN,IN_F,MM,NUM */
    float ShdwObliqueAngle; /* F=No Formula Unit=NUM */
    float ShdwOffsetX; /* F=GUARD(3MM),No Formula Unit=IN,MM,NUM */
    float ShdwOffsetY; /* F=-0.11811023622047DP,-0.125DP,-0.125IN,GUARD(-3MM),No Formula Unit=IN,MM,NUM */
    float ShdwScaleFactor; /* F=No Formula */
    gboolean ShdwType; /* F=No Formula =0,1 */
    gboolean UIVisibility; /* F=No Formula =0 */
};

struct vdx_PageSheet
{
    GSList *children;
    char type;
    unsigned int FillStyle; /* */
    gboolean FillStyle_exists;
    unsigned int LineStyle; /* */
    gboolean LineStyle_exists;
    unsigned int TextStyle; /* */
    gboolean TextStyle_exists;
    char * UniqueID; /* ={A33A67A2-2DDF-4ABB-8EC7-E42A283FE010},{C81BB4E0-8FB8-4C04-B3A7-6B68EC06AFEF} */
};

struct vdx_Para
{
    GSList *children;
    char type;
    unsigned int Bullet; /* F=0,Inh =0,1,2 */
    unsigned int BulletFont; /* F=Inh */
    int BulletFontSize; /* F=Inh =-1 */
    char * BulletStr; /* F=Inh =,&#xe000;, */
    gboolean Flags; /* F=Inh =0 */
    float HorzAlign; /* F=0,1,EndX,0,2)",FlipX*2,If(User.ShapeType&gt;5,0,1),Inh */
    unsigned int IX; /* */
    float IndFirst; /* F=0DP,Inh Unit=MM */
    float IndLeft; /* F=0DP,Inh Unit=IN,MM */
    float IndRight; /* F=0DP,Inh */
    unsigned int LocalizeBulletFont; /* F=Inh */
    float SpAfter; /* F=0DT,Inh Unit=PT */
    float SpBefore; /* F=0DT,Inh Unit=PT */
    float SpLine; /* F=-120%,Inh Unit=DT */
    gboolean TextPosAfterBullet; /* F=Inh =0 */
};

struct vdx_PreviewPicture
{
    GSList *children;
    char type;
    unsigned int Size; /* =1056,1064,1088,1124,1168,1192,1352,1372,1980,20740,20760,20936,21032,21048,210 */
    gboolean Size_exists;
};

struct vdx_PrintProps
{
    GSList *children;
    char type;
    gboolean CenterX; /* F=Inh =0 */
    gboolean CenterY; /* F=Inh =0 */
    gboolean OnPage; /* F=Inh =0 */
    float PageBottomMargin; /* F=Inh =0.25 */
    float PageLeftMargin; /* F=Inh =0.25 */
    float PageRightMargin; /* F=Inh =0.25 */
    float PageTopMargin; /* F=Inh =0.25 */
    gboolean PagesX; /* F=Inh =1 */
    gboolean PagesY; /* F=Inh =1 */
    unsigned int PaperKind; /* F=Inh =0,1,19,3,9 */
    unsigned int PaperSource; /* F=Inh =7 */
    gboolean PrintGrid; /* F=Inh =0 */
    unsigned int PrintPageOrientation; /* F=Inh =0,1,2 */
    gboolean ScaleX; /* F=Inh =1 */
    gboolean ScaleY; /* F=Inh =1 */
};

struct vdx_PrintSetup
{
    GSList *children;
    char type;
    float PageBottomMargin; /* =0.25,0.55,106.63 */
    float PageLeftMargin; /* =0.25,138.38 */
    float PageRightMargin; /* =0.25,138.38 */
    float PageTopMargin; /* =0.25,106.63 */
    unsigned int PaperSize; /* =1,262,8,9 */
    gboolean PrintCenteredH; /* =0 */
    gboolean PrintCenteredV; /* =0 */
    gboolean PrintFitOnPages; /* =0,1 */
    gboolean PrintLandscape; /* =0,1 */
    gboolean PrintPagesAcross; /* =1 */
    gboolean PrintPagesDown; /* =1 */
    gboolean PrintScale; /* =1 */
};

struct vdx_Prop
{
    GSList *children;
    char type;
    gboolean Calendar; /* F=No Formula =0 */
    char * Format; /* F=Guard(ThePage!Prop.Theme.Format),Inh,No Formula,ThePage!Prop.Theme.Format =,& */
    unsigned int ID; /* */
    gboolean Invisible; /* F=Inh,No Formula =0,1 */
    char * Label; /* F=Inh,No Formula =,&#xe000;,&amp;Child Layout Style,&amp;Name,&amp;Preferred Pr */
    unsigned int LangID; /* =1033,1036 */
    char * NameU; /* =ALMode,AdminInterface,AssetNumber,Building,ChildLayoutStyle,Comments,Community */
    char * Prompt; /* F=Inh,No Formula =,&#xe000;,Abteilung,Administrative Schnittstelle,Allow custom */
    char * SortKey; /* F=1,2,3,4,5,6,7,8,No Formula =,&#xe000;,1,2,3,4,5,6,7,8,Aktivposten,Geräte,Net */
    unsigned int Type; /* F="5","7",Inh,No Formula =0,1,2,3,4,5,7 */
    float Value; /* F=-1,6,206,-1)',Guard(ThePage!Prop.Theme),Inh,No Formula,ThePage!Prop.ShowDivid Unit=BOOL,IN,STR */
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
    gboolean LockDelete; /* F=0,GUARD(1),Inh,NOT(ISERROR(Sheet.5!Width)),Not(IsError(Sheet.22!Width)),Not(I */
    gboolean LockEnd; /* F=0,Inh =0 */
    gboolean LockFormat; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockGroup; /* F=0,Inh =0,1 */
    gboolean LockHeight; /* F=0,Inh =0,1 */
    gboolean LockMoveX; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockMoveY; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockRotate; /* F=0,GUARD(0),GUARD(1),Guard(0),Inh =0,1 */
    gboolean LockSelect; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockTextEdit; /* F=0,GUARD(1),Inh,Sheet.5!LockTextEdit,Sheet.5!User.UMLAutoLockTextEdit =0,1 */
    gboolean LockVtxEdit; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockWidth; /* F=0,Inh =0,1 */
};

struct vdx_RulerGrid
{
    GSList *children;
    char type;
    unsigned int XGridDensity; /* F=8,Inh =0,4,8 */
    float XGridOrigin; /* F=0DL,Inh */
    float XGridSpacing; /* F=0DL,Inh Unit=CM,IN,MM */
    unsigned int XRulerDensity; /* F=32,Inh =16,32 */
    float XRulerOrigin; /* F=0DL,Inh */
    unsigned int YGridDensity; /* F=8,Inh =0,4,8 */
    float YGridOrigin; /* F=0DL,Inh */
    float YGridSpacing; /* F=0DL,Inh Unit=CM,IN,MM */
    unsigned int YRulerDensity; /* F=32,Inh =16,32 */
    float YRulerOrigin; /* F=0DL,Inh */
};

struct vdx_Scratch
{
    GSList *children;
    char type;
    float A; /* F=0,0,1)+Int(Sheet.21!Width/Scratch.X1)',0,0,1)+Int(Sheet.38!Width/Scratch.X1)' Unit=DA,IN,MM */
    float B; /* F=0,0,2)+Int(Sheet.21!Height/(Scratch.X1))',0,0,2)+Int(Sheet.38!Height/(Scratch Unit=DEG,IN */
    float C; /* F=270DEG),1,0)',Inh,No Formula,User.Margin*4,_UCON_GEOTYP(Scratch.A1,Scratch.B1 Unit=DL,IN */
    float D; /* F=If(And(FlipX,FlipY),Not(Scratch.C1),Scratch.C1),Inh,No Formula,_UCON_SIMPLE(S Unit=BOOL */
    unsigned int IX; /* */
    float X; /* F=((4/9)*(Controls.X1-((8/27)*Geometry1.X1)-((1/27)*Geometry1.X4)))-((2/9)*(Con Unit=IN,MM,PT */
    float Y; /* F=((4/9)*(Controls.Y1-((8/27)*Geometry1.Y1)-((1/27)*Geometry1.Y4)))-((2/9)*(Con Unit=IN,MM */
};

struct vdx_Shape
{
    GSList *children;
    char type;
    gboolean Del; /* =1 */
    unsigned int FillStyle; /* */
    gboolean FillStyle_exists;
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    gboolean LineStyle_exists;
    unsigned int Master; /* =0,11,12,13,18,19,2,22,23,24,25,26,27,3,33,36,4,5,6,7,8,9 */
    gboolean Master_exists;
    unsigned int MasterShape; /* =10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35, */
    gboolean MasterShape_exists;
    char * Name; /* =Attributes,Class,Class.11,Doppelpfeil 45°-Spitze,Doppelpfeil 45°-Spitze.2,Dr */
    char * NameU; /* =  6 Pkt.   Text.101,  6 Pkt.   Text.105,  6 Pkt.   Text.112,  6 Pkt.   Text.12 */
    unsigned int TextStyle; /* */
    gboolean TextStyle_exists;
    char * Type; /* =Foreign,Group,Guide,Shape */
    char * UniqueID; /* ={001B0C9A-2915-427E-96BF-4779DE5A4600},{0038D543-67DA-45C2-8EB5-5F966B144DD1}, */
};

struct vdx_Shapes
{
    GSList *children;
    char type;
};

struct vdx_SplineKnot
{
    GSList *children;
    char type;
    float A; /* =0,0.019830308804445,0.049324479568668,0.065470075006848,0.075529218639412,0.09 */
    unsigned int IX; /* */
    float X; /* F=Scratch.X2/(12/81),Width*-0.40491401815925,Width*0.17504738278937,Width*0.277 */
    float Y; /* F=Height*-0.1158194583957,Height*-0.27754865922726,Height*0.087705343078715,Hei */
};

struct vdx_SplineStart
{
    GSList *children;
    char type;
    float A; /* =0 */
    float B; /* =0 */
    float C; /* =0.13484716066176,3.1175 */
    float D; /* =3 */
    unsigned int IX; /* */
    float X; /* F=Scratch.X1/(12/81),Width*-0.11 */
    float Y; /* F=Height*0.54325161324641,Scratch.Y1/(12/81) */
};

struct vdx_StyleProp
{
    GSList *children;
    char type;
    gboolean EnableFillProps; /* F=0,1 =0,1 */
    gboolean EnableLineProps; /* F=0,1 =0,1 */
    gboolean EnableTextProps; /* F=0,1 =0,1 */
    gboolean HideForApply; /* F=0,1,FALSE,Inh */
};

struct vdx_StyleSheet
{
    GSList *children;
    char type;
    unsigned int FillStyle; /* */
    gboolean FillStyle_exists;
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    gboolean LineStyle_exists;
    char * Name; /* =1pxl line,30% Gray fill,50% Gray fill,BDMain,Background Face,Background Gradua */
    char * NameU; /* =1-Pixel-Linie,10 % Grau,1pxl line,3-Pixel-Linie,30 % Grau,30% Gray fill,3D Net */
    unsigned int TextStyle; /* */
    gboolean TextStyle_exists;
};

struct vdx_Tab
{
    GSList *children;
    char type;
    gboolean Alignment; /* =0 */
    unsigned int IX; /* */
    float Position; /* Unit=DP,MM */
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
    unsigned int IX; /* */
    unsigned int cp; /* = */
    float fld; /* =0.006,0.009,0.011,0.031,0.042,0.046,0.048,0.061,0.063,0.071,0.081,0.084,0.086, */
    unsigned int pp; /* = */
    unsigned int tp; /* = */
};

struct vdx_TextBlock
{
    GSList *children;
    char type;
    float BottomMargin; /* F=0DP,0DT,0PT,1PT,2PT,4PT,Char.Size*0.5,Inh Unit=PT */
    float DefaultTabStop; /* F=0.59055118110236DP,0.5DP,Inh =0.5,0.59055118110236,0.5905511811023622 */
    float LeftMargin; /* F=0DP,0PT,1PT,2PT,4PT,Inh Unit=PT */
    float RightMargin; /* F=0DP,0PT,1PT,2PT,4PT,Inh Unit=PT */
    unsigned int TextBkgnd; /* F=0,1+FillForegnd,15,2,Inh */
    float TextBkgndTrans; /* F=0%,Inh,No Formula */
    gboolean TextDirection; /* F=0,Inh =0 */
    float TopMargin; /* F=0DP,0DT,0PT,1PT,2PT,4PT,GUARD(IF(Parameters!Geometry1.NoShow,1PT,7.2PT)),Inh Unit=PT */
    unsigned int VerticalAlign; /* F=0,1,2,Inh =0,1,2 */
};

struct vdx_TextXForm
{
    GSList *children;
    char type;
    float TxtAngle; /* F=-Angle,90DEG),0DEG,180DEG)",90DEG),0DEG,180DEG)',GUARD(-90DEG),GUARD(0DEG),Gr Unit=DEG */
    float TxtHeight; /* F=GUARD(0.25IN),GUARD(TEXTHEIGHT(TheText,TxtWidth)),GUARD(TEXTWIDTH(TheText,10) Unit=CM,IN,IN_F,MM */
    float TxtLocPinX; /* F=0.5*TxtWidth,GUARD(TxtWidth),GUARD(TxtWidth*0.5),Guard(TxtWidth*0.5),Inh,TxtW Unit=CM,IN,IN_F,MM,PT */
    float TxtLocPinY; /* F=-90DEG,1,If(Abs(Angle)&gt;90DEG,1,0))",0.5*TxtHeight,GUARD(0.5*TxtHeight),GUA Unit=CM,IN,IN_F,MM */
    float TxtPinX; /* F=(Controls.X2+Controls.X1)/2,0,0,Width/2)",0.5*Width,1,1/2,1)*Width",1,Control Unit=IN,IN_F,MM */
    float TxtPinY; /* F=(Controls.Y2+Controls.Y1)/2,0,1/2,0)*Height",0,Height/2,Height)",0.5*Height,0 Unit=IN,IN_F,MM,PT */
    float TxtWidth; /* F=GUARD(Height),GUARD(TEXTWIDTH(TheText)),GUARD(Width*1),Guard(Height),Guard(Wi Unit=CM,IN,IN_F,MM,PT */
};

struct vdx_User
{
    GSList *children;
    char type;
    unsigned int ID; /* */
    char * NameU; /* =AL_JustOffset,AL_Justification,AL_PropJustOffset,AL_cxBtwnAssts,AL_cxBtwnSubs, */
    char * Prompt; /* F=Inh,No Formula =,&#xe000;,1=left, 2=center, 3=right,(-1)=none,Calculated text */
    float Value; /* F=(0.125IN*User.AntiScale)/2,(PinX-LocPinX+Connections.X1)-BeginX,(PinY-LocPinY Unit=BOOL,DA,DL,GUID,IN,MM,PT,STR */
};

struct vdx_VisioDocument
{
    GSList *children;
    char type;
    unsigned int DocLangID; /* =1033,2057 */
    gboolean DocLangID_exists;
    unsigned int EventList; /* = */
    unsigned int Masters; /* = */
    unsigned int buildnum; /* =3216,5509 */
    gboolean buildnum_exists;
    char * key; /* =049D60A1B3BC9F05FEF68173361940DBB593CBE9BB6EF070E3E7DEEA1577EB422D8AAC442F09A3 */
    gboolean metric; /* =0,1 */
    unsigned int start; /* =190 */
    gboolean start_exists;
    char * version; /* =preserve */
    char * xmlns; /* =http://schemas.microsoft.com/visio/2003/core,urn:schemas-microsoft-com:office: */
};

struct vdx_Window
{
    GSList *children;
    char type;
    char * ContainerType; /* =Page */
    char * Document; /* =C:\Documents and Settings\Gene.GENE-LAPTOP\My Documents\My Shapes\navigationic */
    gboolean DynamicGridEnabled; /* =0,1 */
    unsigned int GlueSettings; /* =47,9 */
    unsigned int ID; /* */
    unsigned int Page; /* =0,2,5,7,8 */
    gboolean Page_exists;
    gboolean ParentWindow; /* =0 */
    unsigned int Sheet; /* =1118 */
    gboolean Sheet_exists;
    gboolean ShowConnectionPoints; /* =0,1 */
    gboolean ShowGrid; /* =0,1 */
    gboolean ShowGuides; /* =1 */
    gboolean ShowPageBreaks; /* =0 */
    gboolean ShowRulers; /* =1 */
    unsigned int SnapExtensions; /* =1,34 */
    unsigned int SnapSettings; /* =295,319,33063,39,65831,65847 */
    unsigned int StencilGroup; /* =10 */
    unsigned int StencilGroupPos; /* =0,1,2,3,4,5,6,7 */
    float TabSplitterPos; /* =0.326,0.33,0.432,0.5,0.807,0.892 */
    float ViewCenterX; /* =0.71194225721785,1.0133333333333,1.7838541666667,12.316272965879,2.19980314960 */
    float ViewCenterY; /* =0.093333333333333,1.7060367454068,10.03280839895,10.767716535433,11.5625,14.85 */
    float ViewScale; /* =-1,0.66145833333333,1,1.5408921933086,1.984375,2,3.0817610062893,3.68778280542 */
    unsigned int WindowHeight; /* =411,414,554,565,603,605,610,621,635,641,643,646,652,675,799,818,828,846,847,85 */
    gboolean WindowHeight_exists;
    int WindowLeft; /* =-134,-185,-195,-196,-238,-240,-247,-248,-300,-4,-5,0,22,3 */
    gboolean WindowLeft_exists;
    unsigned int WindowState; /* =1,1025,1073741824,268435456,67109889 */
    gboolean WindowState_exists;
    int WindowTop; /* =-1,-23,-24,-30,-34,-36,0,1,22,3 */
    gboolean WindowTop_exists;
    char * WindowType; /* =Drawing,Stencil */
    unsigned int WindowWidth; /* =1017,1018,1020,1022,1032,1095,1119,1133,1192,1246,128,1288,1688,179,187,188,23 */
    gboolean WindowWidth_exists;
};

struct vdx_Windows
{
    GSList *children;
    char type;
    unsigned int ClientHeight; /* =605,607,609,612,621,625,641,791,851,864,868,871,875,882,890 */
    gboolean ClientHeight_exists;
    unsigned int ClientWidth; /* =1020,1022,1024,1095,1119,1125,1184,1236,1280,1680,818,828 */
    gboolean ClientWidth_exists;
    unsigned int Window; /* = */
};

struct vdx_XForm
{
    GSList *children;
    char type;
    float Angle; /* F=ATAN2(EndY-BeginY,EndX-BeginX),ATan2(EndY-BeginY,EndX-BeginX),GUARD(0),GUARD( Unit=DEG */
    gboolean FlipX; /* F=GUARD(0),GUARD(FALSE),Guard(0),Guard(EndX&lt;BeginX),Guard(FALSE),Inh =0,1 */
    gboolean FlipY; /* F=BeginY&lt;EndY)",BeginY&lt;EndY)',GUARD(0),GUARD(EndY&lt;BeginY),GUARD(FALSE) */
    float Height; /* F=0.2IN-User.shade*2/3,1/2IN,5MM*User.AntiScale,Abs(EndY-BeginY),GUARD(0.1IN),G Unit=IN,IN_F,MM,PT */
    float LocPinX; /* F=0.5*Width,GUARD(0),GUARD(0.5IN),GUARD(Width*0),GUARD(Width*0.25),GUARD(Width* Unit=IN,IN_F,MM,PT */
    float LocPinY; /* F=0.5*Height,BeginX,Height,0)",EndX)*Height",GUARD(0),GUARD(Height*0),GUARD(Hei Unit=IN,IN_F,MM,PT */
    float PinX; /* F=(BeginX+EndX)/2,AlignCenter+-1.2100542789995E-10DL,AlignCenter+1.210054278999 Unit=IN,IN_F,MM */
    float PinY; /* F=(BeginY+EndY)/2,AlignBottom+0.59055118063851DL,AlignBottom+0.59055118069901DL Unit=IN,IN_F,MM */
    gboolean ResizeMode; /* F=GUARD(0),Inh,No Formula =0,1 */
    float Width; /* F=3,2*User.Margin,0))',Abs(EndX-BeginX),GUARD(0.1563IN),GUARD(0.19685039370079D Unit=IN,IN_F,MM,PT */
};

struct vdx_XForm1D
{
    GSList *children;
    char type;
    float BeginX; /* F=GUARD(Sheet.10!PinX),GUARD(Sheet.5!LocPinX),Guard(Sheet.6!Width+Sheet.9!Width Unit=IN,IN_F,MM */
    float BeginY; /* F=GUARD(Sheet.13!PinY+Sheet.13!Height),GUARD(Sheet.5!LocPinY),GUARD(Sheet.8!Pin Unit=IN,IN_F,MM */
    float EndX; /* F=GUARD(Sheet.13!PinX),GUARD(Sheet.5!Controls.Row_1),GUARD(Sheet.5!Controls.Row Unit=IN,IN_F,MM */
    float EndY; /* F=GUARD(BeginY),GUARD(Sheet.13!PinY+Sheet.13!Height),GUARD(Sheet.5!Controls.Row Unit=IN,IN_F,MM */
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

struct vdx_pp
{
    GSList *children;
    char type;
    unsigned int IX; /* */
};

struct vdx_tp
{
    GSList *children;
    char type;
    unsigned int IX; /* */
};

struct vdx_text
{
    GSList *children;
    char type;
    char * text; /* */
};

enum {
    vdx_types_any = 0,
    vdx_types_Act,
    vdx_types_Align,
    vdx_types_ArcTo,
    vdx_types_BegTrigger,
    vdx_types_BeginX,
    vdx_types_BeginY,
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
    vdx_types_EndX,
    vdx_types_EndY,
    vdx_types_Event,
    vdx_types_EventDblClick,
    vdx_types_EventItem,
    vdx_types_EventList,
    vdx_types_FaceName,
    vdx_types_FaceNames,
    vdx_types_Field,
    vdx_types_Fill,
    vdx_types_FontEntry,
    vdx_types_Fonts,
    vdx_types_Foreign,
    vdx_types_ForeignData,
    vdx_types_Geom,
    vdx_types_Group,
    vdx_types_HeaderFooter,
    vdx_types_HeaderFooterFont,
    vdx_types_Help,
    vdx_types_Hyperlink,
    vdx_types_Icon,
    vdx_types_Image,
    vdx_types_InfiniteLine,
    vdx_types_Layer,
    vdx_types_LayerMem,
    vdx_types_Layout,
    vdx_types_Line,
    vdx_types_LineTo,
    vdx_types_Master,
    vdx_types_Menu,
    vdx_types_Misc,
    vdx_types_MoveTo,
    vdx_types_NURBSTo,
    vdx_types_NameUniv,
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
    vdx_types_Scratch,
    vdx_types_Shape,
    vdx_types_Shapes,
    vdx_types_SplineKnot,
    vdx_types_SplineStart,
    vdx_types_StyleProp,
    vdx_types_StyleSheet,
    vdx_types_Tab,
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
    vdx_types_fld,
    vdx_types_pp,
    vdx_types_tp,
    vdx_types_text
};

extern char * vdx_Units[];
extern char * vdx_Types[];
