%%  Do not edit this file; it is auto-generated from LSR!
\version "2.10.12"

\header { texidoc = "
Odd 20th century time signatures (such as \"5/8\") can often be played
as compound time signatures (e.g. \"3/8 + 2/8\"), which combine two or
more inequal metrics. LilyPond can make such musics quite easy to read
and play, by explicitly printing the compound time signatures and
adapting the automatic beaming behaviour. (You can even add graphic
measure groping indications, the appropriate snippet in this database.)

" }

#(define (compound-time one two num)
  (markup #:override '(baseline-skip . 0) #:number 
   (#:line ((#:column (one num)) #:vcenter "+" (#:column (two num))))))


\relative {
  %% compound time signature hack
  \time 5/8
  \override Staff.TimeSignature #'stencil = #ly:text-interface::print
  \override Staff.TimeSignature #'text = #(compound-time "2" "3" "8" )
  #(override-auto-beam-setting '(end 1 8 5 8) 1 4)
  c8 d e fis gis | c fis, gis e d | c8 d e4  gis8
}
