/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
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

/* Generated Sun May 21 23:11:05 2006 */
/* From: All.vdx Circle1.vdx Circle2.vdx Line1.vdx Line2.vdx Line3.vdx Line4.vdx Line5.vdx Line6.vdx Rectangle1.vdx Rectangle2.vdx Rectangle3.vdx Rectangle4.vdx Text1.vdx Text2.vdx Text3.vdx samp_vdx.vdx Entreprise_etat_desire.vdx animation_tests.vdx basic_tests.vdx curve_tests.vdx pattern_tests.vdx seq_test.vdx text_tests.vdx */


struct vdx_any
{
    GSList *children;
    char type;
};

struct vdx_Act
{
    GSList *children;
    char type;
    float Action; /* F=0,DOCMD(1312),DoCmd(1065),DoCmd(1312),RUNADDON(&quot;CS&quot;),SETF(GetRef(User.ShowFooter),NOT(User.ShowFooter)),SetF("Controls.X1","Width/4")+SetF("Controls.Y1","0")+SetF("Controls.X2","Width*3/4")+SetF("Controls.Y2","0"),_RunAddOn("CS") */
    gboolean BeginGroup; /* F=No Formula =0 */
    unsigned int ButtonFace; /* F=No Formula = */
    gboolean Checked; /* F=No Formula =0 */
    gboolean Disabled; /* F=No Formula =0 */
    unsigned int ID; /* */
    unsigned int IX; /* */
    gboolean Invisible; /* F=No Formula =0 */
    char * Menu; /* F=0),"","_Set As Straight Line")',IF(User.ShowFooter,&quot;Hide &amp;footer&quot;,&quot;Show &amp;footer&quot;) =%P&amp;roperties,%P&amp;ropriétés,%Properties,&#xe000;,Change Arrowhead...,Co&amp;lor Schemes...,Hide &amp;footer,_Set As Straight Line */
    char * NameU; /* =LocalRow0 */
    gboolean ReadOnly; /* F=No Formula =0 */
    unsigned int SortKey; /* F=No Formula = */
    unsigned int TagName; /* F=No Formula = */
};

struct vdx_ArcTo
{
    GSList *children;
    char type;
    float A; /* F=-Width */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Width */
    float Y; /* F=Geometry1.Y1,Height*0.5 */
};

struct vdx_Char
{
    GSList *children;
    char type;
    unsigned int AsianFont; /* F=Inh */
    gboolean Case; /* F=0,Inh =0 */
    Color Color; /* F=0,1,14,15,4,HSL(0,0,240),HSL(150,240,96),Inh */
    float ColorTrans; /* F=0%,Inh */
    unsigned int ComplexScriptFont; /* F=Inh */
    int ComplexScriptSize; /* F=Inh =-1 */
    gboolean DblUnderline; /* F=FALSE,Inh */
    gboolean DoubleStrikethrough; /* F=Inh =0 */
    unsigned int Font; /* F=0,Inh */
    float FontScale; /* F=100%,Inh */
    gboolean Highlight; /* F=Inh =0 */
    unsigned int IX; /* */
    unsigned int LangID; /* F=Inh =1033,1036,3084 */
    float Letterspace; /* F=0DT,Inh */
    gboolean Locale; /* F=0,Inh =0 */
    unsigned int LocalizeFont; /* F=Inh */
    gboolean Overline; /* F=FALSE,Inh */
    gboolean Perpendicular; /* F=FALSE,Inh */
    gboolean Pos; /* F=0,Inh =0 */
    gboolean RTLText; /* F=Inh =0 */
    float Size; /* F=0.125DT,0.16666666666667DT,8PT,Inh */
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
    unsigned int FromSheet; /* =104,12,13,14,144,15,16,18,19,20,21,22,23,24,25,26,27,28,29,3,30,31,34,35,36,38,39,4,41,42,6,7,81,82,9 */
    char * ToCell; /* =Connections.Row_1.X,Connections.Row_2.X,Connections.Row_3.X,Connections.Row_4.X,Connections.X1,Connections.X2,Connections.X3,Connections.X4,Connections.X5,Connections.X6,PinX */
    unsigned int ToPart; /* =100,101,102,103,104,105,3 */
    unsigned int ToSheet; /* =1,10,11,12,13,14,15,16,17,18,19,2,20,22,24,25,29,3,30,31,32,33,35,37,4,40,41,5,57,7,8 */
};

struct vdx_Connection
{
    GSList *children;
    char type;
    gboolean AutoGen; /* F=Inh,No Formula =0 */
    float DirX; /* F=Inh,No Formula */
    float DirY; /* F=Inh,No Formula */
    unsigned int ID; /* */
    unsigned int IX; /* */
    unsigned int Prompt; /* F=No Formula = */
    gboolean Type; /* F=Inh,No Formula =0 */
    float X; /* F=0.5*Width,2,Width*0.18,Width*0.2003))",4,Width*0.0822,Width*0.1929))",4,Width*0.8689,Width*0.8133))",4,Width*0.8689,Width*0.8136))",GUARD(Width*0),GUARD(Width*0.5),GUARD(Width*1),Geometry1.A3,Geometry1.A5,Geometry1.X1,Geometry1.X4,Inh,Width,Width*-0.0013568521031187,Width*0,Width*0.05076230620423,Width*0.16666666666667,Width*0.21428571428571,Width*0.41666666666667,Width*0.49056175551368,Width*0.5,Width*0.50072002644587,Width*0.64450474898236,Width*0.77611940298507,Width*0.77777777777778,Width*0.83333333333333,Width*0.85714285714286,Width*0.91372151167609,Width*1 */
    float Y; /* F=0.5*Height,2,Height*0.05,Height*0.1233))))",3,Height*0,Height*0.1197)))",3,Height*1,Height*0.891))",3,Height*1,Height*0.9023))",4,Height*0.1752,Height*0))",4,Height*0.3991,Height*0.5))",4,Height*0.6385,Height*0.5))",4,Height*0.7982,Height*1))",Geometry1.Y1,Geometry1.Y4,Height,Height*-1.3777239275971E-5,Height*0,Height*0.00022208002855999,Height*0.125,Height*0.16666666666668,Height*0.3515625,Height*0.375,Height*0.49995653375522,Height*0.49999999999999,Height*0.5,Height*0.875,Height*0.87504664828391,Height*1,Inh */
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
    char * Prompt; /* F=Controls.Prompt,Inh =,&#xe000;,Ajuster la position de la courbe,Modify Curvature,Move Line,Move to change feature size.,Reposition Text */
    float X; /* F=0.125IN*User.AntiScale,Inh,Width*0,Width*0.00090144230769251,Width*0.26278409090909,Width*0.26988636363636,Width*0.31245378702903,Width*0.375,Width*0.44950902641433,Width*0.48713174278058,Width*0.5,Width*0.54720866366253,Width*0.625 */
    unsigned int XCon; /* F=IF(OR(STRSAME(SHAPETEXT(TheText),&quot;&quot;),HideText),5,0),Inh =0,1,5,7 */
    float XDyn; /* F=Controls.TextPosition,Controls.X1,Inh,Width,Width/2 */
    float Y; /* F=GUARD(0),Height*-0.0023148148148147,Height*0,Height*0.16279069767442,Height-(User.ScaleFactor*0.25IN),Inh */
    unsigned int YCon; /* F=Inh =0,4,6 */
    float YDyn; /* F=Controls.Row_1.Y,Controls.TextPosition.Y,Controls.Y1,Height/2,Inh */
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
    unsigned int OutputFormat; /* =0,2 */
    gboolean PreviewQuality; /* F=No Formula =0 */
    gboolean PreviewScope; /* F=No Formula =0,1 */
    gboolean ViewMarkup; /* =0 */
};

struct vdx_DocumentProperties
{
    GSList *children;
    char type;
    unsigned int BuildNumberCreated; /* =671351309,738200627,738203013 */
    unsigned int BuildNumberEdited; /* =671351309,738200720,738203013 */
    char * Company; /* =ARS-Solutions,Celox,Microsoft Corporation */
    char * Creator; /* = Pierre Robitaille,Microsoft Corporation,jbreen */
    char * Desc; /* =$Id$ */
    char * Template; /* =C:\Program Files\Microsoft Office\Visio10\1033\Solutions\Block Diagram\Basic Diagram (US units).vst,C:\Program Files\Microsoft Office\Visio11\1033\BASFLO_U.VST */
    char * TimeCreated; /* =2002-03-25T13:54:03,2002-04-04T11:41:36,2002-04-27T14:51:25,2002-05-23T07:02:40,2002-06-20T14:48:18,2002-07-01T08:00:47,2003-07-28T11:32:31,2003-12-11T11:37:52,2006-04-27T15:49:08 */
    char * TimeEdited; /* =2002-05-15T07:40:25,2002-06-18T14:58:23,2002-06-26T08:11:41,2002-07-03T10:22:44,2002-07-12T14:17:48,2002-07-17T13:45:51,2003-10-24T16:57:00,2003-12-11T11:39:33,2003-12-11T11:40:20,2003-12-11T11:41:31,2003-12-11T11:42:48,2003-12-11T11:43:50,2003-12-11T11:44:51,2003-12-11T11:46:42,2003-12-11T11:54:00,2003-12-11T11:57:24,2003-12-11T11:58:26,2003-12-11T11:59:27,2003-12-11T12:00:07,2003-12-11T12:01:03,2003-12-11T12:02:01,2003-12-11T12:04:52,2003-12-11T12:08:01,2006-05-11T17:40:26 */
    char * TimePrinted; /* =2002-03-25T13:54:03,2002-04-17T14:27:52,2002-04-27T14:51:25,2002-05-28T16:05:56,2002-06-20T14:48:18,2002-07-01T08:00:47,2003-07-28T11:32:31,2003-12-11T11:37:52,2006-04-27T16:35:05 */
    char * TimeSaved; /* =2002-05-15T07:40:45,2002-06-18T14:58:41,2002-06-26T08:11:53,2002-07-03T10:23:02,2002-07-12T14:17:51,2002-07-17T13:46:17,2003-10-24T16:57:23,2003-12-11T11:39:40,2003-12-11T11:40:45,2003-12-11T11:41:46,2003-12-11T11:43:03,2003-12-11T11:44:08,2003-12-11T11:45:07,2003-12-11T11:46:58,2003-12-11T11:56:50,2003-12-11T11:57:48,2003-12-11T11:58:45,2003-12-11T11:59:42,2003-12-11T12:00:22,2003-12-11T12:01:23,2003-12-11T12:02:21,2003-12-11T12:05:11,2003-12-11T12:08:21,2006-05-12T11:37:01 */
    char * Title; /* =Basic tests,Curve tests,animation-tests,pattern_tests,text_tests */
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
    unsigned int SnapExtensions; /* =1,34 */
    unsigned int SnapSettings; /* =295,65831,65847 */
    unsigned int TopPage; /* =0,2,5,7 */
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
    float A; /* F=Controls.Row_1,Geometry1.A2,Geometry3.A2,Inh,Width,Width*0.00035734844188,Width*0.038629786733769,Width*0.04367407204592,Width*0.0485,Width*0.048503597491669,Width*0.1327073059605,Width*0.14643387733155,Width*0.14917950793704,Width*0.15457317203245,Width*0.19949695423237,Width*0.1995,Width*0.24599914119324,Width*0.26841375488849,Width*0.3229,Width*0.32290495932947,Width*0.45319261647222,Width*0.4532,Width*0.45457513945703,Width*0.47187231380464,Width*0.5,Width*0.51045135738225,Width*0.5468,Width*0.54680738352777,Width*0.55595836957662,Width*0.61153808676645,Width*0.67709504067052,Width*0.6771,Width*0.69201643694431,Width*0.74567225641247,Width*0.761040165894,Width*0.76195426368741,Width*0.8005,Width*0.80050304576763,Width*0.80645883708386,Width*0.80905547607267,Width*0.8312760863447,Width*0.83177031021134,Width*0.83871918449025,Width*0.83991142283006,Width*0.84686029710897,Width*0.84805253544878,Width*0.85355,Width*0.85355339059327,Width*0.85624629769461,Width*0.8576751719373,Width*0.85886741027711,Width*0.86581628455603,Width*0.86700852289584,Width*0.87395739717476,Width*0.90051321095777,Width*0.90149110878702,Width*0.92677669529664,Width*0.92977649253028,Width*0.93020365145875,Width*0.93238292637885,Width*0.93814081399298,Width*0.94298687559161,Width*0.95149640250833,Width*0.9515,Width*0.96137021326623,Width*0.96700155317681,Width*0.98528555206619,Width*0.99237925319258,Width*2.3042292052189E-7,Width-Geometry1.A2 */
    float B; /* F=Controls.Row_1.Y,Geometry1.B2,Geometry2.B2,Height*0,Height*0.00018283632885901,Height*0.0002,Height*0.00093577386471846,Height*0.048815536468922,Height*0.05573914480436,Height*0.060887869711828,Height*0.0609,Height*0.066378836076452,Height*0.070643360901045,Height*0.0809582024372,Height*0.09021737097769,Height*0.09264335134213,Height*0.14304550700596,Height*0.14374120543817,Height*0.14644660940673,Height*0.14645,Height*0.14650807995959,Height*0.15785502570195,Height*0.18773781457624,Height*0.28451779686444,Height*0.32489727173002,Height*0.3544,Height*0.35443789112062,Height*0.36458991250555,Height*0.36822416754829,Height*0.39595481358611,Height*0.396,Height*0.43047132149445,Height*0.49999999999999,Height*0.54350109362404,Height*0.604,Height*0.60404518641389,Height*0.60646332314462,Height*0.64556210887939,Height*0.6456,Height*0.671891601782,Height*0.7297554221597,Height*0.73581277741775,Height*0.74107024366069,Height*0.76923076923077,Height*0.78437300049102,Height*0.79239717528248,Height*0.79618179098748,Height*0.85074652811942,Height*0.85355339059327,Height*0.85625879456183,Height*0.87840383063015,Height*0.9169656306918,Height*0.93031085196772,Height*0.9391,Height*0.93911213028817,Height*0.9638904953604,Height*0.99500618325399,Height*0.9998,Height*0.99981716367115,Height*0.99999298041709,Height*1,Height-Geometry1.B2,Inh */
    float C; /* F=Inh,_ELLIPSE_THETA(-0.010584331794649,1.0252774665981,8.2610199709935,10.5,Width,Height),_ELLIPSE_THETA(-0.0501546698465,1.328380193895,9.149586311802,8.8958333333333,Width,Height),_ELLIPSE_THETA(-0.13339417893542,1.3170911749587,9.149586311802,8.8958333333333,Width,Height),_ELLIPSE_THETA(-1.1071487177941,1.096132093534,8.2610199709935,11.162514876073,Width,Height),_ELLIPSE_THETA(-1.5345027864364,1.0391157466713,7.9473264127403,8.0961925705443,Width,Height),_ELLIPSE_THETA(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height),_ELLIPSE_THETA(0.0175,4.0709,2.1703,0.521,Width,Height),_ELLIPSE_THETA(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height),_ELLIPSE_THETA(0.1763,4.6667,2.1703,0.521,Width,Height),_Ellipse_Theta(-0.64116564286212,0.90841981854634,1.0003731862713,1.0003733255912,Width,Height),_Ellipse_Theta(-1.5973448632614,1,1.9520268451431,1.5628733255912,Width,Height),_Ellipse_Theta(-1.8995402626436,1.2489989335889,1.0003731862713,1.0003733255912,Width,Height),_Ellipse_Theta(0.37088514706695,0.80064119600695,1.0003731862713,1.0003733255912,Width,Height),_Ellipse_Theta(0.41635315089518,0.90841981854634,1.0003731862713,1.0003733255912,Width,Height) */
    float D; /* F=2*Geometry1.X1/Height,Geometry1.D3,Inh,Width/Height*0.073270013568522,Width/Height*0.097693351424699,Width/Height*0.12211668928087,Width/Height*0.5,Width/Height*0.6158506775896,Width/Height*0.72222222222222,Width/Height*0.80064130751065,Width/Height*0.90841994506016,Width/Height*1,Width/Height*1.0000869400475,_ELLIPSE_ECC(-0.010584331794649,1.0252774665981,8.2610199709935,10.5,Width,Height,Geometry1.C4),_ELLIPSE_ECC(-0.0501546698465,1.328380193895,9.149586311802,8.8958333333333,Width,Height,Geometry1.C4),_ELLIPSE_ECC(-0.13339417893542,1.3170911749587,9.149586311802,8.8958333333333,Width,Height,Geometry1.C2),_ELLIPSE_ECC(-1.1071487177941,1.096132093534,8.2610199709935,11.162514876073,Width,Height,Geometry1.C2),_ELLIPSE_ECC(-1.5345027864364,1.0391157466713,7.9473264127403,8.0961925705443,Width,Height,Geometry1.C2),_ELLIPSE_ECC(-1.5345027864364,1.0391157466713,7.9473264127403,8.0961925705443,Width,Height,Geometry1.C3),_ELLIPSE_ECC(-1.5707963267949,1,2,2,Width,Height,Geometry2.C2),_ELLIPSE_ECC(-3.1415926535898,1,2,2,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.01745329252222,4.0708635693179,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C5),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.0175,4.0709,2.1703,0.521,Width,Height,Geometry1.C5),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.17627825447442,4.6666666666667,2.1703127882354,0.52098000729874,Width,Height,Geometry1.C5),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C2),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C3),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C4),_ELLIPSE_ECC(0.1763,4.6667,2.1703,0.521,Width,Height,Geometry1.C5),_ELLIPSE_ECC(1.5707963267949,1,2,2,Width,Height,Geometry3.C2),_Ellipse_Ecc(-0.64116564286212,0.90841981854634,1.0003731862713,1.0003733255912,Width,Height,Geometry1.C11),_Ellipse_Ecc(-1.5973448632614,1,1.9520268451431,1.5628733255912,Width,Height,Geometry1.C2),_Ellipse_Ecc(-1.8995402626436,1.2489989335889,1.0003731862713,1.0003733255912,Width,Height,Geometry1.C8),_Ellipse_Ecc(0.37088514706695,0.80064119600695,1.0003731862713,1.0003733255912,Width,Height,Geometry1.C7),_Ellipse_Ecc(0.41635315089518,0.90841981854634,1.0003731862713,1.0003733255912,Width,Height,Geometry1.C12) */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Geometry1.X2,Geometry3.X1,Inh,Width*0,Width*0.00057808450186472,Width*0.083976785441528,Width*0.084382782239857,Width*0.12327082631792,Width*0.14635636983687,Width*0.19678663930461,Width*0.20811090826019,Width*0.25984358645123,Width*0.30250009731218,Width*0.30942697519678,Width*0.31939138622637,Width*0.35924093621453,Width*0.37144351436963,Width*0.49995652997627,Width*0.57294301432068,Width*0.57737231688171,Width*0.57809724180447,Width*0.59758899581632,Width*0.615726829128,Width*0.64961824953929,Width*0.65069700350148,Width*0.68060861377363,Width*0.74015641354877,Width*0.74363193913222,Width*0.75,Width*0.7918890917398,Width*0.7919,Width*0.8032,Width*0.80321336069539,Width*0.82571726214012,Width*0.83514246947082,Width*0.84328358208954,Width*0.84735413839891,Width*0.85051491020691,Width*0.85142469470827,Width*0.85549525101763,Width*0.85956580732699,Width*0.86363636363635,Width*0.86526826452549,Width*0.86799726469508,Width*0.87177747625508,Width*0.87467358382669,Width*0.87991858887381,Width*0.94162112379571,Width*0.94978702101797,Width*0.98848837753254,Width*0.9994,Width*0.99942191549813,Width*0.99980800259389,Width*0.99991305995254,Width*0.99999999999999,Width*1,Width-Geometry3.X1,Width/2-Scratch.X1 */
    float Y; /* F=Geometry1.Y1,Geometry1.Y4,Geometry2.Y1,Height*0,Height*0.07522095404727,Height*0.085661311224371,Height*0.09521617662116,Height*0.12195045039423,Height*0.122,Height*0.12207405596001,Height*0.14020542741243,Height*0.16666666666668,Height*0.17019826127327,Height*0.23574183057592,Height*0.2601767628558,Height*0.28468392732891,Height*0.3324,Height*0.33243986480781,Height*0.33333333333334,Height*0.3611,Height*0.36113315618117,Height*0.45332908624209,Height*0.5,Height*0.51,Height*0.53407728006875,Height*0.54004211908006,Height*0.57563930026782,Height*0.57789395902665,Height*0.63886684381884,Height*0.6389,Height*0.64359365071219,Height*0.6675601351922,Height*0.6676,Height*0.67683491865304,Height*0.67683491865305,Height*0.70765444974179,Height*0.80329737249277,Height*0.80381659526035,Height*0.80555555555556,Height*0.81256997242586,Height*0.81997523511459,Height*0.86234671671524,Height*0.86569810241906,Height*0.878,Height*0.87804954960577,Height*0.88524590163934,Height*0.89995612541017,Height*0.99991306751045,Height*1,Height*8.6932489548436E-5,Height-Geometry1.Y1,Height/2,Height/2-Scratch.X1,Inh */
};

