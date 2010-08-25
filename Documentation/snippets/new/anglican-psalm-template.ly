
\version "2.13.30"
\header {
  lsrtags = "vocal-music, template"
  texidoc = "
This template shows one way of setting out an Anglican psalm chant.
It also shows how the verses may be added as stand-alone text under
the music.  The two verses are coded in different styles to
demonstrate more possibilities.

"
  doctitle = "Anglican psalm template"
}

SopranoMusic = \relative g' {
  g1 | c2 b | a1 | \bar "||"
  a1 | d2 c | c b | c1 | \bar "||"
}

AltoMusic = \relative c' {
  e1 | g2 g | f1 |
  f1 | f2 e | d d | e1 |
}

TenorMusic = \relative a {
  c1 | c2 c | c1 |
  d1 | g,2 g | g g | g1 |
}

BassMusic =  \relative c {
  c1 | e2 e | f1 |
  d1 | b2 c | g' g | c,1 |
}

global = {
 \time 2/2
}

dot = \markup {
  \override #'(font-encoding . fetaMusic)
  \raise #0.7 \lookup #"dots.dot"
}

tick = \markup {
  \override #'(font-encoding . fetaMusic)
  \raise #1 \fontsize #-5 { \lookup #'"scripts.rvarcomma" }
}

% Use markup to center the chant on the page
\markup {
  \fill-line {
    " "  % left-justified

\score {  % centered
  <<
    \new ChoirStaff <<
      \new Staff <<
        \global
        \clef "treble"
        \new Voice = "Soprano" <<
          \voiceOne
          \SopranoMusic
        >>
        \new Voice = "Alto" <<
          \voiceTwo
          \AltoMusic
        >>
      >>
      \new Staff <<
        \clef "bass"
        \global
        \new Voice = "Tenor" <<
          \voiceOne
          \TenorMusic
        >>
        \new Voice = "Bass" <<
          \voiceTwo
          \BassMusic
        >>
      >>
    >>
  >>
  \layout {
    \context {
      \Score
      \override SpacingSpanner
                #'base-shortest-duration = #(ly:make-moment 1 2)
    }
    \context {
      \Staff
      \remove Time_signature_engraver
    }
  }
}  % End score

    " "  % right-justified
  }
}  % End markup

\markup {
  \fill-line {
    \column {
      \left-align {
        " " " " " "
        \line {
          \fontsize #5 "O"
          \fontsize #3 "come"
          \fontsize #0
          "let us" \bold "sing" "| unto" \dot "the | Lord : let"
        }
        \line {
          "us heartily"
          \concat { "re" \bold "joice" }
          "in the | strength of | our"
        }
        "sal | vation."
        " "
        \line {
          "    2. Let us come before his presence" \tick "with"
        }
        \line {
          "thanks" \tick "giving * and shew ourselves"
          \tick "glad in" \tick
        }
        "him with psalms."
      }
    }
  }
}
