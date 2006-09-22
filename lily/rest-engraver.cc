/*
  rest-engraver.cc -- implement Rest_engraver

  source file of the GNU LilyPond music typesetter

  (c) 1997--2006 Han-Wen Nienhuys <hanwen@xs4all.nl>
*/

#include "engraver.hh"

#include "dots.hh"
#include "duration.hh"
#include "item.hh"
#include "pitch.hh"
#include "rhythmic-head.hh"
#include "staff-symbol-referencer.hh"
#include "stream-event.hh"

#include "translator.icc"

class Rest_engraver : public Engraver
{
  Stream_event *rest_event_;
  Item *dot_;
  Grob *rest_;
protected:
  void start_translation_timestep ();
  void process_music ();
  DECLARE_TRANSLATOR_LISTENER (rest);
public:
  TRANSLATOR_DECLARATIONS (Rest_engraver);
};

/*
  Should merge with Note_head_engraver
*/
Rest_engraver::Rest_engraver ()
{
  rest_event_ = 0;
  rest_ = 0;
  dot_ = 0;
}

void
Rest_engraver::start_translation_timestep ()
{
  rest_event_ = 0;
  rest_ = 0;
  dot_ = 0;
}

void
Rest_engraver::process_music ()
{
  if (rest_event_ && !rest_)
    {
      rest_ = make_item ("Rest", rest_event_->self_scm ());

      int durlog = unsmob_duration (rest_event_->get_property ("duration"))->duration_log ();

      rest_->set_property ("duration-log",
			   scm_from_int (durlog));

      int dots = unsmob_duration (rest_event_->get_property ("duration"))->dot_count ();

      if (dots)
	{
	  dot_ = make_item ("Dots", SCM_EOL);

	  Rhythmic_head::set_dots (rest_, dot_);
	  dot_->set_parent (rest_, Y_AXIS);
	  dot_->set_property ("dot-count", scm_from_int (dots));
	}

      Pitch *p = unsmob_pitch (rest_event_->get_property ("pitch"));

      /*
	This is ridiculous -- rests don't have pitch, but we act as if
	our nose is bleeding.
      */
      if (p)
	{
	  int pos = p->steps ();
	  SCM c0 = get_property ("middleCPosition");
	  if (scm_is_number (c0))
	    pos += scm_to_int (c0);

	  rest_->set_property ("staff-position", scm_from_int (pos));
	}
    }
}

IMPLEMENT_TRANSLATOR_LISTENER (Rest_engraver, rest);
void
Rest_engraver::listen_rest (Stream_event *ev)
{
  ASSIGN_EVENT_ONCE (rest_event_, ev);
}

ADD_TRANSLATOR (Rest_engraver,
		/* doc */ "",
		/* create */ "Rest Dots",
		/* accept */ "rest-event",
		/* read */ "middleCPosition",
		/* write */ "");
