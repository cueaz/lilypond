/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2011 Han-Wen Nienhuys <hanwen@xs4all.nl>
  Jan Nieuwenhuizen <janneke@gnu.org>

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

#include "beam-scoring-problem.hh"

#include <algorithm>
#include <queue>
#include <set>
using namespace std;

#include "align-interface.hh"
#include "beam.hh"
#include "direction.hh"
#include "directional-element-interface.hh"
#include "grob.hh"
#include "grob-array.hh"
#include "item.hh"
#include "international.hh"
#include "least-squares.hh"
#include "libc-extension.hh"
#include "main.hh"
#include "note-head.hh"
#include "output-def.hh"
#include "pointer-group-interface.hh"
#include "rhythmic-head.hh"
#include "staff-symbol-referencer.hh"
#include "stencil.hh"
#include "stem.hh"
#include "warn.hh"

Real
get_detail (SCM alist, SCM sym, Real def)
{
  SCM entry = scm_assq (sym, alist);

  if (scm_is_pair (entry))
    return robust_scm2double (scm_cdr (entry), def);
  return def;
}

void
Beam_quant_parameters::fill (Grob *him)
{
  SCM details = him->get_property ("details");

  // General
  BEAM_EPS = get_detail (details, ly_symbol2scm ("beam-eps"), 1e-3);
  REGION_SIZE = get_detail (details, ly_symbol2scm ("region-size"), 2);

  // forbidden quants
  SECONDARY_BEAM_DEMERIT = get_detail (details, ly_symbol2scm ("secondary-beam-demerit"), 10.0);
  STEM_LENGTH_DEMERIT_FACTOR = get_detail (details, ly_symbol2scm ("stem-length-demerit-factor"), 5);
  HORIZONTAL_INTER_QUANT_PENALTY = get_detail (details, ly_symbol2scm ("horizontal-inter-quant"), 500);

  STEM_LENGTH_LIMIT_PENALTY = get_detail (details, ly_symbol2scm ("stem-length-limit-penalty"), 5000);
  DAMPING_DIRECTION_PENALTY = get_detail (details, ly_symbol2scm ("damping-direction-penalty"), 800);
  HINT_DIRECTION_PENALTY = get_detail (details, ly_symbol2scm ("hint-direction-penalty"), 20);
  MUSICAL_DIRECTION_FACTOR = get_detail (details, ly_symbol2scm ("musical-direction-factor"), 400);
  IDEAL_SLOPE_FACTOR = get_detail (details, ly_symbol2scm ("ideal-slope-factor"), 10);
  ROUND_TO_ZERO_SLOPE = get_detail (details, ly_symbol2scm ("round-to-zero-slope"), 0.02);

  // Collisions
  COLLISION_PENALTY = get_detail (details, ly_symbol2scm ("collision-penalty"), 500);
  COLLISION_PADDING = get_detail (details, ly_symbol2scm ("collision-padding"), 0.5);
  STEM_COLLISION_FACTOR = get_detail (details, ly_symbol2scm ("stem-collision-factor"), 0.1);
}

// Add x if x is positive, add |x|*fac if x is negative.
static Real
shrink_extra_weight (Real x, Real fac)
{
  return fabs (x) * ((x < 0) ? fac : 1.0);
}

/****************************************************************/

Beam_configuration::Beam_configuration ()
{
  y = Interval (0.0, 0.0);
  demerits = 0.0;
  next_scorer_todo = ORIGINAL_DISTANCE;
}

bool Beam_configuration::done () const
{
  return next_scorer_todo >= NUM_SCORERS;
}

void Beam_configuration::add (Real demerit, const string &reason)
{
  demerits += demerit;

#if DEBUG_BEAM_SCORING
  if (demerit)
    score_card_ += to_string (" %s %.2f", reason.c_str (), demerit);
#endif
}

Beam_configuration *Beam_configuration::new_config (Interval start,
                                                    Interval offset)
{
  Beam_configuration *qs = new Beam_configuration;
  qs->y = Interval (int (start[LEFT]) + offset[LEFT],
                    int (start[RIGHT]) + offset[RIGHT]);

  // This orders the sequence so we try combinations closest to the
  // the ideal offset first.
  Real start_score = abs (offset[RIGHT]) + abs (offset[LEFT]);
  qs->demerits = start_score / 1000.0;
  qs->next_scorer_todo = ORIGINAL_DISTANCE + 1;

  return qs;
}

Real
Beam_scoring_problem::y_at (Real x, Beam_configuration const *p) const
{
  return p->y[LEFT] + (x - x_span_[LEFT]) * p->y.delta () / x_span_.delta ();
}

/****************************************************************/

/*
  TODO:

  - Make all demerits customisable

  - Add demerits for quants per se, as to forbid a specific quant
  entirely
*/

// This is a temporary hack to see how much we can gain by using a
// priority queue on the beams to score.
static int score_count = 0;
LY_DEFINE (ly_beam_score_count, "ly:beam-score-count", 0, 0, 0,
           (),
           "count number of beam scores.")
{
  return scm_from_int (score_count);
}

