/*
  complex-walker.cc -- implement Complex_walker

  source file of the LilyPond music typesetter

  (c) 1997 Han-Wen Nienhuys <hanwen@stack.nl>
*/

#include "staff-column.hh"
#include "voice.hh"
#include "p-score.hh"
#include "complex-staff.hh"
#include "debug.hh"
#include "complex-walker.hh"
#include "walk-regs.hh"

void
Complex_walker::do_post_move()
{
    walk_regs_p_->post_move_processing();
}

void
Complex_walker::do_pre_move()
{
    walk_regs_p_->pre_move_processing();
}

void
Complex_walker::do_announces()
{
    walk_regs_p_->do_announces();
}


void
Complex_walker::try_request(Request*req)
{
    bool b =walk_regs_p_->try_request(req);
    if (!b)
	req->warning("junking request: "  + String(req->name()));
}

void
Complex_walker::process_requests()
{
    Staff_column*c =ptr();

    for (int i=0; i < c->creationreq_l_arr_.size(); i++) {
	try_request(c->creationreq_l_arr_[i]);
    }    
    for (int i=0; i < c->commandreq_l_arr_.size(); i++) {
	try_request(c->commandreq_l_arr_[i]);
    }

    for (int i=0; i < c->musicalreq_l_arr_.size(); i++) {
	try_request(c->musicalreq_l_arr_[i]);
    }

    regs_process_requests();
    do_announces();
}

void
Complex_walker::regs_process_requests()
{
    walk_regs_p_->process_requests();
}

void
Complex_walker::typeset_element(Score_elem *elem_p)
{
    if (!elem_p)
	return;
    if (elem_p->spanner())
	pscore_l_->typeset_spanner(elem_p->spanner(), staff()->pstaff_l_);
    else
	ptr()->typeset_musical_item(elem_p->item()); 
}

Complex_walker::Complex_walker(Complex_staff*s)
    : Staff_walker(s, s->pstaff_l_->pscore_l_)
{
    walk_regs_p_ = new Walker_registers(this);    
    do_post_move();
}


Complex_walker::~Complex_walker()
{
}

Complex_staff*
Complex_walker::staff()
{
    return (Complex_staff*) staff_l_;
}



