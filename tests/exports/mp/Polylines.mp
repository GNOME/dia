% Metapost TeX macro
% Title: Polylines.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:18 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(2.106827,-13.000000)(20.156535,-3.104625)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% set_linewidth 0.100000
%metapost_arc
% center->x = 2.634256% center->y = 8.215782% width = 0.600000% height = 0.600000% angle1 = 149.708304% angle2 = 261.914590% set_line_color 1.000000, 0.000000, 0.000000
  draw (2.375216x,8.064461y)..(2.364187x,8.346405y)..(2.592062x,8.512800y)
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.0000, 0.0000);
% set_line_color 1.000000, 0.000000, 0.000000
  draw (4.822579x,3.874908y)--(2.375216x,8.064461y)
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.0000, 0.0000);
% set_line_color 1.000000, 0.000000, 0.000000
  draw (2.592062x,8.512800y)--(20.100000x,11.000000y)
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.0000, 0.0000);
% set_line_color 1.000000, 0.000000, 0.000000
% fill_polygon
  path p;
  p = (5.038446x,4.001008y)--(5.074780x,3.443173y)--(4.606711x,3.748807y)--cycle;
  fill p 
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.0000, 0.0000);
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (6.200000x,13.000000y)--(9.400000x,7.900000y)--(19.150000x,9.900000y)--(14.403553x,5.153553y)
    withpen pencircle scaled 0.0000x
    dashed dashpattern (on 1.600000x off 0.480000x on 0.080000x off 0.480000x on 0.080000x off 0.480000x);
% set_line_color 1.000000, 1.000000, 1.000000
% fill_polygon
  path p;
  p = (14.050000x,4.800000y)--(14.403553x,4.800000y)--(14.403553x,5.153553y)--(14.050000x,5.153553y)--cycle;
  fill p 
    withpen pencircle scaled 0.0000x
    withcolor (1.0000, 1.0000, 1.0000);
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (14.050000x,4.800000y)--(14.403553x,4.800000y)--(14.403553x,5.153553y)--(14.050000x,5.153553y)--cycle
    withpen pencircle scaled 0.0000x;
endfig;
end;
