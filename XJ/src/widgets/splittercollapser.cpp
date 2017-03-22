/****************************************************************************************
*  YAROCK                                                                               *
*  Copyright (c) 2010-2014 Sebastien amardeilh <sebastien.amardeilh+yarock@gmail.com>   *
*                                                                                       *
*  This program is free software; you can redistribute it and/or modify it under        *
*  the terms of the GNU General Public License as published by the Free Software        *
*  Foundation; either version 2 of the License, or (at your option) any later           *
*  version.                                                                             *
*                                                                                       *
*  This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
*  PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
*                                                                                       *
*  You should have received a copy of the GNU General Public License along with         *
*  this program.  If not, see <http://www.gnu.org/licenses/>.                           *
*****************************************************************************************/


//! this part is based on GwenView (image viewer) source code

#include "splittercollapser.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QSplitter>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QTimeLine>

// Local

enum Direction {
	LTR      = 1 << 0,
	RTL      = 1 << 1,
	Vertical = 1 << 2,
	TTB      = Vertical + (1 << 0),
	BTT      = Vertical + (1 << 1)
};

const int TIMELINE_DURATION = 500;

const qreal MINIMUM_OPACITY = 0.3;

struct ArrowTypes {
	ArrowTypes() {}

	ArrowTypes(Qt::ArrowType t1, Qt::ArrowType t2)
	: visible(t1), notVisible(t2) {}

	Qt::ArrowType visible,
	notVisible;

	Qt::ArrowType get(bool isVisible) {
		return isVisible ? visible : notVisible;
	}
};


/*
********************************************************************************
*                                                                              *
*    Private SplitterCollapser                                                 *
*                                                                              *
********************************************************************************
*/
struct SplitterCollapserPrivate
{
    SplitterCollapser*    q;
    QSplitter*            mSplitter;
    QWidget*              mWidget;
    Direction             mDirection;
    QTimeLine*            mOpacityTimeLine;
    QAction               *mAction;

    bool isVertical() const {
      return mDirection & Vertical;
    }

    void updatePosition()
    {
      int x, y;
      QRect widgetRect = mWidget->geometry();
      int splitterWidth = mSplitter->width();
      int handleWidth = mSplitter->handleWidth();
      int width = q->width();

      if (!isVertical()) {
        //! y position to middle of panel
        y = widgetRect.height()/2 - 50;

        if (mDirection == LTR) {
          if (mWidget->isVisible()) {
            x = widgetRect.right() + handleWidth;
          }
          else {
            x = 0;
          }
        }
        else { // RTL
          if (mWidget->isVisible()) {
            x = widgetRect.left() - handleWidth - width;
          }
          else {
            x = splitterWidth - handleWidth - width;
          }
        }
      }
      else { //! Horizontal
        x = 0;
        y = 0;
      }

      q->move(x, y);
    }


    void updateArrow()
    {
      static QMap<Direction, ArrowTypes> arrowForDirection;
      if (arrowForDirection.isEmpty()) {
        arrowForDirection[LTR] = ArrowTypes(Qt::LeftArrow,  Qt::RightArrow);
        arrowForDirection[RTL] = ArrowTypes(Qt::RightArrow, Qt::LeftArrow);
        arrowForDirection[TTB] = ArrowTypes(Qt::UpArrow,    Qt::DownArrow);
        arrowForDirection[BTT] = ArrowTypes(Qt::DownArrow,  Qt::UpArrow);
      }
      q->setArrowType(arrowForDirection[mDirection].get(mWidget->isVisible()));
    }

    void fadeIn()
    {
      mOpacityTimeLine->setDirection(QTimeLine::Forward);
      startTimeLine();
    }


    void fadeOut()
    {
      mOpacityTimeLine->setDirection(QTimeLine::Backward);
      startTimeLine();
    }


    void startTimeLine()
    {
      if (mOpacityTimeLine->state() != QTimeLine::Running) {
        mOpacityTimeLine->start();
      }
    }
};


/*
********************************************************************************
*                                                                              *
*    Class SplitterCollapser                                                   *
*                                                                              *
********************************************************************************
*/
SplitterCollapser::SplitterCollapser(QSplitter* splitter, QWidget* widget, QAction *action)
: QToolButton()
, d(new SplitterCollapserPrivate)
{
    d->q = this;
    d->mAction = action;

    // We do not want our collapser to be added as a regular widget in the splitter!
    setAttribute(Qt::WA_NoChildEventsForParent);

    // Get QHoverEvents
    setAttribute(Qt::WA_Hover);

    // We do not want to get the focused, otherwise user might trigger
    // it while pressing space to browse through images
    setFocusPolicy(Qt::NoFocus);

    d->mOpacityTimeLine = new QTimeLine(TIMELINE_DURATION, this);
    d->mOpacityTimeLine->setFrameRange(int(MINIMUM_OPACITY * 1000), 1000);
    connect(d->mOpacityTimeLine, SIGNAL(valueChanged(qreal)),SLOT(update()));

    d->mWidget = widget;
    d->mWidget->installEventFilter(this);

    d->mSplitter = splitter;
    setParent(d->mSplitter);

    if (splitter->indexOf(widget) < splitter->count() / 2) {
      d->mDirection = LTR;
    } else {
      d->mDirection = RTL;
    }

    if (splitter->orientation() == Qt::Vertical) {
      // FIXME: Ugly!
      d->mDirection = static_cast<Direction>(int(d->mDirection) + int(TTB));
    }

    connect(this, SIGNAL(clicked()), SLOT(slotClicked()));
    d->updatePosition();
    show();
}


SplitterCollapser::~SplitterCollapser()
{
  delete d;
}


bool SplitterCollapser::eventFilter(QObject*, QEvent* event)
{
  switch (event->type()) {
    case QEvent::Resize:
      d->updatePosition();
      break;

    case QEvent::Show:
    case QEvent::Hide:
      d->updatePosition();
      d->updateArrow();
      break;

    default:
      break;
  }
  return false;
}


QSize SplitterCollapser::sizeHint() const
{
  int extent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  QSize sh(extent * 3 / 4, extent * 240 / 100);
  if (d->isVertical()) {
    sh.transpose();
  }
  return sh;
}


void SplitterCollapser::slotClicked()
{
  //d->mWidget->setVisible(!d->mWidget->isVisible());
  d->mAction->trigger();
}


bool SplitterCollapser::event(QEvent* event)
{
  switch (event->type()) {
    case QEvent::HoverEnter: d->fadeIn();   break;
    case QEvent::HoverLeave: d->fadeOut();  break;
    default: break;
  }
  return QToolButton::event(event);
}


void SplitterCollapser::paintEvent(QPaintEvent*)
{
  QStylePainter painter(this);
  qreal opacity = d->mOpacityTimeLine->currentFrame() / 1000.;
  painter.setOpacity(opacity);

  QStyleOptionToolButton opt;
  initStyleOption(&opt);
  if (d->mDirection == LTR) {
    opt.rect.setLeft(-width());
  } else {
    opt.rect.setWidth(width() * 2);
  }

  painter.drawPrimitive(QStyle::PE_PanelButtonTool, opt);

  QStyleOptionToolButton opt2;
  initStyleOption(&opt2);
  painter.drawControl(QStyle::CE_ToolButtonLabel, opt2);
}


void SplitterCollapser::showEvent(QShowEvent*)
{
  d->updateArrow();
}
