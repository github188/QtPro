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
#include "playlistview.h"
#include "playqueue_model.h"
#include "playqueue_proxymodel.h"
#include "core/mediaitem/mediaitem.h"
#include "core/player/engine_base.h"
#include "core/player/engine.h"

#include "online/lastfm.h"

#include "utilities.h"
#include "settings.h"
#include "global_actions.h"
#include "debug.h"



bool SortByRowUptoDown(const QModelIndex& a, const QModelIndex& b) {
  return a.row() < b.row();
}

bool SortByRowDowntoUp(const QModelIndex& a, const QModelIndex& b) {
  return b.row() < a.row();
}


const int cst_minHeightNormal   = 20;
const int cst_minHeightExtended = 40;
/*
********************************************************************************
*                                                                              *
*    Class PlaylistView                                                        *
*                                                                              *
********************************************************************************
*/
PlaylistView::PlaylistView(QWidget *parent, PlayqueueModel* model) : QListView(parent)
{
    Debug::debug() << "--- PlaylistView creation";

    //! Gui
    this->setFrameShape(QFrame::NoFrame);
    this->setAutoFillBackground(false);
    this->setPalette(QApplication::palette());

    //! widget
    this->setEnabled(true);
    this->setAlternatingRowColors(false);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->setUniformItemSizes(true); //!optim
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //! dragndrop
    this->setDragEnabled(true);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(true);
    this->setDragDropMode(QAbstractItemView::DragDrop);
    this->setMouseTracking(true);

    //! vertical scrolbar setup
    this->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

    //! connection
    connect(ACTIONS()->value(LASTFM_LOVE), SIGNAL(triggered()), this, SLOT(slot_lastfm_love()));
    connect(ACTIONS()->value(PLAYQUEUE_REMOVE_ITEM), SIGNAL(triggered()), this, SLOT(removeSelected()));
    connect(ACTIONS()->value(PLAYQUEUE_JUMP_TO_TRACK), SIGNAL(triggered()), this, SLOT(jumpToCurrentlyPlayingTrack()));
    connect(this, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(slot_itemActivated(const QModelIndex &)));
    
    //! model
    m_model = model;
    this->setModel(m_model->proxy());

    //! Delegate
    m_delegate = new PlaylistDelegate(this, m_model);
    this->setItemDelegate(m_delegate);

    connect(m_model, SIGNAL(needSelectionAfterMove(QList<MEDIA::TrackPtr>)), SLOT(selectItems(QList<MEDIA::TrackPtr>)));
    connect(m_model, SIGNAL(modelCleared()), SLOT(slot_model_cleared()));

    m_drop_indicator_row = -1;
    m_drag_over          = false;
}

/*******************************************************************************
    PlaylistView::slot_itemActivated
*******************************************************************************/
// gestion du doucle click sur un item
void PlaylistView::slot_itemActivated(const QModelIndex &index)
{
    //Debug::debug() << "--- PlaylistView::slot_itemActivated" << index.row();
    QModelIndex source_idx = m_model->proxy()->mapToSource(index);
    if (m_model->rowExists(source_idx.row()))
    {
      m_model->setRequestedTrackAt(source_idx.row());

      emit signal_playlist_itemDoubleClicked();
    }
}

/*******************************************************************************
    PlaylistView::slot_model_cleared
*******************************************************************************/
void PlaylistView::slot_model_cleared()
{
    this->selectionModel()->clear();
}


/*******************************************************************************
    PlaylistView::selectionChanged
*******************************************************************************/
void PlaylistView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QListView::selectionChanged(selected, deselected);

    emit selectionChanged();
}

/*******************************************************************************
    PlaylistView::isTrackSelected
*******************************************************************************/
bool PlaylistView::isTrackSelected()
{
    return  !this->selectionModel()->selection().isEmpty();
}

