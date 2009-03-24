%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.1"

\header {
  lsrtags = "text, vocal-music"

  texidoc = "
To format individual syllables in lyrics, use @code{\\markup @{ ....
@}} on these lyrics.

"
  doctitle = "Formatting lyrics syllables"
} % begin verbatim

% Tip taken from http://lists.gnu.org/archive/html/lilypond-user/2007-12/msg00215.html
\header {
  title = "Markup can be used inside lyrics!"
}

mel = \relative c'' { c4 c c c }
lyr = \lyricmode {
  Lyrics \markup { \italic "can" } \markup {\with-color #red "contain" }
  \markup {\fontsize #8 \bold "Markup!" }
}

<<
  \context Voice = melody \mel
  \context Lyrics \lyricsto melody \lyr
>>

