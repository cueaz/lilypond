%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.1"

\header {
  lsrtags = "simultaneous-notes, chords"

  texidoc = "
Here is a way to display a chord where the same note is played twice
with different accidentals.

"
  doctitle = "Displaying complex chords"
} % begin verbatim

fixA = {
  \once \override Stem #'length = #9
  \once \override Accidental #'extra-offset = #'(0.3 . 0)
}
fixB = {
  \once \override NoteHead #'extra-offset = #'(1.7 . 0)
  \once \override Stem #'rotation = #'(45 0 0)
  \once \override Stem #'extra-offset = #'(-0.2 . -0.2)
  \once \override Stem #'flag-style = #'no-flag
  \once \override Accidental #'extra-offset = #'(3.1 . 0)
}

\relative c' {
  << { \fixA <b d!>8 } \\ { \voiceThree \fixB dis } >> s
}
