
\version "2.1.22"

\header{

    texidoc="Automatic cross-staff knees also work (here we see them
with explicit staff switches)."

     }

\score {
  \notes \context PianoStaff <<
    \context Staff = "up" \notes\relative c''{
      b8[ \change Staff="down" d,, ]
      c[ \change Staff="up" c'' ]
      b,[ \change Staff="down" d^"no knee" ]
    }
    \context Staff = "down" {
      \clef bass 
      s2.
    }
  >>
  \paper{
    raggedright = ##t
  }
}
