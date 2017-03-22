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

#include "file_scene.h"
#include "models/filesystem/file_model.h"
#include "views/filesystem/file_graphicitem.h"
#include "views/local/local_item.h"
#include "views/item_common.h"

#include "playqueue/virtual_playqueue.h"
#include "core/mediaitem/playlist_parser.h"
#include "mediamimedata.h"


#include "settings.h"
#include "global_actions.h"
#include "debug.h"

#include <QDir>
#include <QDirIterator>
#include <QtGui>

/*
********************************************************************************
*                                                                              *
*    Class FileScene                                                           *
*                                                                              *
********************************************************************************
*/
FileScene::FileScene(QWidget* parent) : SceneBase(parent)
{
}


/*******************************************************************************
    initScene
*******************************************************************************/
void FileScene::initScene()
{
    /* init model */
    m_model        = new FileModel(this);
    m_current_path = m_model->rootPath();
 
    /* get folder pixmap */
    QIcon icon = m_model->fileIcon(m_model->index(m_model->rootPath()));
    m_folder_pixmap = icon.pixmap ( QSize(90,90) ,QIcon::Normal, QIcon::On);    

    connect(m_model, SIGNAL(directoryLoaded ( const QString & )), this, SLOT(slot_on_directory_loaded()));

    ACTIONS()->insert(BROWSER_DIR_ITEM_MOUSE_MOVE, new QAction(this));
    connect(ACTIONS()->value(BROWSER_DIR_ITEM_MOUSE_MOVE), SIGNAL(triggered()), this, SLOT(slot_item_mouseMove()));
    
    connect(ACTIONS()->value(BROWSER_UP), SIGNAL(triggered()), this, SLOT(slot_on_go_up()));
    
    loading_directory = false;
    
    setInit(true);
}



/*******************************************************************************
    resizeScene
*******************************************************************************/
void FileScene::resizeScene()
{
    if(loading_directory) return;

    int new_item_count = (parentView()->width()/140 > 2) ? parentView()->width()/140 : 2;

    if(item_count != new_item_count)  
    {
      slot_on_directory_loaded();
    }
    else
    {
      update();
    }
} 

/*******************************************************************************
     setFilter
*******************************************************************************/
void FileScene::setFilter(const QString& filter)
{
    m_filter = filter;
}

/*******************************************************************************
     setData
*******************************************************************************/
void FileScene::setData(const QVariant& data)
{
    QString path = data.toString();
    
    if( QDir(path).exists() )
    {
      SETTINGS()->_filesystem_path = path;
    
      setPath(path);
      
      ACTIONS()->value(BROWSER_UP)->setEnabled(QDir::rootPath() != path);
    }
    else
    {
      Debug::warning() << "   [FileScene] directory not found " << path;
      setPath(QDir::rootPath());
      ACTIONS()->value(BROWSER_UP)->setEnabled(false);
    }
}

    
/*******************************************************************************
    populateScene
*******************************************************************************/
void FileScene::populateScene()
{
}
    
