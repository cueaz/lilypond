\version "2.1.22"
\header {
texidoc = "Grace code should not be confused by nested sequential musics, containing grace notes; practically speaking, this means that the end-bar and measure bar coincide in this example." 

}
\score {
    \notes {
	c''2 { \grace  b'16  c''2 }  \bar "|."
    }
    \paper {

	raggedright = ##t 
    }
}


