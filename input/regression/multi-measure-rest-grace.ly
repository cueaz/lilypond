\version "2.1.22"


\header {
texidoc= "Grace notes and multi-measure rests."
}

\score   {
\notes <<
	\new Staff { R1 R1 R1*3 }
	\new Staff { \clef bass c1 \grace c8 c2 c2 c1  \grace c16 c2 c2 c1 }
>>
}