struct vdx_Event
{
    GSList *children;
    char type;
    gboolean EventDblClick; /* F=DEFAULTEVENT(),Inh,No Formula,OPENTEXTWIN(),_DefaultEvent(),_OpenTextWin() =0 */
    float EventDrop; /* F=No Formula,RUNADDON(&quot;Make Background&quot;)+SETF(&quot;EventDrop&quot;,0),SETF(GetRef(Width),ThePage!PageWidth-4*User.PageMargin)+SETF(GetRef(Height),ThePage!PageHeight-4*User.PageMargin)+SETF(GetRef(PinX),ThePage!PageWidth*0.5)+SETF(GetRef(PinY),ThePage!PageHeight*0.5)+SETF(&quot;EventDrop&quot;,0) */
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
    unsigned int EventCode; /* =1,2 */
    unsigned int ID; /* */
    char * Target; /* =AM,PPT */
    unsigned int TargetArgs; /* =,1,3 */
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
    char * Name; /* =,Arial,Arial Unicode MS,Cordia New,Dhenu,Dotum,Estrangelo Edessa,Gautami,Gulim,Latha,MS Farsi,MS PGothic,Mangal,PMingLiU,Raavi,Sendnya,Shruti,SimSun,Sylfaen,Symbol,Times New Roman,Tunga,Vrinda,Wingdings */
    char * Panos; /* =0 0 4 0 0 0 0 0 0 0,1 10 5 2 5 3 6 3 3 3,2 0 4 0 0 0 0 0 0 0,2 0 5 0 0 0 0 0 0 0,2 1 6 0 3 1 1 1 1 1,2 11 6 0 0 1 1 1 1 1,2 11 6 0 7 2 5 8 2 4,2 11 6 4 2 2 2 2 2 4,2 2 3 0 0 0 0 0 0 0,2 2 6 3 5 4 5 2 3 4,3 8 6 0 0 0 0 0 0 0,5 0 0 0 0 0 0 0 0 0,5 5 1 2 1 7 6 2 5 7 */
    char * UnicodeRanges; /* =-1 -369098753 63 0,-1342176593 1775729915 48 0,-1610612033 1757936891 16 0,-2147459008 0 128 0,0 0 0 0,1048576 0 0 0,131072 0 0 0,1627421663 -2147483648 8 0,16804487 -2147483648 8 0,2097152 0 0 0,262144 0 0 0,3 135135232 0 0,3 137232384 22 0,31367 -2147483648 8 0,32768 0 0 0,4194304 0 0 0,67110535 0 0 0 */
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
    float Format; /* F=FIELDPICTURE(0),FIELDPICTURE(201),FieldPicture(0),FieldPicture(8),Inh */
    unsigned int IX; /* */
    gboolean ObjectKind; /* F=No Formula =0 */
    unsigned int Type; /* F=Inh =0,2,5 */
    unsigned int UICat; /* F=Inh =0,1,3,5 */
    unsigned int UICod; /* F=Inh =0,255,3,4 */
    unsigned int UIFmt; /* F=Inh =0,201,8 */
    float Value; /* F=DOCLASTSAVE(),Inh,PAGENUMBER(),Prop.Row_1,Prop.Row_2,Width */
};

struct vdx_Fill
{
    GSList *children;
    char type;
    Color FillBkgnd; /* F=0,1,14,15,18,HSL(0,0,240),HSL(149,240,185),HSL(149,240,210),HSL(149,240,215),HSL(149,240,225),HSL(150,240,216),HSL(160,0,235),HSL(67,140,190),HSL(67,144,215),HSL(67,145,190),HSL(67,152,222),Inh */
    float FillBkgndTrans; /* F=0%,Inh */
    Color FillForegnd; /* F=0,1,14,15,18,HSL(0,0,240),HSL(145,240,235),HSL(149,240,226),HSL(150,240,96),HSL(160,0,225),HSL(35,240,168),HSL(65,123,221),HSL(67,141,213),HSL(67,143,186),HSL(67,144,215),HSL(67,144,225),HSL(67,144,235),HSL(67,148,188),HSL(68,141,226),Inh */
    float FillForegndTrans; /* F=0%,Inh */
    unsigned int FillPattern; /* F=0,1,GUARD(0),Inh */
    float ShapeShdwObliqueAngle; /* F=Inh */
    float ShapeShdwOffsetX; /* F=Inh */
    float ShapeShdwOffsetY; /* F=Inh */
    float ShapeShdwScaleFactor; /* F=Inh */
    gboolean ShapeShdwType; /* F=Inh =0 */
    unsigned int ShdwBkgnd; /* F=1,Inh */
    float ShdwBkgndTrans; /* F=0%,Inh */
    Color ShdwForegnd; /* F=0,15,HSL(67,141,113),Inh */
    float ShdwForegndTrans; /* F=0%,Inh */
    unsigned int ShdwPattern; /* F=0,Inh */
};

