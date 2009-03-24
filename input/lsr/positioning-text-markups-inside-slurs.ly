%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.1"

\header {
  lsrtags = "expressive-marks, editorial-annotations, tweaks-and-overrides"

  texidoces = "

Los elementos de marcado de texto deben tener la propiedad
@code{outside-staff-priority} establecida al valor falso para que se
impriman por dentro de las ligaduras de expresión.

"
  doctitlees = "Situar los elementos de marcado de texto por dentro de las ligaduras"

%% Translation of GIT committish :0364058d18eb91836302a567c18289209d6e9706
  texidocde = "
Textbeschriftung kann innerhalb von Bögen gesetzt werden, wenn die
@code{outside-staff-priority}-Eigenschaft auf falsch gesetzt wird.

"
  doctitlede = "Textbeschriftung innerhalb von Bögen positionieren"

  texidoc = "
Text markups need to have the @code{outside-staff-priority} property
set to false in order to be printed inside slurs. 

"
  doctitle = "Positioning text markups inside slurs"
} % begin verbatim

\relative c'' {
  \override TextScript #'avoid-slur = #'inside
  \override TextScript #'outside-staff-priority = ##f
  c2(^\markup { \halign #-10 \natural } d4.) c8
}

