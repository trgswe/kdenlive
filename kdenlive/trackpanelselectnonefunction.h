/***************************************************************************
                          trackpanelselectnonefunction  -  description
                             -------------------
    begin                : Mon Dec 29 2003
    copyright            : (C) 2003 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef TRACKPANELSELECTNONEFUNCTION_H
#define TRACKPANELSELECTNONEFUNCTION_H

#include <trackpanelfunction.h>


namespace Gui {
    class KTimeLine;

}
/**
This function does nothing except call a "Select None" on the document. It is generally used as the last of a tracks functions, in order to clear the selection if no clip is there.

@author Jason Wood
*/ class TrackPanelSelectNoneFunction:public TrackPanelFunction
{
  public:
    TrackPanelSelectNoneFunction(Gui::KdenliveApp * app,
	Gui::KTimeLine * timeline, KdenliveDoc * doc);

    ~TrackPanelSelectNoneFunction();

	/**
	Returns true if the specified position should cause this function to activate,
	otherwise returns false.
	*/
    virtual bool mouseApplies(Gui::KTrackPanel *, QMouseEvent *) const;

	/**
	Returns a relevant mouse cursor for the given mouse position
	*/
    virtual QCursor getMouseCursor(Gui::KTrackPanel *, QMouseEvent *);

	/**
	A mouse button has been pressed. Returns true if we want to handle this event
	*/
    virtual bool mousePressed(Gui::KTrackPanel *, QMouseEvent * event);

	/** Processes Mouse double click.*/
    virtual bool mouseDoubleClicked(Gui::KTrackPanel *, QMouseEvent *);


	/**
	Mouse Release Events in the track view area. Returns true if we have finished
	an operation now.
	*/
    virtual bool mouseReleased(Gui::KTrackPanel *, QMouseEvent * event);

	/**
	Processes Mouse Move events in the track view area. Returns true if we are
	continuing with the drag.*/
    virtual bool mouseMoved(Gui::KTrackPanel *, QMouseEvent * event);
  private:
     Gui::KdenliveApp * m_app;
     Gui::KTimeLine * m_timeline;
     KdenliveDoc *m_doc;
     double m_fps;
     bool m_multiselect;
     QPoint m_multiselectStart;
};

#endif
