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

#include "settings.h"
#include "utilities.h"

#include "infosystem/services/service_lyrics.h"
//#include "debug.h"

#include <QBuffer>

YarockSettings* YarockSettings::INSTANCE = 0;

/*
********************************************************************************
*                                                                              *
*    YarockSettings                                                            *
*                                                                              *
********************************************************************************
*/
YarockSettings::YarockSettings()
{
    INSTANCE    = this;
    s = new QSettings(UTIL::CONFIGFILE,QSettings::IniFormat,0);
}


void YarockSettings::readSettings()
{
    // window elements (Startup settings)
    _windowsGeometry     = s->value("Window/geometry").toByteArray();
    _windowsState        = s->value("Window/state").toByteArray();
    _splitterState       = s->value("Window/splitter").toByteArray();

    _showPlayQueuePanel  = s->value("Window/showPlaylistPanel", true).toBool();
    _showMenuPanel       = s->value("Window/showMenuPanel",     true).toBool();
    _showNowPlaying      = s->value("Window/showNowPlaying",    true).toBool();
    _enableSearchPopup   = s->value("Window/searchPopup",       true).toBool();

    // session
#ifdef ENABLE_PHONON
    _engine              = s->value("Session/engine",              1).toInt();    // default = phonon
#elif ENABLE_VLC  
    _engine              = s->value("Session/engine",              2).toInt();    // default = vlc
#else    
    _engine              = s->value("Session/engine",              0).toInt();    // default = null engine
#endif
    
    _viewMode            = s->value("Session/viewMode",            5).toInt();  // ViewAlbum default
    _album_view_type     = s->value("Session/album_view_type",     1).toInt();  // extended default
    _playlist_view_type  = s->value("Session/playlist_view_type",  0).toInt();  // overview default

    _playqueueMode       = s->value("Session/playQueueMode", 4).toInt();    // PLAYQUEUE::MODE_EXTENDED
    _playqueueDuplicate  = s->value("Session/playqueueDuplicate", true).toBool();

    _filesystem_path     = s->value("Session/filesystem",     "").toString();
    
    _hideAtStartup       = s->value("Session/hideAtStartup", false).toBool();
    _is_menu_bar         = s->value("Session/ismenubar", false).toBool();

    /* handle color */
    if(s->contains("Session/color")) {
      QByteArray bytes = s->value("Session/color").toByteArray();
    
      QBuffer buf(&bytes);
      buf.open(QIODevice::ReadOnly);

      QDataStream stream(&buf);
      stream >> _baseColor;
    }
    else
    {
      _baseColor = QColor(0xfca822);
    }

    // features activations (Dynamic settings)
    _useTrayIcon         = s->value("Features/systray", false).toBool();
    _useMpris            = s->value("Features/mpris",   false).toBool();
    _useDbusNotification = s->value("Features/dbus",    false).toBool();
    _useLastFmScrobbler  = s->value("Features/lastFm",  false).toBool();
    _useShortcut         = s->value("Features/shortcut", false).toBool();

    // song info 
    _lyrics_providers =  s->value("SongInfo/providers",  ServiceLyrics::defaultProvidersList()).toStringList();
    
    // audio controler (Startup settings)
    _repeatMode          = s->value("AudioControl/repeat",  0).toInt();
    _shuffleMode         = s->value("AudioControl/shuffle", 0).toInt();
    _volumeLevel         = s->value("AudioControl/volume",  50).toInt();
    _replaygain          = s->value("AudioControl/replaygain", 0).toInt();

    // playback option
    _stopOnPlayqueueClear     = s->value("PlaybackOption/stopOnclear", false).toBool();
    _restorePlayqueue         = s->value("PlaybackOption/restorePlayqueue", false).toBool();
    _restartPlayingAtStartup  = s->value("PlaybackOption/restartPlaying", false).toBool();
    _pauseOnScreenSaver       = s->value("PlaybackOption/screenSaverPause", false).toBool();
    _stopOnScreenSaver        = s->value("PlaybackOption/screenSaverStop", false).toBool();
    _playingUrl               = s->value("PlaybackOption/currentUrl",     "").toString();
    _playingPosition          = s->value("PlaybackOption/currentPosition",  0).toDouble();

    // Shurtcut media key
    _shortcutsKey["play"]        = s->value("Shortcuts/play","Meta+P").toString();
    _shortcutsKey["stop"]        = s->value("Shortcuts/stop","Meta+S").toString();
    _shortcutsKey["prev_track"]  = s->value("Shortcuts/prev_track","Meta+B").toString();
    _shortcutsKey["next_track"]  = s->value("Shortcuts/next_track","Meta+F").toString();
    _shortcutsKey["inc_volume"]  = s->value("Shortcuts/inc_volume","Meta+A").toString();
    _shortcutsKey["dec_volume"]  = s->value("Shortcuts/dec_volume","Meta+Z").toString();
    _shortcutsKey["mute_volume"] = s->value("Shortcuts/mute_volume","Meta+M").toString();
    _shortcutsKey["jump_to_track"] = s->value("Shortcuts/jump_to_track","Meta+J").toString();

    // Equalizer settings
    const int count = s->beginReadArray("Equalizer/presets");
    for (int i=0 ; i<count ; ++i)
    {
      s->setArrayIndex(i);
      _presetEq[ s->value("name").toString() ] = s->value("params").value<Equalizer::EqPreset>();
    }
    s->endArray();

    _currentPreset = s->value("Equalizer/selected_preset", "Custom").toString();
    _enableEq      = s->value("Equalizer/enabled", false).toBool();
}

