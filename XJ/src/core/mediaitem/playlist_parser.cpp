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


#include "playlist_parser.h"
#include "debug.h"

#include <QtCore>

/*******************************************************************************
    Extension
*******************************************************************************/
const QStringList m3u_extension  = QStringList() << "m3u" << "m3u8";
const QStringList pls_extension  = QStringList() << "pls";
const QStringList xspf_extension = QStringList() << "xspf";

/*******************************************************************************
    Internals functions
*******************************************************************************/
QList<MEDIA::TrackPtr>  readM3uPlaylist(QIODevice* device, const QDir& playlist_dir=QDir() );
QList<MEDIA::TrackPtr>  readPlsPlaylist(QIODevice* device, const QDir& playlist_dir=QDir() );
QList<MEDIA::TrackPtr>  readXspfPlaylist(QIODevice* device, const QDir& playlist_dir=QDir() );

void saveM3uPlaylist(QIODevice* device, const QDir& playlist_dir, QList<MEDIA::TrackPtr> list);
void savePlsPlaylist(QIODevice* device, const QDir& playlist_dir, QList<MEDIA::TrackPtr> list);
void saveXspfPlaylist(QIODevice* device, const QDir& playlist_dir, QList<MEDIA::TrackPtr> list);

/*
********************************************************************************
*                                                                              *
*    PlaylistFromFile                                                          *
*                                                                              *
********************************************************************************
*/
QList<MEDIA::TrackPtr> MEDIA::PlaylistFromFile(const QString& filename)
{
    Debug::debug() << "[MEDIA] PlaylistFromFile--> Start "  << QTime::currentTime().second() << ":" << QTime::currentTime().msec();

    //Debug::debug() << "[MEDIA] PlaylistFromFile";
    QList<MEDIA::TrackPtr>   list;

    // Open the file
    QFileInfo info(filename);
    QFile file(filename);


    if (!file.open(QFile::ReadOnly)) {
      Debug::warning() << "Failed to open read only file " << filename;
      return list;
    }

    QString extension = info.suffix().toLower();
    QDir playlist_dir = QDir(info.absoluteDir());

    // choose parser
    if( m3u_extension.contains(extension) )
      list = readM3uPlaylist( &file, playlist_dir);
    else if( pls_extension.contains(extension) )
      list = readPlsPlaylist( &file, playlist_dir);
    else if( xspf_extension.contains(extension ))
      list = readXspfPlaylist( &file, playlist_dir );

    Debug::debug() << "[MEDIA] PlaylistFromFile : read " << list.size() << " entries";
    
    Debug::debug() << "[MEDIA] PlaylistFromFile--> End "  << QTime::currentTime().second() << ":" << QTime::currentTime().msec();
    return list;
}

/*
********************************************************************************
*                                                                              *
*    PlaylistFromBytes                                                         *
*                                                                              *
********************************************************************************
*/
QList<MEDIA::TrackPtr> MEDIA::PlaylistFromBytes(QByteArray& bytes)
{
    Debug::debug() << "MEDIA::PlaylistFromBytes";
    QList<MEDIA::TrackPtr>   list;

    QBuffer buffer(&bytes);
    if (!buffer.open(QFile::ReadOnly)) {
      Debug::debug() << "Failed to open data";
      return list;
    }

    // choose parser
    QByteArray peeked_data = buffer.peek(512);
    if( peeked_data.contains("#EXTM3U") || peeked_data.contains("#EXTINF") )
      list = readM3uPlaylist( &buffer ) ;
    else if( peeked_data.toLower().contains("[playlist]") )
      list = readPlsPlaylist( &buffer ) ;
    else if( peeked_data.contains("<playlist") && peeked_data.contains("<trackList") )
      list = readXspfPlaylist( &buffer ) ;
    else
    {
      // try m3u style
      Debug::warning() << "MEDIA::PlaylistFromBytes unknown format, trying m3u playlist...";
      list = readM3uPlaylist( &buffer ) ;
    }

    return list;
}


