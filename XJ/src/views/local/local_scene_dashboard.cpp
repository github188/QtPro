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


#define MAX_CHART_ENTRY 50

#include "local_scene.h"
#include "views/item_button.h"
#include "models/local/local_track_model.h"

#include "debug.h"

#include <QtGui>

/*
********************************************************************************
*                                                                              *
*    Class LocalScene                                                          *
*        -> dashboard method                                                   *
********************************************************************************
*/
void LocalScene::populateDashBoardScene()
{
    Debug::debug() << " ---- LocalScene::populateDashBoardScene";
    
    //! header item
    HeaderItem* header = new HeaderItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
    header->setText( tr("Dashboard") );
    header->setPos(0, 5);
    addItem(header);
    
    //! content    
    populateMostPlayedAlbum(itemsBoundingRect().height() + 40);
    populateTopRatedAlbum(itemsBoundingRect().height() + 40);
    populateMostPlayedArtist(itemsBoundingRect().height() + 40);
    populateTopRatedArtist(itemsBoundingRect().height() + 40);
}



/*******************************************************************************
    album part
*******************************************************************************/
void LocalScene::populateMostPlayedAlbum(int YPos)
{
    int albumRow  = 0;
    int Column    = 0;
    item_count    = (parentView()->width()/160 > 2) ? parentView()->width()/160 : 2;
    int maxPlaycount = 0;

    int char_entry = 0;

    CategorieGraphicItem *category = new CategorieGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
    category->m_name = QString(tr("Most played albums"));
    category->setPos(0 ,YPos);
    addItem(category);

    //! get Album List from LocalTrackModel
    QList<MEDIA::AlbumPtr> albums = m_localTrackModel->albumItemList;

    //! sort
    qSort(albums.begin(), albums.end(), MEDIA::compareAlbumItemPlaycount);

    //! loop over album MediaItem
    foreach(MEDIA::AlbumPtr album, albums)
    {
      if(album->playcount == 0 || char_entry == MAX_CHART_ENTRY) break;

      if(maxPlaycount == 0)
        maxPlaycount = album->playcount;

      if( !m_localTrackModel->isAlbumFiltered(album) ) continue;

      AlbumGraphicItem_v3 *album_item = new AlbumGraphicItem_v3();
      album_item->media = album;
      album_item->setPos(4+160*Column, YPos + 35 + albumRow*190);

      addItem(album_item);
      char_entry++;

      if(Column < (item_count-1)) {
        Column++;
      }
      else {
        Column = 0;
        albumRow++;
      }
    }

    //! si liste vide --> message
    if(char_entry==0) {
      InfoGraphicItem *info = new InfoGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
      info->_text = tr("No entry found");
      info->setPos( 0 , YPos + 50);
      addItem(info);
    }
}

void LocalScene::populateTopRatedAlbum(int YPos)
{
    int albumRow  = 0;
    int Column    = 0;
    item_count    = (parentView()->width()/160 > 2) ? parentView()->width()/160 : 2;
    int char_entry = 0;

    CategorieGraphicItem *category = new CategorieGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
    category->m_name = QString(tr("Top rated albums"));
    category->setPos(0 ,YPos);
    addItem(category);

    //! get Album List from LocalTrackModel
    QList<MEDIA::AlbumPtr> albums = m_localTrackModel->albumItemList;

    //! sort
    qSort(albums.begin(), albums.end(), MEDIA::compareAlbumItemRating);

    //! loop over album MediaItem
    foreach(MEDIA::AlbumPtr album, albums)
    {
      if(album->rating == 0.0 || char_entry == MAX_CHART_ENTRY) break;

      if( !m_localTrackModel->isAlbumFiltered(album) ) continue;

      AlbumGraphicItem_v4 *album_item = new AlbumGraphicItem_v4();
      album_item->media = album;
      album_item->setPos(4+160*Column, YPos + 35 + albumRow*190);

      addItem(album_item);
      char_entry++;

      if(Column < (item_count-1)) {
        Column++;
      }
      else {
        Column = 0;
        albumRow++;
      }
    }

    //! si liste vide --> message
    if(char_entry==0) {
      InfoGraphicItem *info = new InfoGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
      info->_text = tr("No entry found");
      info->setPos( 0 , YPos + 50);
      addItem(info);
    }
}



