
\version "2.2.0"
\header{
texidoc="
A sharp sign after a double sharp sign, as well as a flat sign
after a double flat sign is automatically prepended with a
natural sign.
"
}



thenotes = \notes \relative cis' { \time 4/4
gisis'4 gis gisis ges |
}

\score { << \context Staff \thenotes
	\context NoteNames  {
		\override NoteNames.NoteName   #'no-spacing-rods = ##f 
		\thenotes
	}
	>>
\paper { raggedright = ##t }
}

