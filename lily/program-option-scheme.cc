/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2001--2023  Han-Wen Nienhuys <hanwen@xs4all.nl>

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

#include "program-option.hh"

#include "profile.hh"
#include "international.hh"
#include "lily-imports.hh"
#include "ly-scm-list.hh"
#include "main.hh"
#include "parse-scm.hh"
#include "string-convert.hh"
#include "warn.hh"
#include "lily-imports.hh"
#include "protected-scm.hh"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

bool debug_skylines;
bool debug_property_callbacks;
bool debug_page_breaking_scoring;

bool music_strings_to_paths;
bool relative_includes;

bool profile_property_accesses = false;
/*
  crash if internally the wrong type is used for a grob property.
*/
bool do_internal_type_checking_global;
bool strict_infinity_checking = false;

static Protected_scm option_hash;

static void
internal_set_option (SCM var, SCM val)
{
  std::string varstr = robust_symbol2string (var, "");
  bool valbool = from_scm<bool> (val);
  SCM val_scm_bool = to_scm (valbool); // could be scm_is_eq (val, SCM_BOOL_T)
  if (0)
    ;
  else if (varstr == "profile-property-accesses")
    {
      profile_property_accesses = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "protected-scheme-parsing")
    {
      parse_protect_global = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "check-internal-types")
    {
      do_internal_type_checking_global = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "debug-gc-assert-parsed-dead")
    {
      parsed_objects_should_be_dead = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "strict-infinity-checking")
    {
      strict_infinity_checking = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "debug-skylines")
    {
      debug_skylines = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "debug-property-callbacks")
    {
      debug_property_callbacks = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "debug-page-breaking-scoring")
    {
      debug_page_breaking_scoring = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "datadir")
    {
      /* ignore input value. */
      val = ly_string2scm (lilypond_datadir);
    }
  else if (varstr == "relative-includes")
    {
      relative_includes = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "warning-as-error")
    {
      /* warning_as_error is defined in flower/warn.cc */
      warning_as_error = valbool;
      val = val_scm_bool;
    }
  else if (varstr == "music-strings-to-paths")
    {
      music_strings_to_paths = valbool;
      val = val_scm_bool;
    }

  scm_hashq_set_x (option_hash, var, val);
}

ssize const HELP_INDENT = 30;
ssize const INDENT = 2;
ssize const SEPARATION = 5;

/*
  Hmmm. should do in SCM / C++  ?
*/
static std::string
get_help_string (SCM alist)
{
  std::vector<std::string> opts;

  for (SCM s = alist; scm_is_pair (s); s = scm_cdr (s))
    {
      SCM sym = scm_caar (s);
      SCM val = scm_cdar (s);
      std::string opt_spec = std::string (INDENT, ' ') + ly_symbol2string (sym)
                             + " (" + ly_scm2string (Lily::scm_to_string (val))
                             + ")";

      if (opt_spec.length () + SEPARATION > HELP_INDENT)
        opt_spec += '\n' + std::string (HELP_INDENT, ' ');
      else
        opt_spec += std::string (HELP_INDENT - opt_spec.length (), ' ');

      SCM opt_help_scm = scm_object_property (
        sym, ly_symbol2scm ("program-option-documentation"));
      std::string opt_help = ly_scm2string (opt_help_scm);
      replace_all (&opt_help, std::string ("\n"),
                   std::string ("\n") + std::string (HELP_INDENT, ' '));

      opts.push_back (opt_spec + opt_help + "\n");
    }

  std::string help ("Options supported by `ly:set-option':\n\n");
  std::sort (opts.begin (), opts.end ());
  for (vsize i = 0; i < opts.size (); i++)
    help += opts[i];
  return help;
}

static bool
is_internal_option (SCM sym)
{
  return scm_is_true (
    scm_object_property (sym, ly_symbol2scm ("program-option-internal?")));
}

static bool
is_accumulative_option (SCM sym)
{
  return scm_is_true (
    scm_object_property (sym, ly_symbol2scm ("program-option-accumulative?")));
}

LY_DEFINE (ly_option_usage, "ly:option-usage", 0, 2, 0,
           (SCM port, SCM internal),
           R"(
Print @code{ly:set-option} usage.  Optional @var{port} argument for the
destination defaults to current output port. Specify @var{internal} to get doc
for internal options.
           )")
{
  SCM alist = SCM_EOL;
  for (SCM s = ly_hash2alist (option_hash); scm_is_pair (s); s = scm_cdr (s))
    {
      if (is_internal_option (scm_caar (s)) == scm_is_true (internal))
        alist = scm_cons (scm_car (s), alist);
    }

  SCM str = ly_string2scm (get_help_string (alist));
  scm_write_line (str, port);

  return SCM_UNSPECIFIED;
}