void Beam_scoring_problem::add_collision (Real x, Interval y,
                                          Real score_factor)
{
  if (edge_dirs_[LEFT] == edge_dirs_[RIGHT])
    {
      Direction d = edge_dirs_[LEFT];

      Real quant_range_y = quant_range_[LEFT][-d]
                           + (x - x_span_[LEFT]) * (quant_range_[RIGHT][-d] - quant_range_[LEFT][-d]) / x_span_.delta ();

      if (d * (quant_range_y - minmax (d, y[UP], y[DOWN])) > 0)
        {
          return;
        }
    }

  Beam_collision c;
  c.beam_y_.set_empty ();

  for (vsize j = 0; j < segments_.size (); j++)
    {
      if (segments_[j].horizontal_.contains (x))
        c.beam_y_.add_point (segments_[j].vertical_count_ * beam_translation_);
      if (segments_[j].horizontal_[LEFT] > x)
        break;
    }
  c.beam_y_.widen (0.5 * beam_thickness_);

  c.x_ = x;

  y *= 1 / staff_space_;
  c.y_ = y;
  c.base_penalty_ = score_factor;
  collisions_.push_back (c);
}

void Beam_scoring_problem::init_collisions (vector<Grob *> grobs)
{
  Grob *common_x = NULL;
  segments_ = Beam::get_beam_segments (beam_, &common_x);
  vector_sort (segments_, beam_segment_less);
  if (common_[X_AXIS] != common_x)
    {
      programming_error ("Disagree on common x. Skipping collisions in beam scoring.");
      return;
    }

  set<Grob *> stems;
  for (vsize i = 0; i < grobs.size (); i++)
    {
      Box b;
      for (Axis a = X_AXIS; a < NO_AXES; incr (a))
        b[a] = grobs[i]->extent (common_[a], a);

      Real width = b[X_AXIS].length ();
      Real width_factor = sqrt (width / staff_space_);

      Direction d = LEFT;
      do
        add_collision (b[X_AXIS][d], b[Y_AXIS], width_factor);
      while (flip (&d) != LEFT);

      Grob *stem = unsmob_grob (grobs[i]->get_object ("stem"));
      if (stem && Stem::has_interface (stem) && Stem::is_normal_stem (stem))
        {
          stems.insert (stem);
        }
    }

  for (set<Grob *>::const_iterator it (stems.begin ()); it != stems.end (); it++)
    {
      Grob *s = *it;
      Real x = s->extent (common_[X_AXIS], X_AXIS).center ();

      Direction stem_dir = get_grob_direction (*it);
      Interval y;
      y.set_full ();
      y[-stem_dir] = Stem::chord_start_y (*it) + (*it)->relative_coordinate (common_[Y_AXIS], Y_AXIS)
                     - beam_->relative_coordinate (common_[Y_AXIS], Y_AXIS);

      Real factor = parameters_.STEM_COLLISION_FACTOR;
      if (!unsmob_grob (s->get_object ("beam")))
        factor = 1.0;
      add_collision (x, y, factor);
    }
}

void Beam_scoring_problem::init_stems ()
{
  extract_grob_set (beam_, "covered-grobs", collisions);
  extract_grob_set (beam_, "stems", stems);
  for (int a = 2; a--;)
    {
      common_[a] = common_refpoint_of_array (stems, beam_, Axis (a));
      common_[a] = common_refpoint_of_array (collisions, common_[a], Axis (a));
    }

  Drul_array<Grob *> edge_stems (Beam::first_normal_stem (beam_),
                                 Beam::last_normal_stem (beam_));
  Direction d = LEFT;
  do
    x_span_[d] = edge_stems[d] ? edge_stems[d]->relative_coordinate (common_[X_AXIS], X_AXIS) : 0.0;
  while (flip (&d) != LEFT);

  Drul_array<bool> dirs_found (0, 0);
  for (vsize i = 0; i < stems.size (); i++)
    {
      Grob *s = stems[i];
      if (!Stem::is_normal_stem (s))
        continue;

      Stem_info si (Stem::get_stem_info (s));
      si.scale (1 / staff_space_);
      stem_infos_.push_back (si);
      dirs_found[si.dir_] = true;

      bool f = to_boolean (s->get_property ("french-beaming"))
               && s != edge_stems[LEFT] && s != edge_stems[RIGHT];

      Real y = Beam::calc_stem_y (beam_, s, common_, x_span_[LEFT], x_span_[RIGHT], CENTER,
                                  Interval (0, 0), f);
      base_lengths_.push_back (y / staff_space_);
      stem_xpositions_.push_back (s->relative_coordinate (common_[X_AXIS], X_AXIS));
    }

  edge_dirs_ = Drul_array<Direction> (CENTER, CENTER);
  if (stem_infos_.size ())
    {
      edge_dirs_ = Drul_array<Direction> (stem_infos_[0].dir_,
                                         stem_infos_.back ().dir_);
    }

  is_xstaff_ = Align_interface::has_interface (common_[Y_AXIS]);
  is_knee_ = dirs_found[LEFT] && dirs_found[RIGHT];

  staff_radius_ = Staff_symbol_referencer::staff_radius (beam_);
  edge_beam_counts_ = Drul_array<int>
                     (Stem::beam_multiplicity (stems[0]).length () + 1,
                      Stem::beam_multiplicity (stems.back ()).length () + 1);

  // TODO - why are we dividing by staff_space_?
  beam_translation_ = Beam::get_beam_translation (beam_) / staff_space_;

  d = LEFT;
  do
    {
      quant_range_[d].set_full ();
      if (!edge_stems[d])
        continue;

      Real stem_offset = edge_stems[d]->relative_coordinate (common_[Y_AXIS], Y_AXIS)
                         - beam_->relative_coordinate (common_[Y_AXIS], Y_AXIS);
      Interval heads = Stem::head_positions (edge_stems[d]) * 0.5 * staff_space_;

      Direction ed = edge_dirs_[d];
      heads.widen (0.5 * staff_space_
                   + (edge_beam_counts_[d] - 1) * beam_translation_ + beam_thickness_ * .5);
      quant_range_[d][-ed] = heads[ed] + stem_offset;
    }
  while (flip (&d) != LEFT);

  init_collisions (collisions);
}

