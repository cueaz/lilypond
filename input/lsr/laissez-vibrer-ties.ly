%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.11.53"

\header {
  lsrtags = "expressive-marks, keyboards, fretted-strings"

  texidoc = "
Laissez vibrer ties have a fixed size. Their formatting can be tuned
using @code{'tie-configuration}. 

"
  doctitle = "Laissez vibrer ties"
} % begin verbatim
\relative c' {
  <c e g>4\laissezVibrer r <c f g>\laissezVibrer r
  <c d f g>4\laissezVibrer r <c d f g>4.\laissezVibrer r8

  <c d e f>4\laissezVibrer r
  \override LaissezVibrerTieColumn #'tie-configuration
     = #`((-7 . ,DOWN)
          (-5 . ,DOWN)
          (-3 . ,UP)
          (-1 . ,UP))
  <c d e f>4\laissezVibrer r
}
