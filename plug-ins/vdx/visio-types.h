/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 *
 * visio-types.h: Visio XML import filter for dia
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


struct vdx_any
{
    GSList *children;
    char type;
};

struct vdx_Act
{
    struct vdx_any any;
    float Action; /* F=0,0,1,0))",1,SETF(&quot;LockDelete&quot;,&quot;Guard(0)&quot;),SETF(&quot;Loc Unit=BOOL */
    gboolean BeginGroup; /* F=No Formula =0 */
    unsigned int ButtonFace; /* F=No Formula = */
    gboolean Checked; /* F=0",0,1,0)",1",1,1,0)",2",2,1,0)",3",4",Inh,LockDelete,LockTextEdit,No Formula */
    gboolean Disabled; /* F=Inh,NOT(User.UMLError),No Formula,Not(User.HasSubordinates) =0,1 */
    unsigned int ID; /* */
    unsigned int IX; /* */
    gboolean Invisible; /* F=No Formula =0 */
    char * Menu; /* F=0),"","_Set As Straight Line")',0),&quot;&quot;,&quot;_ Gerade Linie&quot;)", */
/* ? Act.Menu.Err */
    char * Name; /* =LocalRow0 */
    char * NameU; /* =LocalRow0,Properties,Row_1 */
    gboolean ReadOnly; /* F=No Formula =0 */
    unsigned int SortKey; /* F=No Formula = */
    unsigned int TagName; /* F=No Formula = */
};

struct vdx_Align
{
    struct vdx_any any;
    float AlignBottom; /* F=IntersectY(Sheet.1972!PinX,Sheet.1972!PinY,Sheet.1972!Angle,10.185039370079DL */
    float AlignCenter; /* F=IntersectX(Sheet.1972!PinX,Sheet.1972!PinY,Sheet.1972!Angle,10.185039370079DL */
    gboolean AlignLeft; /* F=_Marker(1) =1 */
    gboolean AlignMiddle; /* F=_Marker(1) =1 */
    gboolean AlignRight; /* F=_Marker(1) =1 */
    float AlignTop; /* F=IntersectY(Sheet.1973!PinX,Sheet.1973!PinY,Sheet.1973!Angle,14.464566929134DL */
};

struct vdx_ArcTo
{
    struct vdx_any any;
    float A; /* F=(Scratch.X4+Scratch.B4)*Scratch.C4,-(Scratch.X1-Scratch.Y1)*0.2929,-(Scratch. Unit=DL,IN,MM,PT */
    gboolean Del; /* =1 */
    unsigned int IX; /* */
    float X; /* F=-Controls.Row_2.Y,-Scratch.X1,-User.Margin,0.4*Scratch.X1,0.6*Scratch.X1,Cont Unit=IN,MM,PT */
    float Y; /* F=-Controls.Row_2.Y,-User.Margin*(3/2),Controls.Row_2.Y,Geometry1.Y1,Geometry1. Unit=CM,IN,MM,PT */
};

struct vdx_Char
{
    struct vdx_any any;
    unsigned int AsianFont; /* F=Inh */
    gboolean Case; /* F=0,Inh =0 */
    Color Color; /* F=0,0,0,19))",0,1,16))",1,14,15,2,4,GUARD(IF(Sheet.5!User.active,0,19)),HSL(0,0 */
    float ColorTrans; /* F=0%,Inh,No Formula */
    unsigned int ComplexScriptFont; /* F=Inh */
    float ComplexScriptSize; /* F=Inh Unit=DT */
    gboolean DblUnderline; /* F=FALSE,Inh,No Formula */
    gboolean Del; /* =1 */
    gboolean DoubleStrikethrough; /* F=Inh =0 */
    unsigned int Font; /* F=0,1,2,Inh */
    float FontScale; /* F=100%,Inh */
    gboolean Highlight; /* F=Inh,No Formula =0 */
    unsigned int IX; /* */
    unsigned int LangID; /* F=Inh =0,1031,1033,1036,1053,2057,3081,3082,3084,4105 */
    float Letterspace; /* F=0DT,Inh,No Formula */
    unsigned int Locale; /* F=0,Inh =0,57 */
    unsigned int LocalizeFont; /* F=Inh */
    gboolean Overline; /* F=FALSE,Inh,No Formula */
    gboolean Perpendicular; /* F=FALSE,Inh,No Formula */
    unsigned int Pos; /* F=0,Inh =0,1,2 */
    gboolean RTLText; /* F=Inh =0 */
    float Size; /* F=(ThePage!PageScale/ThePage!DrawingScale)*Height*0.17,(ThePage!PageScale/ThePa Unit=PT */
    gboolean Strikethru; /* F=FALSE,Inh,No Formula */
    unsigned int Style; /* F=0,3,2,0)))',Inh */
    gboolean UseVertical; /* F=Inh =0 */
};

struct vdx_ColorEntry
{
    struct vdx_any any;
    unsigned int IX; /* */
    char * RGB; /* =#000000,#000080,#0000FF,#007D7B,#008000,#008080,#00FF00,#00FFFF,#1A1A1A,#33333 */
};

struct vdx_Colors
{
    struct vdx_any any;
    unsigned int ColorEntry; /* = */
};

struct vdx_Connect
{
    struct vdx_any any;
    char * FromCell; /* =AlignBottom,AlignTop,BeginX,BeginY,Controls.Row_1,Controls.Row_10,Controls.Row */
    unsigned int FromPart; /* =10,100,101,102,103,104,105,106,107,108,109,11,12,4,6,7,8,9 */
    gboolean FromPart_exists;
    unsigned int FromSheet; /* =10,100,101,102,103,104,105,106,107,108,109,11,110,111,112,113,114,115,116,117, */
    gboolean FromSheet_exists;
    char * ToCell; /* =Angle,Connections.Bottom.X,Connections.Float.X,Connections.Left.X,Connections. */
    unsigned int ToPart; /* =100,101,102,103,104,105,106,107,108,109,111,112,113,114,116,118,123,124,127,12 */
    gboolean ToPart_exists;
    unsigned int ToSheet; /* =1,10,101,102,104,105,107,11,110,111,112,114,115,1152,1153,1155,1157,116,119,12 */
    gboolean ToSheet_exists;
};

struct vdx_Connection
{
    struct vdx_any any;
    gboolean AutoGen; /* F=Inh,No Formula =0,1 */
    float DirX; /* F=-1IN,-25.4MM,Inh,No Formula Unit=IN,MM,NUM */
    float DirY; /* F=-1IN,-25.4MM,Inh,No Formula Unit=IN,MM,NUM */
    unsigned int ID; /* */
    unsigned int IX; /* */
    char * NameU; /* =Bottom,Float,Floating,LAN,Left,Port0,Port1,Port10,Port11,Port12,Port13,Port14, */
    char * Prompt; /* F=No Formula =,&#xe000; */
    gboolean Type; /* F=Inh,No Formula =0 */
    float X; /* F=0.5*Width,2,Width*0.18,Width*0.2003))",4,Width*0.0822,Width*0.1929))",4,Width Unit=IN,MM */
    float Y; /* F=(Geometry2.B2+Geometry1.B5)/2,0.5*Height,2,Height*0.05,Height*0.1233))))",3,H Unit=CM,IN,MM */
};

struct vdx_Connects
{
    struct vdx_any any;
    unsigned int Connect; /* = */
};

