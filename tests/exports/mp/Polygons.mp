% Metapost TeX macro
% Title: Polygons.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:17 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(1.850000,-12.140511)(11.740369,-3.400000)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% set_linewidth 0.000000
% set_line_color 1.000000, 0.000000, 0.000000
% fill_polygon
  path p;
  p = (6.900000x,5.650000y)--(7.000000x,3.400000y)--(8.900000x,5.150000y)--(9.700000x,7.800000y)--(3.300000x,6.800000y)--cycle;
  fill p 
    withpen pencircle scaled 0.0000x
    withcolor (1.0000, 0.0000, 0.0000);
% set_line_color 0.000000, 0.000000, 0.000000
  draw (6.900000x,5.650000y)--(7.000000x,3.400000y)--(8.900000x,5.150000y)--(9.700000x,7.800000y)--(3.300000x,6.800000y)--cycle
    withpen pencircle scaled 0.0000x;
% set_linewidth 0.130000
% set_line_color 0.000000, 1.000000, 0.000000
  draw (10.900000x,12.000000y)--(7.250000x,5.350000y)--(11.650000x,7.600000y)--(11.650000x,7.600000y)--(6.550000x,6.650000y)--(6.300000x,9.900000y)--cycle
    withpen pencircle scaled 0.1300x
    withcolor (0.0000, 1.0000, 0.0000)
    dashed dashpattern (on 1.000000x off 0.475000x on 0.050000x off 0.475000x);
% set_linewidth 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% fill_polygon
  path p;
  p = (1.850000x,6.600000y)--(4.950000x,5.100000y)--(7.800000x,11.500000y)--cycle;
  fill p 
    withpen pencircle scaled 0.0000x
    withcolor (1.0000, 1.0000, 1.0000);
% set_line_color 0.000000, 0.000000, 0.000000
  draw (1.850000x,6.600000y)--(4.950000x,5.100000y)--(7.800000x,11.500000y)--cycle
    withpen pencircle scaled 0.0000x;
endfig;
end;
