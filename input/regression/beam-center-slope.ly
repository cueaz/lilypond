

\version "2.1.22"
\header{

    texidoc="Simple beams on middle staffline are allowed to be
    slightly sloped, even if the notes have ledgers.  Beams reaching
    beyond middle line can have bigger slope."

}

\score{
    \notes\relative c'{
	%%\override Staff.Stem  #'beamed-lengths = #'(3.50)
	%%  c8[ d]
	%%  d[ c]
	%% r2
	%% \override Staff.Stem  #'beamed-lengths = #'(3.26)

         a8[^"small slope" b]
         b[ a]

	 c''[ b]
	 b[ c]
	
	 c,,[ d]
	 d[ c]
	
	 a''[ g]
	 g[ a]

	 c,,[^"bigger slope" e]
	 e[ c]
	
	 a''[ f]
	 f[ a]
    }
    \paper{
	raggedright = ##t
    }
}