Beam_scoring_problem::Beam_scoring_problem (Grob *me, Drul_array<Real> ys)
{
  beam_ = me;
  unquanted_y_ = ys;

  /*
    Calculations are relative to a unit-scaled staff, i.e. the quants are
    divided by the current staff_space_.
  */
  staff_space_ = Staff_symbol_referencer::staff_space (me);
  beam_thickness_ = Beam::get_beam_thickness (me) / staff_space_;
  line_thickness_ = Staff_symbol_referencer::line_thickness (me) / staff_space_;

  // This is the least-squares DY, corrected for concave beams.
  musical_dy_ = robust_scm2double (me->get_property ("least-squares-dy"), 0);

  parameters_.fill (me);
  init_stems ();
}

// Assuming V is not empty, pick a 'reasonable' point inside V.
static Real
point_in_interval (Interval v, Real dist)
{
  if (isinf (v[DOWN]))
    return v[UP] - dist;
  else if (isinf (v[UP]))
    return v[DOWN] + dist;
  else
    return v.center ();
}

/* Set stem's shorten property if unset.

TODO:
take some y-position (chord/beam/nearest?) into account
scmify forced-fraction

This is done in beam because the shorten has to be uniform over the
entire beam.
*/

void
set_minimum_dy (Grob *me, Real *dy)
{
  if (*dy)
    {
      /*
        If dy is smaller than the smallest quant, we
        get absurd direction-sign penalties.
      */

      Real ss = Staff_symbol_referencer::staff_space (me);
      Real beam_thickness = Beam::get_beam_thickness (me) / ss;
      Real slt = Staff_symbol_referencer::line_thickness (me) / ss;
      Real sit = (beam_thickness - slt) / 2;
      Real inter = 0.5;
      Real hang = 1.0 - (beam_thickness - slt) / 2;

      *dy = sign (*dy) * max (fabs (*dy),
                              min (min (sit, inter), hang));
    }
}

Interval
Beam::no_visible_stem_positions (Grob *me, Interval default_value)
{
  extract_grob_set (me, "stems", stems);
  if (stems.empty ())
    return default_value;

  Interval head_positions;
  Slice multiplicity;
  for (vsize i = 0; i < stems.size (); i++)
    {
      head_positions.unite (Stem::head_positions (stems[i]));
      multiplicity.unite (Stem::beam_multiplicity (stems[i]));
    }

  Direction dir = get_grob_direction (me);

  if (!dir)
    programming_error ("The beam should have a direction by now.");

  Real y = head_positions.linear_combination (dir)
           * 0.5 * Staff_symbol_referencer::staff_space (me)
           + dir * get_beam_translation (me) * (multiplicity.length () + 1);

  y /= Staff_symbol_referencer::staff_space (me);
  return Interval (y, y);
}

