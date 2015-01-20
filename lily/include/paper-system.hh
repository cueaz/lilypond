/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2004--2015  Jan Nieuwenhuizen <janneke@gnu.org>

  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PAPER_SYSTEM_HH
#define PAPER_SYSTEM_HH

#include "prob.hh"

/*
  A formatted "system" (A block of titling also is a Paper_system)

  To save memory, we don't keep around the System grobs, but put the
  formatted content of the grob is put into a
  Paper_system. Page-breaking handles Paper_system objects.
*/
Prob *make_paper_system (SCM immutable_init);
void paper_system_set_stencil (Prob *prob, Stencil s);
SCM get_footnotes (SCM expr);

#endif /* PAPER_SYSTEM_HH */
