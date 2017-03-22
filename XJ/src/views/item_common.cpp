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

#include "item_common.h"
#include "settings.h"

#include <QtGui>


/*
********************************************************************************
*                                                                              *
*    Class HeaderItem                                                          *
*                                                                              *
********************************************************************************
*/
HeaderItem::HeaderItem(QWidget* parentView)
{
    setAcceptsHoverEvents(true);
    setAcceptDrops(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    m_parent = parentView;
    
    m_height = 26;
}

HeaderItem::HeaderItem(int height, QWidget* parentView)
{
    setAcceptsHoverEvents(true);
    setAcceptDrops(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    m_parent = parentView;
    
    m_height = height; 
}

    
    
void HeaderItem::setText(const QString& t)
{
    m_text = t;
}

QRectF HeaderItem::boundingRect() const
{
  
    const int w_parent  = m_parent->width() - 10;
    const int width = w_parent < 100 ? 100 : w_parent;

    return QRectF(0, 0, width, m_height);
}

void HeaderItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
Q_UNUSED(option)
Q_UNUSED(widget)

    QColor m_brush_color = SETTINGS()->_baseColor;
    m_brush_color.setAlphaF(0.6);
    
    QRect rect = boundingRect().toRect();
    painter->setPen(QPen( m_brush_color, 0.1, Qt::SolidLine, Qt::RoundCap));
    painter->setBrush(QBrush( m_brush_color ,Qt::SolidPattern));
    painter->drawRoundedRect(rect, 4.0, 4.0);

    //! draw text
    QFont font = QApplication::font();
    font.setBold( true );
    painter->setFont(font);
    painter->setPen(QColor(Qt::white));

    QFontMetrics metric( painter->font() );
    const QString elided = metric.elidedText ( m_text, Qt::ElideRight, boundingRect().toRect().width() -50);
    
    painter->drawText(rect, Qt::AlignCenter | Qt::AlignVCenter, elided);
}


/*
********************************************************************************
*                                                                              *
*    Class HeaderLayoutItem                                                    *
*                                                                              *
********************************************************************************
*/
HeaderLayoutItem::HeaderLayoutItem(QWidget* parentView) : HeaderItem(parentView)
{
    setGraphicsItem(this);
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
}


// Inherited from QGraphicsLayoutItem
void HeaderLayoutItem::setGeometry(const QRectF &geom)
{
    setPos(geom.topLeft());
}

// Inherited from QGraphicsLayoutItem
QSizeF HeaderLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
Q_UNUSED(which);
Q_UNUSED(constraint);
    return boundingRect().size();
}

/*
********************************************************************************
*                                                                              *
*    InfoGraphicItem                                                           *
*                                                                              *
********************************************************************************
*/
InfoGraphicItem::InfoGraphicItem(QWidget* parentView)
{
    setAcceptsHoverEvents(false);
    setAcceptDrops(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    m_parent = parentView;
}

QRectF InfoGraphicItem::boundingRect() const
{
    const int w_parent  = m_parent->width() - 20;
    const int width     = w_parent < 100 ? 100 : w_parent;
    
    return QRectF(0, 0, width, 40);
}

void InfoGraphicItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * )
{
Q_UNUSED(option)
    const QRect rect = boundingRect().toRect();
    const int width  = rect.width(); 

    QFont font_normal = QApplication::font();
    font_normal.setBold(false);
    font_normal.setPointSize(10);

    painter->setFont(font_normal);
    painter->setPen(QApplication::palette().color(QPalette::Disabled,QPalette::WindowText)) ;

    QFontMetrics metric( painter->font() );
    const QString elided = metric.elidedText ( _text, Qt::ElideRight, width -100);

    painter->drawPixmap(rect.adjusted(10,-4,0,0).topLeft() ,QPixmap(":/images/info-message.png") );
    painter->drawText(rect.adjusted(70,6,-100,-10), Qt::AlignVCenter, elided);
}

/*
********************************************************************************
*                                                                              *
*    LoadingGraphicItem                                                        *
*                                                                              *
********************************************************************************
*/
LoadingGraphicItem::LoadingGraphicItem(QWidget* parentView)
{
    setAcceptsHoverEvents(false);
    setAcceptDrops(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slot_timeout()));
    m_timer->start(100); 
     
    m_value  = 0;
    m_parent = parentView;     
}