/*******************************************************************************
    M3U playlist read
*******************************************************************************/
QList<MEDIA::TrackPtr>  readM3uPlaylist(QIODevice* device, const QDir& playlist_dir )
{
    QList<MEDIA::TrackPtr>   list;

    QString data = QString::fromUtf8(device->readAll());

    if(data.isEmpty()) return list;
    data.replace('\r', '\n');
    data.replace("\n\n", "\n");

    QByteArray bytes = data.toUtf8();
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);

    //! metadata
    int       lenght;
    QString   title;
    QString   artist;

    //! main reading loop
    QString line;

    while(!buffer.atEnd()) {
      line = QString::fromUtf8(buffer.readLine()).trimmed();

      if(line.startsWith("#EXT"))
      {
        QString info = line.section(':', 1);
        QString l    = info.section(',', 0, 0);
        
        /* TODO
        bool ok = false;
        int length = l.toInt(&ok);
        if (!ok) {lenght = -1; continue;}
        */

        QString track_info = info.section(',', 1);
        QStringList list   = track_info.split('-');

        if (list.size() <= 1) {
          title   = track_info;
          continue;
        }

        artist = list[0].trimmed();
        title = list[1].trimmed();
      }
      else if (line.startsWith('#'))
      {
        continue;
      }
      else if( !line.isEmpty() )
      {
        //Debug::debug() << "---- readM3uPlaylist -> line " << line << "\n";
        MEDIA::TrackPtr track = MEDIA::TrackPtr(new MEDIA::Track());

        //! Find the Track location
        if (line.contains(QRegExp("^[a-z]+://"))) {
          QUrl url(line);
          if (url.isValid()) {
              track->setType(TYPE_STREAM);
              track->id          = -1;
              track->url         = url.toString();
              track->name        = title.isEmpty() ? url.toString() : title;
              track->isFavorite  = false;
              track->isPlaying   = false;
              track->isBroken    = false;
              track->isPlayed    = false;
              track->isStopAfter = false;

              list.append(track);
          }
        }
        else {

          QString file_path = line;

          file_path = QDir::fromNativeSeparators(file_path);

          // Make the path absolute
          if (!QDir::isAbsolutePath(file_path))
            file_path = playlist_dir.absoluteFilePath(file_path);

          // Use the canonical path
          if (QFile::exists(file_path))
            file_path = QFileInfo(file_path).canonicalFilePath();

          track->setType(TYPE_TRACK);
          track->id          =  -1;
          track->url         =  file_path;
          track->title       =  title.isEmpty() ? QString() : title;
          track->duration    =  (lenght!=-1) ? lenght : 0;
          track->artist      =  artist.isEmpty() ? QString() : artist;
          track->isPlaying   =  false;
          track->isBroken    =  !QFile::exists(file_path);
          track->isPlayed    =  false;
          track->isStopAfter =  false;

          track->albumGain  =  0.0;
          track->albumPeak  =  0.0;
          track->trackGain  =  0.0;
          track->trackPeak  =  0.0;

          list.append(track);
        }


        lenght   = -1;
        title    = "";
        artist   = "";
      }
    } // end while

  return list;
}

/*******************************************************************************
    .pls playlist read
*******************************************************************************/
QList<MEDIA::TrackPtr>  readPlsPlaylist(QIODevice* device, const QDir& playlist_dir )
{
    Debug::debug() << "start readPlsPlaylist";
    QList<MEDIA::TrackPtr> list;

    MEDIA::TrackPtr mi = MEDIA::TrackPtr(0);

    while (!device->atEnd()) {
      QString line = QString::fromUtf8(device->readLine()).trimmed();
      int equals = line.indexOf('=');
      QString key = line.left(equals).toLower();
      QString value = line.mid(equals + 1);

      if (key.startsWith("file"))
      {
        mi = MEDIA::TrackPtr(new MEDIA::Track());
        list.append(mi);

        //! Find the Track location
        if (value.contains(QRegExp("^[a-z]+://"))) {
          QUrl url(value);
          if (url.isValid()) {
              // Debug::debug() << "---- readPlsPlaylist -> url.isValid()" << url;
              mi->setType(TYPE_STREAM);
              mi->id          = -1;
              mi->url         = value;
              mi->name        = value;
              mi->isFavorite  = false;
              mi->isPlaying   = false;
              mi->isBroken    = false;
              mi->isPlayed    = false;
              mi->isStopAfter = false;
          }
        }
        else {
          QString file_path = value;

          file_path = QDir::fromNativeSeparators(file_path);

          // Make the path absolute
          if (!QDir::isAbsolutePath(file_path))
            file_path = playlist_dir.absoluteFilePath(file_path);

          // Use the canonical path
          if (QFile::exists(file_path))
            file_path = QFileInfo(file_path).canonicalFilePath();

          mi->setType(TYPE_TRACK);
          mi->id          =  -1;
          mi->url         =  file_path;
          mi->isPlaying   =  false;
          mi->isBroken    =  QFile::exists(file_path) ? false : true;
          mi->isPlayed    =  false;
          mi->isStopAfter =  false;
        }
      } // key is filename
      else if (key.startsWith("title"))
      {
        if(mi->type() == TYPE_TRACK)
          mi->title = value;
        else
          mi->name = value;
      }
      else if (key.startsWith("length"))
      {
        if(mi->type() == TYPE_TRACK)
          mi->duration = value.toInt();
      }
    } // fin while

    Debug::debug() << "end readPlsPlaylist";

    return list;
}



