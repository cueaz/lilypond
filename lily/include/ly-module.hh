/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2002--2015 Han-Wen Nienhuys <hanwen@xs4all.nl>

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
#ifndef LY_MODULE_HH
#define LY_MODULE_HH

#include "config.hh"
#include "lily-guile.hh"

SCM ly_make_module (bool safe);
SCM ly_module_copy (SCM dest, SCM src);
SCM ly_module_2_alist (SCM mod);
SCM ly_module_lookup (SCM module, SCM sym);
SCM ly_modules_lookup (SCM modules, SCM sym, SCM);
SCM ly_module_symbols (SCM mod);
void ly_reexport_module (SCM mod);
inline bool ly_is_module (SCM x) { return SCM_MODULEP (x); }

SCM ly_use_module (SCM mod, SCM used);

/* For backward compatability with Guile 1.8 */
#if !HAVE_GUILE_HASH_FUNC
typedef SCM (*scm_t_hash_fold_fn) (GUILE_ELLIPSIS);
typedef SCM (*scm_t_hash_handle_fn) (GUILE_ELLIPSIS);
#endif

#endif /* LY_MODULE_HH */