/*******************************************************************************
    PlaylistView::selectItems
*******************************************************************************/
void PlaylistView::selectItems(QList<MEDIA::TrackPtr> list)
{
    this->selectionModel()->clear();

    foreach(MEDIA::TrackPtr track, list) {
      QModelIndex proxy_idx = m_model->proxy()->mapFromSource(
         m_model->index(m_model->rowForTrack(track), 0)
      );
      
      this->selectionModel()->select(proxy_idx, QItemSelectionModel::Select);
    }
}

/*******************************************************************************
    PlaylistView::getSelectedMediaItem
*******************************************************************************/
const MEDIA::TrackPtr PlaylistView::firstSelectedTrack()
{
    // use when play/pause action from mainwindow
    // we get first the selected item and play it (so we have to activate it)
    QItemSelection selection = selectionModel()->selection();
    if (selection.isEmpty())
       MEDIA::TrackPtr(0);

    PlayqueueProxyModel *proxy_model = qobject_cast<PlayqueueProxyModel*>(model());
    selection = proxy_model->mapSelectionToSource(selection);

    QModelIndexList idx_selection = selection.indexes();


    qSort(idx_selection.begin(), idx_selection.end(), SortByRowUptoDown);

    QModelIndex selected_model_index = idx_selection.first();
    if (m_model->rowExists(selected_model_index.row()))
    {
      m_model->setRequestedTrackAt(selected_model_index.row());

      return  m_model->trackAt(selected_model_index.row());
    }

    return MEDIA::TrackPtr(0);
}

/*******************************************************************************
    PlaylistView::removeSelected
*******************************************************************************/
void PlaylistView::removeSelected()
{
    //Debug::debug() << "--- PlaylistView::removeSelected";

    QItemSelection selection = selectionModel()->selection();
    if (selection.isEmpty())
      return;

    PlayqueueProxyModel *proxy_model = qobject_cast<PlayqueueProxyModel*>(model());
    selection = proxy_model->mapSelectionToSource(selection);

    QModelIndexList idx_selection = selection.indexes();

    qSort(idx_selection.begin(), idx_selection.end(), SortByRowDowntoUp);

    foreach (const QModelIndex& selected_model_index, idx_selection) {
       m_model->removeRows(selected_model_index.row(), 1, QModelIndex());
    }
    this->selectionModel()->clear();
}




/*******************************************************************************
    PlaylistView::jumpToCurrentlyPlayingTrack
*******************************************************************************/
void PlaylistView::jumpToCurrentlyPlayingTrack()
{
    /* WARNING test */  
    MEDIA::TrackPtr track = Engine::instance()->playingTrack();
    int row = m_model->rowForTrack(track);
    
    if (row > 0) 
    {
      QModelIndex model_idx = m_model->proxy()->mapFromSource(
         m_model->index(row, 0)
      );

      if (!model_idx.isValid())
        return;
      
      // Scroll to the item
      scrollTo(model_idx, QAbstractItemView::PositionAtCenter);
    } 
}


