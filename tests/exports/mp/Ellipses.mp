% Metapost TeX macro
% Title: Ellipses.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:12 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(3.095000,-9.700000)(11.700000,-2.300000)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
  path p;
  p = (11.700000x,3.550000y)..(8.325000x,4.800000y)..(4.950000x,3.550000y)..(8.325000x,2.300000y)..cycle;
  fill p withcolor (0.000000,0.000000,1.000000);
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (11.700000x,3.550000y)..(8.325000x,4.800000y)..(4.950000x,3.550000y)..(8.325000x,2.300000y)..cycle
    withpen pencircle scaled 0.0000x;
% set_linewidth 0.110000
% set_line_color 1.000000, 0.000000, 0.000000
  draw (7.150000x,6.100000y)..(5.150000x,9.500000y)..(3.150000x,6.100000y)..(5.150000x,2.700000y)..cycle
    withpen pencircle scaled 0.1100x
    withcolor (1.0000, 0.0000, 0.0000)
    dashed dashpattern (on 1.000000x off 0.300000x on 0.050000x off 0.300000x on 0.050000x off 0.300000x);
  path p;
  p = (11.550000x,6.625000y)..(8.475000x,9.700000y)..(5.400000x,6.625000y)..(8.475000x,3.550000y)..cycle;
  fill p withcolor (1.000000,1.000000,1.000000);
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (11.550000x,6.625000y)..(8.475000x,9.700000y)..(5.400000x,6.625000y)..(8.475000x,3.550000y)..cycle
    withpen pencircle scaled 0.0000x
    dashed dashpattern (on 3.000000x off 3.000000x);
endfig;
end;
