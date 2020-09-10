%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.di.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.21.2"

\header {
  lsrtags = "expressive-marks, tweaks-and-overrides"

  texidoc = "
Caesura marks can be created by overriding the @code{'text} property of
the @code{BreathingSign} object.

A curved caesura mark is also available.

"
  doctitle = "Inserting a caesura"
} % begin verbatim

\relative c'' {
  \override BreathingSign.text = \markup {
    \musicglyph "scripts.caesura.straight"
  }
  c8 e4. \breathe g8. e16 c4

  \override BreathingSign.text = \markup {
    \musicglyph "scripts.caesura.curved"
  }
  g8 e'4. \breathe g8. e16 c4
}
