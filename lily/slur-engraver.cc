/*
  slur-engraver.cc -- implement Slur_engraver

  source file of the GNU LilyPond music typesetter

  (c) 1997--2006 Han-Wen Nienhuys <hanwen@xs4all.nl>
*/

#include "engraver.hh"

#include "context.hh"
#include "directional-element-interface.hh"
#include "international.hh"
#include "note-column.hh"
#include "slur.hh"
#include "spanner.hh"
#include "stream-event.hh"
#include "warn.hh"

#include "translator.icc"

/*
  It is possible that a slur starts and ends on the same note.  At
  least, it is for phrasing slurs: a note can be both beginning and
  ending of a phrase.
*/
class Slur_engraver : public Engraver
{
  Drul_array<Stream_event *> events_;
  Stream_event *running_slur_start_;
  vector<Grob*> slurs_;
  vector<Grob*> end_slurs_;

  void set_melisma (bool);

protected:
  DECLARE_TRANSLATOR_LISTENER (slur);
  DECLARE_ACKNOWLEDGER (accidental);
  DECLARE_ACKNOWLEDGER (dynamic_line_spanner);
  DECLARE_ACKNOWLEDGER (fingering);
  DECLARE_ACKNOWLEDGER (note_column);
  DECLARE_ACKNOWLEDGER (script);
  DECLARE_ACKNOWLEDGER (text_script);
  DECLARE_ACKNOWLEDGER (tie);
  DECLARE_ACKNOWLEDGER (tuplet_number);
  void acknowledge_extra_object (Grob_info);
  void stop_translation_timestep ();
  virtual void finalize ();
  void process_music ();

public:
  TRANSLATOR_DECLARATIONS (Slur_engraver);
};

Slur_engraver::Slur_engraver ()
{
  events_[START] = events_[STOP] = 0;
}

IMPLEMENT_TRANSLATOR_LISTENER (Slur_engraver, slur);
void
Slur_engraver::listen_slur (Stream_event *ev)
{
  Direction d = to_dir (ev->get_property ("span-direction"));
  if (d == START)
    ASSIGN_EVENT_ONCE (events_[START], ev);
  else if (d == STOP)
    ASSIGN_EVENT_ONCE (events_[STOP], ev);
  else ev->origin ()->warning (_ ("Invalid direction of slur-event"));
}

void
Slur_engraver::set_melisma (bool m)
{
  context ()->set_property ("slurMelismaBusy", m ? SCM_BOOL_T : SCM_BOOL_F);
}

void
Slur_engraver::acknowledge_note_column (Grob_info info)
{
  Grob *e = info.grob ();
  for (vsize i = slurs_.size (); i--;)
    Slur::add_column (slurs_[i], e);
  for (vsize i = end_slurs_.size (); i--;)
    Slur::add_column (end_slurs_[i], e);
}

void
Slur_engraver::acknowledge_extra_object (Grob_info info)
{
  Slur::auxiliary_acknowledge_extra_object (info, slurs_, end_slurs_);
}

void
Slur_engraver::acknowledge_accidental (Grob_info info)
{
  acknowledge_extra_object (info);
}

void
Slur_engraver::acknowledge_dynamic_line_spanner (Grob_info info)
{
  acknowledge_extra_object (info);
}

void
Slur_engraver::acknowledge_fingering (Grob_info info)
{
  acknowledge_extra_object (info);
}

void
Slur_engraver::acknowledge_tuplet_number (Grob_info info)
{
  acknowledge_extra_object (info);
}


void
Slur_engraver::acknowledge_script (Grob_info info)
{
  if (!info.grob ()->internal_has_interface (ly_symbol2scm ("dynamic-interface")))
    acknowledge_extra_object (info);
}

void
Slur_engraver::acknowledge_text_script (Grob_info info)
{
  acknowledge_extra_object (info);
}

void
Slur_engraver::acknowledge_tie (Grob_info info)
{
  acknowledge_extra_object (info);
}

void
Slur_engraver::finalize ()
{
  if (slurs_.size ())
    slurs_[0]->warning (_ ("unterminated slur"));
}

void
Slur_engraver::process_music ()
{
  if (events_[STOP])
    {
      if (slurs_.size () == 0)
	events_[STOP]->origin ()->warning (_ ("can't end slur"));

      
      end_slurs_ = slurs_;
      slurs_.clear ();
    }

  if (events_[START] && slurs_.empty ())
    {
      Stream_event *ev = events_[START];

      bool double_slurs = to_boolean (get_property ("doubleSlurs"));

      Grob *slur = make_spanner ("Slur", events_[START]->self_scm ());
      Direction updown = to_dir (ev->get_property ("direction"));
      if (updown && !double_slurs)
	set_grob_direction (slur, updown);

      slurs_.push_back (slur);

      if (double_slurs)
	{
	  set_grob_direction (slur, DOWN);
	  slur = make_spanner ("Slur", events_[START]->self_scm ());
	  set_grob_direction (slur, UP);
	  slurs_.push_back (slur);
	}
    }
  set_melisma (slurs_.size ());
}

void
Slur_engraver::stop_translation_timestep ()
{
  for (vsize i = 0; i < end_slurs_.size (); i++)
    announce_end_grob (end_slurs_[i], SCM_EOL);
  end_slurs_.clear ();
  events_[START] = events_[STOP] = 0;
}

ADD_ACKNOWLEDGER (Slur_engraver, accidental);
ADD_ACKNOWLEDGER (Slur_engraver, dynamic_line_spanner);
ADD_ACKNOWLEDGER (Slur_engraver, fingering);
ADD_ACKNOWLEDGER (Slur_engraver, note_column);
ADD_ACKNOWLEDGER (Slur_engraver, script);
ADD_ACKNOWLEDGER (Slur_engraver, text_script);
ADD_ACKNOWLEDGER (Slur_engraver, tie);
ADD_ACKNOWLEDGER (Slur_engraver, tuplet_number);
ADD_TRANSLATOR (Slur_engraver,
		/* doc */ "Build slur grobs from slur events",
		/* create */ "Slur",
		/* accept */ "slur-event",
		/* read */ "slurMelismaBusy doubleSlurs",
		/* write */ "");