/*******************************************************************************
    .xspf playlist read
*******************************************************************************/
QList<MEDIA::TrackPtr>  readXspfPlaylist(QIODevice* device, const QDir& playlist_dir )
{
    Debug::debug() << "[MEDIA] readXspfPlaylist";

    QList<MEDIA::TrackPtr>  list;

    QXmlStreamReader xml(device);

    MEDIA::TrackPtr mi = MEDIA::TrackPtr(0);

    
    
    while(!xml.atEnd() && !xml.hasError())
    {
       xml.readNext();
       if (xml.isStartElement() && xml.name() == "trackList")
         break;
    }
    

    while (!xml.atEnd() && !xml.hasError())
    {
       xml.readNext();
      
      if (xml.isStartElement() && xml.name() == "track")
      {
        //Debug::debug() << "---- readXspfPlaylist -> NEW Track ";
        mi = MEDIA::TrackPtr(new MEDIA::Track());
      }
      else if (xml.isStartElement() && xml.name() == "location")
      {
            QString file_path = QString(xml.readElementText());

            //Debug::debug() << "---- readXspfPlaylist -> Find the Track location" << file_path;
            if (!MEDIA::isLocal(file_path)) {
              QUrl url(file_path);
              if (url.isValid()) {

                //Debug::debug() << "---- readXspfPlaylist -> it's an url";
                if(mi) {
                  mi->setType(TYPE_STREAM);
                  mi->id          = -1;
                  mi->url         = file_path;
                  mi->name        = file_path;
                  mi->isFavorite  = false;
                  mi->isPlaying   = false;
                  mi->isBroken    = false;
                  mi->isPlayed    = false;
                  mi->isStopAfter = false;
                }
              }
            }
            else {
              //Debug::debug() << "---- readXspfPlaylist -> it's a local file";

              file_path = QDir::fromNativeSeparators(file_path);
              //Debug::debug() << "---- readXspfPlaylist -> file_path" << file_path;

              // Make the path absolute
              if (!QDir::isAbsolutePath(file_path))
                file_path = playlist_dir.absoluteFilePath(file_path);
              //Debug::debug() << "---- readXspfPlaylist -> file_path" << file_path;

              // Use the canonical path
              if (QFile::exists(file_path))
                file_path = QFileInfo(file_path).canonicalFilePath();
              //Debug::debug() << "---- readXspfPlaylist -> file_path" << file_path;

              if(mi) {
                mi->setType(TYPE_TRACK);
                mi->id          =  -1;
                mi->url         =  file_path;
                mi->isPlaying   =  false;
                mi->isBroken    =  !QFile::exists(file_path);
                mi->isPlayed    =  false;
                mi->isStopAfter =  false;
             }
           }
      } // end location
      else if (xml.isStartElement() && xml.name() == "title")
      {
          if(mi->type() == TYPE_TRACK)
            mi->title = QString(xml.readElementText());
          else
            mi->name = QString(xml.readElementText());
      }
      else if (xml.isStartElement() && xml.name() == "category")
      {
          if(mi->type() == TYPE_STREAM)
            mi->categorie = QString(xml.readElementText());
      }
      else if (xml.isEndElement() && xml.name() == "track")
      {
        //Debug::debug() << "---- readXspfPlaylist -> list.append(mi)" << mi;
        if(mi)
          list.append(mi);
        mi = MEDIA::TrackPtr(0);
      }
    }  // End while xml end

    //Debug::debug() << "readXspfPlaylist -> END OK";

    return list;
}


/*
********************************************************************************
*                                                                              *
*    PlaylistToFile                                                            *
*                                                                              *
********************************************************************************
*/
void MEDIA::PlaylistToFile(const QString& filename, QList<MEDIA::TrackPtr> list)
{
    Debug::debug() << "[MEDIA] PlaylistToFile track count :" << list.size();

    // Open the file
    QFileInfo info(filename);

    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
      Debug::warning() << "Failed to open file for writing" << filename;
      return;
    }

    QString extension = info.suffix().toLower();
    QDir playlist_dir = QDir(info.absoluteDir());

    if( m3u_extension.contains(extension) )
      saveM3uPlaylist(&file, playlist_dir, list);
    else if( pls_extension.contains(extension) )
      savePlsPlaylist(&file, playlist_dir, list);
    else if( xspf_extension.contains(extension) )
      saveXspfPlaylist(&file, playlist_dir, list);

    file.close();
}