/*
  Compute a first approximation to the beam slope.
*/
MAKE_SCHEME_CALLBACK (Beam, calc_least_squares_positions, 2);
SCM
Beam::calc_least_squares_positions (SCM smob, SCM /* posns */)
{
  Grob *me = unsmob_grob (smob);

  int count = normal_stem_count (me);
  Interval pos (0, 0);
  if (count < 1)
    return ly_interval2scm (no_visible_stem_positions (me, pos));

  vector<Real> x_posns;
  extract_grob_set (me, "normal-stems", stems);
  Grob *commonx = common_refpoint_of_array (stems, me, X_AXIS);
  Grob *commony = common_refpoint_of_array (stems, me, Y_AXIS);

  Real my_y = me->relative_coordinate (commony, Y_AXIS);

  Grob *fvs = first_normal_stem (me);
  Grob *lvs = last_normal_stem (me);

  Interval ideal (Stem::get_stem_info (fvs).ideal_y_
                  + fvs->relative_coordinate (commony, Y_AXIS) - my_y,
                  Stem::get_stem_info (lvs).ideal_y_
                  + lvs->relative_coordinate (commony, Y_AXIS) - my_y);

  Real x0 = first_normal_stem (me)->relative_coordinate (commonx, X_AXIS);
  for (vsize i = 0; i < stems.size (); i++)
    {
      Grob *s = stems[i];

      Real x = s->relative_coordinate (commonx, X_AXIS) - x0;
      x_posns.push_back (x);
    }
  Real dx = last_normal_stem (me)->relative_coordinate (commonx, X_AXIS) - x0;

  Real y = 0;
  Real slope = 0;
  Real dy = 0;
  Real ldy = 0.0;
  if (!ideal.delta ())
    {
      Interval chord (Stem::chord_start_y (stems[0]),
                      Stem::chord_start_y (stems.back ()));

      /* Simple beams (2 stems) on middle line should be allowed to be
         slightly sloped.

         However, if both stems reach middle line,
         ideal[LEFT] == ideal[RIGHT] and ideal.delta () == 0.

         For that case, we apply artificial slope */
      if (!ideal[LEFT] && chord.delta () && count == 2)
        {
          /* FIXME. -> UP */
          Direction d = (Direction) (sign (chord.delta ()) * UP);
          pos[d] = get_beam_thickness (me) / 2;
          pos[-d] = -pos[d];
        }
      else
        pos = ideal;

      /*
        For broken beams this doesn't work well. In this case, the
        slope esp. of the first part of a broken beam should predict
        where the second part goes.
      */
      ldy = pos[RIGHT] - pos[LEFT];
    }
  else
    {
      vector<Offset> ideals;
      for (vsize i = 0; i < stems.size (); i++)
        {
          Grob *s = stems[i];
          ideals.push_back (Offset (x_posns[i],
                                    Stem::get_stem_info (s).ideal_y_
                                    + s->relative_coordinate (commony, Y_AXIS)
                                    - my_y));
        }

      minimise_least_squares (&slope, &y, ideals);

      dy = slope * dx;

      set_minimum_dy (me, &dy);

      ldy = dy;
      pos = Interval (y, (y + dy));
    }

  /*
    "position" is relative to the staff.
  */
  scale_drul (&pos, 1 / Staff_symbol_referencer::staff_space (me));

  me->set_property ("least-squares-dy", scm_from_double (ldy));
  return ly_interval2scm (pos);
}

/* This neat trick is by Werner Lemberg,
   damped = tanh (slope)
   corresponds with some tables in [Wanske] CHECKME */
MAKE_SCHEME_CALLBACK (Beam, slope_damping, 2);
SCM
Beam::slope_damping (SCM smob, SCM posns)
{
  Grob *me = unsmob_grob (smob);
  Drul_array<Real> pos = ly_scm2interval (posns);

  if (normal_stem_count (me) <= 1)
    return posns;

  SCM s = me->get_property ("damping");
  Real damping = scm_to_double (s);
  Real concaveness = robust_scm2double (me->get_property ("concaveness"), 0.0);
  if (concaveness >= 10000)
    {
      pos[LEFT] = pos[RIGHT];
      me->set_property ("least-squares-dy", scm_from_double (0));
      damping = 0;
    }

  if (damping)
    {
      scale_drul (&pos, Staff_symbol_referencer::staff_space (me));

      Real dy = pos[RIGHT] - pos[LEFT];

      Grob *fvs = first_normal_stem (me);
      Grob *lvs = last_normal_stem (me);

      Grob *commonx = fvs->common_refpoint (lvs, X_AXIS);

      Real dx = last_normal_stem (me)->relative_coordinate (commonx, X_AXIS)
                - first_normal_stem (me)->relative_coordinate (commonx, X_AXIS);

      Real slope = dy && dx ? dy / dx : 0;

      slope = 0.6 * tanh (slope) / (damping + concaveness);

      Real damped_dy = slope * dx;

      set_minimum_dy (me, &damped_dy);

      pos[LEFT] += (dy - damped_dy) / 2;
      pos[RIGHT] -= (dy - damped_dy) / 2;

      scale_drul (&pos, 1 / Staff_symbol_referencer::staff_space (me));
    }

  return ly_interval2scm (pos);
}

