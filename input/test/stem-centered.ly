
\version "2.1.22"
% possible rename to ancient- something.
\header {
    texidoc ="@cindex Ancient Mensural Note Heads
Mensural note heads. "
}

\score {
	\notes {
	\relative c'' {
		\override NoteHead  #'style = #'mensural
		c\maxima*1/8
		c\longa*1/4 c\breve*1/2 c1 c2 c4 c8 
		}
	}
	\paper{raggedright=##t}
}

