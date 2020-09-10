%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.di.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.21.2"

\header {
  lsrtags = "headword"

  texidoc = "
Text headword

"
  doctitle = "Text headword"
} % begin verbatim

%% http://lsr.di.unimi.it/LSR/Item?id=829
%% see also http://lilypond.org/doc/v2.18/Documentation/notation/text

% NR 1.8 Text

% L. v. Beethoven, Op. 110
% Piano sonata 31
% measures 1 - 7 (following Henle Urtext edition)

\include "english.ly"

\new PianoStaff <<
  % upper staff
  \new Staff {
    \clef treble
    \key af \major
    \time 3/4
    \tempo "Moderato cantabile molto espressivo"

    <c'' af'>4.( <af' ef'>8 ) q8.[ q16] |
    <df'' g'>4 <bf' g'>2 |
    <af' ef''>4.( <af' df''>8[) <af' ef''>-.( <af' f''>-.)] |
    << { ef''8.[( d''16]) df''8\trill\fermata ~
           \oneVoice df''32[ c'' df'' ef''] }\\
       { g'4 g'8 s } >>
     \grace { df''32[ ef''] } f''8[ ef''16 df''] |
%
% 5
%
    c''4.( ef''4 af''8) |
    af''4( g''2) |
    bf''4.( g''4 ef''8) |
  }


  \new Dynamics {
    s2.-\tweak padding #-1
       -\tweak baseline-skip #0
       -\markup \center-column {
                  \line { \dynamic p \italic { con amabilità } }
                  \line { \hspace #3 (sanft) } } |
    s2. |
    s2.\< |
    s8..\p s32\< s16..\> s64\! s8 s4\> |
%
% 5
%
    s2.*3\! |
  }


  % lower staff
  \new Staff {
    \clef bass
    \key af \major
    \time 3/4

    <af, ef>4. \stemUp <c ef>8 q8.[ q16] \stemNeutral |
    <bf, ef>4 <df ef>2 |
    << { ef8[( af c' bf) c' df'] } \\
       { c4.( f8[) ef8-.( df-.]) } >> |
    <ef bf>4 q8_\fermata r r4 | \clef treble
%
% 5
%
    af16[ <c' ef'> q q] af[ <c' ef'> q q] af[ <c' ef'> q q] |
    bf16[ <df' ef'> q q] bf[ <df' ef'> q q] bf[ <df' ef'> q q] |
    df'16[ <ef' g' bf'> q q] df'[ <ef' g' bf'> q q]
      df'[ <ef' g' bf'> q q] |
  }
>>