/*
  We can't combine with previous function, since check concave and
  slope damping comes first.

  TODO: we should use the concaveness to control the amount of damping
  applied.
*/
MAKE_SCHEME_CALLBACK (Beam, shift_region_to_valid, 2);
SCM
Beam::shift_region_to_valid (SCM grob, SCM posns)
{
  Grob *me = unsmob_grob (grob);

  /*
    Code dup.
  */
  vector<Real> x_posns;
  extract_grob_set (me, "stems", stems);
  extract_grob_set (me, "covered-grobs", covered);

  Grob *common[NO_AXES] = { me, me };
  for (Axis a = X_AXIS; a < NO_AXES; incr (a))
    {
      common[a] = common_refpoint_of_array (stems, me, a);
      common[a] = common_refpoint_of_array (covered, common[a], a);
    }
  Grob *fvs = first_normal_stem (me);

  if (!fvs)
    return posns;
  Interval x_span;
  x_span[LEFT] = fvs->relative_coordinate (common[X_AXIS], X_AXIS);
  for (vsize i = 0; i < stems.size (); i++)
    {
      Grob *s = stems[i];

      Real x = s->relative_coordinate (common[X_AXIS], X_AXIS) - x_span[LEFT];
      x_posns.push_back (x);
    }

  Grob *lvs = last_normal_stem (me);
  x_span[RIGHT] = lvs->relative_coordinate (common[X_AXIS], X_AXIS);

  Drul_array<Real> pos = ly_scm2interval (posns);

  scale_drul (&pos, Staff_symbol_referencer::staff_space (me));

  Real beam_dy = pos[RIGHT] - pos[LEFT];
  Real beam_left_y = pos[LEFT];
  Real slope = x_span.delta () ? (beam_dy / x_span.delta ()) : 0.0;

  /*
    Shift the positions so that we have a chance of finding good
    quants (i.e. no short stem failures.)
  */
  Interval feasible_left_point;
  feasible_left_point.set_full ();

  for (vsize i = 0; i < stems.size (); i++)
    {
      Grob *s = stems[i];
      if (Stem::is_invisible (s))
        continue;

      Direction d = get_grob_direction (s);
      Real left_y
        = Stem::get_stem_info (s).shortest_y_
          - slope * x_posns [i];

      /*
        left_y is now relative to the stem S. We want relative to
        ourselves, so translate:
      */
      left_y
      += + s->relative_coordinate (common[Y_AXIS], Y_AXIS)
         - me->relative_coordinate (common[Y_AXIS], Y_AXIS);

      Interval flp;
      flp.set_full ();
      flp[-d] = left_y;

      feasible_left_point.intersect (flp);
    }

  vector<Grob *> filtered;
  /*
    We only update these for objects that are too large for quanting
    to find a workaround.  Typically, these are notes with
    stems, and timesig/keysig/clef, which take out the entire area
    inside the staff as feasible.

    The code below disregards the thickness and multiplicity of the
    beam.  This should not be a problem, as the beam quanting will
    take care of computing the impact those exactly.
  */
  Real min_y_size = 2.0;

  // A list of intervals into which beams may not fall
  vector<Interval> forbidden_intervals;

  for (vsize i = 0; i < covered.size (); i++)
    {
      if (!covered[i]->is_live ())
        continue;

      if (Beam::has_interface (covered[i]) && is_cross_staff (covered[i]))
        continue;

      Box b;
      for (Axis a = X_AXIS; a < NO_AXES; incr (a))
        b[a] = covered[i]->extent (common[a], a);

      if (b[X_AXIS].is_empty () || b[Y_AXIS].is_empty ())
        continue;

      if (intersection (b[X_AXIS], x_span).is_empty ())
        continue;

      filtered.push_back (covered[i]);
      Grob *head_stem = Rhythmic_head::get_stem (covered[i]);
      if (head_stem && Stem::is_normal_stem (head_stem)
          && Note_head::has_interface (covered[i]))
        {
          if (Stem::get_beam (head_stem))
            {
              /*
                We must assume that stems are infinitely long in this
                case, as asking for the length of the stem typically
                leads to circular dependencies.

                This strategy assumes that we don't want to handle the
                collision of beams in opposite non-forced directions
                with this code, where shortening the stems of both
                would resolve the problem, eg.

                 x    x
                |    |
                =====

                =====
                |   |
                x   x

                Such beams would need a coordinating grob to resolve
                the collision, since both will likely want to occupy
                the centerline.
              */
              Direction stemdir = get_grob_direction (head_stem);
              b[Y_AXIS][stemdir] = stemdir * infinity_f;
            }
          else
            {
              // TODO - should we include the extent of the stem here?
            }
        }

      if (b[Y_AXIS].length () < min_y_size)
        continue;

      Direction d = LEFT;
      do
        {
          Real x = b[X_AXIS][d] - x_span[LEFT];
          Real dy = slope * x;

          Direction yd = DOWN;
          Interval disallowed;
          do
            {
              Real left_y = b[Y_AXIS][yd];

              left_y -= dy;

              // Translate back to beam as ref point.
              left_y -= me->relative_coordinate (common[Y_AXIS], Y_AXIS);

              disallowed[yd] = left_y;
            }
          while (flip (&yd) != DOWN);

          forbidden_intervals.push_back (disallowed);
        }
      while (flip (&d) != LEFT);
    }

  Grob_array *arr
    = Pointer_group_interface::get_grob_array (me,
                                               ly_symbol2scm ("covered-grobs"));
  arr->set_array (filtered);

  vector_sort (forbidden_intervals, Interval::left_less);
  Real epsilon = 1.0e-10;
  Interval feasible_beam_placements (beam_left_y, beam_left_y);

  /*
    forbidden_intervals contains a vector of intervals in which
    the beam cannot start.  it iterates through these intervals,
    pushing feasible_beam_placements epsilon over or epsilon under a
    collision.  when this type of change happens, the loop is marked
    as "dirty" and re-iterated.

    TODO: figure out a faster ways that this loop can happen via
    a better search algorithm and/or OOP.
  */

  bool dirty = false;
  do
    {
      dirty = false;
      for (vsize i = 0; i < forbidden_intervals.size (); i++)
        {
          Direction d = DOWN;
          do
            {
              if (forbidden_intervals[i][d] == d * infinity_f)
                feasible_beam_placements[d] = d * infinity_f;
              else if (forbidden_intervals[i].contains (feasible_beam_placements[d]))
                {
                  feasible_beam_placements[d] = d * epsilon + forbidden_intervals[i][d];
                  dirty = true;
                }
            }
          while (flip (&d) != DOWN);
        }
    }
  while (dirty);

  // if the beam placement falls out of the feasible region, we push it
  // to infinity so that it can never be a feasible candidate below
  Direction d = DOWN;
  do
    {
      if (!feasible_left_point.contains (feasible_beam_placements[d]))
        feasible_beam_placements[d] = d * infinity_f;
    }
  while (flip (&d) != DOWN);

  if ((feasible_beam_placements[UP] == infinity_f && feasible_beam_placements[DOWN] == -infinity_f) && !feasible_left_point.is_empty ())
    {
      // We are somewhat screwed: we have a collision, but at least
      // there is a way to satisfy stem length constraints.
      beam_left_y = point_in_interval (feasible_left_point, 2.0);
    }
  else if (!feasible_left_point.is_empty ())
    {
      // Only one of them offers is feasible solution. Pick that one.
      if (abs (beam_left_y - feasible_beam_placements[DOWN]) > abs (beam_left_y - feasible_beam_placements[UP]))
        beam_left_y = feasible_beam_placements[UP];
      else
        beam_left_y = feasible_beam_placements[DOWN];
    }
  else
    {
      // We are completely screwed.
      me->warning (_ ("no viable initial configuration found: may not find good beam slope"));
    }

  pos = Drul_array<Real> (beam_left_y, (beam_left_y + beam_dy));
  scale_drul (&pos, 1 / Staff_symbol_referencer::staff_space (me));

  return ly_interval2scm (pos);
}