/*******************************************************************************
    PlaylistView::paintEvent
*******************************************************************************/
void PlaylistView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    
    if(model()->rowCount() < 1)
    {
      const QString message = tr("Add file to play or Drag and Drop File from Collection");
      QFont font = QApplication::font();
      font.setPointSize(10);
      painter.setFont(font);

      const QRect textBox = rect().adjusted(10, 0, -10, 0);

      painter.setPen(QApplication::palette().color(QPalette::Disabled,QPalette::WindowText) );

      painter.drawText(textBox.adjusted(0, 70, 0, 0), Qt::AlignHCenter | Qt::TextWordWrap, message);

      const QPixmap pix = QPixmap(":/images/info-message.png") ;
      painter.drawPixmap(textBox.width()/2 - pix.width()/2, 10 ,pix);
    }

    if (m_drop_indicator_row != -1)
    {
      // Find the y position of the drop indicator
      QModelIndex drop_index = model()->index(m_drop_indicator_row, 0);

      int drop_pos = -1;
      switch (this->dropIndicatorPosition()) {
        case QAbstractItemView::OnItem: break;

        case QAbstractItemView::AboveItem: drop_pos = visualRect(drop_index).top(); break;

        case QAbstractItemView::BelowItem: drop_pos = visualRect(drop_index).bottom() + 1;break;

        case QAbstractItemView::OnViewport:
          if (model()->rowCount() == 0)
            drop_pos = 1;
          else
            drop_pos = visualRect(model()->index(model()->rowCount() - 1, 0)).bottom() + 1;
        break;
      }

      // Draw a nice gradient first
      QColor line_color(QApplication::palette().color(QPalette::Highlight));
      QColor shadow_color(line_color.lighter(140));
      QColor shadow_fadeout_color(shadow_color);
      shadow_color.setAlpha(255);
      shadow_fadeout_color.setAlpha(0);

      QLinearGradient gradient(QPoint(0, drop_pos - 5),QPoint(0, drop_pos + 5));
      gradient.setColorAt(0.0, shadow_fadeout_color);
      gradient.setColorAt(0.5, shadow_color);
      gradient.setColorAt(1.0, shadow_fadeout_color);
      QPen gradient_pen(QBrush(gradient), 5 * 2);
      painter.setPen(gradient_pen);
      painter.drawLine(QPoint(0, drop_pos),QPoint(width(), drop_pos));

      // Now draw the line on top
      QPen line_pen(line_color, 2);
      painter.setPen(line_pen);
      painter.drawLine(QPoint(0, drop_pos),QPoint(width(), drop_pos));
    } // end if m_drop_indicator_row != -1

    QListView::paintEvent(event);
}


void PlaylistView::dragMoveEvent(QDragMoveEvent *event)
{

    QListView::dragMoveEvent(event);
    QModelIndex index(indexAt(event->pos()));
    m_drop_indicator_row = index.isValid() ? index.row() : 0;
}

void PlaylistView::dragEnterEvent(QDragEnterEvent *event)
{
    QListView::dragEnterEvent(event);
    m_drag_over = true;
}

void PlaylistView::dragLeaveEvent(QDragLeaveEvent *event)
{
    QListView::dragLeaveEvent(event);
    m_drag_over = false;
    m_drop_indicator_row = -1;
}

void PlaylistView::dropEvent(QDropEvent *event)
{
    QListView::dropEvent(event);
    m_drop_indicator_row = -1;
    m_drag_over = false;
}

void PlaylistView::mouseMoveEvent(QMouseEvent* event)
{
  if (!m_drag_over)
    QListView::mouseMoveEvent(event);
}

/*******************************************************************************
    PlaylistView::keyPressEvent
*******************************************************************************/
void PlaylistView::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Delete))
    {
      removeSelected();
      event->accept();
    }
    else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
      QItemSelection selection = selectionModel()->selection();
      if (selection.isEmpty())
        return;

      /* keep proxy selection indexes, as map to source is done in slot_itemActivated  */
      QModelIndexList idx_selection = selection.indexes();

      qSort(idx_selection.begin(), idx_selection.end(), SortByRowUptoDown);
      QModelIndex selected_model_index = idx_selection.first();
      if (m_model->rowExists(selected_model_index.row()))
        slot_itemActivated(selected_model_index);

      event->accept();
    }
    else
    {
      QListView::keyPressEvent(event);
    }
}

/*******************************************************************************
    PlaylistView::slot_lastfm_love
*******************************************************************************/
void PlaylistView::slot_lastfm_love()
{
    Debug::debug() << "--- PlaylistView : slot_lastfm_love";
    QItemSelection selection = selectionModel()->selection();
    if (selection.isEmpty())
      return;

    PlayqueueProxyModel *proxy_model = qobject_cast<PlayqueueProxyModel*>(model());
    selection = proxy_model->mapSelectionToSource(selection);

    QModelIndexList idx_selection = selection.indexes();

    foreach (const QModelIndex& selected_model_index, idx_selection)
    {
        if (m_model->rowExists(selected_model_index.row()))
        {
          MEDIA::TrackPtr track = m_model->trackAt( selected_model_index.row() );
          if(track->type() == TYPE_TRACK)
            LastFmService::instance()->love(track);
        }
    }
}