/*******************************************************************************
    artist part
*******************************************************************************/
void LocalScene::populateMostPlayedArtist(int YPos)
{
    int artistRow  = 0;
    int Column     = 0;
    item_count = (parentView()->width()/160 > 2) ? parentView()->width()/160 : 2;
    int char_entry    = 0;

    
    CategorieGraphicItem *category = new CategorieGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
    category->m_name = QString(tr("Most played artists"));
    category->setPos(0 ,YPos);
    addItem(category);

    //! get artists List from LocalTrackModel
    QList<MEDIA::ArtistPtr> artists;
    for (int i=0 ; i < m_localTrackModel->rootItem()->childCount(); i++ ) {
       artists << MEDIA::ArtistPtr::staticCast(m_localTrackModel->rootItem()->child(i));
     }

    //! sort
    qSort(artists.begin(), artists.end(), MEDIA::compareArtistItemPlaycount);


    //! loop over artist MediaItem
    foreach(MEDIA::ArtistPtr artist, artists)
    {
      if(artist->playcount == 0 || char_entry == MAX_CHART_ENTRY) break;

      if( !m_localTrackModel->isArtistFiltered(artist) ) continue;


      ArtistGraphicItem_v2 *artist_item = new ArtistGraphicItem_v2();
      artist_item->media = artist;
      artist_item->setPos(4+160*Column, YPos + 35 + artistRow*190);

      addItem(artist_item);
      char_entry++;

      //! ALBUM COVER LOOP
      artist->album_covers.clear();
      for(int j = artist->childCount()-1 ; j >= 0; j--) {
          MEDIA::AlbumPtr album = MEDIA::AlbumPtr::staticCast(artist->child(j));
          if(!m_localTrackModel->isAlbumFiltered(album) ) continue;

          artist->album_covers.prepend(album);

          //!WARNING limite de l'affichage � 6 cover max
          if(artist->album_covers.size() >=6) break;
      }

      if(Column < (item_count-1)) {
        Column++;
      }
      else {
        Column = 0;
        artistRow++;
      }
    }

    //! si liste vide --> message
    if(char_entry==0) {
      InfoGraphicItem *info = new InfoGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
      info->_text = tr("No entry found");
      info->setPos( 0 , YPos + 50);
      addItem(info);
    }
}


void LocalScene::populateTopRatedArtist(int YPos)
{
    int artistRow  = 0;
    int Column     = 0;
    item_count = (parentView()->width()/160 > 2) ? parentView()->width()/160 : 2;
    int char_entry = 0;

    CategorieGraphicItem *category = new CategorieGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
    category->m_name = QString(tr("Top rated artists"));
    category->setPos(0 ,YPos);
    addItem(category);

    //! get artists List from LocalTrackModel
    QList<MEDIA::ArtistPtr> artists;
    for (int i=0 ; i < m_localTrackModel->rootItem()->childCount(); i++ ) {
       artists << MEDIA::ArtistPtr::staticCast(m_localTrackModel->rootItem()->child(i));
     }

    //! sort
    qSort(artists.begin(), artists.end(), MEDIA::compareArtistItemRating);

    //! loop over artist MediaItem
    foreach(MEDIA::ArtistPtr artist, artists)
    {
      if(artist->rating == 0.0 || char_entry == MAX_CHART_ENTRY) break;

      if( !m_localTrackModel->isArtistFiltered(artist) ) continue;


      ArtistGraphicItem_v3 *artist_item = new ArtistGraphicItem_v3();
      artist_item->media = artist;
      artist_item->setPos(4+160*Column, YPos + 35 + artistRow*190);

      addItem(artist_item);
      char_entry++;

      //! ALBUM COVER LOOP
      artist->album_covers.clear();
      for(int j = artist->childCount()-1 ; j >= 0; j--) {
          MEDIA::AlbumPtr album = MEDIA::AlbumPtr::staticCast(artist->child(j));
          if(!m_localTrackModel->isAlbumFiltered(album) ) continue;

          artist->album_covers.prepend(album);

          //!WARNING limite de l'affichage � 6 cover max
          if(artist->album_covers.size() >=6) break;
      }


      if(Column < (item_count-1)) {
        Column++;
      }
      else {
        Column = 0;
        artistRow++;
      }
    }

    //! si liste vide --> message
    if(char_entry==0) {
      InfoGraphicItem *info = new InfoGraphicItem(qobject_cast<QGraphicsView*> (parentView())->viewport());
      info->_text = tr("No entry found");
      info->setPos( 0 , YPos + 50);
      addItem(info);
    }
}