void
Beam_scoring_problem::generate_quants (vector<Beam_configuration *> *scores) const
{
  int region_size = (int) parameters_.REGION_SIZE;

  // Knees and collisions are harder, lets try some more possibilities
  if (is_knee_)
    region_size += 2;
  if (collisions_.size ())
    region_size += 2;

  Real straddle = 0.0;
  Real sit = (beam_thickness_ - line_thickness_) / 2;
  Real inter = 0.5;
  Real hang = 1.0 - (beam_thickness_ - line_thickness_) / 2;
  Real base_quants [] = {straddle, sit, inter, hang};
  int num_base_quants = int (sizeof (base_quants) / sizeof (Real));

  /*
    Asymetry ? should run to <= region_size ?
  */
  vector<Real> unshifted_quants;
  for (int i = -region_size; i < region_size; i++)
    for (int j = 0; j < num_base_quants; j++)
      {
        unshifted_quants.push_back (i + base_quants[j]);
      }

  for (vsize i = 0; i < unshifted_quants.size (); i++)
    for (vsize j = 0; j < unshifted_quants.size (); j++)
      {
        Beam_configuration *c
          = Beam_configuration::new_config (unquanted_y_,
                                            Interval (unshifted_quants[i],
                                                      unshifted_quants[j]));

        Direction d = LEFT;
        do
          {
            if (!quant_range_[d].contains (c->y[d]))
              {
                delete c;
                c = NULL;
                break;
              }
          }
        while (flip (&d) != LEFT);
        if (c)
          scores->push_back (c);
      }

}

void Beam_scoring_problem::one_scorer (Beam_configuration *config) const
{
  score_count++;
  switch (config->next_scorer_todo)
    {
    case SLOPE_IDEAL:
      score_slope_ideal (config);
      break;
    case SLOPE_DIRECTION:
      score_slope_direction (config);
      break;
    case SLOPE_MUSICAL:
      score_slope_musical (config);
      break;
    case FORBIDDEN:
      score_forbidden_quants (config);
      break;
    case STEM_LENGTHS:
      score_stem_lengths (config);
      break;
    case COLLISIONS:
      score_collisions (config);
      break;
    case HORIZONTAL_INTER:
      score_horizontal_inter_quants (config);
      break;

    case NUM_SCORERS:
    case ORIGINAL_DISTANCE:
    default:
      assert (false);
    }
  config->next_scorer_todo++;
}

Beam_configuration *
Beam_scoring_problem::force_score (SCM inspect_quants, const vector<Beam_configuration *> &configs) const
{
  Drul_array<Real> ins = ly_scm2interval (inspect_quants);
  Real mindist = 1e6;
  Beam_configuration *best = NULL;
  for (vsize i = 0; i < configs.size (); i++)
    {
      Real d = fabs (configs[i]->y[LEFT] - ins[LEFT]) + fabs (configs[i]->y[RIGHT] - ins[RIGHT]);
      if (d < mindist)
        {
          best = configs[i];
          mindist = d;
        }
    }
  if (mindist > 1e5)
    programming_error ("cannot find quant");

  while (!best->done ())
    one_scorer (best);

  return best;
}