/*******************************************************************************
    slot_on_directory_loaded
*******************************************************************************/
void FileScene::slot_on_directory_loaded()
{
    loading_directory = true;
    Debug::debug() << "   [FileScene] slot_on_directory_loaded";

    /* clear the scene FIRST */
    clear();
    m_infosize = 0;
    
    QStringList urls;
    
    /* populate scene */
    int folderRow     = 0;
    int trackRow      = 0;
    int Column        = 0;
    int categorieRow  = 0;
    
    item_count = (parentView()->width()/140 > 2) ? parentView()->width()/140 : 2;


    
    //! header item
    HeaderItem* header = new HeaderItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
    header->setText( tr("Directories") );
    header->setPos(0, 5);
    addItem(header);    

    categorieRow++;
    
    CategorieGraphicItem *cat = new CategorieGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
    cat->m_name = m_model->rootPath();
    cat->setPos( 0 ,10 + categorieRow*50);
    categorieRow++;
    addItem(cat);


    
    QModelIndex root_idx = m_model->index(m_current_path);
    
    
    for(int childRow = 0; childRow < m_model->rowCount(root_idx); ++childRow) 
    {
        QModelIndex childIndex = root_idx.child(childRow, 0);

        QFileInfo fileInfo = m_model->fileInfo ( childIndex );

        const QString path = fileInfo.canonicalFilePath();

        if(!m_filter.isEmpty() && !path.contains(m_filter,Qt::CaseInsensitive)) continue;

        if(fileInfo.isDir()) {
          DirectoryGraphicItem *item = new DirectoryGraphicItem();
          item->setPath(fileInfo.canonicalFilePath());
          item->setDirname(QDir(fileInfo.canonicalFilePath()).dirName());
          item->setPixmap(m_folder_pixmap);
          item->setPos(4+140*Column, 10 + folderRow*150 + categorieRow*50);
          addItem(item);
          //Debug::debug() << "FileScene::PopulateScene add item : " << item->path();
          m_infosize++;
        } // isDir
        else
        {
          urls << path;
          continue;
        }

        if(Column < (item_count-1)) {
          Column++;
        }
        else {
          Column = 0;
          folderRow++;
        }
    }  

    if(Column > 0)
      folderRow++;
    
    foreach (QString url, urls) 
    {    
        TrackGraphicItem_v4 *track_item = new TrackGraphicItem_v4();
      
        MEDIA::TrackPtr track = MEDIA::FromDataBase(url);
  
        //Debug::debug() << "FileScene try find url : " << url;
        if(!track) 
        {
          track = MEDIA::TrackPtr(new MEDIA::Track());

          track->id           =  -1;
          track->url          =  url;
          track->isPlaying    =  false;
          track->isBroken     =  false;
          track->isPlayed     =  false;
          track->isStopAfter  =  false;
  
          map_graphic_items[url] = track_item;
          QtConcurrent::run(this, &FileScene::async_load_item, track, url);
        }


        track_item->media = track;
        track_item->setPos(30,  10 + folderRow*150 + categorieRow*50 + trackRow*20);
        track_item->_width = parentView()->width()-30-20;
        addItem(track_item);
        trackRow++; 
        m_infosize++;


    }
    
    
    if(m_infosize==0) {
      InfoGraphicItem *info = new InfoGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
      info->_text = tr("No entry found");
      info->setPos( 0 , 10 + folderRow*150 + categorieRow*50);
      addItem(info);
    }    
    
    
    /* we need to ajust SceneRect */
    setSceneRect ( itemsBoundingRect().adjusted(0, -10, 0, 40) );
    loading_directory = false;
}

/*******************************************************************************
    async_load_item
*******************************************************************************/
void FileScene::async_load_item(MEDIA::TrackPtr track,QString url)
{
    //Debug::debug() << "FileScene::async_load_item";
    if ( MEDIA::isPlaylistFile(url) ) return;
    
    track = MEDIA::FromLocalFile(url);
    track->id = -2;
    
    if( map_graphic_items.contains(url) ) {
      //Debug::debug() << "FileScene::async_load_item update track " << track;
      QGraphicsItem* item = map_graphic_items.take(url);

      TrackGraphicItem_v4 *trackItem = static_cast<TrackGraphicItem_v4*>(item);
      trackItem->media = track;
      
      trackItem->update();
    }
}



/*******************************************************************************
    mouseDoubleClickEvent
*******************************************************************************/
void FileScene::slot_on_go_up()
{
    //Debug::debug() << "   [FileScene] slot_on_go_up";
    QModelIndex root_idx = m_model->index(m_current_path);
    
    QModelIndex parent_idx = root_idx.parent();
    
    if(parent_idx.isValid()) 
    {
      QFileInfo fileInfo = m_model->fileInfo ( parent_idx );

      const QString path = fileInfo.canonicalFilePath();
      //Debug::debug() << "   [FileScene] slot_on_go_up parent path " << path;
      setPath(path);
    }
}


/*******************************************************************************
    setPath
*******************************************************************************/
void FileScene::setPath(const QString& path )
{
    Debug::debug() << "   [FileScene] setPath " << path;
    if(m_current_path == path)
    {
      slot_on_directory_loaded();
    }
    else
    {
      m_current_path = path;
      m_model->setRootPath(m_current_path);
    }
}

/*******************************************************************************
    contextMenuEvent
*******************************************************************************/
void FileScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    //Debug::debug() << "   [FileScene] contextMenuEvent";
    event->ignore();
}

