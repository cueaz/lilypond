/*
  slur.cc -- implement  Slur

  source file of the LilyPond music typesetter

  (c) 1996, 1997 Han-Wen Nienhuys <hanwen@stack.nl>
*/

/*

  TODO:
  
  think about crossing stems.
  Begin and end should be treated as a Script.
  
 */
#include "slur.hh"
#include "scalar.hh"
#include "lookup.hh"
#include "paper-def.hh"
#include "note-column.hh"
#include "stem.hh"
#include "p-col.hh"
#include "molecule.hh"
#include "debug.hh"
#include "boxes.hh"



void
Slur::add(Note_column*n)
{
    encompass_arr_.push(n);
    add_dependency(n);
}

void
Slur::set_default_dir()
{
    dir_i_ = -1;
    for (int i=0; i < encompass_arr_.size(); i ++) {
	if (encompass_arr_[i]->dir_i_ < 0) {
	    dir_i_ =1;
	    break;
	}
    }
}

void
Slur::do_pre_processing()
{
    right_col_l_  = encompass_arr_.top()->pcol_l_;
    left_col_l_ = encompass_arr_[0]->pcol_l_;    
}

Spanner*
Slur::do_break_at(PCol*l, PCol*r) const
{
    assert(l->line_l_ == r->line_l_);
    Slur*ret = new Slur(*this);

    ret->encompass_arr_.set_size(0);
    for (int i =0; i < encompass_arr_.size(); i++) {
	if (encompass_arr_[i]->pcol_l_->line_l_==l->line_l_)
	    ret->encompass_arr_.push(encompass_arr_[i]);
    }

    return ret;
}

void
Slur::do_post_processing()
{
    if (!dir_i_)
	set_default_dir();
    Real inter_f = paper()->internote();
    if (encompass_arr_[0]->stem_l_)
        left_pos_i_ = rint(encompass_arr_[0]->stem_l_->height()[dir_i_]/inter_f);
    else
        left_pos_i_ = 0;
    if (encompass_arr_.top()->stem_l_)
        right_pos_i_ = rint(encompass_arr_.top()->stem_l_->height()[dir_i_]/inter_f);
    else
        right_pos_i_ = 0;

    left_pos_i_ += dir_i_;
    right_pos_i_ += dir_i_;
}
IMPLEMENT_STATIC_NAME(Slur);