Drul_array<Real>
Beam_scoring_problem::solve () const
{
  vector<Beam_configuration *> configs;
  generate_quants (&configs);

  if (configs.empty ())
    {
      programming_error ("No viable beam quanting found.  Using unquanted y value.");
      return unquanted_y_;
    }

  Beam_configuration *best = NULL;

  bool debug
    = to_boolean (beam_->layout ()->lookup_variable (ly_symbol2scm ("debug-beam-scoring")));
  SCM inspect_quants = beam_->get_property ("inspect-quants");
  if (scm_is_pair (inspect_quants))
    {
      debug = true;
      best = force_score (inspect_quants, configs);
    }
  else
    {
      std::priority_queue < Beam_configuration *, std::vector<Beam_configuration *>,
          Beam_configuration_less > queue;
      for (vsize i = 0; i < configs.size (); i++)
        queue.push (configs[i]);

      /*
        TODO

        It would be neat if we generated new configurations on the
        fly, depending on the best complete score so far, eg.

        if (best->done()) {
          if (best->demerits < sqrt(queue.size())
            break;
          while (best->demerits > sqrt(queue.size()) {
            generate and insert new configuration
          }
        }

        that would allow us to do away with region_size altogether.
      */
      while (true)
        {
          best = queue.top ();
          if (best->done ())
            break;

          queue.pop ();
          one_scorer (best);
          queue.push (best);
        }
    }

  Interval final_positions = best->y;

#if DEBUG_BEAM_SCORING
  if (debug)
    {
      // debug quanting
      int completed = 0;
      for (vsize i = 0; i < configs.size (); i++)
        {
          if (configs[i]->done ())
            completed++;
        }

      string card = best->score_card_ + to_string (" c%d/%d", completed, configs.size ());
      beam_->set_property ("annotation", ly_string2scm (card));
    }
#endif

  junk_pointers (configs);
  return final_positions;
}

void
Beam_scoring_problem::score_stem_lengths (Beam_configuration *config) const
{
  Real limit_penalty = parameters_.STEM_LENGTH_LIMIT_PENALTY;
  Drul_array<Real> score (0, 0);
  Drul_array<int> count (0, 0);

  for (vsize i = 0; i < stem_xpositions_.size (); i++)
    {
      Real x = stem_xpositions_[i];
      Real dx = x_span_.delta ();
      Real beam_y = dx
                    ? config->y[RIGHT] * (x - x_span_[LEFT]) / dx + config->y[LEFT] * (x_span_[RIGHT] - x) / dx
                    : (config->y[RIGHT] + config->y[LEFT]) / 2;
      Real current_y = beam_y + base_lengths_[i];
      Real length_pen = parameters_.STEM_LENGTH_DEMERIT_FACTOR;

      Stem_info info = stem_infos_[i];
      Direction d = info.dir_;

      score[d] += limit_penalty * max (0.0, (d * (info.shortest_y_ - current_y)));

      Real ideal_diff = d * (current_y - info.ideal_y_);
      Real ideal_score = shrink_extra_weight (ideal_diff, 1.5);

      /* We introduce a power, to make the scoring strictly
         convex. Otherwise a symmetric knee beam (up/down/up/down)
         does not have an optimum in the middle. */
      if (is_knee_)
        ideal_score = pow (ideal_score, 1.1);

      score[d] += length_pen * ideal_score;
      count[d]++;
    }

  /* Divide by number of stems, to make the measure scale-free. */
  Direction d = DOWN;
  do
    score[d] /= max (count[d], 1);
  while (flip (&d) != DOWN);

  config->add (score[LEFT] + score[RIGHT], "L");
}

void
Beam_scoring_problem::score_slope_direction (Beam_configuration *config) const
{
  Real dy = config->y.delta ();
  Real damped_dy = unquanted_y_.delta ();
  Real dem = 0.0;
  /*
    DAMPING_DIRECTION_PENALTY is a very harsh measure, while for
    complex beaming patterns, horizontal is often a good choice.

    TODO: find a way to incorporate the complexity of the beam in this
    penalty.
  */
  if (sign (damped_dy) != sign (dy))
    {
      if (!dy)
        {
          if (fabs (damped_dy / x_span_.delta ()) > parameters_.ROUND_TO_ZERO_SLOPE)
            dem += parameters_.DAMPING_DIRECTION_PENALTY;
          else
            dem += parameters_.HINT_DIRECTION_PENALTY;
        }
      else
        dem += parameters_.DAMPING_DIRECTION_PENALTY;
    }

  config->add (dem, "Sd");
}

// Score for going against the direction of the musical pattern
void
Beam_scoring_problem::score_slope_musical (Beam_configuration *config) const
{
  Real dy = config->y.delta ();
  Real dem = parameters_.MUSICAL_DIRECTION_FACTOR
             * max (0.0, (fabs (dy) - fabs (musical_dy_)));
  config->add (dem, "Sm");
}

// Score deviation from calculated ideal slope.
void
Beam_scoring_problem::score_slope_ideal (Beam_configuration *config) const
{
  Real dy = config->y.delta ();
  Real damped_dy = unquanted_y_.delta ();
  Real dem = 0.0;

  Real slope_penalty = parameters_.IDEAL_SLOPE_FACTOR;

  /* Xstaff beams tend to use extreme slopes to get short stems. We
     put in a penalty here. */
  if (is_xstaff_)
    slope_penalty *= 10;

  /* Huh, why would a too steep beam be better than a too flat one ? */
  dem += shrink_extra_weight (fabs (damped_dy) - fabs (dy), 1.5)
         * slope_penalty;

  config->add (dem, "Si");
}