struct vdx_FontEntry
{
    GSList *children;
    char type;
    unsigned int Attributes; /* =23040,23108,4096,4160 */
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
    gboolean NoShow; /* F=Inh,NOT(Sheet.5!User.ShowFooter),No Formula,User.shapelook&lt;&gt;2 =0,1 */
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

struct vdx_Help
{
    GSList *children;
    char type;
    char * Copyright; /* =,&#xe000;,Copyright (c) 2003 Microsoft Corporation.  All rights reserved.,Copyright (c) 2003 Microsoft Corporation. Tous droits réservés.,Copyright 1999 Visio Corporation.  All rights reserved.,Copyright 2001 Microsoft Corporation.  All rights reserved. */
    char * HelpTopic; /* =,&#xe000;,Vis_D120.chm!#55640,Vis_SAD.chm!#49963,Vis_SCon.chm!#22767,Vis_SDFD.chm!#22834,Vis_SDFD.chm!#22835,Vis_SE.chm!#20000,Vis_SE.chm!#49207,Vis_SE.chm!#49271,Vis_SFB.chm!#50032,Vis_SFB.chm!#50033,Vis_SFB.chm!#50049,Vis_STQM.chm!#50381,Vis_STQM.chm!#50394,Vis_Sba.chm!#22425 */
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
    float ColorTrans; /* F=No Formula =0,0.5 */
    gboolean Glue; /* =1 */
    unsigned int IX; /* */
    gboolean Lock; /* =0 */
    char * Name; /* =Connector,Diagramme de flux,Flowchart,Lien,Lifelines,animation,viewBox */
    char * NameUniv; /* F=No Formula =&#xe000;,Connector,Flowchart,Lifelines,animation,viewBox */
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
    unsigned int BeginArrow; /* F=0,Inh =0,13,4 */
    unsigned int BeginArrowSize; /* F=1,2,Inh =0,1,2,4 */
    unsigned int EndArrow; /* F=0,Inh =0,1,12,13,4,5 */
    unsigned int EndArrowSize; /* F=1,2,Inh =0,1,2,4 */
    gboolean LineCap; /* F=0,Inh =0 */
    Color LineColor; /* F=0,1,14,15,4,HSL(0,0,240),HSL(150,240,220),HSL(150,240,229),HSL(150,240,96),Inh */
    float LineColorTrans; /* F=0%,Inh */
    unsigned int LinePattern; /* F=0,1,23,Inh */
    float LineWeight; /* F=0.0033333333333333DT,0.01DT,0PT,Inh */
    float Rounding; /* F=0DL,Inh */
};

struct vdx_LineTo
{
    GSList *children;
    char type;
    gboolean Del; /* =1 */
    unsigned int IX; /* */
    float X; /* F=Geometry1.X1,Inh,Width*0,Width*0.16666666666667,Width*0.18367346938776,Width*0.18588873812754,Width*0.20881670533643,Width*0.25,Width*0.29466357308585,Width*0.33333333333333,Width*0.40924501022906,Width*0.41292660128891,Width*0.41383989145183,Width*0.5,Width*0.54409769335143,Width*0.59294436906377,Width*0.59628770301624,Width*0.66666666666667,Width*0.69141531322506,Width*0.82700135685209,Width*0.83333333333333,Width*0.83673469387755,Width*1,Width-0.1326,Width-Geometry1.X1 */
    float Y; /* F=Geometry1.Y1,Height,Height*0,Height*0.16666666666668,Height*0.16666666666669,Height*0.29616087751371,Height*0.41133455210238,Height*0.5,Height*0.52833638025594,Height*0.6,Height*0.6654478976234,Height*0.75,Height*0.99999999999999,Height*1,Inh */
};

struct vdx_Master
{
    GSList *children;
    char type;
    unsigned int AlignName; /* =2 */
    char * BaseID; /* ={0AA6B785-DB09-499C-925F-74330DFD3FD7},{15A50647-7503-42A7-8B21-76CD97299D15},{2158C736-7D89-465E-B3D0-1D4C55D8BDC4},{2260FB21-E7DE-4F61-949E-679548813B45},{3C992ADB-9BBB-42F2-8C44-491D5E4881E7},{489687F9-D7EA-4474-B09D-47BBF5D2687D},{5BCB8E08-8CCD-4081-84D5-4992704BCB0F},{62BF39B4-4548-11D3-BD21-00C04F798E61},{71DCC243-4548-11D3-BD21-00C04F798E61},{71DCC244-4548-11D3-BD21-00C04F798E61},{7813AA6D-FF4D-4795-B39A-7FF08597892B},{7C295579-2F17-4D19-956C-9049B19A00E3},{8F95553C-6E0B-4606-90B1-5E9623A6BFE0},{91F16D4A-EF56-4817-AF42-BD4BC69C9E7C},{ABB259A0-1B93-4962-A7E4-B7C0D1A1FF86},{AF01B45C-154E-4D56-93CA-FE805FA182D9},{DAA2821A-86F5-4AC3-85AF-331CD90283A0},{E1FA9B44-3D3B-4AA9-A408-74985C2E8F8C},{EC7A7328-4547-11D3-BD21-00C04F798E61},{EFBCE221-4C44-4C3F-A68C-28015488BFF3},{F7290A45-E3AD-11D2-AE4F-006008C9F5A9},{F7E7A8A7-8385-4DA3-B598-2A8A026BD323},{FBF36E04-E186-4DD0-A762-A8A893608B7A} */
    gboolean Hidden; /* =0 */
    unsigned int ID; /* */
    gboolean IconSize; /* =1 */
    gboolean IconUpdate; /* =0,1 */
    gboolean MatchByName; /* =0,1 */
    char * Name; /* =Background cosmic,Border retro,Circuit opérations 1,Closed half arrow,Cloud,Comparaison 1,Decision,Dynamic connector,Lien ligne,Process,Terminator,animate */
    char * NameU; /* =Background cosmic,Border retro,Center drag circle,Close half arrow,Closed full arrow,Cloud,Cloud.2,Compare 1,Decision,Dynamic connector,Entity 2,Line connector,Object,Open full arrow,Open half arrow,OpenHalfArrow,Process,Terminator,TestArrow,Wavy connector 1,Work flow loop 1,animateMotion,animation,viewBox */
    unsigned int PatternFlags; /* =0,1026,2 */
    char * Prompt; /* =,Connector with multiple, adjustable curves. ,Drag onto the page to add a full-page sized background image.,Drag onto the page to add a page-sized border. Drag handles to resize.,Drag onto the page to indicate the beginning or end of a program flow.,Drag the shape onto the drawing page.,Endpoints determine center &amp; circumference of circle. "Length" is radius.,Faire glisser la forme sur la page de dessin.,Faire glisser sur la page, puis faire glisser les extrémités vers les x bleus des formes (la couleur rouge indique une connexion).,Represents a compartmentalized entity. Each compartment automatically resizes with text.,Represents an instance of an object.,This connector automatically routes between the shapes it connects. */
    char * UniqueID; /* ={002A9108-0000-0000-8E40-00608CF305B2},{019A1AE0-013A-0000-8E40-00608CF305B2},{05056CC9-000A-0000-8E40-00608CF305B2},{0508892C-0018-0000-8E40-00608CF305B2},{0508DF4B-0011-0000-8E40-00608CF305B2},{0508DFE7-0012-0000-8E40-00608CF305B2},{0EDBE7FA-0005-0000-8E40-00608CF305B2},{0EDE3C25-0007-0000-8E40-00608CF305B2},{0EEBFF9C-0006-0000-8E40-00608CF305B2},{0FB0B4DD-0004-0000-8E40-00608CF305B2},{10D81B28-000A-0000-8E40-00608CF305B2},{143D3406-0009-0000-8E40-00608CF305B2},{145F5302-0000-0000-8E40-00608CF305B2},{1FBE0BD2-0018-0000-8E40-00608CF305B2},{1FBEA284-0000-0000-8E40-00608CF305B2},{1FBEA37E-0002-0000-8E40-00608CF305B2},{1FBEB428-0012-0000-8E40-00608CF305B2},{1FC13D53-001E-0000-8E40-00608CF305B2},{1FC1468B-002B-0000-8E40-00608CF305B2},{1FEDA3F7-0010-0000-8E40-00608CF305B2},{1FEF70B8-0006-0000-8E40-00608CF305B2},{1FF7960C-0012-0000-8E40-00608CF305B2},{242EA79F-0005-0000-8E40-00608CF305B2},{4700008F-0000-0000-8E40-00608CF305B2},{6D06526D-0002-0000-8E40-00608CF305B2} */
};

struct vdx_Misc
{
    GSList *children;
    char type;
    unsigned int BegTrigger; /* F=No Formula,_XFTRIGGER(Cloud.2!EventXFMod),_XFTRIGGER(Cloud.3!EventXFMod),_XFTRIGGER(Compare 1!EventXFMod),_XFTRIGGER(Decision!EventXFMod),_XFTRIGGER(Decision.15!EventXFMod),_XFTRIGGER(Decision.17!EventXFMod),_XFTRIGGER(Decision.19!EventXFMod),_XFTRIGGER(EventXFMod),_XFTRIGGER(Process!EventXFMod),_XFTRIGGER(Process.11!EventXFMod),_XFTRIGGER(Process.14!EventXFMod),_XFTRIGGER(Process.16!EventXFMod),_XFTRIGGER(Process.18!EventXFMod),_XFTrigger(EventXFMod) =0,1,2 */
    gboolean Calendar; /* F=Inh =0 */
    char * Comment; /* F=Inh =,&#xe000; */
    gboolean DropOnPageScale; /* F=Inh =1 */
    unsigned int DynFeedback; /* F=0,Inh =0,2 */
    unsigned int EndTrigger; /* F=No Formula,_XFTRIGGER(Cloud.4!EventXFMod),_XFTRIGGER(Compare 1!EventXFMod),_XFTRIGGER(Decision!EventXFMod),_XFTRIGGER(Decision.15!EventXFMod),_XFTRIGGER(Decision.17!EventXFMod),_XFTRIGGER(Decision.19!EventXFMod),_XFTRIGGER(EventXFMod),_XFTRIGGER(Process!EventXFMod),_XFTRIGGER(Process.11!EventXFMod),_XFTRIGGER(Process.12!EventXFMod),_XFTRIGGER(Process.14!EventXFMod),_XFTRIGGER(Process.16!EventXFMod),_XFTRIGGER(Process.18!EventXFMod),_XFTRIGGER(Sheet.2!EventXFMod),_XFTRIGGER(Terminator!EventXFMod),_XFTRIGGER(Terminator.13!EventXFMod),_XFTrigger(EventXFMod) =0,1,2 */
    unsigned int GlueType; /* F=0,Inh =0,2,3 */
    gboolean HideText; /* F=FALSE,Inh,NOT(Sheet.5!User.ShowFooter) */
    gboolean IsDropSource; /* F=FALSE,Inh */
    unsigned int LangID; /* F=Inh =1033,1036 */
    gboolean LocalizeMerge; /* F=Inh =0 */
    gboolean NoAlignBox; /* F=FALSE,Inh */
    gboolean NoCtlHandles; /* F=FALSE,Inh */
    gboolean NoLiveDynamics; /* F=FALSE,Inh */
    gboolean NoObjHandles; /* F=FALSE,Inh */
    gboolean NonPrinting; /* F=FALSE,Inh,TRUE */
    unsigned int ObjType; /* F=0,Inh =0,1,12,2,4 */
    char * ShapeKeywords; /* F=Inh =,Background,backdrop,scenery,watermark,behind,cosmic,Cloud,conceptual,web,site,internet,Comparaison,comparaison,contraste,révision,diagramme de flux,audit,inspection,compte,tenue de la comptabilité,tâche,gestion,processus,Decision,Yes/No,flowchart,basic,information,flow,data,process,joiner,association,business,process,Six,6,Sigma,ISO,9000,Opérations,flux,circuit,ingénierie,TQM,diagramme de flux,flux,diagramme,total,qualité,ISO,9000,QS,gestion,gestion,processus,Process,function,task,Basic,Flowchart,flow,data,process,joiner,association,business,process,Six,6,Sigma,ISO,9000,Terminator,stop,begin,end,program,flow,Basic,Flowchart,information,data,process,joiner,association,business,process,Six,6,Sigma,ISO,9000,border,edge,margin,boundary,frame,retro,ligne,lien,TQM,diagramme de flux,flux,diagramme,total,qualité,ISO,9000,QS,gestion,gestion,processus */
    gboolean UpdateAlignBox; /* F=FALSE,Inh */
    unsigned int WalkPreference; /* F=0,Inh =0,2 */
};

struct vdx_MoveTo
{
    GSList *children;
    char type;
    unsigned int IX; /* */
    float X; /* F=-Width,Geometry1.X2,Geometry3.X2,Inh,MIN(Height/2,Width/4),Width*0,Width*0.14372094553556,Width*0.20811090826019,Width*0.22122318663661,Width*0.292343387471,Width*0.37144351436963,Width*0.5,Width*0.68060861377363,Width*1,Width-Geometry3.X2 */
    float Y; /* F=Geometry1.Y2,Geometry2.Y2,Height*0,Height*0.2159464763334,Height*0.21682159115636,Height*0.29750929829753,Height*0.39287809665782,Height*0.41133455210238,Height*0.5,Height*0.63886684381884,Height*0.6389,Height*0.67683491865304,Height*0.86234671671524,Height*0.9406482424948,Height*1,Height-Geometry1.Y2,Height/2,Inh */
};

struct vdx_NURBSTo
{
    GSList *children;
    char type;
    float A; /* =13.370618992314,4.246853645432,5.7985449361335,8.5832190019452 */
    gboolean B; /* =1 */
    gboolean C; /* =0 */
    gboolean D; /* =1 */
    float E; /* F=NURBS(12.641456682609,3,0,0,0.077695244759418,0.67702102041097,0,1,0.41485578010731,-0.11509002992772,0,1,1.0323027599339,-0.0065131415970837,0,1),NURBS(12.641456682609,3,0,0,0.12954013688023,0.85270772793162,0,1,0.39395593393987,-0.49679757603964,0,1,0.7449855631155,0.27915964030968,0,1),NURBS(12.641456682609,3,0,0,0.1495151728327,0.52393205381689,0,1,0.35559910397995,-0.25532771424142,0,1,0.70347767902184,0.12431836177192,0,1),NURBS(15.323246294838,3,0,0,0.59796464748333,0.43297049347213,0,1,0.72273318660381,-0.55548054972616,0,1,1.1063227090043,0.68877899745299,0,1,0.11521650787312,1.316794384739,9.6546937938217,1,-0.14486747941007,0.00079287474786231,11.79443879433,1),NURBS(5.3748367344281, 3, 0, 0, 0.25018539551719,-0.00091377929777925,0,1, 0.14900046028787,1.8179233105919,0,1, 0.47692727446522,0.15313817294366,0,1, 0.83211810099703,1.6391013720618,2.9621903733318,1, 0.75056178372151,0.0011555651285438,3.723294332662,1),NURBS(8.0600877078024,3,0,0,0.13468719982157,1.006171628831,0,1,0.21884039763364,1.3634557523562,0,1,0.76471353068066,-0.36248291957078,0,1,0.91085996934462,0.26020785394201,4.259231734366,1) */
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
    unsigned int BackPage; /* =4 */
    gboolean Background; /* =1 */
    unsigned int ID; /* */
    char * Name; /* =ARP Sequence,Detail,Overview,VBackground */
    char * NameU; /* =ATM Sequence,Bottom-aligned,Demo,Detail,Overview,Page-1,Page-2,Patterns,Top-aligned,Transformed,VBackground,Wrapped,animate,animateMotion */
    float ViewCenterX; /* =0,0.64955357142857,1.0133333333333,1.7838541666667,3.1979166666667,4.1041666666667,4.241935483871,4.25,5.49375,5.5,5.8425196850394,6.2677165354331,6.309375,6.5942857142857,8.4910714285714 */
    float ViewCenterY; /* =0,0.093333333333333,0.62723214285714,11.5625,14.854166666667,2.4485714285714,3.7244094488189,4.1338582677165,4.25,5.494623655914,5.5,5.5078125,7.8020833333333,9.8151041666667 */
    float ViewScale; /* =-1,0.66145833333333,1,1.5408921933086,2,3.0817610062893,3.6877828054299,4.4186795491143 */
};

struct vdx_PageLayout
{
    GSList *children;
    char type;
    float AvenueSizeX; /* F=0.29527559055118DL,0.375DL,Inh */
    float AvenueSizeY; /* F=0.29527559055118DL,0.375DL,Inh */
    float BlockSizeX; /* F=0.19685039370079DL,0.25DL,Inh */
    float BlockSizeY; /* F=0.19685039370079DL,0.25DL,Inh */
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
    float LineToLineX; /* F=0.098425196850394DL,0.125DL,Inh */
    float LineToLineY; /* F=0.098425196850394DL,0.125DL,Inh */
    float LineToNodeX; /* F=0.098425196850394DL,0.125DL,Inh */
    float LineToNodeY; /* F=0.098425196850394DL,0.125DL,Inh */
    gboolean PageLineJumpDirX; /* F=0,Inh =0 */
    gboolean PageLineJumpDirY; /* F=0,Inh =0 */
    gboolean PageShapeSplit; /* F=Inh =0,1 */
    gboolean PlaceDepth; /* F=0,Inh =0,1 */
    gboolean PlaceFlip; /* F=0,Inh =0 */
    unsigned int PlaceStyle; /* F=0,Inh */
    gboolean PlowCode; /* F=0,Inh =0 */
    gboolean ResizePage; /* F=FALSE,Inh */
    unsigned int RouteStyle; /* F=0,Inh */
};

struct vdx_PageProps
{
    GSList *children;
    char type;
    float DrawingScale; /* F=No Formula */
    gboolean DrawingScaleType; /* F=No Formula =0 */
    unsigned int DrawingSizeType; /* F=No Formula =0,1,2,3,4,5,6 */
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
    unsigned int Size; /* =1056,1064,1088,1124,1168,1192,1352,1372,21032,21048,3168,4976,752 */
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
    unsigned int PaperKind; /* F=Inh =0,1,9 */
    unsigned int PaperSource; /* F=Inh =7 */
    gboolean PrintGrid; /* F=Inh =0 */
    unsigned int PrintPageOrientation; /* =0,1,2 */
    gboolean ScaleX; /* F=Inh =1 */
    gboolean ScaleY; /* F=Inh =1 */
};

struct vdx_PrintSetup
{
    GSList *children;
    char type;
    float PageBottomMargin; /* =0.25,0.55 */
    float PageLeftMargin; /* =0.25 */
    float PageRightMargin; /* =0.25 */
    float PageTopMargin; /* =0.25 */
    unsigned int PaperSize; /* =1,262,9 */
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
    char * Format; /* F=Inh,No Formula =,&#xe000;,@,always;whenNotActive;never,auto;auto-reverse;,indefinite;,indefinite;;3,linear;discrete;paced;spline,media;indefinite;,media;indefinite;;10,media;indefinite;;10;35,media;indefinite;;5,media;indefinite;;5;20,media;indefinite;;5;20;16,media;indefinite;;5;3,none;sum,remove;freeze,replace;sum */
    unsigned int ID; /* */
    gboolean Invisible; /* F=No Formula =0 */
    char * Label; /* F=Inh =@accumulate,@additive,@attributeName,@attributeType,@begin,@by,@calcMode,@dur,@end,@fill,@from,@keyPoints,@keySplines,@keyTimes,@path,@repeatCount,@repeatDur,@restart,@rotate,@to,@values,@xlink:href,Attribute1,Attribute2,Cost,Coût,Duration,Durée,Element,Resources,Ressources,mpath,svg-element.1,svgAttribute,svgElement */
    unsigned int LangID; /* =1033,1036 */
    char * NameU; /* =Cost,Duration,Resources */
    char * Prompt; /* F=No Formula =,&#xe000;,Enter the cost associated with this process.,Enter the duration of this step.,Enter the number of people required to complete this task. */
    char * SortKey; /* F=No Formula =,&#xe000; */
    unsigned int Type; /* F=Inh,No Formula =0,1,2,4,7 */
    float Value; /* F=Inh,No Formula */
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
    gboolean LockDelete; /* F=0,Inh,NOT(ISERROR(Sheet.5!Width)),Not(IsError(Sheet.8!Width)) =0,1 */
    gboolean LockEnd; /* F=0,Inh =0 */
    gboolean LockFormat; /* F=0,Inh =0,1 */
    gboolean LockGroup; /* F=0,Inh =0,1 */
    gboolean LockHeight; /* F=0,Inh =0,1 */
    gboolean LockMoveX; /* F=0,Inh =0,1 */
    gboolean LockMoveY; /* F=0,Inh =0,1 */
    gboolean LockRotate; /* F=0,GUARD(0),Inh =0,1 */
    gboolean LockSelect; /* F=0,Inh =0 */
    gboolean LockTextEdit; /* F=0,Inh =0,1 */
    gboolean LockVtxEdit; /* F=0,Inh =0,1 */
    gboolean LockWidth; /* F=0,Inh =0,1 */
};

struct vdx_RulerGrid
{
    GSList *children;
    char type;
    unsigned int XGridDensity; /* F=8,Inh =4,8 */
    float XGridOrigin; /* F=0DL,Inh */
    float XGridSpacing; /* F=0DL,Inh */
    unsigned int XRulerDensity; /* F=32,Inh =16,32 */
    float XRulerOrigin; /* F=0DL,Inh */
    unsigned int YGridDensity; /* F=8,Inh =4,8 */
    float YGridOrigin; /* F=0DL,Inh */
    float YGridSpacing; /* F=0DL,Inh */
    unsigned int YRulerDensity; /* F=32,Inh =16,32 */
    float YRulerOrigin; /* F=0DL,Inh */
};

struct vdx_Scratch
{
    GSList *children;
    char type;
    float A; /* F=Inh,No Formula,Sheet.5!Scratch.Y1 */
    gboolean B; /* F=No Formula =0 */
    gboolean C; /* F=No Formula =0 */
    gboolean D; /* F=No Formula =0 */
    unsigned int IX; /* */
    float X; /* F=((4/9)*(Controls.X1-((8/27)*Geometry1.X1)-((1/27)*Geometry1.X4)))-((2/9)*(Controls.X2-((8/27)*Geometry1.X4)-((1/27)*Geometry1.X1))),((4/9)*(Controls.X2-((8/27)*Geometry1.X4)-((1/27)*Geometry1.X1)))-((2/9)*(Controls.X1-((8/27)*Geometry1.X1)-((1/27)*Geometry1.X4))),1MM*User.AntiScale,Inh,MIN(Scratch.Y1,MIN(Width,Height)/2) */
    float Y; /* F=((4/9)*(Controls.Y1-((8/27)*Geometry1.Y1)-((1/27)*Geometry1.Y4)))-((2/9)*(Controls.Y2-((8/27)*Geometry1.Y4)-((1/27)*Geometry1.Y1))),((4/9)*(Controls.Y2-((8/27)*Geometry1.Y4)-((1/27)*Geometry1.Y1)))-((2/9)*(Controls.Y1-((8/27)*Geometry1.Y1)-((1/27)*Geometry1.Y4))),0.5MM*User.AntiScale,Inh,Sheet.5!Scratch.X1 */
};

struct vdx_Shape
{
    GSList *children;
    char type;
    gboolean Del; /* =1 */
    unsigned int Field; /* = */
    unsigned int FillStyle; /* */
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    unsigned int Master; /* =0,2,3,4,5,6,7,8 */
    unsigned int MasterShape; /* =10,11,12,13,14,4,6,7,8,9 */
    char * Name; /* =animate,animate.12,animate.5,animate.8 */
    char * NameU; /* =Background cosmic,Border retro,Box1,Box2,Closed full arrow.16,Closed full arrow.3,Closed full arrow.34,Closed full arrow.36,Cloud,Cloud.2,Cloud.3,Cloud.4,Compare 1,Decision,Decision.15,Decision.17,Decision.19,Dynamic connector,HC_VB_12_L0_R0_T0_B0,HC_VB_12_L0_R0_T0_B0.45,HC_VC_12_L0_R0_T0_B0,HC_VC_12_L0_R0_T0_B0.37,HC_VT_12_L0_R0_T0_B0,HC_VT_12_L0_R0_T0_B0.41,HL_VB_12_L0_R0_T0_B0,HL_VC_12_L0_R0_T0_B0,HL_VT_12_L0_R0_T0_B0,HR_VB_12_L0_R0_T0_B0,HR_VC_12_L0_R0_T0_B0,HR_VT_12_L0_R0_T0_B0,Line connector,Process,Process.11,Process.12,Process.14,Process.16,Process.18,Terminator,Work flow loop 1,animateMotion,animation,animation.12,animation.5,animation.8,default,groupOfRects,h2w2,h2w4,h4w2,h4w2r45,line.arc.arc.line,openLtBluePath,openTriangle,path3,poem1,poem2,poem3,poem4,poem5,poem6,poem7,poem8,poem9,rectWithAttributes,redTriangle,view1,view2,viewBox,viewBox.11,yellowRect,zoom1,zoom2,zoomEnd */
    unsigned int Tabs; /* = */
    unsigned int TextStyle; /* */
    char * Type; /* =Group,Guide,Shape */
    char * UniqueID; /* ={0BA72DF2-5191-4CAB-9120-1B0B716454F3},{52A11EA5-F639-4EC9-BFFD-8D528CEA5BAC},{6AC00A6B-4D15-4815-9EB9-5FBE6A399A1D},{91E79894-0A48-4623-8EC1-5B0ED93C6C4A},{A83A676E-847C-4613-B008-D7D07B189300},{B73F33F1-A74B-447A-926B-F0997F1B61F1},{E956DE5A-5006-4751-B9D9-0C14A2FA16D9},{EC370BC5-A8D6-421A-A2F7-8765FE718385} */
    unsigned int cp; /* = */
    char * fld; /* =1,Friday, October 24, 2003 */
    unsigned int pp; /* = */
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
    gboolean A; /* =0 */
    unsigned int IX; /* */
    float X; /* F=Scratch.X2/(12/81),Width*1 */
    float Y; /* F=Height*0.5,Scratch.Y2/(12/81) */
};

struct vdx_SplineStart
{
    GSList *children;
    char type;
    gboolean A; /* =0 */
    gboolean B; /* =0 */
    float C; /* =3.1175 */
    unsigned int D; /* =3 */
    unsigned int IX; /* */
    float X; /* F=Scratch.X1/(12/81) =0.41010498687667 */
    float Y; /* F=Scratch.Y1/(12/81) =0.44291338582677 */
};

struct vdx_StyleProp
{
    GSList *children;
    char type;
    gboolean EnableFillProps; /* F=1 =0,1 */
    gboolean EnableLineProps; /* F=1 =0,1 */
    gboolean EnableTextProps; /* F=1 =0,1 */
    gboolean HideForApply; /* F=0,1,FALSE */
};

struct vdx_StyleSheet
{
    GSList *children;
    char type;
    unsigned int FillStyle; /* */
    unsigned int ID; /* */
    unsigned int LineStyle; /* */
    char * Name; /* =BDMain,Background Face,Background Graduated,Background Highlight,Background Shadow,Border,Border graduated,Connecteur,Connector,Flow Connector Text,Flow Normal,Flux - Bordure,Flux - Normal,Flèche - Début,Guide,Hairline,No Style,None,Normal,Text Only,Visio 00,Visio 01,Visio 02,Visio 03,Visio 10,Visio 11,Visio 12,Visio 13,Visio 20,Visio 21,Visio 22,Visio 23,Visio 50,Visio 51,Visio 52,Visio 53,Visio 70,Visio 80,Visio 90 */
    char * NameU; /* =Arrow Start,BDMain,Background Face,Background Graduated,Background Highlight,Background Shadow,Basic,Basic Shadow,Border,Border graduated,Connector,DFD Normal,Flow Border,Flow Connector Text,Flow Normal,Guide,Hairline,No Style,None,Normal,Text Only,Visio 00,Visio 01,Visio 02,Visio 03,Visio 10,Visio 11,Visio 12,Visio 13,Visio 20,Visio 21,Visio 22,Visio 23,Visio 50,Visio 51,Visio 52,Visio 53,Visio 70,Visio 80,Visio 90 */
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
    unsigned int IX; /* */
};

struct vdx_TextBlock
{
    GSList *children;
    char type;
    float BottomMargin; /* F=0DP,0DT,1PT,4PT,Inh */
    float DefaultTabStop; /* F=0.59055118110236DP,0.5DP,Inh */
    float LeftMargin; /* F=0DP,1PT,4PT,Inh */
    float RightMargin; /* F=0DP,1PT,4PT,Inh */
    unsigned int TextBkgnd; /* F=0,1+FillForegnd,2,Inh */
    float TextBkgndTrans; /* F=0%,Inh */
    gboolean TextDirection; /* F=0,Inh =0 */
    float TopMargin; /* F=0DP,0DT,1PT,4PT,Inh */
    unsigned int VerticalAlign; /* F=0,1,2,Inh =0,1,2 */
};

struct vdx_TextXForm
{
    GSList *children;
    char type;
    float TxtAngle; /* F=-Angle,GUARD(0DEG),IF(BITXOR(FlipX,FlipY),1,-1)*Angle,If(BitXOr(FlipX,FlipY),Angle,-Angle),Inh */
    float TxtHeight; /* F=GUARD(0.25IN),GUARD(TEXTHEIGHT(TheText,TxtWidth)),Height*0.20588235294118,Height*0.25,Height*0.75,Height*1,Height*2,Inh,TEXTHEIGHT(TheText,TxtWidth),TextHeight(TheText,TxtWidth) */
    float TxtLocPinX; /* F=GUARD(TxtWidth*0.5),Inh,TxtWidth*0.5 */
    float TxtLocPinY; /* F=GUARD(TxtHeight),GUARD(TxtHeight*0.5),Inh,TxtHeight*0,TxtHeight*0.5,TxtHeight*1 */
    float TxtPinX; /* F=(Controls.X2+Controls.X1)/2,Controls.Row_1,Controls.X1,GUARD(Width*0.5),Inh,SETATREF(Controls.TextPosition),Width*0,Width*0.36346305825845,Width*0.38630173108821,Width*0.38892276603657,Width*0.39914163090129,Width*0.42491699828998,Width*0.43191115085809,Width*0.4415930558769,Width*0.4462577864368,Width*0.44661248536475,Width*0.44724680691187,Width*0.4487436633527,Width*0.44956855712329,Width*0.45223864169963,Width*0.45246690208072,Width*0.46043005718141,Width*0.46045763442929,Width*0.47916666666667,Width*0.48348348348349,Width*0.48849049179668,Width*0.48849735737075,Width*0.49324324324323,Width*0.49700598802395,Width*0.49862808670418,Width*0.5,Width*0.50538922155689,Width*0.50898203592816,Width*0.50910733614041,Width*0.51165917885824,Width*0.52689898592676,Width*0.53343563408238,Width*0.53359197313641,Width*0.53363868441886,Width*0.53487840703292,Width*0.5360979405389,Width*0.53649603489521,Width*0.56461046510204,Width*0.56544530268223,Width*0.57987509222543,Width*0.60973296544045,Width*0.61363636363637,Width*0.6595898776402,Width*0.67740905963519,Width*0.69146285604222,Width*0.74307005549841,Width*0.74563224438042,Width*1.1800797307831,Width*1.2002878056589,Width*1.2058823529412 */
    float TxtPinY; /* F=(Controls.Y2+Controls.Y1)/2,Controls.Row_1.Y,Controls.Y1,GUARD(-0.125IN),GUARD(Height),Height*-0.125,Height*0.17645064457783,Height*0.19902424658414,Height*0.43505769936806,Height*0.442429709381,Height*0.5,Height*0.7338230241399,Height*4,Inh,SETATREF(Controls.TextPosition.Y) */
    float TxtWidth; /* F=GUARD(Width*1),Inh,MAX(5*Char.Size,TEXTWIDTH(TheText)),MAX(TEXTWIDTH(TheText),5*Char.Size),Max(TextWidth(TheText),5*Char.Size),Width*0.32352941176471,Width*0.36240716158303,Width*0.75279583142312,Width*0.76502266159613,Width*0.78439604626725,Width*0.7951591271096,Width*0.83333333333333,Width*0.8384706341309,Width*0.99481037924152,Width*1,Width*1.0577713334336,Width*1.0803639892393,Width*1.2,Width*1.2574850299401,Width*1.3413173652695,Width*1.3453453453454,Width*1.4187510163689,Width*1.4192356108295,Width*1.4251497005988,Width*1.5300453231923,Width*1.75,Width*1.7711260638004,Width*1.7869266950281,Width*1.9750917014319,Width*17.058719601221,Width*2.0631231064437,Width*2.3743692502086,Width*2.3919452263573,Width*2.6428639219146,Width*27.125445609219,Width*3,Width*3.0411056895814,Width*3.090687873938,Width*3.2557941179556,Width*3.8277616996012,Width*4.3076660637567,Width*5.2092705887292 */
};

struct vdx_User
{
    GSList *children;
    char type;
    unsigned int ID; /* */
    char * NameU; /* =AntiScale,Background,ControlY,Margin,PageMargin,Scale,ScaleFactor,Scheme,SchemeName,ShowFooter,shapelook,shapetype,solsh,visVersion */
    char * Prompt; /* F=No Formula =,&#xe000; */
    float Value; /* F=0.25IN*User.AntiScale,Controls.Row_1,IF(AND(User.Scale&gt;0.125,User.Scale&lt;8),1,User.Scale),Inh,Max(0,Min(Controls.Y1,Height)),No Formula,ThePage!DrawingScale/ThePage!PageScale */
};

struct vdx_VisioDocument
{
    GSList *children;
    char type;
    unsigned int DocLangID; /* =1033 */
    unsigned int EventList; /* = */
    unsigned int Masters; /* = */
    unsigned int buildnum; /* =3216,5509 */
    char * key; /* =9A489E93324ABB2E6788D708190DE55796AAC0E6D7ADA5D4AAA39868CDC95131D7251A0A1482E0B9A511B44E73D33FB18DEB2121F3688B1F9BEC889951DE584B,9F9D0DE06B95F5E9626C0D8F3FE058D8A9171702DF08E80D2568B3CD1299B9F0F04F6B4D530A8194F5E7CE8818E77CA90B41DFDD34D849AC7DA3D8C3E85DD922 */
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
    char * Document; /* =C:\Documents and Settings\pierrer\My Documents\My Shapes\Background.vss,C:\Documents and Settings\pierrer\My Documents\My Shapes\forme Pierre.VSS,C:\Program Files\Microsoft Office\Visio10\1033\Solutions\Block Diagram\Basic Shapes (US units).vss,C:\Program Files\Microsoft Office\Visio10\1033\Solutions\Flowchart\Data Flow Diagram Shapes (US units).vss,C:\Program Files\Microsoft Office\Visio10\1033\Solutions\Visio Extras\Backgrounds (US units).vss,C:\Program Files\Microsoft Office\Visio10\1033\Solutions\Visio Extras\Borders and Titles (US units).vss,C:\Program Files\Microsoft Office\Visio11\1033\ARROWS_U.VSS,C:\Program Files\Microsoft Office\Visio11\1033\BACKGR_U.VSS,C:\Program Files\Microsoft Office\Visio11\1033\BASFLO_U.VSS,C:\Program Files\Microsoft Office\Visio11\1033\BORDER_U.VSS */
    gboolean DynamicGridEnabled; /* =0,1 */
    unsigned int GlueSettings; /* =9 */
    unsigned int ID; /* */
    unsigned int Page; /* =0,2,5,7 */
    gboolean ParentWindow; /* =0 */
    gboolean ShowConnectionPoints; /* =0,1 */
    gboolean ShowGrid; /* =0,1 */
    gboolean ShowGuides; /* =1 */
    gboolean ShowPageBreaks; /* =0 */
    gboolean ShowRulers; /* =1 */
    unsigned int SnapExtensions; /* =1,34 */
    unsigned int SnapSettings; /* =295,65831,65847 */
    unsigned int StencilGroup; /* =10 */
    unsigned int StencilGroupPos; /* =0,1,2,3 */
    float TabSplitterPos; /* =0.326,0.33,0.432,0.5 */
    float ViewCenterX; /* =1.0133333333333,1.7838541666667,4.1041666666667,4.25,5.5,5.8425196850394,6.2677165354331,6.309375,6.5942857142857 */
    float ViewCenterY; /* =0.093333333333333,11.5625,14.854166666667,2.4485714285714,3.7244094488189,4.1338582677165,4.25,5.5078125,9.8151041666667 */
    float ViewScale; /* =-1,0.66145833333333,1,1.5408921933086,2,3.0817610062893,3.6877828054299 */
    unsigned int WindowHeight; /* =414,635,652,675,799,818,828,846,847,851,875,884,928 */
    int WindowLeft; /* =-195,-238,-240,-247,-4,0 */
    unsigned int WindowState; /* =1,1025,1073741824,268435456,67109889 */
    int WindowTop; /* =-1,-23,-30,-34,0,1 */
    char * WindowType; /* =Drawing,Stencil */
    unsigned int WindowWidth; /* =1022,1032,1095,1119,1288,187,232,234,239 */
};

struct vdx_Windows
{
    GSList *children;
    char type;
    unsigned int ClientHeight; /* =625,641,851,871,875,890 */
    unsigned int ClientWidth; /* =1022,1024,1095,1119,1280 */
    unsigned int Window; /* = */
};

struct vdx_XForm
{
    GSList *children;
    char type;
    float Angle; /* F=ATAN2(EndY-BeginY,EndX-BeginX),ATan2(EndY-BeginY,EndX-BeginX),GUARD(0DA),GUARD(0DEG),GUARD(ATAN2(EndY-BeginY,EndX-BeginX)),Inh */
    gboolean FlipX; /* F=GUARD(0),GUARD(FALSE),Guard(0),Inh =0 */
    gboolean FlipY; /* F=BeginY&lt;EndY)",BeginY&lt;EndY)',GUARD(0),GUARD(FALSE),Guard(0),Inh =0,1 */
    float Height; /* F=GUARD(0.25DL),GUARD(0IN),GUARD(EndY-BeginY),GUARD(Sheet.5!User.Margin*2),GUARD(Sheet.5!User.Margin*4),GUARD(TEXTHEIGHT(TheText,100)),GUARD(TEXTHEIGHT(TheText,Width)),GUARD(ThePage!PageHeight),GUARD(Width),Guard(Sheet.8!Height-Sheet.8!User.ControlY),Guard(Sheet.8!User.ControlY),Inh,Sheet.1!Height*0.071428571428571,Sheet.1!Height*0.099206349206349,Sheet.11!Height*0.375,Sheet.11!Height*0.5,Sheet.11!Height*1,Sheet.116!Height*0.057142857142857,Sheet.116!Height*0.079365079365079,Sheet.125!Height*0.3,Sheet.125!Height*0.35,Sheet.125!Height*0.69444444444444,Sheet.125!Height*0.7,Sheet.126!Height*0.15,Sheet.126!Height*0.1625,Sheet.126!Height*0.34722222222222,Sheet.126!Height*0.35,Sheet.127!Height*0.15,Sheet.127!Height*0.1625,Sheet.127!Height*0.175,Sheet.127!Height*0.34722222222222,Sheet.127!Height*0.35,Sheet.128!Height*0.15,Sheet.128!Height*0.1625,Sheet.128!Height*0.175,Sheet.128!Height*0.34722222222222,Sheet.128!Height*0.35,Sheet.129!Height*0.15,Sheet.129!Height*0.1625,Sheet.129!Height*0.175,Sheet.129!Height*0.34722222222222,Sheet.129!Height*0.35,Sheet.13!Height*0.20520231213873,Sheet.13!Height*0.20552344251766,Sheet.13!Height*0.37888198757764,Sheet.13!Height*0.86266390614217,Sheet.13!Height*0.97615606936416,Sheet.13!Height*1,Sheet.130!Height*0.11881188118812,Sheet.130!Height*0.12871287128713,Sheet.130!Height*0.27502750275028,Sheet.130!Height*0.27722772277228,Sheet.131!Height*0.099585062240664,Sheet.131!Height*0.10373443983402,Sheet.131!Height*0.10788381742739,Sheet.131!Height*0.23052097740894,Sheet.131!Height*0.23236514522822,Sheet.132!Height*0.099173553719008,Sheet.132!Height*0.10743801652893,Sheet.132!Height*0.11570247933884,Sheet.132!Height*0.22956841138659,Sheet.132!Height*0.23140495867769,Sheet.133!Height*0.075,Sheet.133!Height*0.0875,Sheet.133!Height*0.17361111111111,Sheet.133!Height*0.175,Sheet.15!Height*0.19809250957112,Sheet.153!Height*0.098630136986301,Sheet.153!Height*0.13698630136986,Sheet.17!Height*0.20520231213873,Sheet.17!Height*0.20552344251766,Sheet.17!Height*0.37888198757764,Sheet.17!Height*0.86266390614217,Sheet.17!Height*0.97615606936416,Sheet.2!Height*0.10876132930514,Sheet.2!Height*0.15105740181269,Sheet.21!Height*0.20520231213873,Sheet.21!Height*0.20552344251766,Sheet.21!Height*0.37888198757764,Sheet.21!Height*0.86266390614217,Sheet.21!Height*0.97615606936416,Sheet.22!Height*0.19809250957112,Sheet.25!Height*0.12526495527344,Sheet.25!Height*0.17397910454644,Sheet.25!Height*0.20279276420184,Sheet.25!Height*0.20311012377023,Sheet.25!Height*0.37888198757764,Sheet.25!Height*0.86266390614217,Sheet.25!Height*0.96433990626556,Sheet.29!Height*0.19809250957112,Sheet.29!Height*0.20279276420184,Sheet.29!Height*0.20311012377023,Sheet.29!Height*0.37888198757764,Sheet.29!Height*0.86266390614217,Sheet.29!Height*0.96433671850205,Sheet.3!Height*0.099483959344999,Sheet.3!Height*0.12996389891697,Sheet.3!Height*0.13817216575694,Sheet.3!Height*0.18050541516245,Sheet.33!Height*0.20279276420184,Sheet.33!Height*0.20311012377023,Sheet.33!Height*0.37888198757764,Sheet.33!Height*0.86266390614217,Sheet.33!Height*0.96433671850205,Sheet.36!Height*0.19809250957112,Sheet.37!Height*0.21005917159763,Sheet.37!Height*0.2103879026956,Sheet.37!Height*0.62111801242236,Sheet.37!Height*0.86266390614217,Sheet.37!Height*1.6051075952293,Sheet.4!Height*0.099237146738209,Sheet.4!Height*0.10374639769452,Sheet.4!Height*0.13782937046973,Sheet.4!Height*0.14409221902017,Sheet.4!Height*0.37888198757764,Sheet.4!Height*0.86266390614217,Sheet.41!Height*0.15384615384615,Sheet.41!Height*0.20520231213873,Sheet.41!Height*0.20552344251766,Sheet.41!Height*0.21367521367521,Sheet.41!Height*0.62111801242236,Sheet.41!Height*0.86266390614217,Sheet.41!Height*1.5679190751445,Sheet.43!Width*0.012650444375613+Sheet.43!Height*0.17835569816237,Sheet.43!Width*0.01278349879506+Sheet.43!Height*0.17814811098948,Sheet.45!Height*0.20279276420184,Sheet.45!Height*0.20311012377023,Sheet.45!Height*0.62111801242236,Sheet.45!Height*0.86266390614217,Sheet.45!Height*1.549508092669,Sheet.46!Height*0.16949152542373,Sheet.46!Height*0.23540489642185,Sheet.49!Height*0.11289245156981,Sheet.49!Height*0.11556446225785,Sheet.49!Height*0.11693757886143,Sheet.49!Height*0.59316770186336,Sheet.49!Height*0.86266390614217,Sheet.5!Height*0.13540489455871,Sheet.5!Height*0.37888198757764,Sheet.5!Height*0.68154114815818,Sheet.5!Height*0.86266390614217,Sheet.5!Height*0.86328545433369,Sheet.5!Height*0.95415760742145,Sheet.5!Height*0.96078370191743,Sheet.5!Height*0.97687564569339,Sheet.5!Height*0.99959368396533,Sheet.5!Height*1,Sheet.5!Width*1.4338381192427E-5+Sheet.5!Height*0.90893245628241,Sheet.50!Height*0.19809250957112,Sheet.51!Height*0.21005917159763,Sheet.51!Height*0.2103879026956,Sheet.51!Height*0.66568672118605,Sheet.53!Height*0.59316770186336,Sheet.53!Height*0.86266390614217,Sheet.55!Height*0.21005917159763,Sheet.55!Height*0.2103879026956,Sheet.55!Height*0.6660416378065,Sheet.57!Height*0.19809250957112,Sheet.57!Height*0.59316770186336,Sheet.57!Height*0.86266390614217,Sheet.59!Height*0.21005917159763,Sheet.59!Height*0.2103879026956,Sheet.59!Height*0.66582510589903,Sheet.60!Height*0.17733990147783,Sheet.60!Height*0.24630541871921,Sheet.61!Height*0.53416149068324,Sheet.61!Height*0.56521739130435,Sheet.61!Height*0.86266390614217,Sheet.63!Height*0.20520231213873,Sheet.63!Height*0.20552344251766,Sheet.63!Height*0.65101156069364,Sheet.64!Height*0.19809250957112,Sheet.65!Height*0.56521739130435,Sheet.65!Height*0.86266390614217,Sheet.67!Height*0.20520231213873,Sheet.67!Height*0.20552344251766,Sheet.67!Height*0.65101156069364,Sheet.69!Height*0.53416149068323,Sheet.69!Height*0.56521739130435,Sheet.69!Height*0.86266390614217,Sheet.7!Height*0.19809250957112,Sheet.7!Height*0.21005917159763,Sheet.7!Height*0.2103879026956,Sheet.7!Height*0.99926035502959,Sheet.71!Height*0.19809250957112,Sheet.71!Height*0.20520231213873,Sheet.71!Height*0.20552344251766,Sheet.71!Height*0.65020012505301,Sheet.75!Height*0.20279276420184,Sheet.75!Height*0.20311012377023,Sheet.75!Height*0.64273919560683,Sheet.79!Height*0.20279276420184,Sheet.79!Height*0.20311012377023,Sheet.79!Height*0.64300179695567,Sheet.8!Height*0.19809250957112,Sheet.8!Height*0.21005917159763,Sheet.8!Height*0.2103879026956,Sheet.8!Height*0.99926035502959,Sheet.83!Height*0.20279276420184,Sheet.83!Height*0.20311012377023,Sheet.83!Height*0.64266877294729,Sheet.87!Height*0.21005917159763,Sheet.87!Height*0.2103879026956,Sheet.87!Height*1.0704144917999,Sheet.9!Height*0.21005917159763,Sheet.9!Height*0.2103879026956,Sheet.9!Height*0.37888198757764,Sheet.9!Height*0.86266390614217,Sheet.9!Height*0.99926035502959,Sheet.91!Height*0.20520231213873,Sheet.91!Height*0.20552344251766,Sheet.91!Height*1.0447976878613,Sheet.95!Height*0.20279276420184,Sheet.95!Height*0.20311012377023,Sheet.95!Height*1.0339574738178,Sheet.97!Height*0.085510688836105,Sheet.97!Height*0.1187648456057,ThePage!PageHeight-4*User.PageMargin */
    float LocPinX; /* F=GUARD(Width*0),GUARD(Width*0.25),GUARD(Width*0.5),GUARD(Width*0.75),GUARD(Width*1),Inh,Width*0,Width*0.5,Width/2 */
    float LocPinY; /* F=GUARD(Height*0),GUARD(Height*0.5),GUARD(Height*1),Height,Height*0,Height*0.39287809665782,Height*0.5,Height*1,Height/2,Inh */
    float PinX; /* F=(BeginX+EndX)/2,GUARD((BeginX+EndX)/2),GUARD(-LocPinX),GUARD(0),GUARD(IF(Width&gt;Sheet.8!Width*0.75,Sheet.8!Width*0.25,Sheet.8!Width*0.5+(Sheet.8!Width*0.25)-(Width*0.5))),GUARD(MAX(Width*1.25,Sheet.10!PinX+Sheet.10!Width)),GUARD(MIN(Sheet.5!Width-Width*1.25,Sheet.5!Width-((Sheet.5!Width-Sheet.13!PinX)+Sheet.13!Width))),GUARD(Sheet.5!Width),GUARD(Sheet.5!Width+LocPinX),GUARD(Sheet.5!Width-IF(Width&gt;Sheet.11!Width*0.75,Sheet.11!Width*0.25,Sheet.11!Width*0.5+(Sheet.11!Width*0.25)-(Width*0.5))),GUARD(Width/2),Inh,Sheet.1!Width*0.44526315789474,Sheet.1!Width*0.5,Sheet.11!Width*0.11538461538462,Sheet.11!Width*0.26923076923077,Sheet.11!Width*0.84615384615385,Sheet.116!Width*0.5,Sheet.116!Width*0.54166666666667,Sheet.125!Width*0.44074074074074,Sheet.125!Width*0.45648148148148,Sheet.125!Width*0.5,Sheet.126!Width*0.05531914893617,Sheet.126!Width*0.085106382978724,Sheet.126!Width*0.42659574468085,Sheet.126!Width*0.47446808510638,Sheet.126!Width*0.48936170212766,Sheet.126!Width*0.51063829787234,Sheet.127!Width*0.095238095238095,Sheet.127!Width*0.096428571428572,Sheet.127!Width*0.47619047619048,Sheet.127!Width*0.4952380952381,Sheet.127!Width*0.5,Sheet.128!Width*0.165,Sheet.128!Width*0.1925,Sheet.128!Width*0.43125,Sheet.128!Width*0.43375,Sheet.128!Width*0.4375,Sheet.128!Width*0.5,Sheet.129!Width*0.45384615384615,Sheet.129!Width*0.47115384615385,Sheet.129!Width*0.47692307692308,Sheet.129!Width*0.47884615384615,Sheet.129!Width*0.5,Sheet.13!Width*0.49981549815498,Sheet.13!Width*0.49986530172414,Sheet.13!Width*0.5,Sheet.13!Width*0.50479704797048,Sheet.130!Width*0.104,Sheet.130!Width*0.412,Sheet.130!Width*0.472,Sheet.130!Width*0.488,Sheet.130!Width*0.49,Sheet.130!Width*0.496,Sheet.130!Width*0.5,Sheet.131!Width*0.16666666666667,Sheet.131!Width*0.16875,Sheet.131!Width*0.35416666666667,Sheet.131!Width*0.42916666666667,Sheet.131!Width*0.45208333333333,Sheet.131!Width*0.49166666666667,Sheet.131!Width*0.5,Sheet.132!Width*0.23571428571429,Sheet.132!Width*0.31666666666667,Sheet.132!Width*0.35238095238095,Sheet.132!Width*0.47619047619048,Sheet.132!Width*0.48333333333333,Sheet.132!Width*0.48571428571429,Sheet.132!Width*0.5,Sheet.133!Width*0.14722222222222,Sheet.133!Width*0.22222222222222,Sheet.133!Width*0.30277777777778,Sheet.133!Width*0.38333333333333,Sheet.133!Width*0.41666666666667,Sheet.133!Width*0.425,Sheet.133!Width*0.43888888888889,Sheet.133!Width*0.45277777777778,Sheet.133!Width*0.5,Sheet.15!Width*0.49997023809524,Sheet.15!Width*0.50014880952381,Sheet.15!Width*0.50032738095238,Sheet.153!Width*0.42986425339367,Sheet.153!Width*0.51131221719457,Sheet.17!Width*0.35867158671587,Sheet.17!Width*0.47822878228782,Sheet.17!Width*0.49043642241379,Sheet.17!Width*0.5,Sheet.2!Width*0.47421052631579,Sheet.2!Width*0.5,Sheet.21!Width*0.5,Sheet.21!Width*0.50942887931035,Sheet.21!Width*0.51808118081181,Sheet.21!Width*0.64261992619926,Sheet.22!Width*0.2821130952381,Sheet.22!Width*0.29002976190476,Sheet.22!Width*0.29377976190476,Sheet.25!Width*0.42986425339367,Sheet.25!Width*0.49981549815498,Sheet.25!Width*0.49981933643712,Sheet.25!Width*0.5,Sheet.25!Width*0.50479704797048,Sheet.25!Width*0.52126696832579,Sheet.29!Width*0.35867158671587,Sheet.29!Width*0.47822878228782,Sheet.29!Width*0.49039100721626,Sheet.29!Width*0.5,Sheet.29!Width*0.70657738095238,Sheet.29!Width*0.71026785714286,Sheet.29!Width*0.718125,Sheet.3!Width*0.42986425339367,Sheet.3!Width*0.46894736842105,Sheet.3!Width*0.5,Sheet.3!Width*0.5232915480434,Sheet.33!Width*0.5,Sheet.33!Width*0.50941608896961,Sheet.33!Width*0.51808118081181,Sheet.33!Width*0.64261992619926,Sheet.36!Width*0.50020833333333,Sheet.37!Width*0.49748743718593,Sheet.37!Width*0.49991222886541,Sheet.37!Width*0.5,Sheet.37!Width*0.50006582729144,Sheet.4!Width*0.42986425339367,Sheet.4!Width*0.49526315789474,Sheet.4!Width*0.49649446494465,Sheet.4!Width*0.5,Sheet.4!Width*0.50479704797048,Sheet.4!Width*0.52941176470588,Sheet.41!Width*0.3572864321608,Sheet.41!Width*0.48165829145729,Sheet.41!Width*0.49988948227722,Sheet.41!Width*0.5,Sheet.41!Width*0.51631578947368,Sheet.43!Width*0.4338987144005,Sheet.43!Width*0.46171081659862,Sheet.43!Width*0.48723961000411,Sheet.45!Width*0.49982486743507,Sheet.45!Width*0.5,Sheet.45!Width*0.51717544448172,Sheet.45!Width*0.64545454545455,Sheet.46!Width*0.42986425339367,Sheet.46!Width*0.52036199095023,Sheet.49!Width*0.15425531914894,Sheet.49!Width*0.49874055415617,Sheet.49!Width*0.5,Sheet.49!Width*0.84042553191489,Sheet.49!Width*0.84574468085106,Sheet.5!Width*0,Sheet.5!Width*0.35867158671587,Sheet.5!Width*0.45588235294118,Sheet.5!Width*0.46748978898473,Sheet.5!Width*0.47254029403523,Sheet.5!Width*0.47822878228782,Sheet.5!Width*0.48457934159314,Sheet.5!Width*0.48594235123491,Sheet.5!Width*0.5,Sheet.5!Width*0.50505050505051,Sheet.5!Width*0.51020408163265,Sheet.5!Width*0.93229166666667,Sheet.50!Width*0.49997023809524,Sheet.50!Width*0.50014880952381,Sheet.50!Width*0.50032738095238,Sheet.51!Width*0.50274421565124,Sheet.51!Width*0.50287356321839,Sheet.53!Width*0.35492810469666,Sheet.53!Width*0.4797596363873,Sheet.53!Width*0.5,Sheet.55!Width*0.32105592322755,Sheet.55!Width*0.32614942528736,Sheet.57!Width*0.2821130952381,Sheet.57!Width*0.29002976190476,Sheet.57!Width*0.29377976190476,Sheet.57!Width*0.5,Sheet.57!Width*0.51448269419403,Sheet.57!Width*0.64253164556962,Sheet.59!Width*0.67097701149425,Sheet.59!Width*0.6758038160324,Sheet.60!Width*0.4978947368421,Sheet.60!Width*0.5,Sheet.61!Width*0.49874055415617,Sheet.61!Width*0.5,Sheet.63!Width*0.49838203132947,Sheet.63!Width*0.4985632183908,Sheet.64!Width*0.70657738095238,Sheet.64!Width*0.71026785714286,Sheet.64!Width*0.718125,Sheet.65!Width*0.35492810469666,Sheet.65!Width*0.4797596363873,Sheet.65!Width*0.5,Sheet.67!Width*0.32985860585332,Sheet.67!Width*0.33045977011494,Sheet.69!Width*0.5,Sheet.69!Width*0.51448269419403,Sheet.69!Width*0.64253164556962,Sheet.7!Width*0.49986530172414,Sheet.7!Width*0.5,Sheet.7!Width*0.50020833333333,Sheet.71!Width*0.48618055555556,Sheet.71!Width*0.500625,Sheet.71!Width*0.51354166666667,Sheet.71!Width*0.67097701149425,Sheet.71!Width*0.67572309012251,Sheet.75!Width*0.4984081355094,Sheet.75!Width*0.4985632183908,Sheet.79!Width*0.3210308908046,Sheet.79!Width*0.32614942528736,Sheet.8!Width*0.4904806576464,Sheet.8!Width*0.5,Sheet.8!Width*0.50014880952381,Sheet.8!Width*0.50026785714286,Sheet.8!Width*0.50032738095238,Sheet.83!Width*0.67097701149425,Sheet.83!Width*0.67574953661809,Sheet.87!Width*0.49986221172802,Sheet.87!Width*0.5,Sheet.9!Width*0.5,Sheet.9!Width*0.50942887931035,Sheet.9!Width*0.51808118081181,Sheet.9!Width*0.64261992619926,Sheet.91!Width*0.49836143075171,Sheet.91!Width*0.4985632183908,Sheet.95!Width*0.49996221171758,Sheet.95!Width*0.5,Sheet.97!Width*0.5,ThePage!PageWidth*0.5 */
    float PinY; /* F=(BeginY+EndY)/2,GUARD((BeginY+EndY)/2),GUARD(0),GUARD(0.5*Sheet.11!Height),GUARD(Height/2),GUARD(MAX(Height+Sheet.5!User.Margin*0.55,Sheet.13!PinY+Sheet.13!Height)),GUARD(MIN(Sheet.5!Height-(Height+Sheet.5!User.Margin),Sheet.10!PinY-Sheet.10!Height)),GUARD(Sheet.5!Height),GUARD(Sheet.5!Height*0.5+1.2MM*Sheet.5!User.AntiScale),GUARD(Sheet.5!Height-(0.5*Sheet.9!Height)),Guard(Sheet.8!User.ControlY),Inh,Sheet.1!Height*0.049603174603175,Sheet.1!Height*0.96428571428571,Sheet.11!Height*0.1875,Sheet.11!Height*0.5,Sheet.11!Height*0.75,Sheet.116!Height*0.03968253968254,Sheet.116!Height*0.97142857142857,Sheet.125!Height*0.15,Sheet.125!Height*0.65,Sheet.125!Height*0.65277777777778,Sheet.126!Height*0.08125,Sheet.126!Height*0.31875,Sheet.126!Height*0.575,Sheet.126!Height*0.825,Sheet.126!Height*0.82638888888889,Sheet.127!Height*0.08125,Sheet.127!Height*0.31875,Sheet.127!Height*0.575,Sheet.127!Height*0.825,Sheet.127!Height*0.82638888888889,Sheet.128!Height*0.08125,Sheet.128!Height*0.33125,Sheet.128!Height*0.5625,Sheet.128!Height*0.825,Sheet.128!Height*0.82638888888889,Sheet.129!Height*0.075,Sheet.129!Height*0.325,Sheet.129!Height*0.575,Sheet.129!Height*0.81875,Sheet.129!Height*0.825,Sheet.129!Height*0.82638888888889,Sheet.13!Height*0.18944099378882,Sheet.13!Height*0.47722567287785,Sheet.13!Height*0.48916184971098,Sheet.13!Height*0.5,Sheet.13!Height*0.80434782608696,Sheet.13!Height*0.89723827874117,Sheet.13!Height*0.89739884393064,Sheet.130!Height*0.064356435643564,Sheet.130!Height*0.26732673267327,Sheet.130!Height*0.47029702970297,Sheet.130!Height*0.66336633663366,Sheet.130!Height*0.86138613861386,Sheet.130!Height*0.86248624862486,Sheet.131!Height*0.051867219917013,Sheet.131!Height*0.21991701244813,Sheet.131!Height*0.38589211618257,Sheet.131!Height*0.55186721991701,Sheet.131!Height*0.71369294605809,Sheet.131!Height*0.88381742738589,Sheet.131!Height*0.88473951129553,Sheet.132!Height*0.053719008264463,Sheet.132!Height*0.22314049586777,Sheet.132!Height*0.38842975206612,Sheet.132!Height*0.55371900826446,Sheet.132!Height*0.71900826446281,Sheet.132!Height*0.8801652892562,Sheet.132!Height*0.88429752066116,Sheet.132!Height*0.8852157943067,Sheet.133!Height*0.0375,Sheet.133!Height*0.1625,Sheet.133!Height*0.28125,Sheet.133!Height*0.4125,Sheet.133!Height*0.53125,Sheet.133!Height*0.6625,Sheet.133!Height*0.7875,Sheet.133!Height*0.9125,Sheet.133!Height*0.91319444444444,Sheet.15!Height*0.3014965534694,Sheet.15!Height*0.7217202239026,Sheet.15!Height*1.1034040438983,Sheet.153!Height*0.068493150684931,Sheet.153!Height*0.95068493150685,Sheet.17!Height*0.18944099378882,Sheet.17!Height*0.47722567287785,Sheet.17!Height*0.48916184971098,Sheet.17!Height*0.81055900621118,Sheet.17!Height*0.89723827874117,Sheet.17!Height*0.89739884393064,Sheet.2!Height*0.075528700906344,Sheet.2!Height*0.94561933534743,Sheet.21!Height*0.18944099378882,Sheet.21!Height*0.47722567287785,Sheet.21!Height*0.48916184971098,Sheet.21!Height*0.81055900621118,Sheet.21!Height*0.89723827874117,Sheet.21!Height*0.89739884393064,Sheet.22!Height*0.09904625478556,Sheet.22!Height*0.5197342607662,Sheet.22!Height*0.90095374521444,Sheet.25!Height*0.086989552273221,Sheet.25!Height*0.10155506188512,Sheet.25!Height*0.10171374166931,Sheet.25!Height*0.28509500526088,Sheet.25!Height*0.47722567287785,Sheet.25!Height*0.51890432317405,Sheet.25!Height*0.55590062111801,Sheet.25!Height*1.1770186335404,Sheet.29!Height*0.099046254785562,Sheet.29!Height*0.10155506188512,Sheet.29!Height*0.10171374166931,Sheet.29!Height*0.47722567287785,Sheet.29!Height*0.51890272929229,Sheet.29!Height*0.5197342607662,Sheet.29!Height*0.55590062111801,Sheet.29!Height*0.90095374521444,Sheet.29!Height*1.1770186335404,Sheet.3!Height*0.069086082878472,Sheet.3!Height*0.090252707581227,Sheet.3!Height*0.22651221609263,Sheet.3!Height*0.93501805054152,Sheet.33!Height*0.10155506188512,Sheet.33!Height*0.10171374166931,Sheet.33!Height*0.47722567287785,Sheet.33!Height*0.51890272929229,Sheet.33!Height*0.55590062111801,Sheet.33!Height*1.1832298136646,Sheet.36!Height*0.099046254785561,Sheet.36!Height*0.5200558131328,Sheet.36!Height*0.90095374521444,Sheet.37!Height*-0.62111801242236,Sheet.37!Height*0.31055900621118,Sheet.37!Height*0.47452125323595,Sheet.37!Height*0.47722567287785,Sheet.37!Height*0.49243918474688,Sheet.37!Height*0.49260355029586,Sheet.4!Height*-0.18322981366459,Sheet.4!Height*0.068914685234867,Sheet.4!Height*0.072046109510086,Sheet.4!Height*0.22843118411043,Sheet.4!Height*0.43788819875777,Sheet.4!Height*0.47722567287785,Sheet.4!Height*0.94812680115274,Sheet.41!Height*-0.62111801242236,Sheet.41!Height*0.10683760683761,Sheet.41!Height*0.11921965317919,Sheet.41!Height*0.31055900621118,Sheet.41!Height*0.47722567287785,Sheet.41!Height*0.89723827874117,Sheet.41!Height*0.89739884393064,Sheet.41!Height*0.92307692307692,Sheet.43!Height*-0.31112755727713,Sheet.43!Height*0.33089769562889,Sheet.43!Height*0.97456540301027,Sheet.45!Height*-0.62111801242236,Sheet.45!Height*0.10155506188512,Sheet.45!Height*0.10171374166931,Sheet.45!Height*0.31055900621118,Sheet.45!Height*0.47722567287785,Sheet.45!Height*0.83576642335767,Sheet.46!Height*0.11770244821092,Sheet.46!Height*0.38135593220339,Sheet.49!Height*0.048136645962733,Sheet.49!Height*0.058468789430713,Sheet.49!Height*0.29425517702071,Sheet.49!Height*0.47722567287785,Sheet.49!Height*0.5289467824538,Sheet.49!Height*0.66831811771691,Sheet.49!Height*0.80661322645291,Sheet.49!Height*0.94221776887108,Sheet.49!Height*0.97981366459627,Sheet.5!Height*-0.18944099378882,Sheet.5!Height*0.38661296665764,Sheet.5!Height*0.43204904320152,Sheet.5!Height*0.43788819875777,Sheet.5!Height*0.45466019591492,Sheet.5!Height*0.47722567287785,Sheet.5!Height*0.4774851197454,Sheet.5!Height*0.48884413888137,Sheet.5!Height*0.5,Sheet.5!Height*0.50020315801734,Sheet.5!Height*0.51960814904129,Sheet.5!Height*0.76555844231269,Sheet.50!Height*0.3014965534694,Sheet.50!Height*0.7217202239026,Sheet.50!Height*1.1034040438983,Sheet.51!Height*0.49243918474688,Sheet.51!Height*0.49260355029586,Sheet.51!Height*0.49704454402498,Sheet.53!Height*0.048136645962733,Sheet.53!Height*0.47722567287785,Sheet.53!Height*0.97981366459627,Sheet.55!Height*0.49243918474688,Sheet.55!Height*0.49260355029586,Sheet.55!Height*0.4972220023352,Sheet.57!Height*0.048136645962733,Sheet.57!Height*0.09904625478556,Sheet.57!Height*0.47722567287785,Sheet.57!Height*0.5197342607662,Sheet.57!Height*0.90095374521444,Sheet.57!Height*0.97981366459627,Sheet.59!Height*0.49243918474688,Sheet.59!Height*0.49260355029586,Sheet.59!Height*0.49711373638147,Sheet.60!Height*0.12315270935961,Sheet.60!Height*0.91133004926108,Sheet.61!Height*0.47722567287785,Sheet.61!Height*0.71739130434783,Sheet.61!Height*1.6645962732919,Sheet.63!Height*0.65932080924856,Sheet.63!Height*0.89723827874116,Sheet.63!Height*0.89739884393064,Sheet.64!Height*0.099046254785562,Sheet.64!Height*0.5197342607662,Sheet.64!Height*0.90095374521444,Sheet.65!Height*0.47722567287785,Sheet.65!Height*0.71739130434783,Sheet.65!Height*1.6490683229814,Sheet.67!Height*0.65932080924855,Sheet.67!Height*0.89723827874116,Sheet.67!Height*0.89739884393064,Sheet.69!Height*0.47722567287785,Sheet.69!Height*0.73291925465839,Sheet.69!Height*1.6490683229814,Sheet.7!Height*0.099046254785561,Sheet.7!Height*0.49243918474688,Sheet.7!Height*0.49260355029586,Sheet.7!Height*0.49963017751479,Sheet.7!Height*0.5200558131328,Sheet.7!Height*0.90095374521444,Sheet.71!Height*-0.3338038600214,Sheet.71!Height*0.18516850350071,Sheet.71!Height*0.65972652706887,Sheet.71!Height*0.70305741741212,Sheet.71!Height*0.89723827874116,Sheet.71!Height*0.89739884393064,Sheet.75!Height*0.10155506188512,Sheet.75!Height*0.10171374166931,Sheet.75!Height*0.34596496435372,Sheet.79!Height*0.10155506188512,Sheet.79!Height*0.10171374166931,Sheet.79!Height*0.34609626502815,Sheet.8!Height*-0.10340404389828,Sheet.8!Height*0.31728396208236,Sheet.8!Height*0.49243918474688,Sheet.8!Height*0.49260355029586,Sheet.8!Height*0.49963017751479,Sheet.8!Height*0.6985034465306,Sheet.83!Height*0.10155506188512,Sheet.83!Height*0.10171374166931,Sheet.83!Height*0.3459141060684,Sheet.87!Height*0.48047351808932,Sheet.87!Height*0.49243918474688,Sheet.87!Height*0.49260355029586,Sheet.9!Height*-0.18944099378882,Sheet.9!Height*0.43788819875777,Sheet.9!Height*0.47722567287785,Sheet.9!Height*0.49243918474688,Sheet.9!Height*0.49260355029586,Sheet.9!Height*0.49963017751479,Sheet.91!Height*0.41281310211946,Sheet.91!Height*0.89723827874116,Sheet.91!Height*0.89739884393064,Sheet.95!Height*0.10155506188512,Sheet.95!Height*0.10171374166931,Sheet.95!Height*0.55712472231038,Sheet.97!Height*0.05938242280285,Sheet.97!Height*0.95724465558195,ThePage!PageHeight*0.5 */
    gboolean ResizeMode; /* F=Inh =0 */
    float Width; /* F=GUARD(0.25DL),GUARD(1.8MM*Sheet.5!User.AntiScale),GUARD(EndX-BeginX),GUARD(Height*4.044),GUARD(MAX(12MM*Sheet.5!User.AntiScale,TEXTWIDTH(TheText))),GUARD(SQRT((EndX-BeginX)^2+(EndY-BeginY)^2)),GUARD(TEXTWIDTH(TheText,100)),GUARD(ThePage!PageWidth),Inh,SQRT((EndX-BeginX)^2+(EndY-BeginY)^2),Sheet.1!Width*0.74842105263158,Sheet.1!Width*1,Sheet.11!Width*0.23076923076923,Sheet.11!Width*0.30769230769231,Sheet.11!Width*0.53846153846154,Sheet.116!Width*0.83333333333333,Sheet.116!Width*0.87962962962963,Sheet.125!Width*0.86296296296296,Sheet.125!Width*0.89444444444444,Sheet.125!Width*1,Sheet.126!Width*0.093617021276596,Sheet.126!Width*0.14893617021277,Sheet.126!Width*0.83617021276596,Sheet.126!Width*0.92340425531915,Sheet.126!Width*0.97872340425532,Sheet.126!Width*1.0212765957447,Sheet.127!Width*0.16666666666667,Sheet.127!Width*0.16904761904762,Sheet.127!Width*0.93333333333333,Sheet.127!Width*0.96666666666667,Sheet.127!Width*1,Sheet.128!Width*0.31,Sheet.128!Width*0.355,Sheet.128!Width*0.8375,Sheet.128!Width*0.8425,Sheet.128!Width*0.875,Sheet.128!Width*1,Sheet.129!Width*0.86923076923077,Sheet.129!Width*0.90384615384615,Sheet.129!Width*0.91538461538461,Sheet.129!Width*0.91923076923077,Sheet.129!Width*1,Sheet.13!Width*0.62767527675277,Sheet.13!Width*0.87675276752768,Sheet.13!Width*0.96686422413793,Sheet.13!Width*1,Sheet.130!Width*0.176,Sheet.130!Width*0.776,Sheet.130!Width*0.904,Sheet.130!Width*0.94,Sheet.130!Width*0.944,Sheet.130!Width*0.992,Sheet.130!Width*1,Sheet.131!Width*0.29166666666667,Sheet.131!Width*0.29583333333333,Sheet.131!Width*0.66666666666667,Sheet.131!Width*0.80833333333333,Sheet.131!Width*0.8625,Sheet.131!Width*0.94166666666667,Sheet.131!Width*1,Sheet.132!Width*0.42380952380952,Sheet.132!Width*0.58571428571429,Sheet.132!Width*0.66666666666667,Sheet.132!Width*0.9047619047619,Sheet.132!Width*0.91904761904762,Sheet.132!Width*0.92380952380952,Sheet.132!Width*1,Sheet.133!Width*0.23888888888889,Sheet.133!Width*0.38888888888889,Sheet.133!Width*0.55,Sheet.133!Width*0.7,Sheet.133!Width*0.77777777777778,Sheet.133!Width*0.79444444444444,Sheet.133!Width*0.81111111111111,Sheet.133!Width*0.83888888888889,Sheet.133!Width*1,Sheet.15!Width*0.65291666666667,Sheet.15!Width*0.66875,Sheet.15!Width*0.67625,Sheet.153!Width*0.81447963800905,Sheet.153!Width*0.85972850678733,Sheet.17!Width*0.62767527675277,Sheet.17!Width*0.87675276752768,Sheet.17!Width*0.96740301724138,Sheet.17!Width*1,Sheet.2!Width*0.74842105263158,Sheet.2!Width*1,Sheet.21!Width*0.62767527675277,Sheet.21!Width*0.87675276752768,Sheet.21!Width*0.96713362068966,Sheet.21!Width*1,Sheet.22!Width*0.65291666666667,Sheet.22!Width*0.66875,Sheet.22!Width*0.67625,Sheet.25!Width*0.62767527675277,Sheet.25!Width*0.81447963800905,Sheet.25!Width*0.85972850678733,Sheet.25!Width*0.87675276752768,Sheet.25!Width*0.96731108666734,Sheet.25!Width*1,Sheet.29!Width*0.62767527675277,Sheet.29!Width*0.65291666666667,Sheet.29!Width*0.66875,Sheet.29!Width*0.67625,Sheet.29!Width*0.87675276752768,Sheet.29!Width*0.966955054533,Sheet.29!Width*1,Sheet.3!Width*0.74842105263158,Sheet.3!Width*0.81447963800905,Sheet.3!Width*0.85972850678733,Sheet.3!Width*1,Sheet.33!Width*0.62767527675277,Sheet.33!Width*0.87675276752768,Sheet.33!Width*0.96715920137113,Sheet.33!Width*1,Sheet.36!Width*0.65291666666667,Sheet.36!Width*0.66875,Sheet.36!Width*0.67625,Sheet.37!Width*0.65125628140704,Sheet.37!Width*0.89936583285431,Sheet.37!Width*0.96712145439377,Sheet.37!Width*1,Sheet.4!Width*0.62767527675277,Sheet.4!Width*0.74842105263158,Sheet.4!Width*0.81447963800905,Sheet.4!Width*0.85972850678733,Sheet.4!Width*0.87675276752768,Sheet.4!Width*1,Sheet.41!Width*0.64221105527638,Sheet.41!Width*0.74842105263158,Sheet.41!Width*0.89095477386935,Sheet.41!Width*0.96707432240038,Sheet.41!Width*1,Sheet.43!Width*0.58717955086764+Sheet.43!Height*0.1025609076296,Sheet.43!Width*0.60141874865511+Sheet.43!Height*0.10504802600224,Sheet.43!Width*0.60887229478507+Sheet.43!Height*0.10512050181772,Sheet.45!Width*0.65454545454545,Sheet.45!Width*0.91110365649111,Sheet.45!Width*0.96694509271607,Sheet.45!Width*1,Sheet.46!Width*0.81447963800905,Sheet.46!Width*0.85972850678733,Sheet.49!Width*0.30851063829787,Sheet.49!Width*0.65289672544081,Sheet.49!Width*0.90680100755668,Sheet.49!Width*1,Sheet.5!Width*0.13541666666667,Sheet.5!Width*0.62767527675277,Sheet.5!Width*0.87675276752768,Sheet.5!Width*0.91176470588234,Sheet.5!Width*0.93497957796943,Sheet.5!Width*0.97188470246981,Sheet.5!Width*0.97216588643489+Sheet.5!Height*9.1497320480209E-6,Sheet.5!Width*0.97959183673469,Sheet.5!Width*0.99999999999999,Sheet.5!Width*1,Sheet.50!Width*0.65291666666667,Sheet.50!Width*0.66875,Sheet.50!Width*0.67625,Sheet.51!Width*0.64465524685844,Sheet.51!Width*0.67241379310345,Sheet.53!Width*0.65034530194168,Sheet.53!Width*0.89942279271659,Sheet.53!Width*1,Sheet.55!Width*0.64488527998169,Sheet.55!Width*0.66379310344828,Sheet.57!Width*0.64708860759494,Sheet.57!Width*0.65291666666667,Sheet.57!Width*0.66875,Sheet.57!Width*0.67625,Sheet.57!Width*0.90318651034612,Sheet.57!Width*1,Sheet.59!Width*0.64444121850992,Sheet.59!Width*0.66379310344828,Sheet.60!Width*0.74842105263158,Sheet.60!Width*1,Sheet.61!Width*0.65289672544081,Sheet.61!Width*0.90680100755668,Sheet.61!Width*1,Sheet.63!Width*0.64475892584681,Sheet.63!Width*0.66379310344828,Sheet.64!Width*0.65291666666667,Sheet.64!Width*0.66875,Sheet.64!Width*0.67625,Sheet.65!Width*0.65034530194168,Sheet.65!Width*0.89942279271659,Sheet.65!Width*1,Sheet.67!Width*0.64484554597701,Sheet.67!Width*0.6551724137931,Sheet.69!Width*0.64708860759494,Sheet.69!Width*0.90318651034612,Sheet.69!Width*1,Sheet.7!Width*0.65291666666667,Sheet.7!Width*0.66875,Sheet.7!Width*0.67625,Sheet.7!Width*0.96740301724138,Sheet.7!Width*1,Sheet.71!Width*0.64419043311896,Sheet.71!Width*0.65291666666667,Sheet.71!Width*0.66379310344828,Sheet.71!Width*0.66875,Sheet.71!Width*0.67625,Sheet.75!Width*0.64471950782769,Sheet.75!Width*0.66379310344828,Sheet.79!Width*0.64493534482759,Sheet.79!Width*0.66379310344828,Sheet.8!Width*0.65291666666667,Sheet.8!Width*0.66875,Sheet.8!Width*0.67625,Sheet.8!Width*0.96731454677617,Sheet.8!Width*1,Sheet.83!Width*0.64454977733853,Sheet.83!Width*0.66379310344828,Sheet.87!Width*0.64467212826809,Sheet.87!Width*0.67241379310345,Sheet.9!Width*0.62767527675277,Sheet.9!Width*0.87675276752768,Sheet.9!Width*0.96713362068965,Sheet.9!Width*1,Sheet.91!Width*0.64480012700233,Sheet.91!Width*0.66379310344828,Sheet.95!Width*0.64484157696704,Sheet.95!Width*0.6551724137931,Sheet.97!Width*0.83333333333333,Sheet.97!Width*0.87962962962963,Sqrt((EndX-BeginX)^2+(EndY-BeginY)^2),ThePage!PageWidth-4*User.PageMargin */
};

struct vdx_XForm1D
{
    GSList *children;
    char type;
    float BeginX; /* F=GUARD(Sheet.10!PinX),Inh,PAR(PNT(Cloud.2!Connections.Row_2.X,Cloud.2!Connections.Row_2.Y)),PAR(PNT(Cloud.3!Connections.Row_4.X,Cloud.3!Connections.Row_4.Y)),PAR(PNT(Compare 1!Connections.X1,Compare 1!Connections.Y1)),PAR(PNT(Compare 1!Connections.X4,Compare 1!Connections.Y4)),PAR(PNT(Decision!Connections.X2,Decision!Connections.Y2)),PAR(PNT(Decision.17!Connections.X2,Decision.17!Connections.Y2)),PAR(PNT(Decision.19!Connections.X3,Decision.19!Connections.Y3)),Par(Pnt(Sheet.17!Connections.X1,Sheet.17!Connections.Y1)),Par(Pnt(Sheet.20!Connections.X1,Sheet.20!Connections.Y1)),Par(Pnt(Sheet.20!Connections.X3,Sheet.20!Connections.Y3)),Par(Pnt(Sheet.22!Connections.X1,Sheet.22!Connections.Y1)),Par(Pnt(Sheet.29!Connections.X1,Sheet.29!Connections.Y1)),Par(Pnt(Sheet.30!Connections.X1,Sheet.30!Connections.Y1)),Par(Pnt(Sheet.31!Connections.X1,Sheet.31!Connections.Y1)),Par(Pnt(Sheet.31!Connections.X6,Sheet.31!Connections.Y6)),Par(Pnt(Sheet.35!Connections.X1,Sheet.35!Connections.Y1)),Par(Pnt(Sheet.41!Connections.X6,Sheet.41!Connections.Y6)),Par(Pnt(animateMotion!Connections.X1,animateMotion!Connections.Y1)),Par(Pnt(animation!Connections.X1,animation!Connections.Y1)),Par(Pnt(animation.12!Connections.X1,animation.12!Connections.Y1)),Par(Pnt(animation.5!Connections.X1,animation.5!Connections.Y1)),Par(Pnt(animation.8!Connections.X1,animation.8!Connections.Y1)),Par(Pnt(zoom1!Connections.X1,zoom1!Connections.Y1)),Par(Pnt(zoom2!Connections.X1,zoom2!Connections.Y1)),Par(Pnt(zoomEnd!Connections.X1,zoomEnd!Connections.Y1)),Sheet.1!Width*0.097368421052631,Sheet.1!Width*0.099473684210526,Sheet.1!Width*0.099473684210527,Sheet.1!Width*0.10894736842105,Sheet.1!Width*0.11842105263158,Sheet.1!Width*0.81473684210526,Sheet.116!Width*0.020833333333334,Sheet.116!Width*0.041666666666667,Sheet.116!Width*0.0625,Sheet.116!Width*0.9375,Sheet.116!Width*1.8503717077086E-16,Sheet.15!Width*1.8503717077086E-16,Sheet.15!Width*3.7007434154172E-16,Sheet.15!Width*5.5511151231258E-16,Sheet.153!Width*0.02262443438914,Sheet.153!Width*0.026696832579186,Sheet.153!Width*0.042986425339366,Sheet.153!Width*0.063348416289593,Sheet.153!Width*0.93891402714932,Sheet.2!Width*0.1,Sheet.2!Width*0.10947368421053,Sheet.2!Width*0.11842105263158,Sheet.2!Width*0.84368421052632,Sheet.22!Width*1.8503717077086E-16,Sheet.22!Width*3.7007434154172E-16,Sheet.22!Width*5.5511151231258E-16,Sheet.25!Width*0.02262443438914,Sheet.25!Width*0.040723981900452,Sheet.25!Width*0.061085972850679,Sheet.25!Width*0.081447963800905,Sheet.25!Width*0.14253393665158,Sheet.25!Width*0.93665158371041,Sheet.29!Width*1.8503717077086E-16,Sheet.29!Width*3.7007434154172E-16,Sheet.29!Width*5.5511151231258E-16,Sheet.3!Width*0.02262443438914,Sheet.3!Width*0.034603765237974,Sheet.3!Width*0.0549657561882,Sheet.3!Width*0.075327747138426,Sheet.3!Width*0.094736842105263,Sheet.3!Width*0.099473684210527,Sheet.3!Width*0.10421052631579,Sheet.3!Width*0.11368421052632,Sheet.3!Width*0.11842105263158,Sheet.3!Width*0.13030512270404,Sheet.3!Width*0.83842105263158,Sheet.3!Width*0.93867759203896,Sheet.36!Width*1.8503717077086E-16,Sheet.36!Width*3.7007434154172E-16,Sheet.36!Width*5.5511151231258E-16,Sheet.4!Width*0.02262443438914,Sheet.4!Width*0.032579185520362,Sheet.4!Width*0.0549657561882,Sheet.4!Width*0.075327747138426,Sheet.4!Width*0.13031674208145,Sheet.4!Width*0.14736842105263,Sheet.4!Width*0.15210526315789,Sheet.4!Width*0.15421052631579,Sheet.4!Width*0.19210526315789,Sheet.4!Width*0.86736842105263,Sheet.4!Width*0.93867759203896,Sheet.41!Width*0.14736842105263,Sheet.41!Width*0.15631578947368,Sheet.41!Width*0.16578947368421,Sheet.41!Width*0.21315789473684,Sheet.41!Width*0.89052631578947,Sheet.43!Width*0.0041666666666658,Sheet.43!Width*0.0041666666666666,Sheet.46!Width*0.022624434389141,Sheet.46!Width*0.052036199095023,Sheet.46!Width*0.064253393665159,Sheet.46!Width*0.092760180995476,Sheet.46!Width*0.1131221719457,Sheet.46!Width*0.21493212669683,Sheet.46!Width*0.92760180995475,Sheet.5!Width*0,Sheet.50!Width*1.8503717077086E-16,Sheet.50!Width*3.7007434154172E-16,Sheet.50!Width*5.5511151231258E-16,Sheet.57!Width*1.8503717077086E-16,Sheet.57!Width*3.7007434154172E-16,Sheet.57!Width*5.5511151231258E-16,Sheet.60!Width*0.12368421052632,Sheet.60!Width*0.13315789473684,Sheet.60!Width*0.13789473684211,Sheet.60!Width*0.14263157894737,Sheet.60!Width*0.17578947368421,Sheet.60!Width*0.86736842105263,Sheet.64!Width*1.8503717077086E-16,Sheet.64!Width*3.7007434154172E-16,Sheet.64!Width*5.5511151231258E-16,Sheet.7!Width*1.8503717077086E-16,Sheet.7!Width*3.7007434154172E-16,Sheet.7!Width*5.5511151231258E-16,Sheet.71!Width*3.7007434154172E-15,Sheet.8!Width*1.8503717077086E-16,Sheet.8!Width*3.7007434154172E-16,Sheet.8!Width*5.5511151231258E-16,Sheet.97!Width*0.016666666666667,Sheet.97!Width*0.020833333333334,Sheet.97!Width*0.041666666666667,Sheet.97!Width*0.075000000000001,Sheet.97!Width*0.9375,Sheet.97!Width*1.8503717077086E-16,_WALKGLUE(BegTrigger,EndTrigger,WalkPreference) */
    float BeginY; /* F=GUARD(Sheet.13!PinY+Sheet.13!Height),GUARD(Sheet.8!PinY),Inh,PAR(PNT(Cloud.2!Connections.Row_2.X,Cloud.2!Connections.Row_2.Y)),PAR(PNT(Cloud.3!Connections.Row_4.X,Cloud.3!Connections.Row_4.Y)),PAR(PNT(Compare 1!Connections.X1,Compare 1!Connections.Y1)),PAR(PNT(Compare 1!Connections.X4,Compare 1!Connections.Y4)),PAR(PNT(Decision!Connections.X2,Decision!Connections.Y2)),PAR(PNT(Decision.17!Connections.X2,Decision.17!Connections.Y2)),PAR(PNT(Decision.19!Connections.X3,Decision.19!Connections.Y3)),Par(Pnt(Sheet.17!Connections.X1,Sheet.17!Connections.Y1)),Par(Pnt(Sheet.20!Connections.X1,Sheet.20!Connections.Y1)),Par(Pnt(Sheet.20!Connections.X3,Sheet.20!Connections.Y3)),Par(Pnt(Sheet.22!Connections.X1,Sheet.22!Connections.Y1)),Par(Pnt(Sheet.29!Connections.X1,Sheet.29!Connections.Y1)),Par(Pnt(Sheet.30!Connections.X1,Sheet.30!Connections.Y1)),Par(Pnt(Sheet.31!Connections.X1,Sheet.31!Connections.Y1)),Par(Pnt(Sheet.31!Connections.X6,Sheet.31!Connections.Y6)),Par(Pnt(Sheet.35!Connections.X1,Sheet.35!Connections.Y1)),Par(Pnt(Sheet.41!Connections.X6,Sheet.41!Connections.Y6)),Par(Pnt(animateMotion!Connections.X1,animateMotion!Connections.Y1)),Par(Pnt(animation!Connections.X1,animation!Connections.Y1)),Par(Pnt(animation.12!Connections.X1,animation.12!Connections.Y1)),Par(Pnt(animation.5!Connections.X1,animation.5!Connections.Y1)),Par(Pnt(animation.8!Connections.X1,animation.8!Connections.Y1)),Par(Pnt(zoom1!Connections.X1,zoom1!Connections.Y1)),Par(Pnt(zoom2!Connections.X1,zoom2!Connections.Y1)),Par(Pnt(zoomEnd!Connections.X1,zoomEnd!Connections.Y1)),Sheet.1!Height*0.15178571428571,Sheet.1!Height*0.2156887755102,Sheet.1!Height*0.33035714285714,Sheet.1!Height*0.39426020408163,Sheet.1!Height*0.50892857142857,Sheet.1!Height*0.57283163265306,Sheet.1!Height*0.6875,Sheet.1!Height*0.75140306122449,Sheet.1!Height*0.86619897959184,Sheet.1!Height*0.92997448979592,Sheet.1!Height*1,Sheet.116!Height*0.095204081632653,Sheet.116!Height*0.16336734693878,Sheet.116!Height*0.28571428571429,Sheet.116!Height*0.35387755102041,Sheet.116!Height*0.47622448979592,Sheet.116!Height*0.54438775510204,Sheet.116!Height*0.66663265306122,Sheet.116!Height*0.73479591836735,Sheet.116!Height*0.85714285714286,Sheet.116!Height*0.92530612244898,Sheet.116!Height*1,Sheet.15!Height*0.23820413583672,Sheet.15!Height*0.5289211395723,Sheet.15!Height*0.7800837196992,Sheet.153!Height*0.21923679060665,Sheet.153!Height*0.33671232876712,Sheet.153!Height*0.38356164383562,Sheet.153!Height*0.5012133072407,Sheet.153!Height*0.54788649706458,Sheet.153!Height*0.66553816046967,Sheet.153!Height*0.71238747553816,Sheet.153!Height*0.83003913894325,Sheet.153!Height*0.87671232876712,Sheet.153!Height*0.99436399217221,Sheet.153!Height*1,Sheet.2!Height*0.22507552870091,Sheet.2!Height*0.32237807509711,Sheet.2!Height*0.38821752265861,Sheet.2!Height*0.48552006905481,Sheet.2!Height*0.55135951661631,Sheet.2!Height*0.64866206301252,Sheet.2!Height*0.71450151057402,Sheet.2!Height*0.81180405697022,Sheet.2!Height*0.87783772119119,Sheet.2!Height*0.97494605092792,Sheet.2!Height*1,Sheet.22!Height*0.23820413583672,Sheet.22!Height*0.5289211395723,Sheet.22!Height*0.7800837196992,Sheet.25!Height*0.22246252762416,Sheet.25!Height*0.26160782614711,Sheet.25!Height*0.34258267223458,Sheet.25!Height*0.37367522363281,Sheet.25!Height*0.41818902023891,Sheet.25!Height*0.53025641772461,Sheet.25!Height*0.5747702143307,Sheet.25!Height*0.6868376118164,Sheet.25!Height*0.7313514084225,Sheet.25!Height*0.8434188059082,Sheet.25!Height*0.8879326025143,Sheet.25!Height*1,Sheet.29!Height*0.23820413583672,Sheet.29!Height*0.5289211395723,Sheet.29!Height*0.7800837196992,Sheet.3!Height*0.17677023642013,Sheet.3!Height*0.21834031943215,Sheet.3!Height*0.22833935018051,Sheet.3!Height*0.30414523436721,Sheet.3!Height*0.33683282100914,Sheet.3!Height*0.34461062403301,Sheet.3!Height*0.38408770169801,Sheet.3!Height*0.39079422382671,Sheet.3!Height*0.502580203275,Sheet.3!Height*0.50706549767922,Sheet.3!Height*0.54983508396388,Sheet.3!Height*0.55324909747292,Sheet.3!Height*0.66850523546827,Sheet.3!Height*0.66952037132543,Sheet.3!Height*0.71558246622974,Sheet.3!Height*0.71570397111913,Sheet.3!Height*0.83197524497163,Sheet.3!Height*0.83425261773414,Sheet.3!Height*0.87848375451264,Sheet.3!Height*0.88150749842301,Sheet.3!Height*0.99447653429603,Sheet.3!Height*1,Sheet.36!Height*0.23820413583672,Sheet.36!Height*0.5289211395723,Sheet.36!Height*0.7800837196992,Sheet.4!Height*0.17855084396871,Sheet.4!Height*0.17881261074132,Sheet.4!Height*0.22027956134265,Sheet.4!Height*0.24042815973652,Sheet.4!Height*0.30587160040435,Sheet.4!Height*0.33847809147548,Sheet.4!Height*0.35158501440922,Sheet.4!Height*0.38561573617613,Sheet.4!Height*0.41346233017703,Sheet.4!Height*0.50381426630896,Sheet.4!Height*0.52461918484973,Sheet.4!Height*0.55095191100961,Sheet.4!Height*0.58631123919308,Sheet.4!Height*0.66932765033304,Sheet.4!Height*0.69746809386579,Sheet.4!Height*0.71628808584309,Sheet.4!Height*0.75916014820914,Sheet.4!Height*0.83466382516652,Sheet.4!Height*0.87031700288184,Sheet.4!Height*0.88180146986717,Sheet.4!Height*0.93219431864965,Sheet.4!Height*1,Sheet.41!Height*0.26923076923077,Sheet.41!Height*0.36098901098901,Sheet.41!Height*0.42307692307692,Sheet.41!Height*0.51483516483517,Sheet.41!Height*0.57692307692308,Sheet.41!Height*0.66868131868132,Sheet.41!Height*0.73076923076923,Sheet.41!Height*0.82252747252747,Sheet.41!Height*0.88461538461538,Sheet.41!Height*0.97637362637363,Sheet.41!Height*1,Sheet.43!Height*0.27302930189472,Sheet.43!Height*0.79308511502752,Sheet.43!Height*1.3131409281603,Sheet.46!Height*0.29661016949153,Sheet.46!Height*0.33202179176755,Sheet.46!Height*0.40496368038741,Sheet.46!Height*0.43280871670702,Sheet.46!Height*0.47306295399516,Sheet.46!Height*0.57445520581114,Sheet.46!Height*0.61440677966102,Sheet.46!Height*0.71549636803874,Sheet.46!Height*0.75575060532688,Sheet.46!Height*0.85653753026634,Sheet.46!Height*0.90677966101695,Sheet.46!Height*1,Sheet.50!Height*0.23820413583672,Sheet.50!Height*0.5289211395723,Sheet.50!Height*0.7800837196992,Sheet.57!Height*0.23820413583672,Sheet.57!Height*0.5289211395723,Sheet.57!Height*0.7800837196992,Sheet.60!Height*0.29824067558058,Sheet.60!Height*0.40369458128079,Sheet.60!Height*0.44581280788177,Sheet.60!Height*0.55158339197748,Sheet.60!Height*0.59338494018297,Sheet.60!Height*0.69915552427868,Sheet.60!Height*0.74159042927516,Sheet.60!Height*0.84704433497537,Sheet.60!Height*0.88916256157636,Sheet.60!Height*0.99493314567206,Sheet.60!Height*1,Sheet.64!Height*0.23820413583672,Sheet.64!Height*0.5289211395723,Sheet.64!Height*0.7800837196992,Sheet.7!Height*0.23820413583672,Sheet.7!Height*0.5289211395723,Sheet.7!Height*0.7800837196992,Sheet.71!Height*0.13596093080704,Sheet.71!Height*0.52600279065664,Sheet.71!Height*0.91604465050624,Sheet.8!Height*0.23820413583672,Sheet.8!Height*0.5289211395723,Sheet.8!Height*0.7800837196992,Sheet.97!Height*0.18764845605701,Sheet.97!Height*0.28965049202579,Sheet.97!Height*0.35866983372922,Sheet.97!Height*0.460671869698,Sheet.97!Height*0.52969121140142,Sheet.97!Height*0.63169324737021,Sheet.97!Height*0.70071258907363,Sheet.97!Height*0.80271462504242,Sheet.97!Height*0.87173396674584,Sheet.97!Height*0.97373600271463,Sheet.97!Height*1,_WALKGLUE(BegTrigger,EndTrigger,WalkPreference) */
    float EndX; /* F=GUARD(Sheet.13!PinX),Inh,PAR(PNT(Cloud.4!Connections.Row_3.X,Cloud.4!Connections.Row_3.Y)),PAR(PNT(Compare 1!Connections.X2,Compare 1!Connections.Y2)),PAR(PNT(Compare 1!Connections.X3,Compare 1!Connections.Y3)),PAR(PNT(Process!Connections.X2,Process!Connections.Y2)),PAR(PNT(Process.18!Connections.X3,Process.18!Connections.Y3)),PAR(PNT(Process.18!Connections.X4,Process.18!Connections.Y4)),PAR(PNT(Sheet.2!Connections.Row_1.X,Sheet.2!Connections.Row_1.Y)),Par(Pnt(Box1!Connections.X1,Box1!Connections.Y1)),Par(Pnt(Sheet.1!Connections.X1,Sheet.1!Connections.Y1)),Par(Pnt(Sheet.17!Connections.X2,Sheet.17!Connections.Y2)),Par(Pnt(Sheet.17!Connections.X5,Sheet.17!Connections.Y5)),Par(Pnt(Sheet.2!Connections.X1,Sheet.2!Connections.Y1)),Par(Pnt(Sheet.20!Connections.X2,Sheet.20!Connections.Y2)),Par(Pnt(Sheet.20!Connections.X4,Sheet.20!Connections.Y4)),Par(Pnt(Sheet.24!Connections.X1,Sheet.24!Connections.Y1)),Par(Pnt(Sheet.31!Connections.X2,Sheet.31!Connections.Y2)),Par(Pnt(Sheet.35!Connections.X2,Sheet.35!Connections.Y2)),Par(Pnt(Sheet.41!Connections.X2,Sheet.41!Connections.Y2)),Par(Pnt(Sheet.57!Connections.X4,Sheet.57!Connections.Y4)),Par(Pnt(animateMotion!Connections.X1,animateMotion!Connections.Y1)),Par(Pnt(animation!Connections.X1,animation!Connections.Y1)),Par(Pnt(animation.8!Connections.X1,animation.8!Connections.Y1)),Par(Pnt(default!Connections.X1,default!Connections.Y1)),Par(Pnt(path3!Connections.X2,path3!Connections.Y2)),Par(Pnt(view1!Connections.X1,view1!Connections.Y1)),Par(Pnt(view2!Connections.X1,view2!Connections.Y1)),Par(Pnt(viewBox!Connections.X1,viewBox!Connections.Y1)),Par(Pnt(viewBox.11!Connections.X1,viewBox.11!Connections.Y1)),Par(Pnt(yellowRect!Connections.X1,yellowRect!Connections.Y1)),Par(Pnt(zoom1!Connections.X1,zoom1!Connections.Y1)),Par(Pnt(zoom2!Connections.X1,zoom2!Connections.Y1)),Sheet.1!Width*0.099473684210527,Sheet.1!Width*0.10894736842105,Sheet.1!Width*0.11842105263158,Sheet.1!Width*0.81473684210526,Sheet.1!Width*0.85526315789474,Sheet.116!Width*0.020833333333334,Sheet.116!Width*0.041666666666667,Sheet.116!Width*0.0625,Sheet.116!Width*0.9375,Sheet.116!Width*1,Sheet.15!Width*1,Sheet.153!Width*0.026696832579186,Sheet.153!Width*0.042986425339366,Sheet.153!Width*0.063348416289593,Sheet.153!Width*0.93891402714932,Sheet.153!Width*1,Sheet.2!Width*0.1,Sheet.2!Width*0.10947368421053,Sheet.2!Width*0.11894736842105,Sheet.2!Width*0.84368421052632,Sheet.2!Width*0.85789473684211,Sheet.22!Width*1,Sheet.25!Width*0.040723981900452,Sheet.25!Width*0.061085972850679,Sheet.25!Width*0.081447963800905,Sheet.25!Width*0.14253393665158,Sheet.25!Width*0.93665158371041,Sheet.25!Width*1,Sheet.29!Width*1,Sheet.3!Width*0.034603765237974,Sheet.3!Width*0.0549657561882,Sheet.3!Width*0.075327747138426,Sheet.3!Width*0.099473684210527,Sheet.3!Width*0.10421052631579,Sheet.3!Width*0.11368421052632,Sheet.3!Width*0.11842105263158,Sheet.3!Width*0.13030512270404,Sheet.3!Width*0.83842105263158,Sheet.3!Width*0.85263157894737,Sheet.3!Width*0.93867759203896,Sheet.3!Width*1,Sheet.36!Width*1,Sheet.4!Width*0.032579185520362,Sheet.4!Width*0.0549657561882,Sheet.4!Width*0.075327747138426,Sheet.4!Width*0.13031674208145,Sheet.4!Width*0.15210526315789,Sheet.4!Width*0.15421052631579,Sheet.4!Width*0.19210526315789,Sheet.4!Width*0.86736842105263,Sheet.4!Width*0.90526315789474,Sheet.4!Width*0.93867759203896,Sheet.4!Width*1,Sheet.41!Width*0.15631578947368,Sheet.41!Width*0.16578947368421,Sheet.41!Width*0.21315789473684,Sheet.41!Width*0.89105263157895,Sheet.41!Width*0.90526315789474,Sheet.43!Width*1.0041666666667,Sheet.46!Width*0.052036199095023,Sheet.46!Width*0.064253393665159,Sheet.46!Width*0.092760180995476,Sheet.46!Width*0.1131221719457,Sheet.46!Width*0.21493212669683,Sheet.46!Width*0.92760180995475,Sheet.46!Width*1,Sheet.5!Width*1,Sheet.50!Width*1,Sheet.57!Width*1,Sheet.60!Width*0.13315789473684,Sheet.60!Width*0.13789473684211,Sheet.60!Width*0.14263157894737,Sheet.60!Width*0.17578947368421,Sheet.60!Width*0.86736842105263,Sheet.60!Width*0.88157894736842,Sheet.64!Width*1,Sheet.7!Width*1,Sheet.71!Width*1,Sheet.8!Width*1,Sheet.97!Width*0.016666666666667,Sheet.97!Width*0.020833333333334,Sheet.97!Width*0.041666666666667,Sheet.97!Width*0.075000000000001,Sheet.97!Width*0.9375,Sheet.97!Width*1,_WALKGLUE(EndTrigger,BegTrigger,WalkPreference) */
    float EndY; /* F=GUARD(Sheet.13!PinY+Sheet.13!Height),GUARD(Sheet.8!PinY),Inh,PAR(PNT(Cloud.4!Connections.Row_3.X,Cloud.4!Connections.Row_3.Y)),PAR(PNT(Compare 1!Connections.X2,Compare 1!Connections.Y2)),PAR(PNT(Compare 1!Connections.X3,Compare 1!Connections.Y3)),PAR(PNT(Process!Connections.X2,Process!Connections.Y2)),PAR(PNT(Process.18!Connections.X3,Process.18!Connections.Y3)),PAR(PNT(Process.18!Connections.X4,Process.18!Connections.Y4)),PAR(PNT(Sheet.2!Connections.Row_1.X,Sheet.2!Connections.Row_1.Y)),Par(Pnt(Box1!Connections.X1,Box1!Connections.Y1)),Par(Pnt(Sheet.1!Connections.X1,Sheet.1!Connections.Y1)),Par(Pnt(Sheet.17!Connections.X2,Sheet.17!Connections.Y2)),Par(Pnt(Sheet.17!Connections.X5,Sheet.17!Connections.Y5)),Par(Pnt(Sheet.2!Connections.X1,Sheet.2!Connections.Y1)),Par(Pnt(Sheet.20!Connections.X2,Sheet.20!Connections.Y2)),Par(Pnt(Sheet.20!Connections.X4,Sheet.20!Connections.Y4)),Par(Pnt(Sheet.24!Connections.X1,Sheet.24!Connections.Y1)),Par(Pnt(Sheet.31!Connections.X2,Sheet.31!Connections.Y2)),Par(Pnt(Sheet.35!Connections.X2,Sheet.35!Connections.Y2)),Par(Pnt(Sheet.41!Connections.X2,Sheet.41!Connections.Y2)),Par(Pnt(Sheet.57!Connections.X4,Sheet.57!Connections.Y4)),Par(Pnt(animateMotion!Connections.X1,animateMotion!Connections.Y1)),Par(Pnt(animation!Connections.X1,animation!Connections.Y1)),Par(Pnt(animation.8!Connections.X1,animation.8!Connections.Y1)),Par(Pnt(default!Connections.X1,default!Connections.Y1)),Par(Pnt(path3!Connections.X2,path3!Connections.Y2)),Par(Pnt(view1!Connections.X1,view1!Connections.Y1)),Par(Pnt(view2!Connections.X1,view2!Connections.Y1)),Par(Pnt(viewBox!Connections.X1,viewBox!Connections.Y1)),Par(Pnt(viewBox.11!Connections.X1,viewBox.11!Connections.Y1)),Par(Pnt(yellowRect!Connections.X1,yellowRect!Connections.Y1)),Par(Pnt(zoom1!Connections.X1,zoom1!Connections.Y1)),Par(Pnt(zoom2!Connections.X1,zoom2!Connections.Y1)),Sheet.1!Height*0.15178571428571,Sheet.1!Height*0.2156887755102,Sheet.1!Height*0.33035714285714,Sheet.1!Height*0.39426020408163,Sheet.1!Height*0.50892857142857,Sheet.1!Height*0.57283163265306,Sheet.1!Height*0.6875,Sheet.1!Height*0.75140306122449,Sheet.1!Height*0.86617889030612,Sheet.1!Height*0.86619897959184,Sheet.1!Height*0.92997448979592,Sheet.1!Height*0.9299759247449,Sheet.1!Height*1,Sheet.116!Height*0.095204081632653,Sheet.116!Height*0.16336734693878,Sheet.116!Height*0.28571428571429,Sheet.116!Height*0.35387755102041,Sheet.116!Height*0.47622448979592,Sheet.116!Height*0.54438775510204,Sheet.116!Height*0.66663265306122,Sheet.116!Height*0.73479591836735,Sheet.116!Height*0.85714285714286,Sheet.116!Height*0.92530612244898,Sheet.116!Height*1,Sheet.15!Height*0.23820413583672,Sheet.15!Height*0.5289211395723,Sheet.15!Height*0.7800837196992,Sheet.153!Height*0.21923679060665,Sheet.153!Height*0.33671232876712,Sheet.153!Height*0.38356164383562,Sheet.153!Height*0.5012133072407,Sheet.153!Height*0.54788649706458,Sheet.153!Height*0.66553816046967,Sheet.153!Height*0.71238747553816,Sheet.153!Height*0.83003913894325,Sheet.153!Height*0.87671232876712,Sheet.153!Height*0.99436399217221,Sheet.153!Height*1,Sheet.2!Height*0.22507552870091,Sheet.2!Height*0.32237807509711,Sheet.2!Height*0.38821752265861,Sheet.2!Height*0.48552006905481,Sheet.2!Height*0.55135951661631,Sheet.2!Height*0.64866206301252,Sheet.2!Height*0.71450151057402,Sheet.2!Height*0.81180405697022,Sheet.2!Height*0.87779693569271,Sheet.2!Height*0.87783772119119,Sheet.2!Height*0.97494605092792,Sheet.2!Height*1,Sheet.22!Height*0.23820413583672,Sheet.22!Height*0.5289211395723,Sheet.22!Height*0.7800837196992,Sheet.25!Height*0.22246252762416,Sheet.25!Height*0.26160782614711,Sheet.25!Height*0.34258267223458,Sheet.25!Height*0.37367522363281,Sheet.25!Height*0.41818902023891,Sheet.25!Height*0.53025641772461,Sheet.25!Height*0.5747702143307,Sheet.25!Height*0.6868376118164,Sheet.25!Height*0.7313514084225,Sheet.25!Height*0.8434188059082,Sheet.25!Height*0.8879326025143,Sheet.25!Height*1,Sheet.29!Height*0.23820413583672,Sheet.29!Height*0.5289211395723,Sheet.29!Height*0.7800837196992,Sheet.3!Height*0.17677023642013,Sheet.3!Height*0.21834031943215,Sheet.3!Height*0.22833935018051,Sheet.3!Height*0.30414523436721,Sheet.3!Height*0.33683282100914,Sheet.3!Height*0.34461062403301,Sheet.3!Height*0.38408770169801,Sheet.3!Height*0.39079422382671,Sheet.3!Height*0.502580203275,Sheet.3!Height*0.50706549767922,Sheet.3!Height*0.54983508396388,Sheet.3!Height*0.55324909747292,Sheet.3!Height*0.66850523546827,Sheet.3!Height*0.66952037132543,Sheet.3!Height*0.71558246622974,Sheet.3!Height*0.71570397111913,Sheet.3!Height*0.83197524497163,Sheet.3!Height*0.83425261773414,Sheet.3!Height*0.87843501805054,Sheet.3!Height*0.87848375451264,Sheet.3!Height*0.88150749842301,Sheet.3!Height*0.99447653429603,Sheet.3!Height*1,Sheet.36!Height*0.23820413583672,Sheet.36!Height*0.5289211395723,Sheet.36!Height*0.7800837196992,Sheet.4!Height*0.17855084396871,Sheet.4!Height*0.17881261074132,Sheet.4!Height*0.22027956134265,Sheet.4!Height*0.24042815973652,Sheet.4!Height*0.30587160040435,Sheet.4!Height*0.33847809147548,Sheet.4!Height*0.35158501440922,Sheet.4!Height*0.38561573617613,Sheet.4!Height*0.41346233017703,Sheet.4!Height*0.50381426630896,Sheet.4!Height*0.52461918484973,Sheet.4!Height*0.55095191100961,Sheet.4!Height*0.58631123919308,Sheet.4!Height*0.66932765033304,Sheet.4!Height*0.69746809386579,Sheet.4!Height*0.71628808584309,Sheet.4!Height*0.75916014820914,Sheet.4!Height*0.83466382516652,Sheet.4!Height*0.87031700288184,Sheet.4!Height*0.88180146986717,Sheet.4!Height*0.93219431864965,Sheet.4!Height*1,Sheet.41!Height*0.26923076923077,Sheet.41!Height*0.36098901098901,Sheet.41!Height*0.42307692307692,Sheet.41!Height*0.51483516483517,Sheet.41!Height*0.57692307692308,Sheet.41!Height*0.66868131868132,Sheet.41!Height*0.73076923076923,Sheet.41!Height*0.82252747252747,Sheet.41!Height*0.88461538461538,Sheet.41!Height*0.97637362637363,Sheet.41!Height*1,Sheet.43!Height*-0.24702651123808,Sheet.43!Height*0.27302930189472,Sheet.43!Height*0.79308511502752,Sheet.46!Height*0.29661016949153,Sheet.46!Height*0.33202179176755,Sheet.46!Height*0.40496368038741,Sheet.46!Height*0.43280871670702,Sheet.46!Height*0.47306295399516,Sheet.46!Height*0.57445520581114,Sheet.46!Height*0.61440677966102,Sheet.46!Height*0.71549636803874,Sheet.46!Height*0.75575060532688,Sheet.46!Height*0.85653753026634,Sheet.46!Height*0.90677966101695,Sheet.46!Height*1,Sheet.50!Height*0.23820413583672,Sheet.50!Height*0.5289211395723,Sheet.50!Height*0.7800837196992,Sheet.57!Height*0.23820413583672,Sheet.57!Height*0.5289211395723,Sheet.57!Height*0.7800837196992,Sheet.60!Height*0.29824067558058,Sheet.60!Height*0.40369458128079,Sheet.60!Height*0.44581280788177,Sheet.60!Height*0.55158339197748,Sheet.60!Height*0.59338494018297,Sheet.60!Height*0.69915552427868,Sheet.60!Height*0.74159042927516,Sheet.60!Height*0.84704433497537,Sheet.60!Height*0.88916256157636,Sheet.60!Height*0.99493314567206,Sheet.60!Height*1,Sheet.64!Height*0.23820413583672,Sheet.64!Height*0.5289211395723,Sheet.64!Height*0.7800837196992,Sheet.7!Height*0.23820413583672,Sheet.7!Height*0.5289211395723,Sheet.7!Height*0.7800837196992,Sheet.71!Height*-0.12406697575936,Sheet.71!Height*0.26597488409024,Sheet.71!Height*0.65601674393984,Sheet.8!Height*0.23820413583672,Sheet.8!Height*0.5289211395723,Sheet.8!Height*0.7800837196992,Sheet.97!Height*0.18764845605701,Sheet.97!Height*0.28965049202579,Sheet.97!Height*0.35866983372922,Sheet.97!Height*0.460671869698,Sheet.97!Height*0.52969121140142,Sheet.97!Height*0.63169324737021,Sheet.97!Height*0.70071258907363,Sheet.97!Height*0.80271462504242,Sheet.97!Height*0.87173396674584,Sheet.97!Height*0.97373600271463,Sheet.97!Height*1,_WALKGLUE(EndTrigger,BegTrigger,WalkPreference) */
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