struct vdx_Control
{
    struct vdx_any any;
    gboolean CanGlue; /* F=Inh =0,1 */
    unsigned int ID; /* */
    unsigned int IX; /* */
    char * NameU; /* =TextPosition */
    char * Prompt; /* F=&quot;Flytta böj &quot;&amp;-User.1stLegDir+3,&quot;Flytta böj &quot;&amp;- */
    float X; /* F=(Controls.Row_3+IF(User.LegIndex1&lt;7,Width,Controls.Row_5))/2,(Controls.Row Unit=IN,MM */
    float XCon; /* F=(Controls.Row_1&gt;Height/2)*2+2+5*NOT(Actions.Row_2.Action),(Controls.Row_1& */
    float XDyn; /* F=Controls.Row_1,Controls.Row_1.XDyn,Controls.Row_10,Controls.Row_11,Controls.R Unit=IN,M,MM */
    float Y; /* F=(Controls.Row_2.Y+IF(User.LegIndex1&lt;6,Height,Controls.Row_4.Y))/2,(Control Unit=IN,MM,PT */
    float YCon; /* F=(Controls.Row_1.Y&gt;Height/2)*2+2,(Controls.Row_1.Y&gt;Height/2)*2+2+5*NOT(U */
    float YDyn; /* F=Controls.Row_1.Y,Controls.Row_1.YDyn,Controls.Row_2.Y,Controls.Row_3.Y,Contro Unit=CM,IN,M,MM,PT */
};

struct vdx_CustomProp
{
    struct vdx_any any;
    char * Name; /* =_TemplateID */
    char * PropType; /* =String */
};

struct vdx_CustomProps
{
    struct vdx_any any;
    char * CustomProp; /* =TC010483951033,TC010492811033,TC010498121033,TC010498511033 */
};

struct vdx_DocProps
{
    struct vdx_any any;
    gboolean AddMarkup; /* F=No Formula =0 */
    unsigned int DocLangID; /* =1033,1053,2057 */
    gboolean LockPreview; /* F=No Formula =0,1 */
    unsigned int OutputFormat; /* F=No Formula =0,2 */
    gboolean PreviewQuality; /* F=No Formula =0 */
    unsigned int PreviewScope; /* F=No Formula =0,1,2 */
    gboolean ViewMarkup; /* F=No Formula =0 */
};

struct vdx_DocumentProperties
{
    struct vdx_any any;
    char * AlternateNames; /* =?,rt\export,Ց粑ࠈ?խ粑᫤ඔ㷤? */
    int BuildNumberCreated; /* =-1,1245,2072,671351279,671351309,671353298,671355894,738200627,738200720,73820 */
    unsigned int BuildNumberEdited; /* =671351309,671352994,671353097,671353298,671353406,738200720,738203013 */
    char * Company; /* =*,ARS-Solutions,Agilent Technologies, Inc.,Artis Group,C2SAT,Celox,GarrettCom  */
    char * Creator; /* = Pierre Robitaille,Administrator,Christian Ridderström,Copyright (c) 2001 Mic */
    char * Desc; /* =$Id$,A zoom of figure  */
    char * Subject; /* =Sample file for Visio Viewer,Sample file for Visio Web Component */
    char * Template; /* =C:\Archivos de programa\Microsoft Office\Visio11\1033\UMLMOD_M.VST,C:\Document */
    char * TimeCreated; /* =2001-03-14T11:57:54,2002-01-24T12:57:03,2002-02-28T14:18:35,2002-03-25T13:54:0 */
    char * TimeEdited; /* =1899-12-30T00:00:00,2001-10-23T14:49:27,2002-02-28T14:20:31,2002-05-15T07:40:2 */
    char * TimePrinted; /* =2001-03-14T11:57:54,2002-02-28T14:18:35,2002-03-25T13:54:03,2002-04-17T14:27:5 */
    char * TimeSaved; /* =2001-10-23T14:49:30,2002-02-28T14:20:34,2002-05-15T07:40:45,2002-06-18T14:58:4 */
    char * Title; /* =Axis1 Gyro&amp;Servo unit,Basic tests,Circle with angles for testing TCM,Curve */
};

struct vdx_DocumentSettings
{
    struct vdx_any any;
    unsigned int DefaultFillStyle; /* */
    gboolean DefaultFillStyle_exists;
    unsigned int DefaultGuideStyle; /* */
    gboolean DefaultGuideStyle_exists;
    unsigned int DefaultLineStyle; /* */
    gboolean DefaultLineStyle_exists;
    unsigned int DefaultTextStyle; /* */
    gboolean DefaultTextStyle_exists;
    gboolean DynamicGridEnabled; /* =0,1 */
    unsigned int GlueSettings; /* =11,47,9 */
    gboolean ProtectBkgnds; /* =0 */
    gboolean ProtectMasters; /* =0 */
    gboolean ProtectShapes; /* =0 */
    gboolean ProtectStyles; /* =0 */
    unsigned int SnapExtensions; /* =1,34 */
    unsigned int SnapSettings; /* =295,319,32807,33063,39,65831,65847 */
    unsigned int TopPage; /* =0,2,4,5,7,8 */
    gboolean TopPage_exists;
};

struct vdx_DocumentSheet
{
    struct vdx_any any;
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
    struct vdx_any any;
    float A; /* F=Inh,Width*0,Width*0.021265284423179,Width*0.021346344438398,Width*0.026441036 Unit=DL,IN,MM */
    float B; /* F=Height*0,Height*0.050632911392406,Height*0.061728395014259,Height*0.072100313 Unit=DL,IN,MM */
    float C; /* F=Geometry1.X1,Inh,Width*0.01063264221159,Width*0.010673172221386,Width*0.01322 Unit=DL,IN,MM */
    float D; /* F=Height*0,Height*0.057129798903111,Height*0.094167364045729,Height*0.101265822 Unit=DL,IN,MM */
    unsigned int IX; /* */
    float X; /* F=Inh,Width*0.01063264221159,Width*0.010673172219199,Width*0.013220518239041,Wi Unit=IN,MM */
    float Y; /* F=Height*0.050632911392406,Height*0.057129798903111,Height*0.061728395039554,He Unit=IN,MM */
};

struct vdx_EllipticalArcTo
{
    struct vdx_any any;
    float A; /* F=Controls.Row_1,Controls.X1,Geometry1.A2,Geometry3.A2,Inh,Width,Width*0,Width* Unit=DL,IN,MM */
    float B; /* F=2*Scratch.X1,Controls.Row_1.Y,Controls.Y1,Geometry1.B2,Geometry1.Y1-Scratch.Y Unit=DL,IN,MM */
    float C; /* F=-1E-14,Inh,_ELLIPSE_THETA(-0.004496519859392,1,0.76771653543307,1.14175745231 Unit=DA */
    float D; /* F=0.5*Width/Scratch.Y1,2*Geometry1.X1/Height,Geometry1.D3,Inh,Width/Height*0.01 */
/* ? EllipticalArcTo.D.Err */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Geometry1.X2,Geometry2.X1,Geometry2.X2,Geometry3.X1,Geometry4.X1 Unit=IN,MM */
    float Y; /* F=Geometry1.Y1,Geometry1.Y2,Geometry1.Y4,Geometry2.Y1,Geometry3.Y1,Geometry4.Y1 Unit=IN,MM */
};