/*
********************************************************************************
*                                                                              *
*    Class PlaylistDelegate                                                    *
*                                                                              *
********************************************************************************
*/
PlaylistDelegate::PlaylistDelegate(QObject *parent, PlayqueueModel* model)
: QItemDelegate(parent)
{
    m_model      = model;

    icon_media_playing  = QIcon(":/images/media-playing.png");
    icon_media_broken   = QIcon(":/images/media-broken-18x18.png");
    icon_media_track    = QIcon(":/images/track-48x48.png");
    icon_media_stream   = QIcon(":/images/media-url-48x48.png");


    font_normal = QApplication::font();
    font_normal.setStyleStrategy(QFont::PreferAntialias);
    font_normal.setBold(false);

    font_bold   = QApplication::font();
    font_bold.setStyleStrategy(QFont::PreferAntialias);
    font_bold.setBold(true);

    _mode = PLAYQUEUE::MODE_EXTENDED;
}

void PlaylistDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    //! get the data object
    QModelIndex mappedIndex = m_model->proxy()->mapToSource(index);
    MEDIA::TrackPtr track   = m_model->trackAt(mappedIndex.row());

    bool isTrack     = (track->type() == TYPE_TRACK) ? true : false;
    bool isBroken    = track->isBroken;
    bool isSelected  = option.state & QStyle::State_Selected;

    const int left   = option.rect.left();
    const int top    = option.rect.top();
    const int width  = option.rect.width();
    const int height = option.rect.height();

    // Turn on antialiasing
    painter->setRenderHint(QPainter::Antialiasing, true);
  
    //! draw background
    QStyleOptionViewItemV4 opt(option);
    opt.state |= QStyle::State_Active;
    opt.state |= QStyle::State_HasFocus;

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    //! set painter font & color
    painter->setFont(font_normal);
    painter->setPen( opt.palette.color ( (track->id!=-1 || isSelected) ? QPalette::Normal : QPalette::Disabled, isSelected ? QPalette::HighlightedText : QPalette::WindowText) );

    QFontMetrics fm( font_normal );

    /*---------------------------------------------*/
    /* TITLE/ALBUM/ARTIST view                     */
    /* --------------------------------------------*/
    if(_mode != PLAYQUEUE::MODE_EXTENDED)
    {
        //! Paint Icon
        QIcon icon = this->getIcon(track);
        icon.paint(painter, left + 3, top + 2, 16, 16, Qt::AlignCenter, QIcon::Normal);

        //! Paint duration
        QRect rectDuree = QRect();
        if(isTrack && !isBroken)
        {
          const QString durationText = track->durationToString();
          painter->drawText(left + width - 50,top, 49, height,Qt::AlignVCenter | Qt::AlignRight, durationText);
          rectDuree = fm.boundingRect ( durationText );
        }

        //! Paint track info
        QString trackinfo = this->getTrackInfo(track);
        trackinfo = fm.elidedText ( trackinfo, Qt::ElideRight, width-51 );
        painter->drawText(left + 25,top,width-rectDuree.width()-5, 20,Qt::AlignVCenter | Qt::AlignLeft, trackinfo);
    }
    /*---------------------------------------------*/
    /* EXTENDED view                               */
    /* --------------------------------------------*/
    else
    {
        //! Paint Icon
        QIcon icon = this->getIcon(track);
        icon.paint(painter, left + 3, top+(height-16)/2, 16, 16, Qt::AlignCenter, QIcon::Normal);

        // Prepare rectangle for painting
        QRect rectDuree = QRect();

        if(!isBroken)
        {
            if(isTrack)
            {
              //! Paint duration
              const QString durationText = track->durationToString();
              painter->drawText(left + width - 50,top, 49, height,Qt::AlignVCenter | Qt::AlignRight, durationText);
              rectDuree = fm.boundingRect ( durationText );
            }

            //! Paint track info
            QString info_2  = isTrack ? track->artist + " - " + track->album : track->url;

            info_2 = fm.elidedText ( info_2, Qt::ElideRight, width-51 );
            painter->drawText(left + 25,top+height/2+1,width-rectDuree.width()-5, fm.height()+2,Qt::AlignTop | Qt::AlignLeft, info_2);

            painter->setFont(font_bold);
            fm = QFontMetrics(font_bold);

            QString track_title  = isTrack ? track->title : track->name;
            track_title = fm.elidedText ( track_title, Qt::ElideRight, width-51 );
            painter->drawText(left + 25,top+height/2-fm.height()-1,width-rectDuree.width()-5, fm.height()+2,Qt::AlignTop | Qt::AlignLeft, track_title);
        }
        else
        {
            //! Paint track info
            QString track_info = fm.elidedText ( track->url, Qt::ElideRight, width-51 );
            painter->drawText(left + 25,top+height/2+1,width-rectDuree.width()-5, fm.height()+2,Qt::AlignTop | Qt::AlignLeft, track_info);

            painter->setFont(font_bold);
            fm = QFontMetrics(font_bold);

            QString info_1  = tr("media reading error");
            info_1 = fm.elidedText ( info_1, Qt::ElideRight, width-51 );
            painter->drawText(left + 25,top+height/2-fm.height()-1,width-rectDuree.width()-5, fm.height()+2,Qt::AlignTop | Qt::AlignLeft, info_1);
        }
    }

    //! draw flag for "stop after" action
    if(track->isStopAfter) {
      int y = (height - 16)/2 /* 16 = stopbox height */;
      drawStop(painter, QRect(left + width - 41,top+y, 40, 16) );
    }
}

