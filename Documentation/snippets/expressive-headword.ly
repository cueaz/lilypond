%% DO NOT EDIT this file manually; it was automatically
%% generated from the LilyPond Snippet Repository
%% (http://lsr.di.unimi.it).
%%
%% Make any changes in the LSR itself, or in
%% `Documentation/snippets/new/`, then run
%% `scripts/auxiliar/makelsr.pl`.
%%
%% This file is in the public domain.

\version "2.24.0"

\header {
  lsrtags = "headword"

  texidoc = "
Expressive headword
"

  doctitle = "Expressive headword"
} % begin verbatim


% NR 1.3 Expressive marks

% L. v. Beethoven, Op. 49 no. 1
% Piano sonata 19 - "Leichte Sonate"
% measures 1 - 12

\include "english.ly"

\new PianoStaff <<

   % RH Staff
   \new Staff {
      \clef treble
      \key g \major
      \time 6/8
      \partial 2
      \once \override TextScript.padding = #2
      d'8 \staccato
      ^ \markup { \column {
         RONDO
         \italic Allegro } }
      d'8 \staccato
      g'8 \staccato
      a'8 \staccato

      |

      b'8 [ (
      g'8 ] )
      e'8 \staccato
      e' \staccato
      a'8 \staccato
      b'8 \staccato

      |

      c''8 [ (
      a'8 ] )
      e''8 \staccato
      d''8 \staccato
      c''8 \staccato
      b'8 \staccato

      |

      a'8 \staccato
      g'8 \staccato
      a'8 \staccato
      \acciaccatura { g'16 [ a'16 ] }
      bf'8
      a'8 \staccato
      g'8 \staccato

      |

      fs'8 [ (
      d'8 ] )
      d'8 \staccato
      d'8 \staccato
      g'8 \staccato
      a'8 \staccato

      |

      b'8 [ (
      g'8 ] )
      e'8 \staccato
      e'8 \staccato
      a'8 \staccato
      b'8 \staccato

      |

      c''8 [ (
      a'8 ] )
      e''8 \staccato
      d''8 \staccato
      c''8 \staccato
      b'8 \staccato

      |

      a'8 \staccato
      g'8 \staccato
      a'8 \staccato
      <<
         {
            \voiceOne
            d'8
            g'8
            fs'8
            \oneVoice
         }
         \new Voice {
            \voiceTwo
            d'4
            c'8
            \oneVoice
         }
      >>

      |

      <b g'>4 \tenuto
      d'8 \staccato
      g'8 \staccato
      b'8 \staccato
      d''8 \staccato

      |

      d''8 (
      <c'' a'>8 \staccato )
      <c'' a'>8 \staccato
      d''8 (
      <b' g'>8 \staccato )
      <b' g'>8 \staccato

      |

      d''8 (
      <c'' a'>8 \staccato )
      <c'' a'>8 \staccato
      d''8 (
      <b' g'>8 \staccato )
      <b' g'>8 \staccato

      |

      d''8 \staccato
      <c'' a'>8 \staccato
      <b' g'>8 \staccato
      d'' \staccato
      <c'' a'>8 \staccato
      <b' g'>8 \staccato

      |

      <d'' c'' a'>4 \fermata
      r8 r4 r8
   }

   % LH Staff
   \new Staff {
      \clef bass
      \key g \major
      \time 6/8
      \partial 2
      r8
      r8
      <d' b>8 \staccato
      <c' a>8 \staccato

      |

      <b g>4
      r8
      r8
      <e' c'>8 \staccato
      <d' b>8 \staccato

      |

      <c' a>4
      r8
      r8
      <a fs>8 \staccato
      <b g>8 \staccato

      |

      <c' a>8 \staccato
      <b d'>8 \staccato
      <e' c'>8 \staccato
      <e' cs'>4. (

      |

      d'4 )
      r8
      r8
      <d' b!>8 \staccato
      <c'! a>8 \staccato

      |

      <b g>4
      r8
      r8
      <e' c'>8 \staccato
      <d' b>8 \staccato

      |

      <c' a>4
      r8
      r8
      <a fs>8 \staccato
      <b g>8 \staccato

      |

      <c' a>8 \staccato
      <d' b>8 \staccato
      <e' c'>8 \staccato
      <b d>4
      <a d>8 \staccato

      |

      <g g,>4 \tenuto
      r8
      r4
      r8

      |

      r8
      <d' fs>8 \staccato
      <d' fs>8 \staccato
      r8
      <d' g>8 \staccato
      <d' g>8 \staccato

      |

      r8
      <d' fs>8 \staccato
      <d' fs>8 \staccato
      r8
      <d' g>8 \staccato
      <d' g>8 \staccato

      |

      r8
      <d' fs>8 \staccato
      <d' g>8 \staccato
      r8
      <d' fs>8 \staccato
      <d' g>8 \staccato

      |

      <d' fs>4 \fermata
      r8 r4 r8
   }
>>