struct vdx_Event
{
    struct vdx_any any;
    unsigned int EventDblClick; /* F=1001&quot;)",DEFAULTEVENT(),GUARD(0),GUARD(DOCMD(1312)),Inh,NA(),No Formula,O */
/* ? Event.EventDblClick.Err */
    float EventDrop; /* F=0,DOCMD(1048),DOCMD(1312)+SETF(&quot;EventDrop&quot;,GUARD(DOCMD(1046))),DOCM */
    gboolean EventXFMod; /* F=0,SETF(&quot;Width&quot;,Char.Size),0)",No Formula =0 */
    gboolean TheData; /* F=No Formula =0 */
    gboolean TheText; /* F=No Formula =0 */
};

struct vdx_EventItem
{
    struct vdx_any any;
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
    struct vdx_any any;
    unsigned int EventItem; /* = */
};

struct vdx_FaceName
{
    struct vdx_any any;
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
    struct vdx_any any;
    unsigned int FaceName; /* = */
};

struct vdx_Field
{
    struct vdx_any any;
    gboolean Calendar; /* F=No Formula =0 */
    gboolean Del; /* =1 */
    gboolean EditMode; /* F=Inh =0 */
    float Format; /* F=FIELDPICTURE(0),FIELDPICTURE(1),FIELDPICTURE(201),FIELDPICTURE(21),FIELDPICTU Unit=STR */
    unsigned int IX; /* */
    gboolean ObjectKind; /* F=No Formula =0 */
    unsigned int Type; /* F=Inh =0,2,5 */
    unsigned int UICat; /* F=Inh =0,1,2,3,5 */
    unsigned int UICod; /* F=Inh =0,1,2,255,3,4,6,8 */
    unsigned int UIFmt; /* F=Inh =0,1,20,201,21,255,34,37,7,8 */
    float Value; /* F=ABS(Controls.Row_1)+0M,ABS(Width),COMPANY(),CREATOR(),Creator(),DOCLASTSAVE() Unit=DATE,DL,M,STR */
};

struct vdx_Fill
{
    struct vdx_any any;
    Color FillBkgnd; /* F=0,1,14,15,18,2,8,HSL(0,0,128),HSL(0,0,181),HSL(0,0,210),HSL(0,0,240),HSL(0,0, */
    float FillBkgndTrans; /* F=0%,Inh,No Formula */
    Color FillForegnd; /* F=0,0,0,20)",0,1,17)",0,10,19))",1,10,11,12,13,14,15,17,18,19,2,20,21,23,3,4,5, */
    float FillForegndTrans; /* F=0%,Inh,No Formula */
    unsigned int FillPattern; /* F=0,1,31,GUARD(0),GUARD(1),Guard(0),Inh */
    float ShapeShdwObliqueAngle; /* F=Inh */
    float ShapeShdwOffsetX; /* F=Inh Unit=IN */
    float ShapeShdwOffsetY; /* F=Inh Unit=IN */
    float ShapeShdwScaleFactor; /* F=Inh */
    gboolean ShapeShdwType; /* F=Inh =0,1 */
    unsigned int ShdwBkgnd; /* F=1,GUARD(0),Inh */
    float ShdwBkgndTrans; /* F=0%,Inh,No Formula */
    Color ShdwForegnd; /* F=0,15,8,GUARD(0),HSL(0,0,205),HSL(0,0,206),HSL(0,0,208),HSL(0,0,210),HSL(0,0,2 */
    float ShdwForegndTrans; /* F=0%,Inh,No Formula */
    unsigned int ShdwPattern; /* F=0,0),1,0)',GUARD(0),Guard(0),Inh */
};

struct vdx_FontEntry
{
    struct vdx_any any;
    unsigned int Attributes; /* =0,16896,19072,19140,19172,23040,23108,23140,4096,4160,4196 */
    gboolean Attributes_exists;
    unsigned int CharSet; /* =0,2 */
    gboolean CharSet_exists;
    unsigned int ID; /* */
    char * Name; /* =Arial,Arial Narrow,Courier,Courier-Bold,Helvetica-Bold,Monotype Sorts,Symbol,T */
    unsigned int PitchAndFamily; /* =18,2,32,34 */
    gboolean PitchAndFamily_exists;
    gboolean Unicode; /* =0 */
    gboolean Weight; /* =0 */
};

struct vdx_Fonts
{
    struct vdx_any any;
    unsigned int FontEntry; /* = */
};

struct vdx_Foreign
{
    struct vdx_any any;
    float ImgHeight; /* F=Height*1,Height*1.0169491525424,Height*1.0189032166643,Height*1.0189032166645 Unit=IN */
    float ImgOffsetX; /* F=ImgWidth*-0.00091096324461408,ImgWidth*-0.00091096324461409,ImgWidth*-0.00092 Unit=IN */
    float ImgOffsetY; /* F=ImgHeight*-0.0086805555555564,ImgHeight*-0.0095961281708911,ImgHeight*-0.0095 Unit=IN */
    float ImgWidth; /* F=Inh,Width*1,Width*1*NOT(Sheet.5!User.GenCheck),Width*1.0027802049408,Width*1. Unit=IN */
};

struct vdx_ForeignData
{
    struct vdx_any any;
    float CompressionLevel; /* =0.05,1.000000 */
    char * CompressionType; /* =GIF,JPEG,PNG */
    unsigned int ExtentX; /* =10112,10370,10413,10520,10523,107,10883,10948,11,11013,11043,11073,11138,11203 */
    gboolean ExtentX_exists;
    unsigned int ExtentY; /* =1034,10434,10504,1052,1061,1070,10807,1083,1093,11,11123,1123,1153,116,118,12, */
    gboolean ExtentY_exists;
    char * ForeignType; /* =Bitmap,EnhMetaFile,MetaFile,Object */
    unsigned int MappingMode; /* =8 */
    gboolean MappingMode_exists;
    float ObjectHeight; /* =0.6748031496063,0.775098,1.389201,2,3.5566929133858,6.5590551181102 */
    unsigned int ObjectType; /* =33280,49664 */
    gboolean ObjectType_exists;
    float ObjectWidth; /* =0.787402,1.2582677165354,2,20.204724409449,3.740157,9.0783464566929 */
    gboolean ShowAsIcon; /* =0 */
};

struct vdx_Geom
{
    struct vdx_any any;
    unsigned int IX; /* */
    gboolean NoFill; /* F=GUARD(1),GUARD(TRUE),Guard(1),Inh =0,1 */
    gboolean NoLine; /* F=Inh,No Formula =0,1 */
    gboolean NoShow; /* F=0)",0,0,1)",0,User.ShapeType&lt;2))))',1",1)",1)',1))",1))',1,0,1)",1,1,0)",2 */
    gboolean NoSnap; /* F=Inh,No Formula =0 */
};

struct vdx_Group
{
    struct vdx_any any;
    unsigned int DisplayMode; /* F=2,Inh =2 */
    gboolean DontMoveChildren; /* F=FALSE,Inh */
    gboolean IsDropTarget; /* F=FALSE,Inh */
    gboolean IsSnapTarget; /* F=Inh,TRUE */
    gboolean IsTextEditTarget; /* F=Inh,TRUE */
    gboolean SelectMode; /* F=1,Inh =0,1 */
};