LY_DEFINE (ly_add_option, "ly:add-option", 3, 0, 1,
           (SCM sym, SCM val, SCM description, SCM rest),
           R"(
Add a program option @var{sym}.  @var{val} is the default value and
@var{description} is a string description.

Passing @code{#internal? #t} makes the option an internal option, not
displayed in the @command{lilypond -dhelp} output (but displayed in
@command{lilypond -dhelp-internal}.

Passing @code{#:accumulative? #t} makes the option accumulative,
which gathers @code{-d} values in a list instead of letting the
last @code{-d} flag overwrite the others.
           )")
{
  if (!option_hash.is_bound ())
    option_hash = scm_c_make_hash_table (11);
  LY_ASSERT_TYPE (ly_is_symbol, sym, 1);
  LY_ASSERT_TYPE (scm_is_string, description, 3);
  SCM internal = SCM_BOOL_F;
  SCM accumulative = SCM_BOOL_F;
  scm_c_bind_keyword_arguments (
    "ly:add-option", rest, static_cast<scm_t_keyword_arguments_flags> (0),
    ly_keyword2scm ("internal?"), &internal, ly_keyword2scm ("accumulative?"),
    &accumulative, SCM_UNDEFINED);

  if (scm_is_true (internal))
    scm_set_object_property_x (sym, ly_symbol2scm ("program-option-internal?"),
                               SCM_BOOL_T);

  if (scm_is_true (accumulative))
    scm_set_object_property_x (
      sym, ly_symbol2scm ("program-option-accumulative?"), SCM_BOOL_T);

  internal_set_option (sym, val);

  scm_set_object_property_x (
    sym, ly_symbol2scm ("program-option-documentation"), description);

  return SCM_UNSPECIFIED;
}

LY_DEFINE (ly_set_option, "ly:set-option", 1, 1, 0, (SCM var, SCM val),
           R"(
Set a program option.
           )")
{
  LY_ASSERT_TYPE (ly_is_symbol, var, 1);

  if (SCM_UNBNDP (val))
    val = SCM_BOOL_T;

  std::string varstr = robust_symbol2string (var, "");
  if (varstr.substr (0, 3) == std::string ("no-"))
    {
      varstr = varstr.substr (3, varstr.length () - 3);
      var = ly_symbol2scm (varstr);
      val = to_scm (!from_scm<bool> (val));
    }

  if (is_accumulative_option (var))
    {
      warning (
        _f ("option %s is accumulative; use ly:append-to-option instead of "
            "ly:set-option",
            varstr.c_str ()));
      return SCM_UNSPECIFIED;
    }
  SCM handle = scm_hashq_get_handle (option_hash, var);
  if (scm_is_false (handle))
    warning (_f ("no such internal option: %s", varstr.c_str ()));

  internal_set_option (var, val);
  return SCM_UNSPECIFIED;
}

LY_DEFINE (ly_append_to_option, "ly:append-to-option", 2, 0, 0,
           (SCM var, SCM val),
           R"(
Add a value to an accumulative program option.
           )")
{
  LY_ASSERT_TYPE (ly_is_symbol, var, 1);
  SCM handle = scm_hashq_get_handle (option_hash, var);
  if (scm_is_false (handle))
    {
      warning (
        _f ("no such program option: %s", ly_symbol2string (var).c_str ()));
      return SCM_UNSPECIFIED;
    }
  if (!is_accumulative_option (var))
    {
      warning (_f ("option %s is not accumulative; use ly:set-option instead "
                   "of ly:add-to-option",
                   ly_symbol2string (var).c_str ()));
    }
  scm_set_cdr_x (handle, scm_cons (val, scm_cdr (handle)));
  return SCM_UNSPECIFIED;
}

LY_DEFINE (ly_command_line_options, "ly:command-line-options", 0, 0, 0, (),
           R"(
The Scheme options specified on the command line with option @option{-d}.
           )")
{
  return ly_string2scm (init_scheme_variables_global);
}

LY_DEFINE (ly_command_line_code, "ly:command-line-code", 0, 0, 0, (),
           R"(
The Scheme code specified on the command line with option @option{-e}.
           )")
{
  return ly_string2scm (init_scheme_code_global);
}

LY_DEFINE (ly_verbose_output_p, "ly:verbose-output?", 0, 0, 0, (),
           R"(
Was verbose output requested, i.e., is the log level at least @code{DEBUG}?
           )")
{
  return to_scm (is_loglevel (LOG_DEBUG));
}

LY_DEFINE (ly_all_options, "ly:all-options", 0, 0, 0, (),
           R"(
Get all option settings in an alist.
           )")
{
  // Unlike ly:get-option, ly:all-options does not reverse accumulative options,
  // since this is only used to restore the same set of options with
  // ly:reset-options.
  return ly_hash2alist (option_hash);
}

LY_DEFINE (ly_reset_options, "ly:reset-options", 1, 0, 0, (SCM alist),
           R"(
Reset all program options to the values in @var{alist}.
           )")
{
  for (SCM pair : as_ly_scm_list (alist))
    {
      LY_ASSERT_TYPE (scm_is_pair, pair, 0);
      internal_set_option (scm_car (pair), scm_cdr (pair));
    }
  return SCM_UNSPECIFIED;
}

LY_DEFINE (ly_get_option, "ly:get-option", 1, 0, 0, (SCM var),
           R"(
Get a global option setting.
           )")
{
  LY_ASSERT_TYPE (ly_is_symbol, var, 1);
  SCM ret = scm_hashq_ref (option_hash, var, SCM_BOOL_F);
  // The values of accumulative options are internally stored in reverse (for
  // efficiency).
  if (is_accumulative_option (var))
    ret = scm_reverse (ret);
  return ret;
}