/*******************************************************************************
    m3u playlist save
*******************************************************************************/
void saveM3uPlaylist(QIODevice* device, const QDir& playlist_dir, QList<MEDIA::TrackPtr> list)
{
    device->write("#EXTM3U\n");

    foreach (MEDIA::TrackPtr media, list)
    {
      if (!media) continue;
      if (media->type() != TYPE_TRACK && media->type() != TYPE_STREAM) continue;

      QString info;
      QString media_path;
      if (media->type() == TYPE_TRACK ) {
        info = QString("#EXTINF:%1,%2 - %3\n").arg(QString::number(media->duration))
                                              .arg(media->artist)
                                              .arg(media->album);
        device->write(info.toUtf8());

        //! Get the path to MediaItem relative to the Playlist File directory
        media_path = playlist_dir.relativeFilePath(media->url);
      }
      else {
        info = QString("#EXTINF:\n");
        info = QString("#EXTINF:%1,%2 - %3\n").arg(0)
                                              .arg("")
                                              .arg(media->name);

        device->write(info.toUtf8());
        media_path = media->url;
      }

    device->write(media_path.toUtf8());
    device->write("\n");
    }
}


/*******************************************************************************
    .pls playlist save
*******************************************************************************/
void savePlsPlaylist(QIODevice* device, const QDir& playlist_dir, QList<MEDIA::TrackPtr> list)
{
    QTextStream stream(device);
    stream << "[playlist]" << endl;
    stream << "Version=2" << endl;
    stream << "NumberOfEntries=" << list.size() << endl;

    int n = 1;
    foreach (MEDIA::TrackPtr media, list)
    {
      if (!media) continue;
      if (media->type() != TYPE_TRACK && media->type() != TYPE_STREAM) continue;

      // write metadata
      if (media->type() == TYPE_TRACK ) {
        QString media_path = media->url;
        media_path = playlist_dir.relativeFilePath(media_path).toUtf8();

        stream << "File" << n << "=" << media_path << endl;
        stream << "Title" << n << "=" << media->title << endl;
        stream << "Length" << n << "=" << media->duration << endl;
        ++n;
      }
      else { // TYPE_STREAM
        stream << "File" << n << "=" << QString(media->url).toUtf8() << endl;
        stream << "Title" << n << "=" << QString(media->name).toUtf8() << endl;
        stream << "Length" << n << "=" << endl;
        ++n;
      }
    } // fin Foreach
}



/*******************************************************************************
    .xspf playlist save
*******************************************************************************/
void saveXspfPlaylist(QIODevice* device, const QDir& playlist_dir, QList<MEDIA::TrackPtr> list)
{
    QXmlStreamWriter xml(device);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("playlist");
    xml.writeAttribute("version", "1");
    xml.writeDefaultNamespace("http://xspf.org/ns/0/");

    xml.writeStartElement("trackList");
    foreach (MEDIA::TrackPtr media, list)
    {
      if (!media) continue;
      if (media->type() != TYPE_TRACK && media->type() != TYPE_STREAM) continue;

      xml.writeStartElement("track");

      QString media_path;
      // write metadata
      if (media->type() == TYPE_TRACK ) {
        media_path = playlist_dir.relativeFilePath(media->url).toUtf8();

        xml.writeTextElement("location", media_path);
        xml.writeTextElement("title", media->title);
        xml.writeTextElement("creator", media->artist);
        xml.writeTextElement("album", media->album);
        xml.writeTextElement("duration", QString::number(media->duration* 1000));
      }
      else { // TYPE_STREAM
        media_path = media->url;

        xml.writeTextElement("location", media_path);
        xml.writeTextElement("title", media->name);
        
        if( !media->categorie.isEmpty() )
        {
          xml.writeStartElement("extension");
          xml.writeAttribute("application", "yarock");
          xml.writeTextElement("category",  media->categorie);
          xml.writeEndElement(); //extension  
        }
      }

      xml.writeEndElement(); //track
    }
    xml.writeEndElement(); //trackList
    xml.writeEndElement(); //playlist
    xml.writeEndDocument();
}