struct vdx_HeaderFooter
{
    struct vdx_any any;
    char * FooterLeft; /* =&amp;f&amp;e&amp;n */
    float FooterMargin; /* Unit=IN_F,MM */
    char * HeaderFooterColor; /* =#000000 */
    unsigned int HeaderFooterFont; /* */
    char * HeaderLeft; /* =KVR GL/3 */
    float HeaderMargin; /* Unit=IN_F,MM */
    char * HeaderRight; /* =&amp;D */
};

struct vdx_HeaderFooterFont
{
    struct vdx_any any;
    gboolean CharSet; /* =0 */
    unsigned int ClipPrecision; /* =0,2 */
    gboolean ClipPrecision_exists;
    gboolean Escapement; /* =0 */
    char * FaceName; /* =Arial,Arial Narrow,牁慩l */
    int Height; /* =-11,-13,-16 */
    gboolean Height_exists;
    gboolean Italic; /* =0 */
    gboolean Orientation; /* =0 */
    unsigned int OutPrecision; /* =0,3 */
    gboolean OutPrecision_exists;
    unsigned int PitchAndFamily; /* =0,34 */
    gboolean PitchAndFamily_exists;
    gboolean Quality; /* =0,1 */
    gboolean StrikeOut; /* =0 */
    gboolean Underline; /* =0 */
    unsigned int Weight; /* =0,400 */
    gboolean Weight_exists;
    gboolean Width; /* =0 */
};

struct vdx_Help
{
    struct vdx_any any;
    char * Copyright; /* F=Inh =,&#xe000;,Copyright (C) 1997 Visio Corporation. Med ensamrätt.,Copyrigh */
    char * HelpTopic; /* F=Inh =,!#52533,!#52846,!#55170,!#55174,!#55477,!#55488,!#55499,&#xe000;,ET.HLP */
};

struct vdx_Hyperlink
{
    struct vdx_any any;
    char * Address; /* =,http://netc.members.microsoft.com/,http://officeupdate.com/visio/,http://vdxt */
    gboolean Default; /* F=No Formula =0 */
    char * Description; /* =,VDXtoSVG Home Page,Visio Network Solutions */
    char * ExtraInfo; /* F=No Formula =,&#xe000; */
    unsigned int Frame; /* F=No Formula = */
    unsigned int ID; /* */
    gboolean Invisible; /* F=No Formula =0 */
    char * NameU; /* =Row_2,Row_21 */
    gboolean NewWindow; /* F=No Formula =0 */
    unsigned int SortKey; /* F=No Formula = */
    char * SubAddress; /* F=No Formula =,&#xe000;,Detail */
};

struct vdx_Icon
{
    struct vdx_any any;
    unsigned int IX; /* */
};

struct vdx_Image
{
    struct vdx_any any;
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
    struct vdx_any any;
    float A; /* F=IF(Width&gt;0,Width,1DL),If(Width&gt;0,Width,0.039370078740157DL) Unit=DL */
    float B; /* F=Height*0.5 Unit=DL */
    unsigned int IX; /* */
    float X; /* F=Width*0 */
    float Y; /* F=Height*0.5 */
};

struct vdx_Layer
{
    struct vdx_any any;
    gboolean Active; /* =0 */
    unsigned int Color; /* =255 */
    float ColorTrans; /* F=No Formula =0,0.5 */
    gboolean Glue; /* =0,1 */
    unsigned int IX; /* */
    gboolean Lock; /* =0 */
    char * Name; /* F=No Formula =,&#xe000;,Annotations,Apple,Background form,Bezeichnung,CE,Connec */
    char * NameUniv; /* F=No Formula =,&#xe000;,Annotations,Apple,Bezeichnung,CE,Connector,Cray,Digital */
/* ? Layer.NameUniv.Err */
    gboolean Print; /* =0,1 */
    gboolean Snap; /* =0,1 */
    unsigned int Status; /* =0,2 */
    gboolean Visible; /* =0,1 */
};

struct vdx_LayerMem
{
    struct vdx_any any;
    char * LayerMember; /* F=No Formula =,&#xe000;,0,0;1,1,2,4,5,6, */
};

struct vdx_Layout
{
    struct vdx_any any;
    unsigned int ConFixedCode; /* F=0,Inh =0,3,6 */
    gboolean ConLineJumpCode; /* F=0,Inh =0 */
    gboolean ConLineJumpDirX; /* F=0,Inh =0 */
    gboolean ConLineJumpDirY; /* F=0,Inh =0 */
    unsigned int ConLineJumpStyle; /* F=0,Inh */
    unsigned int ConLineRouteExt; /* F=0,Inh,No Formula =0,1,2 */
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
    struct vdx_any any;
    unsigned int BeginArrow; /* F=0,3,41,4)))',4,GUARD(0),Inh,USE(&quot;Composite&quot;) =0,1,10,11,13,14,15,25 */
    unsigned int BeginArrowSize; /* F=1,2,IF(User.UMLError,0,2),Inh =0,1,2,3,4 */
    unsigned int EndArrow; /* F=0,3,41,4)))',4,GUARD(0),Inh =0,1,10,11,12,13,14,15,16,4,5,9 */
    unsigned int EndArrowSize; /* F=1,2,IF(User.UMLError,0,2),Inh =0,1,2,3,4 */
    gboolean LineCap; /* F=0,1,Inh =0,1 */
    Color LineColor; /* F=0,1,14,15,2,3,4,GUARD(Sheet.5!FillForegnd),HSL(0,0,0),HSL(0,0,240),HSL(0,0,60 */
    float LineColorTrans; /* F=0%,Inh,No Formula */
    unsigned int LinePattern; /* F=0,1,1,0,1)',2,23,3,4,3,1)))',9,GUARD(1),Inh,Sheet.8!LinePattern */
    float LineWeight; /* F=0.0033333333333333DT,0.01DT,0.03DT,0.12PT,0.24PT,0.254MM,0.5MM,0PT,GUARD(0.24 Unit=IN,MM,PT */
    float Rounding; /* F=0DL,4),0.25IN,0)',4,3*User.Margin,0))',GUARD(0),Inh Unit=FT,IN,MM */
};

struct vdx_LineTo
{
    struct vdx_any any;
    gboolean Del; /* =1 */
    unsigned int IX; /* */
    float X; /* F=(Width+Scratch.Y1)/2,-0.5*Scratch.A1*AND(Scratch.B1&lt;&gt;Scratch.C1,Scratch Unit=IN,IN_F,MM,PT */
    float Y; /* F=(Height-Scratch.X1)/2,-2*User.Margin,-Controls.Row_2.Y,-Scratch.A1,-User.Marg Unit=CM,IN,MM,PT */
};

struct vdx_Master
{
    struct vdx_any any;
    unsigned int AlignName; /* =2 */
    gboolean AlignName_exists;
    char * BaseID; /* ={0018B32E-000A-F00D-0000-000000000000},{001AFBE6-0020-F00D-0000-000000000000}, */
    gboolean Hidden; /* =0,1 */
    unsigned int ID; /* */
    unsigned int IconSize; /* =1,3,4 */
    gboolean IconSize_exists;
    gboolean IconUpdate; /* =0,1 */
    gboolean MatchByName; /* =0,1 */
    char * Name; /* =  Enkel  1D-pil, Dubbel  1D-pil,167152-001 (R),2/8V, 2/16V &amp; 2/16N,2950T-4 */
    char * NameU; /* =  12 Pkt.   Text,  6 Pkt.   Text,  Enkel  1D-pil, Dubbel  1D-pil,167152-001 (R */
    unsigned int PatternFlags; /* =0,1026,2 */
    gboolean PatternFlags_exists;
    char * Prompt; /* =,3-D box with variable depth and orientation.,3-D vertical bar shape. Open eit */
    char * UniqueID; /* ={0008892C-0000-0000-8E40-00608CF305B2},{0008892C-0002-0000-8E40-00608CF305B2}, */
};

