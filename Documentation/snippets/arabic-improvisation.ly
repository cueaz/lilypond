%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.di.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.21.2"

\header {
  lsrtags = "world-music"

  texidoc = "
For improvisations or @emph{taqasim} which are temporarily free, the
time signature can be omitted and @code{\\cadenzaOn} can be
used.  Adjusting the accidental style might be required, since the
absence of bar lines will cause the accidental to be marked only
once.  Here is an example of what could be the start of a @emph{hijaz}
improvisation:

"
  doctitle = "Arabic improvisation"
} % begin verbatim

\include "arabic.ly"

\relative sol' {
  \key re \kurd
  \accidentalStyle forget
  \cadenzaOn
  sol4 sol sol sol fad mib sol1 fad8 mib re4. r8 mib1 fad sol
}
