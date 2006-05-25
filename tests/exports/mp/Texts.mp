% Metapost TeX macro
% Title: Texts.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:21 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(3.050000,-6.561719)(15.615625,-1.502344)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% set_line_color 0.000000, 0.000000, 0.000000
  label.rt(btex {\usefont{OT1}{phv}{m}{it} This is a string in sans italic 0.8} etex scaled 2.4,(8.000000x,2.000000y));
% set_line_color 0.000000, 0.000000, 0.000000
  label.lft(btex {\usefont{OT1}{cmtt}{m}{n} This is a string in monospace 0.4} etex scaled 1.2,(8.000000x,4.000000y));
% set_line_color 0.000000, 0.000000, 0.000000
  label(btex {\usefont{OT1}{cmr}{b}{n} This is a string in serif bold 1.0} etex scaled 3,(8.000000x,6.000000y));
endfig;
end;