struct vdx_Misc
{
    struct vdx_any any;
    unsigned int BegTrigger; /* F=Inh,No Formula,_XFTRIGGER(Class!EventXFMod),_XFTRIGGER(Class.102!EventXFMod), */
/* ? Misc.BegTrigger.Err */
    gboolean Calendar; /* F=Inh =0 */
    char * Comment; /* F=INDEX(0,INDEX(0,MASTERNAME(),&quot;:&quot;),&quot;.&quot;),Inh,REF() =,&#xe00 */
/* ? Misc.Comment.Err */
    gboolean DropOnPageScale; /* F=Inh =1 */
    unsigned int DynFeedback; /* F=0,Inh =0,2 */
    unsigned int EndTrigger; /* F=Inh,No Formula,_XFTRIGGER('PBX / comm. hub'!EventXFMod),_XFTRIGGER('PBX / com */
    unsigned int GlueType; /* F=0,Inh =0,2,3 */
    gboolean HideText; /* F=FALSE,FORMAT(Char.Size,&quot;0.000&quot;)&lt;0.5,GUARD((BITAND(Sheet.5!User.U */
    gboolean IsDropSource; /* F=FALSE,Inh */
    unsigned int LangID; /* F=Inh =1033,1036,1053,2057 */
    gboolean LocalizeMerge; /* F=Inh =0 */
    gboolean NoAlignBox; /* F=FALSE,GUARD(1),GUARD(TRUE),Inh */
    gboolean NoCtlHandles; /* F=FALSE,GUARD(TRUE),Inh */
    gboolean NoLiveDynamics; /* F=FALSE,Inh */
    gboolean NoObjHandles; /* F=FALSE,GUARD(TRUE),Inh */
    gboolean NonPrinting; /* F=FALSE,GUARD(TRUE),Inh,TRUE */
    unsigned int ObjType; /* F=0,Inh,No Formula =0,1,12,2,4,8,9 */
    char * ShapeKeywords; /* F=Inh =,ATM/FastGB,etherswitch,logical,network,system,topography,lines,connecti */
    gboolean UpdateAlignBox; /* F=FALSE,GUARD(FALSE),Inh */
    unsigned int WalkPreference; /* F=0,Inh =0,2,3 */
};

struct vdx_MoveTo
{
    struct vdx_any any;
    unsigned int IX; /* */
    float X; /* F=(Width*1-Height*0.5)*NOT(Scratch.A1),(Width-Scratch.Y1)/2,-Geometry1.X2,-Scra Unit=IN,IN_F,MM,PT */
    float Y; /* F=(Height+Scratch.X1)/2,-2*User.Margin,-Height*0.2,-Height*0.25,-Scratch.A1,-Us Unit=CM,IN,MM,PT */
};

struct vdx_NURBSTo
{
    struct vdx_any any;
    float A; /* F=Inh =-0.06462336099700583,-0.314670638477898,-0.6860878983991592,-0.701050922 */
    float B; /* F=Inh =1 */
    float C; /* F=Inh =-0.2847946620216469,-0.314670638477898,-0.412922655298515,-0.69374622906 */
    float D; /* F=Inh =1 */
    char * E; /* F=Inh,NURBS(-0.28479466686661, 3, 0, 0, 1.0000000000684,0.83800682410212,-1.570 Unit=NURBS */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Inh,Width*0,Width*0.00021847584805948,Width*0.00027894734671036, */
    float Y; /* F=Geometry1.Y1,Height*0,Height*0.00042228834096785,Height*0.0057045664904984,He */
};

struct vdx_Page
{
    struct vdx_any any;
    unsigned int BackPage; /* =10,4,5,6,8 */
    gboolean BackPage_exists;
    gboolean Background; /* =1 */
    unsigned int ID; /* */
    char * Name; /* =ARP Sequence,About This Template,Background-Main,Background-Screen 1,Backgroun */
    char * NameU; /* =ATM Sequence,About This Template,Background-Screen 1,Background-Screen 2,Blatt */
    float ViewCenterX; /* =0,0.64955357142857,1.0133333333333,1.7838541666667,12.05,12.316272965879,2.199 */
    float ViewCenterY; /* =0,0.093333333333333,0.62723214285714,10.03280839895,10.767716535433,11.5625,14 */
    float ViewScale; /* =-1,-2,0.5,0.66145833333333,0.75,1,1.5408921933086,2,3.0817610062893,3.68778280 */
};

struct vdx_PageLayout
{
    struct vdx_any any;
    float AvenueSizeX; /* F=0.29527559055118DL,0.375DL,Inh Unit=IN,MM */
    float AvenueSizeY; /* F=0.29527559055118DL,0.375DL,Inh Unit=IN,MM */
    float BlockSizeX; /* F=0.19685039370079DL,0.25DL,Inh Unit=IN,MM */
    float BlockSizeY; /* F=0.19685039370079DL,0.25DL,Inh Unit=IN,MM */
    gboolean CtrlAsInput; /* F=FALSE,Inh */
    gboolean DynamicsOff; /* F=FALSE,Inh */
    gboolean EnableGrid; /* F=FALSE,Inh */
    unsigned int LineAdjustFrom; /* F=0,Inh =0,2,3 */
    unsigned int LineAdjustTo; /* F=0,Inh =0,2,3 */
    gboolean LineJumpCode; /* F=1,Inh =0,1 */
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
    struct vdx_any any;
    float DrawingScale; /* F=No Formula Unit=F_I,IN,IN_F,M,MM */
    unsigned int DrawingScaleType; /* F=No Formula =0,1,3,4 */
    unsigned int DrawingSizeType; /* F=No Formula =0,1,2,3,4,5,6 */
    gboolean InhibitSnap; /* F=No Formula =0 */
    float PageHeight; /* F=No Formula Unit=CM,DL,F_I,IN,IN_F,M,MM,NUM */
    float PageScale; /* F=No Formula Unit=CM,IN,IN_F,MM */
    float PageWidth; /* F=No Formula Unit=CM,DL,F_I,IN,IN_F,M,MM,NUM */
    float ShdwObliqueAngle; /* F=0DA,No Formula Unit=NUM */
    float ShdwOffsetX; /* F=GUARD(3MM),No Formula Unit=IN,MM,NUM */
    float ShdwOffsetY; /* F=-0.11811023622047DP,-0.125DP,-0.125IN,-3MM,GUARD(-3MM),No Formula Unit=IN,MM,NUM */
    float ShdwScaleFactor; /* F=100%,No Formula */
    gboolean ShdwType; /* F=No Formula =0,1 */
    gboolean UIVisibility; /* F=No Formula =0 */
};

struct vdx_PageSheet
{
    struct vdx_any any;
    unsigned int FillStyle; /* */
    gboolean FillStyle_exists;
    unsigned int LineStyle; /* */
    gboolean LineStyle_exists;
    unsigned int TextStyle; /* */
    gboolean TextStyle_exists;
    char * UniqueID; /* ={17B46475-6543-443F-B3D6-78FB9005398D},{312C523C-40D5-46D4-B641-3C5070AAF772}, */
};

