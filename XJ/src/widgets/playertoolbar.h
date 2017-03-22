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

#ifndef _PLAYER_TOOLBAR_H_
#define _PLAYER_TOOLBAR_H_

// Qt
#include <QToolBar>
#include <QToolButton>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include <QSlider>


/*
********************************************************************************
*                                                                              *
*    Class VolumeToolButton                                                    *
*                                                                              *
********************************************************************************
*/
class VolumeToolButton : public QToolButton
{
Q_OBJECT
  public:
    VolumeToolButton(QWidget *);
    
  private slots:
    void slot_volume_change();
    void slot_mute_change();
    void slot_mute_toggle_action();
    void slot_apply_volume(int);
    
  private:
    QLabel       *m_volume_label;
    QSlider      *m_slider;
};


/*
********************************************************************************
*                                                                              *
*    Class PlayerToolBar                                                       *
*                                                                              *
********************************************************************************
*/
class EngineBase;
class PlayerToolBar : public QToolBar
{
  Q_OBJECT
  public:
    PlayerToolBar(QWidget *parent);

  private:
    EngineBase       *m_player;

    QLabel           *m_currentTime;
    QLabel           *m_totalTime;
    QLabel           *m_playingTrack;

  private :
    void clear();

  private slots:
    void slot_update_track_playing_info();
    void slot_update_time_position(qint64);
    void slot_update_total_time(qint64);
};

#endif // _PLAYER_TOOLBAR_H_