/*******************************************************************************
    mouseDoubleClickEvent
*******************************************************************************/
void FileScene::mouseDoubleClickEvent ( QGraphicsSceneMouseEvent * event )
{
    Debug::debug() << "   [FileScene] mouseDoubleClickEvent";
    if(event->button() != Qt::LeftButton) {
      event->ignore();
      return;      
    }
    
    m_mouseGrabbedItem = this->itemAt(event->scenePos());

    if(!m_mouseGrabbedItem) {
      event->ignore();
      return;
    }

    if(m_mouseGrabbedItem->type() == GraphicsItem::FileSystemType)
    {
      DirectoryGraphicItem *item = static_cast<DirectoryGraphicItem*>(m_mouseGrabbedItem);
      
      QString new_path = m_model->rootPath() + "/" + item->dirname();
      //Debug::debug() << "   [FileScene] mouseDoubleClickEvent " << item->dirname();
  
      emit load_directory(new_path);
    }
    else if( m_mouseGrabbedItem->type() == GraphicsItem::TrackType ) 
    {
      TrackGraphicItem *item = static_cast<TrackGraphicItem*>(m_mouseGrabbedItem);
      MEDIA::TrackPtr track = item->media;
      
      if( MEDIA::isPlaylistFile(track->url) )
      {
        QList<MEDIA::TrackPtr> list = MEDIA::PlaylistFromFile(track->url);
        VirtualPlayqueue::instance()->addTracksAndPlayAt(list, 0);
      }
      else
      {
        QList<MEDIA::TrackPtr> tracks;
        TrackGraphicItem *item = static_cast<TrackGraphicItem*>(m_mouseGrabbedItem);
        tracks << item->media;
        VirtualPlayqueue::instance()->addTracksAndPlayAt(tracks, 0);

        /* force scene update (item playing state may change */
        QTimer::singleShot(250 /*ms*/, this, SLOT(update()));
      }
    }
        
    // no need to propagate to item
    event->accept();
}

/*******************************************************************************
    slot_item_mouseMove
*******************************************************************************/
void FileScene::slot_item_mouseMove()
{
    //Debug::debug() << "   [FileScene] slot_item_mouseMove";

    // single drag and drop
    QGraphicsItem *gItem = qvariant_cast<QGraphicsItem *>( (ACTIONS()->value(BROWSER_DIR_ITEM_MOUSE_MOVE))->data() );

    switch(gItem->type()) 
    {
      case GraphicsItem::FileSystemType : startDirectoriesDrag(gItem);break;
      case GraphicsItem::TrackType      : startTracksDrag(gItem);     break;
      default:break;
    }
}

/*******************************************************************************
    startDirectoriesDrag
*******************************************************************************/
void FileScene::startDirectoriesDrag(QGraphicsItem* i)
{
    QList<QUrl> urls;
    if(this->selectedItems().isEmpty() || !selectedItems().contains(i))
    {
      DirectoryGraphicItem *item = static_cast<DirectoryGraphicItem*>(i);

      urls << QUrl( item->path() );
    }
    else /* multiple directory drags */
    {
      foreach(QGraphicsItem* gi, selectedItems())
      {
        DirectoryGraphicItem *item = static_cast<DirectoryGraphicItem*>(gi);      
        urls << QUrl( item->path() );
      }
    }
    
    QMimeData* mimedata = new QMimeData();
    mimedata->setUrls(urls);                  
                       
    
    QDrag *drag = new QDrag(parentView());
    drag->setPixmap(m_folder_pixmap);
    drag->setMimeData(mimedata);
    drag->exec();
}

/*******************************************************************************
    startTracksDrag
*******************************************************************************/
void FileScene::startTracksDrag(QGraphicsItem* i)
{
    if(this->selectedItems().isEmpty() || !selectedItems().contains(i))
    {
        TrackGraphicItem *item = static_cast<TrackGraphicItem *>(i);
        item->startDrag(parentView());
    }  
    else /* multiple tracks drags */
    {
      MediaMimeData* mimedata = new MediaMimeData(SOURCE_COLLECTION);

      foreach(QGraphicsItem* gi, selectedItems())
      {
        TrackGraphicItem *item = static_cast<TrackGraphicItem *>(gi);
        mimedata->addTrack(item->media);
      }

      QDrag *drag = new QDrag(parentView());
      drag->setMimeData(mimedata);
      drag->exec();
    }
}