struct vdx_Para
{
    struct vdx_any any;
    unsigned int Bullet; /* F=0,Inh =0,1,2 */
    unsigned int BulletFont; /* F=Inh */
    char * BulletFontSize; /* F=Inh =-1,0) */
    char * BulletStr; /* F=Inh =,&#xe000;,&amp;#xe000;, */
    gboolean Flags; /* F=Inh =0 */
    unsigned int HorzAlign; /* F=0,1,EndX,0,2)",FlipX*2,If(User.ShapeType&gt;5,0,1),Inh */
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

struct vdx_PolylineTo
{
    struct vdx_any any;
    float A; /* F=POLYLINE(0, 0, 0,1, 0,0, 1,0),POLYLINE(0, 0, 0.085708617154953,0, 0.085708617 Unit=POLYLINE */
    unsigned int IX; /* */
    float X; /* F=Width*0,Width*0.13061305469215,Width*0.2612261093843,Width*0.39183916407645,W */
    float Y; /* F=Height*0,Height*0.17690045450495,Height*0.35380090900989,Height*0.53070136351 */
};

struct vdx_PreviewPicture
{
    struct vdx_any any;
    unsigned int Size; /* =1056,1064,1088,1124,1168,1192,1352,1372,1980,20740,20760,20936,20980,21032,210 */
    gboolean Size_exists;
};

struct vdx_PrintProps
{
    struct vdx_any any;
    gboolean CenterX; /* F=Inh =0,1 */
    gboolean CenterY; /* F=Inh =0,1 */
    gboolean OnPage; /* F=Inh =0,1 */
    float PageBottomMargin; /* F=Inh Unit=IN,MM */
    float PageLeftMargin; /* F=Inh Unit=IN,MM */
    float PageRightMargin; /* F=Inh Unit=IN,MM */
    float PageTopMargin; /* F=Inh Unit=IN,MM */
    gboolean PagesX; /* F=Inh =1 */
    gboolean PagesY; /* F=Inh =1 */
    unsigned int PaperKind; /* F=Inh =0,1,17,19,3,5,8,9 */
    unsigned int PaperSource; /* F=Inh =7 */
    gboolean PrintGrid; /* F=Inh =0 */
    unsigned int PrintPageOrientation; /* F=Inh =0,1,2 */
    float ScaleX; /* F=Inh =0.76,1 */
    gboolean ScaleY; /* F=Inh =1 */
};

struct vdx_PrintSetup
{
    struct vdx_any any;
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
    struct vdx_any any;
    gboolean Calendar; /* F=No Formula =0 */
    char * Format; /* F=Guard(ThePage!Prop.Theme.Format),Inh,No Formula,ThePage!Prop.Theme.Format =,& */
    unsigned int ID; /* */
    gboolean Invisible; /* F=Inh,No Formula =0,1 */
    char * Label; /* F=Inh,No Formula =,&#xe000;,&amp;Child Layout Style,&amp;Name,&amp;Preferred Pr */
    unsigned int LangID; /* F=Inh =1033,1036,1053 */
    char * Name; /* =Disk_Vertical */
    char * NameU; /* =1stLegDir,ALMode,AdditionalInfo,AdminInterface,AssetNumber,BelongsTo,Building, */
    char * Prompt; /* F=Inh,No Formula =,&#xe000;,Abteilung,Administrative Interface,Administrative S */
    char * SortKey; /* F=1,2,3,4,5,6,7,8,Inh,No Formula =, Choice, Switch,&#xe000;,1,2,3,4,5,6,7,8,Akt */
    unsigned int Type; /* F="5","7",&quot;5&quot;,&quot;7&quot;,Inh,No Formula =0,1,2,3,4,5,7 */
    float Value; /* F=-1,6,206,-1)',Guard(ThePage!Prop.Theme),Inh,No Formula,ThePage!Prop.ShowDivid Unit=BOOL,IN,STR */
    gboolean Verify; /* F=Inh,No Formula =0,1 */
};

struct vdx_Protection
{
    struct vdx_any any;
    gboolean LockAspect; /* F=0,Inh =0,1 */
    gboolean LockBegin; /* F=0,Inh =0 */
    gboolean LockCalcWH; /* F=0,Inh =0,1 */
    gboolean LockCrop; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockCustProp; /* F=Inh =0 */
    gboolean LockDelete; /* F=0,GUARD(1),Inh,NOT(ISERROR(Sheet.18!Width)),NOT(ISERROR(Sheet.5!Width)),NOT(I */
    gboolean LockEnd; /* F=0,Inh =0 */
    gboolean LockFormat; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockGroup; /* F=0,GUARD(1),Inh =0,1 */
    gboolean LockHeight; /* F=0,GUARD(1),Inh =0,1 */
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
    struct vdx_any any;
    unsigned int XGridDensity; /* F=8,Inh =0,4,8 */
    float XGridOrigin; /* F=0DL,Inh Unit=IN */
    float XGridSpacing; /* F=0DL,Inh Unit=CM,IN,MM */
    unsigned int XRulerDensity; /* F=32,Inh =16,32 */
    float XRulerOrigin; /* F=0DL,Inh Unit=IN,MM */
    unsigned int YGridDensity; /* F=8,Inh =0,4,8 */
    float YGridOrigin; /* F=0DL,Inh Unit=IN */
    float YGridSpacing; /* F=0DL,Inh Unit=CM,IN,MM */
    unsigned int YRulerDensity; /* F=32,Inh =16,32 */
    float YRulerOrigin; /* F=0DL,Inh Unit=IN */
};

struct vdx_Scratch
{
    struct vdx_any any;
    float A; /* F=0)",0,0,1)+Int(Sheet.21!Width/Scratch.X1)',0,0,1)+Int(Sheet.38!Width/Scratch. Unit=BOOL,DA,DL,DT,IN,MM,STR */
    float B; /* F=(LineWeight+(EndArrowSize+2)*0.02IN*Scratch.B1),(LineWeight+(EndArrowSize+2)* Unit=BOOL,DEG,DL,DT,IN,MM,PT,STR */
    float C; /* F=(ABS(Width)&lt;2*Scratch.B2),0,Geometry1.Y7,Geometry1.Y1)",0,LineWeight*-3,0) Unit=BOOL,DA,DL,IN,MM,PT,STR */
    float D; /* F=1,1,-1)",7,(Geometry1.X7+Geometry1.X8)/2,Width))",7,Geometry1.Y7,(Geometry1.Y Unit=BOOL,DL,STR */
    unsigned int IX; /* */
    float X; /* F=((4/9)*(Controls.X1-((8/27)*Geometry1.X1)-((1/27)*Geometry1.X4)))-((2/9)*(Con Unit=CM,IN,MM,PT */
    float Y; /* F=((4/9)*(Controls.Y1-((8/27)*Geometry1.Y1)-((1/27)*Geometry1.Y4)))-((2/9)*(Con Unit=IN,MM,PT */
};

struct vdx_Shape
{
    struct vdx_any any;
    unsigned int Data1; /* =,1001 */
    unsigned int Data2; /* = */
    unsigned int Data3; /* = */
    gboolean Del; /* =1 */
    unsigned int FillStyle; /* */
    gboolean FillStyle_exists;
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    gboolean LineStyle_exists;
    unsigned int Master; /* =0,10,101,106,107,108,11,112,113,114,12,13,14,15,16,17,18,19,2,20,21,22,23,24,2 */
    gboolean Master_exists;
    unsigned int MasterShape; /* =10,100,101,11,12,121,125,13,14,144,147,148,149,15,150,151,152,153,154,156,16,1 */
    gboolean MasterShape_exists;
    char * Name; /* =28_216_BSeries_SAN_Switch,4xx4_dualSCSI_Intfc,54x4_Rear,9740 ECM,Attributes,CA */
    char * NameU; /* =  6 Pkt.   Text.101,  6 Pkt.   Text.105,  6 Pkt.   Text.112,  6 Pkt.   Text.12 */
    unsigned int TextStyle; /* */
    gboolean TextStyle_exists;
    char * Type; /* =Foreign,Group,Guide,Shape */
    char * UniqueID; /* ={001B0C9A-2915-427E-96BF-4779DE5A4600},{0038D543-67DA-45C2-8EB5-5F966B144DD1}, */
};

struct vdx_Shapes
{
    struct vdx_any any;
};

struct vdx_SplineKnot
{
    struct vdx_any any;
    float A; /* F=Inh =0,0.01588792876351469,0.019830308804445,0.049324479568668,0.055118110236 */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Inh,Scratch.X2/(12/81),Sheet.77!Width*1.4396670792842,Sheet.77!W */
    float Y; /* F=Geometry1.Y1,Height*-0.049254006388619,Height*-0.10723150683158,Height*-0.115 Unit=MM */
};

struct vdx_SplineStart
{
    struct vdx_any any;
    float A; /* F=Inh =0 */
    float B; /* F=Inh =0 */
    float C; /* F=Inh =0.13484716066176,0.1698959365932391,0.1910708009830822,0.197567972399904 */
    float D; /* F=Inh =2,3 */
    unsigned int IX; /* */
    float X; /* F=2*Controls.Row_1-Width/2,Inh,Scratch.X1/(12/81),Width*-0.00041659196377884,Wi */
    float Y; /* F=2*Controls.Row_1.Y,Height*-0.11077030280116,Height*-0.29823512702716,Height*- Unit=IN,MM */
};

struct vdx_StyleProp
{
    struct vdx_any any;
    gboolean EnableFillProps; /* F=0,1,Inh =0,1 */
    gboolean EnableLineProps; /* F=0,1,Inh =0,1 */
    gboolean EnableTextProps; /* F=0,1,Inh =0,1 */
    gboolean HideForApply; /* F=0,1,FALSE,Inh,No Formula */
};

struct vdx_StyleSheet
{
    struct vdx_any any;
    unsigned int FillStyle; /* */
    gboolean FillStyle_exists;
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    gboolean LineStyle_exists;
    char * Name; /* =10 % grå,10% Gray fill,1pxl line,30 % grå,30% Gray fill,3D - förgrund,3D -  */
    char * NameU; /* =1-Pixel-Linie,10 % Grau,10 % grå,10% Gray fill,1pxl line,3-Pixel-Linie,30 % G */
    unsigned int TextStyle; /* */
    gboolean TextStyle_exists;
};

struct vdx_Tab
{
    struct vdx_any any;
    gboolean Alignment; /* =0 */
    unsigned int IX; /* */
    float Position; /* Unit=DP,MM */
};

struct vdx_Tabs
{
    struct vdx_any any;
    unsigned int IX; /* */
};

struct vdx_Text
{
    struct vdx_any any;
    unsigned int IX; /* */
    unsigned int cp; /* = */
    float fld; /* =0.006,0.009,0.011,0.031,0.042,0.046,0.048,0.061,0.063,0.071,0.081,0.084,0.086, */
    unsigned int pp; /* = */
    unsigned int tp; /* = */
};

struct vdx_TextBlock
{
    struct vdx_any any;
    float BottomMargin; /* F=0DP,0DT,0PT,1PT,2PT,4PT,Char.Size*0.5,Char.Size/4,Inh Unit=PT */
    float DefaultTabStop; /* F=0.59055118110236DP,0.5DP,Inh =0.5,0.590551,0.59055118110236,0.590551181102362 */
    float LeftMargin; /* F=0DP,0PT,1PT,2PT,4PT,Char.Size/2,Inh Unit=PT */
    float RightMargin; /* F=0DP,0PT,1PT,2PT,4PT,Char.Size/2,Inh Unit=PT */
    unsigned int TextBkgnd; /* F=0,1+FillForegnd,15,2,Inh */
    float TextBkgndTrans; /* F=0%,Inh,No Formula */
    gboolean TextDirection; /* F=0,Inh =0 */
    float TopMargin; /* F=0DP,0DT,0PT,1PT,2PT,4PT,Char.Size/4,GUARD(IF(Parameters!Geometry1.NoShow,1PT, Unit=PT */
    unsigned int VerticalAlign; /* F=0,1,2,Inh =0,1,2 */
};

struct vdx_TextXForm
{
    struct vdx_any any;
    float TxtAngle; /* F=-Angle,-Scratch.A1,90DEG),0DEG,180DEG)",90DEG),0DEG,180DEG)',GRAVITY(Angle,-6 Unit=DEG */
/* ? TextXForm.TxtAngle.Err */
    float TxtHeight; /* F=0,TEXTHEIGHT(TheText,TxtWidth),TEXTWIDTH(TheText)))",1,TEXTWIDTH(TheText),TEX Unit=CM,IN,IN_F,MM */
    float TxtLocPinX; /* F=(TxtWidth/2)*Para.HorzAlign,0.5*TxtWidth,GUARD(TxtWidth),GUARD(TxtWidth*0.5), Unit=CM,IN,IN_F,MM,PT */
    float TxtLocPinY; /* F=-90DEG,1,If(Abs(Angle)&gt;90DEG,1,0))",0.5*TxtHeight,GUARD(0.5*TxtHeight),GUA Unit=CM,IN,IN_F,MM,PT */
    float TxtPinX; /* F=(Controls.X2+Controls.X1)/2,0,0,Width/2)",0.5*Width,1,1/2,1)*Width",1,Control Unit=IN,IN_F,MM,PT */
    float TxtPinY; /* F=(Controls.Y2+Controls.Y1)/2,-TxtHeight/2,0,1/2,0)*Height",0,Height/2,Height)" Unit=CM,IN,IN_F,MM,PT */
    float TxtWidth; /* F=0,TEXTWIDTH(TheText),TEXTHEIGHT(TheText,TEXTWIDTH(TheText)))",0,TEXTWIDTH(The Unit=CM,IN,IN_F,MM,PT */
};

struct vdx_User
{
    struct vdx_any any;
    unsigned int ID; /* */
    char * Name; /* =216Nff,216Npp */
    char * NameU; /* =1stLegDir,1stLegDirInd,1stLegDirLook,216V,216Vff,216Vpp,28V,28Vpp,AL_ChildLayo */
    char * Prompt; /* F=Inh,No Formula =,&#xe000;,0 = Horizontal, 1 = Vertical,1=left, 2=center, 3=ri */
    float Value; /* F=(0.125IN*User.AntiScale)/2,(PinX-LocPinX+Connections.X1)-BeginX,(PinY-LocPinY Unit=BOOL,COLOR,DA,DL,GUID,IN,MM,PNT,PT,STR */
};

struct vdx_VisioDocument
{
    struct vdx_any any;
    unsigned int DocLangID; /* =1033,1053,2057 */
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
    struct vdx_any any;
    char * ContainerType; /* =Page */
    char * Document; /* =C:\Archivos de programa\Microsoft Office\Visio11\1033\UMLACT_M.VSS,C:\Archivos */
    gboolean DynamicGridEnabled; /* =0,1 */
    unsigned int GlueSettings; /* =11,47,9 */
    unsigned int ID; /* */
    unsigned int Page; /* =0,2,4,5,7,8 */
    gboolean Page_exists;
    unsigned int ParentWindow; /* =0,3,4,8 */
    gboolean ParentWindow_exists;
    gboolean ReadOnly; /* =1 */
    unsigned int Sheet; /* =1118,137,152,153,154,155,157,2,45,5 */
    gboolean Sheet_exists;
    gboolean ShowConnectionPoints; /* =0,1 */
    gboolean ShowGrid; /* =0,1 */
    gboolean ShowGuides; /* =1 */
    gboolean ShowPageBreaks; /* =0 */
    gboolean ShowRulers; /* =1 */
    unsigned int SnapExtensions; /* =1,34 */
    unsigned int SnapSettings; /* =295,319,32807,33063,39,65831,65847 */
    unsigned int StencilGroup; /* =10,11 */
    unsigned int StencilGroupPos; /* =0,1,2,255,3,4,5,6,7 */
    float TabSplitterPos; /* =0.326,0.33,0.432,0.5,0.807,0.892 */
    float ViewCenterX; /* =0.15616797900262,0.31003937007874,0.31496062992126,0.71194225721785,1.01333333 */
    float ViewCenterY; /* =0.093333333333333,0.24184476940382,0.39370078740157,0.45931758530184,0.4921259 */
    float ViewScale; /* =-1,-2,-3,0.5,0.66145833333333,0.75,1,1.5408921933086,1.984375,2,3.081761006289 */
    unsigned int WindowHeight; /* =24,244,262,280,283,354,373,392,411,414,430,431,459,465,554,565,603,605,610,621 */
    gboolean WindowHeight_exists;
    int WindowLeft; /* =-134,-185,-186,-195,-196,-238,-240,-247,-248,-289,-300,-4,-5,0,119,139,158,178 */
    gboolean WindowLeft_exists;
    unsigned int WindowState; /* =1,1025,1073741824,268435456,536870912,67109889 */
    gboolean WindowState_exists;
    int WindowTop; /* =-1,-175,-23,-24,-30,-34,-36,-59,0,1,101,117,134,1480,151,16,22,246,265,284,3,3 */
    gboolean WindowTop_exists;
    char * WindowType; /* =Drawing,Stencil */
    unsigned int WindowWidth; /* =1017,1018,1020,1022,1032,1095,1119,1133,1164,1192,1246,128,1288,160,1688,179,1 */
    gboolean WindowWidth_exists;
};

struct vdx_Windows
{
    struct vdx_any any;
    unsigned int ClientHeight; /* =587,605,607,609,612,621,625,641,643,681,791,851,855,864,868,871,875,882,890 */
    gboolean ClientHeight_exists;
    unsigned int ClientWidth; /* =1020,1022,1024,1095,1119,1125,1156,1184,1236,1280,1680,818,819,828 */
    gboolean ClientWidth_exists;
    unsigned int Window; /* = */
};

struct vdx_XForm
{
    struct vdx_any any;
    float Angle; /* F=-0.5235987756666,-90DEG,-Scratch.B1*90DEG,ATAN2(EndY-BeginY,EndX-BeginX),ATan Unit=DEG */
    gboolean FlipX; /* F=FlipY,GUARD(0),GUARD(FALSE),Guard(0),Guard(EndX&lt;BeginX),Guard(FALSE),Inh,S */
    gboolean FlipY; /* F=BeginY&lt;EndY)",BeginY&lt;EndY)',FlipX,GUARD(0),GUARD(EndY&lt;BeginY),GUARD( */
    float Height; /* F=0.2IN-User.shade*2/3,1/2IN,5MM*User.AntiScale,Abs(EndY-BeginY),GUARD(-0.19685 Unit=CM,IN,IN_F,MM,PT */
    float LocPinX; /* F=0.5*Width,GUARD(0),GUARD(0.5IN),GUARD(Width*0),GUARD(Width*0.25),GUARD(Width* Unit=IN,IN_F,M,MM,PT */
    float LocPinY; /* F=0.5*Height,BeginX,Height,0)",EndX)*Height",GUARD(0),GUARD(Height*0),GUARD(Hei Unit=CM,IN,IN_F,MM,PT */
    float PinX; /* F=(BeginX+EndX)/2,AlignCenter+-1.2100542789995E-10DL,AlignCenter+1.210054278999 Unit=IN,IN_F,MM */
    float PinY; /* F=(BeginY+EndY)/2,AlignBottom+0.59055118063851DL,AlignBottom+0.59055118069901DL Unit=IN,IN_F,MM */
    unsigned int ResizeMode; /* F=GUARD(0),Inh,No Formula =0,1,2 */
    float Width; /* F=3,2*User.Margin,0))',Abs(EndX-BeginX),GUARD(-0.19685039370079DL),GUARD(-0.25D Unit=IN,IN_F,MM,PT */
};

struct vdx_XForm1D
{
    struct vdx_any any;
    float BeginX; /* F=GUARD(Sheet.10!PinX),GUARD(Sheet.5!LocPinX),Guard(Sheet.6!Width+Sheet.9!Width Unit=IN,IN_F,MM */
/* ? XForm1D.BeginX.Err */
    float BeginY; /* F=GUARD(Sheet.13!PinY+Sheet.13!Height),GUARD(Sheet.5!LocPinY),GUARD(Sheet.8!Pin Unit=IN,IN_F,MM */
/* ? XForm1D.BeginY.Err */
    float EndX; /* F=GUARD(BeginX-Sheet.5!Scratch.X1),GUARD(Sheet.13!PinX),GUARD(Sheet.5!Controls. Unit=IN,IN_F,MM */
/* ? XForm1D.EndX.Err */
    float EndY; /* F=BeginY+Sheet.6!Scratch.Y1,GUARD(BeginY),GUARD(BeginY-Sheet.5!Scratch.Y1),GUAR Unit=IN,IN_F,MM */
/* ? XForm1D.EndY.Err */
};

struct vdx_cp
{
    struct vdx_any any;
    unsigned int IX; /* */
};

struct vdx_fld
{
    struct vdx_any any;
    unsigned int IX; /* */
};

struct vdx_pp
{
    struct vdx_any any;
    unsigned int IX; /* */
};

struct vdx_tp
{
    struct vdx_any any;
    unsigned int IX; /* */
};

struct vdx_text
{
    struct vdx_any any;
    char * text; /* */
};

enum {
    vdx_types_any = 0,
    vdx_types_Act,
    vdx_types_Align,
    vdx_types_ArcTo,
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
    vdx_types_Misc,
    vdx_types_MoveTo,
    vdx_types_NURBSTo,
    vdx_types_Page,
    vdx_types_PageLayout,
    vdx_types_PageProps,
    vdx_types_PageSheet,
    vdx_types_Para,
    vdx_types_PolylineTo,
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
