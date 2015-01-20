/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2015 Han-Wen Nienhuys <hanwen@xs4all.nl>

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

#ifndef AXES_HH
#define AXES_HH

#include <cassert>

enum Axis
{
  X_AXIS = 0,
  Y_AXIS = 1,
  NO_AXES = 2,
};

static inline
Axis
incr (Axis &a)
{
  assert (a < NO_AXES);
  a = Axis (int (a) + 1);
  return a;
}

static inline
Axis
other_axis (Axis a)
{
  return a == Y_AXIS ? X_AXIS : Y_AXIS;
}

#endif // AXES_HH
