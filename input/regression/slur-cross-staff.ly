
\version "2.1.22"
\header{
texidoc="
The same goes for slurs. They behave decently when broken across
linebreak.
"
}

	
\score{
	\context PianoStaff <<
	\context Staff=one \notes\relative c'{
		\stemUp \slurUp
		 c4( c \change Staff=two c  c) |
		\change Staff=one
		\stemUp \slurUp
		 c4( c \change Staff=two c  c) |
		\stemUp \slurUp
		 c4( c \change Staff=one c  c) |
		\change Staff=two
		\stemUp \slurUp
		 c4( c \change Staff=one c  c) |
		\change Staff=two
		\stemUp \slurUp
		 c4( \change Staff=one c c  c) |
		r2
		\change Staff=two
		\stemUp \slurUp
		 c4( \change Staff=one c
		   \break
		c  c)
		r2
%		\stemDown \slurDown
%		 c4( \change Staff=two c c \change Staff=one  c)
		\stemDown \slurDown
		 d4( \change Staff=two c c \change Staff=one  d)
		\change Staff=two
		\stemUp \slurUp
		 c4( \change Staff=one c c \change Staff=two  c)
		r1
	}
	\context Staff=two \notes\relative c'{
		\clef bass
		s1 s1 s1 s1 s1 s1 s1 s1 s1 s1
	}
	>>
}