static Real
my_modf (Real x)
{
  return x - floor (x);
}

// TODO - there is some overlap with forbidden quants, but for
// horizontal beams, it is much more serious to have stafflines
// appearing in the wrong place, so we have a separate scorer.
void
Beam_scoring_problem::score_horizontal_inter_quants (Beam_configuration *config) const
{
  if (config->y.delta () == 0.0
      && abs (config->y[LEFT]) < staff_radius_ * staff_space_)
    {
      Real yshift = config->y[LEFT] - 0.5 * staff_space_;
      if (fabs (my_round (yshift) - yshift) < 0.01 * staff_space_)
        config->add (parameters_.HORIZONTAL_INTER_QUANT_PENALTY, "H");
    }
}

/*
  TODO: The fixed value SECONDARY_BEAM_DEMERIT is probably flawed:
  because for 32nd and 64th beams the forbidden quants are relatively
  more important than stem lengths.
*/
void
Beam_scoring_problem::score_forbidden_quants (Beam_configuration *config) const
{
  Real dy = config->y.delta ();

  Real extra_demerit = parameters_.SECONDARY_BEAM_DEMERIT
                       / max (edge_beam_counts_[LEFT], edge_beam_counts_[RIGHT]);

  Direction d = LEFT;
  Real dem = 0.0;
  Real eps = parameters_.BEAM_EPS;

  do
    {
      for (int j = 1; j <= edge_beam_counts_[d]; j++)
        {
          Direction stem_dir = edge_dirs_[d];

          /*
            The 2.2 factor is to provide a little leniency for
            borderline cases. If we do 2.0, then the upper outer line
            will be in the gap of the (2, sit) quant, leading to a
            false demerit.
          */
          Real gap1 = config->y[d] - stem_dir * ((j - 1) * beam_translation_ + beam_thickness_ / 2 - line_thickness_ / 2.2);
          Real gap2 = config->y[d] - stem_dir * (j * beam_translation_ - beam_thickness_ / 2 + line_thickness_ / 2.2);

          Interval gap;
          gap.add_point (gap1);
          gap.add_point (gap2);

          for (Real k = -staff_radius_;
               k <= staff_radius_ + eps; k += 1.0)
            if (gap.contains (k))
              {
                Real dist = min (fabs (gap[UP] - k), fabs (gap[DOWN] - k));

                /*
                  this parameter is tuned to grace-stem-length.ly
                */
                Real fixed_demerit = 0.4;

                dem += extra_demerit
                       * (fixed_demerit
                          + (1 - fixed_demerit) * (dist / gap.length ()) * 2);
              }
        }
    }
  while ((flip (&d)) != LEFT);

  if (max (edge_beam_counts_[LEFT], edge_beam_counts_[RIGHT]) >= 2)
    {
      Real straddle = 0.0;
      Real sit = (beam_thickness_ - line_thickness_) / 2;
      Real inter = 0.5;
      Real hang = 1.0 - (beam_thickness_ - line_thickness_) / 2;

      Direction d = LEFT;
      do
        {
          if (edge_beam_counts_[d] >= 2
              && fabs (config->y[d] - edge_dirs_[d] * beam_translation_) < staff_radius_ + inter)
            {
              // TODO up/down symmetry.
              if (edge_dirs_[d] == UP && dy <= eps
                  && fabs (my_modf (config->y[d]) - sit) < eps)
                dem += extra_demerit;

              if (edge_dirs_[d] == DOWN && dy >= eps
                  && fabs (my_modf (config->y[d]) - hang) < eps)
                dem += extra_demerit;
            }

          if (edge_beam_counts_[d] >= 3
              && fabs (config->y[d] - 2 * edge_dirs_[d] * beam_translation_) < staff_radius_ + inter)
            {
              // TODO up/down symmetry.
              if (edge_dirs_[d] == UP && dy <= eps
                  && fabs (my_modf (config->y[d]) - straddle) < eps)
                dem += extra_demerit;

              if (edge_dirs_[d] == DOWN && dy >= eps
                  && fabs (my_modf (config->y[d]) - straddle) < eps)
                dem += extra_demerit;
            }
        }
      while (flip (&d) != LEFT);
    }

  config->add (dem, "F");
}

void
Beam_scoring_problem::score_collisions (Beam_configuration *config) const
{
  Real demerits = 0.0;
  for (vsize i = 0; i < collisions_.size (); i++)
    {
      Interval collision_y = collisions_[i].y_;
      Real x = collisions_[i].x_;

      Real center_beam_y = y_at (x, config);
      Interval beam_y = center_beam_y + collisions_[i].beam_y_;

      Real dist = infinity_f;
      if (!intersection (beam_y, collision_y).is_empty ())
        dist = 0.0;
      else
        dist = min (beam_y.distance (collision_y[DOWN]),
                    beam_y.distance (collision_y[UP]));

      Real scale_free
        = max (parameters_.COLLISION_PADDING - dist, 0.0) /
          parameters_.COLLISION_PADDING;
      demerits
      += collisions_[i].base_penalty_ *
         pow (scale_free, 3) * parameters_.COLLISION_PENALTY;
    }

  config->add (demerits, "C");
}
