\header {

    texidoc = "When notes are colliding, the resolution depends on the
    dots: notes with dots should go to the right, if there could be
    confusion to which notes the dots belong."
}
\version "2.1.22"
\score { 
  \notes \relative c'' { 
    << <a c>2\\ { <b d>4 <b d>4 }   >>
    << { <a c>2 } \\ { <b d>4. <b e>8 } >> 
  }
  \paper {  raggedright = ##t } 
}   