QRectF LoadingGraphicItem::boundingRect() const
{
    const int w_parent  = m_parent->width() - 20;
    const int width     = w_parent < 100 ? 100 : w_parent;
    
    return QRectF(0, 0, width, 40);
}

void LoadingGraphicItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * )
{
Q_UNUSED(option)
    const QRect rect = boundingRect().toRect();
    const int width  = rect.width();     

    QFont font_normal = QApplication::font();
    font_normal.setBold(false);
    font_normal.setPointSize(10);

    painter->setFont(font_normal);
    painter->setPen(QApplication::palette().color(QPalette::Disabled,QPalette::WindowText)) ;

    QFontMetrics metric( painter->font() );
    const QString elided = metric.elidedText ( _text, Qt::ElideRight, width -100);

    /* draw spinner  */
    static const int constParts=8;
    QRectF rectangle(10, 4, 30, 30);
    QColor col(QApplication::palette().color(QPalette::Text));
    painter->setRenderHint(QPainter::Antialiasing, true);

    double size=(360*16)/(2.0*constParts);
    for (int i=0; i<constParts; ++i) {
        col.setAlphaF((constParts-i)/(1.0*constParts));
        painter->setPen(QPen(col, 2));
        painter->drawArc(rectangle, (((constSpinnerSteps-m_value)*1.0)/(constSpinnerSteps*1.0)*360*16)+(i*2.0*size), size);
    }
    
    /* draw text  */
    painter->drawText(rect.adjusted(70,6,-100,-10), Qt::AlignVCenter, elided);
}

void LoadingGraphicItem::slot_timeout()
{
    if (++m_value>=constSpinnerSteps)
      m_value = 0;

    update();
}


/*
********************************************************************************
*                                                                              *
*    CategorieGraphicItem                                                      *
*                                                                              *
********************************************************************************
*/
CategorieGraphicItem::CategorieGraphicItem(QWidget* parentView)
{
    setAcceptsHoverEvents(false);
    setAcceptDrops(false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    m_parent = parentView; 
}

QRectF CategorieGraphicItem::boundingRect() const
{
    const int w_parent  = m_parent->width() - 10;
    const int width     = w_parent < 100 ? 100 : w_parent;
    
    return QRectF(0, 0, width, 30);
}

void CategorieGraphicItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * )
{
Q_UNUSED(option)

    //! draw categorie text
    QFont font = QApplication::font();
    font.setBold(true);
    font.setPointSize(14);

    painter->setFont( font );

    QColor inlineColor  = SETTINGS()->_baseColor;
    QColor outlineColor = QColor (inlineColor.lighter(140) );

    painter->setPen(inlineColor);
    painter->drawStaticText(5,0, QStaticText(m_name));

    //BEGIN: horizontal line
    {
      QPoint start(boundingRect().toRect().left()+3, boundingRect().toRect().top() + 22);
      QPoint horizontalGradTop(boundingRect().toRect().topLeft());
      horizontalGradTop.rx() += boundingRect().toRect().width() - 6;
      QLinearGradient gradient(start, horizontalGradTop);
      gradient.setColorAt(0, inlineColor);
      gradient.setColorAt(1, outlineColor);
      painter->fillRect(QRect(start, QSize(boundingRect().toRect().width() - 6, 1)), gradient);
    }
    //END: horizontal line
}


/*
********************************************************************************
*                                                                              *
*    CategorieLayoutItem                                                       *
*                                                                              *
********************************************************************************
*/
CategorieLayoutItem::CategorieLayoutItem(QWidget* parentView) :  CategorieGraphicItem(parentView)
{
    setGraphicsItem(this);
    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
}


// Inherited from QGraphicsLayoutItem
void CategorieLayoutItem::setGeometry(const QRectF &geom)
{
    setPos(geom.topLeft());
}

// Inherited from QGraphicsLayoutItem
QSizeF CategorieLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
Q_UNUSED(which);
Q_UNUSED(constraint);
    return boundingRect().size();
}