void YarockSettings::writeSettings()
{
    // window elements (Startup settings)
    s->beginGroup("Window");
    s->setValue("geometry",           _windowsGeometry);
    s->setValue("state",              _windowsState);
    s->setValue("splitter",           _splitterState);

    s->setValue("showPlaylistPanel",  _showPlayQueuePanel);
    s->setValue("showMenuPanel",      _showMenuPanel);
    s->setValue("showNowPlaying",     _showNowPlaying);
    s->setValue("searchPopup",        _enableSearchPopup);
    s->endGroup();

    // session
    s->beginGroup("Session");
    s->setValue("engine",             _engine);
    s->setValue("viewMode",           _viewMode);
    s->setValue("album_view_type",    _album_view_type);
    s->setValue("playlist_view_type", _playlist_view_type);
    s->setValue("playQueueMode",      _playqueueMode);
    s->setValue("playqueueDuplicate", _playqueueDuplicate);
    s->setValue("hideAtStartup",      _hideAtStartup);
    s->setValue("ismenubar",          _is_menu_bar);
    s->setValue("filesystem",         _filesystem_path);
        
      /* handle color */
      QByteArray byteArray;      
      QBuffer buffer(&byteArray);
      buffer.open(QIODevice::WriteOnly);
    
      QDataStream stream(&buffer);
      stream << _baseColor;
      buffer.close();

      s->setValue("color",             byteArray);

    s->endGroup();

    // features activations (Dynamic settings)
    s->beginGroup("Features");
    s->setValue("systray",           _useTrayIcon);
    s->setValue("mpris",             _useMpris);
    s->setValue("dbus",              _useDbusNotification);
    s->setValue("lastFm",            _useLastFmScrobbler);
    s->setValue("shortcut",          _useShortcut);
    s->endGroup();

    // song info 
    s->beginGroup("SongInfo");
    s->setValue("providers",          _lyrics_providers);
    s->endGroup();
    
    // audio controler (Startup settings)
    s->beginGroup("AudioControl");
    s->setValue("repeat",            _repeatMode);
    s->setValue("shuffle",           _shuffleMode);
    s->setValue("volume",            _volumeLevel);
    s->setValue("replaygain",        _replaygain);
    s->endGroup();

    // playback option
    s->beginGroup("PlaybackOption");
    s->setValue("stopOnclear",       _stopOnPlayqueueClear);
    s->setValue("restorePlayqueue",  _restorePlayqueue);
    s->setValue("restartPlaying",    _restartPlayingAtStartup);
    s->setValue("screenSaverPause",  _pauseOnScreenSaver);
    s->setValue("screenSaverStop",   _stopOnScreenSaver);
    s->setValue("currentUrl",        _playingUrl);
    s->setValue("currentPosition",   _playingPosition);
    s->endGroup();

    // Shurtcut media key
    s->beginGroup("Shortcuts");
    s->setValue("play",             _shortcutsKey["play"]);
    s->setValue("stop",             _shortcutsKey["stop"]);
    s->setValue("prev_track",       _shortcutsKey["prev_track"]);
    s->setValue("next_track",       _shortcutsKey["next_track"]);
    s->setValue("inc_volume",       _shortcutsKey["inc_volume"]);
    s->setValue("dec_volume",       _shortcutsKey["dec_volume"]);
    s->setValue("mute_volume",      _shortcutsKey["mute_volume"]);
    s->setValue("jump_to_track",    _shortcutsKey["jump_to_track"]);
    s->endGroup();

    // Equalizer settings
    s->beginGroup("Equalizer");
    s->beginWriteArray("presets", _presetEq.count());
    int i = 0;
    foreach (const QString& name, _presetEq.keys()) {
      s->setArrayIndex(i++);
      s->setValue("name", name);
      s->setValue("params", QVariant::fromValue(_presetEq[name]));
    }
    s->endArray();
    s->setValue("selected_preset", _currentPreset);
    s->setValue("enabled", _enableEq);
    s->endGroup();

    // final sync to write setting to file
    s->sync ();
}

