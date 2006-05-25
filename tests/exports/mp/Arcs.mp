% Metapost TeX macro
% Title: Arcs.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:03 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(4.107426,-11.254441)(15.959577,-2.943983)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% set_linewidth 0.080000
%metapost_arc
% center->x = 10.502208% center->y = 5.035582% width = 10.841850% height = 10.841850% angle1 = 166.781024% angle2 = 287.696139% set_line_color 1.000000, 0.000000, 0.000000
  draw (5.224919x,3.795961y)..(6.821686x,9.015556y)..(12.150000x,10.200000y)
    withpen pencircle scaled 0.0800x
    withcolor (1.0000, 0.0000, 0.0000)
    dashed dashpattern (on 0.050000x off 0.050000x);
% set_line_color 1.000000, 0.000000, 0.000000
% fill_polygon
  path p;
  p = (5.322189x,3.435009y)--(5.404326x,3.987959y)--(5.205586x,3.791420y)--(4.929111x,3.832489y)--cycle;
  fill p 
    withpen pencircle scaled 0.0800x
    withcolor (1.0000, 0.0000, 0.0000);
% set_line_color 1.000000, 0.000000, 0.000000
  draw (5.322189x,3.435009y)--(5.404326x,3.987959y)--(5.205586x,3.791420y)--(4.929111x,3.832489y)--cycle
    withpen pencircle scaled 0.0800x
    withcolor (1.0000, 0.0000, 0.0000);
% set_linewidth 0.000000
%metapost_arc
% center->x = 17.173216% center->y = 15.594670% width = 26.562500% height = 26.562500% angle1 = 112.223383% angle2 = 151.812467% set_line_color 0.000000, 0.000000, 0.000000
  draw (12.150000x,3.300000y)..(8.283238x,5.727558y)..(5.467039x,9.321152y)
    withpen pencircle scaled 0.0000x;
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (11.687142x,3.489109y)--(12.244555x,3.531429y)
    withpen pencircle scaled 0.0000x;
% set_line_color 0.000000, 0.000000, 0.000000
  draw (11.687142x,3.489109y)--(12.055445x,3.068571y)
    withpen pencircle scaled 0.0000x;
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (11.781696x,3.720538y)--(11.592587x,3.257680y)
    withpen pencircle scaled 0.0000x;
% set_line_color 1.000000, 1.000000, 1.000000
% fill_polygon
  path p;
  p = (4.950000x,10.400000y)--(4.632475x,9.613121y)--(5.419354x,9.295596y)--(5.736879x,10.082475y)--cycle;
  fill p 
    withpen pencircle scaled 0.0000x
    withcolor (1.0000, 1.0000, 1.0000);
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (4.950000x,10.400000y)--(4.632475x,9.613121y)--(5.419354x,9.295596y)--(5.736879x,10.082475y)--cycle
    withpen pencircle scaled 0.0000x;
% set_linewidth 0.000000
%metapost_arc
% center->x = 13.322977% center->y = 6.763539% width = 5.273200% height = 5.273200% angle1 = 289.428723% angle2 = 270.587237% set_line_color 0.000000, 0.000000, 0.000000
  draw (14.200000x,9.250000y)..(12.864775x,4.167058y)..(13.350000x,9.400000y)
    withpen pencircle scaled 0.0000x;
endfig;
end;