QSize PlaylistDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
Q_UNUSED(index)
    int height;
    int min;
    const int padding = 4;
    if (_mode == PLAYQUEUE::MODE_EXTENDED)
    {
      height  =    QFontMetrics(font_normal).height()
                 + QFontMetrics(font_bold).height()
                 + 4 ; /*padding between line */
      min = cst_minHeightExtended;
    }
    else
    {
      height = QFontMetrics(font_normal).height();
      min = cst_minHeightNormal;
    }
    return  QSize( option.rect.width(), qMax(height + padding * 2, min) );
}

QString PlaylistDelegate::getTrackInfo(const MEDIA::TrackPtr track) const
{
    if(track->isBroken) {
      return track->url;
    }
    else if (track->type() == TYPE_TRACK) {
        switch( _mode ) {
          case PLAYQUEUE::MODE_TITLE  : return QString(track->title);break;
          case PLAYQUEUE::MODE_ALBUM  : return QString(track->album + " - " + track->title);break;
          case PLAYQUEUE::MODE_ARTIST : return QString(track->artist + " - " + track->album + " - " + track->title);break;
          default : return track->title;break;
        }
    }
    else {
      return track->name;
    }

    return QString::null;
}

QIcon PlaylistDelegate::getIcon(const MEDIA::TrackPtr track) const
{
    if(track->isPlaying)
      return icon_media_playing;
    else if (track->isBroken)
      return icon_media_broken;
    else if (track->type() == TYPE_TRACK)
      return icon_media_track;
    else
      return icon_media_stream;
}

void PlaylistDelegate::drawStop(QPainter * painter, QRect rect) const
{
    QFont font = QApplication::font();
    font.setBold(true);
    font.setPointSize(7);

    painter->setFont(font);

    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(0.0,  qRgb(102, 150, 227) );
    gradient.setColorAt(1.0, qRgb(77, 121, 200));

    painter->setRenderHint(QPainter::Antialiasing);

    // Draw the box
    painter->setPen(QPen(Qt::white, 1));
    painter->setBrush(gradient);
    painter->drawRoundedRect(rect, 3, 3);

    // Draw the text
    painter->drawText(rect, Qt::AlignCenter, "stop");
}
