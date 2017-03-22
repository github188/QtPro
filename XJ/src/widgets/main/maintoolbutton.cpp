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

#include "maintoolbutton.h"
#include "playlistwidget.h"
#include "threadmanager.h"
#include "core/database/databasemanager.h"
#include "settings.h"
#include "global_actions.h"
#include "debug.h"


/*
********************************************************************************
*                                                                              *
*    Class MainToolButton                                                      *
*                                                                              *
********************************************************************************
*/
MainToolButton::MainToolButton( QWidget* parent ) : QToolButton(parent)
{
    this->setAutoRaise(true);
    this->setIcon(QIcon(":/images/settings.png"));
    this->setToolTip(tr("Tools"));
    this->setPopupMode(QToolButton::InstantPopup);

    m_menuChooseDbAction = 0;
    m_menuChooseDb = new QMenu(tr("Choose database"));

    QMap<ENUM_ACTION, QAction*> *actions = ACTIONS();

    m_menu = new QMenu();

    QMenu *m0 = m_menu->addMenu(tr("&Show/Hide panel"));
    m0->addAction(actions->value(APP_SHOW_PLAYQUEUE));
    m0->addAction(actions->value(APP_SHOW_MENU));
    m0->addAction(actions->value(APP_SHOW_NOW_PLAYING));
    m_menu->addSeparator();

    QMenu *m1 = m_menu->addMenu(tr("&Add to playqueue"));
    m1->addAction(actions->value(PLAYQUEUE_ADD_FILE));
    m1->addAction(actions->value(PLAYQUEUE_ADD_DIR));
    m1->addAction(actions->value(PLAYQUEUE_ADD_URL));
    m_menu->addAction(actions->value(PLAYQUEUE_CLEAR));
    m_menu->addAction(actions->value(PLAYQUEUE_SAVE));
    m_menu->addAction(actions->value(PLAYQUEUE_AUTOSAVE));

    QMenu *m2 = m_menu->addMenu(tr("&Choose playlist mode"));

    QActionGroup* group = new QActionGroup(this);
      // WARNING actions are already connected in mainwindows
      group->addAction(actions->value(PLAYQUEUE_MODE_TITLE));
      group->addAction(actions->value(PLAYQUEUE_MODE_ALBUM));
      group->addAction(actions->value(PLAYQUEUE_MODE_ARTIST));
      group->addAction(actions->value(PLAYQUEUE_MODE_EXTENDED));
    m2->addActions(group->actions());
    m_menu->addSeparator();

    m_menu->addAction(actions->value(DIALOG_DB_OPERATION));
    m_menu->addAction(actions->value(ENGINE_AUDIO_EQ));
    m_menu->addSeparator();

    m_menu->addAction(actions->value(DIALOG_PLAYLIST_EDITOR));
    m_menu->addSeparator();

    m_menu->addAction(actions->value(APP_MODE_COMPACT));
    m_menu->addSeparator();
    
    m_menu->addAction(actions->value(APP_SHOW_YAROCK_ABOUT));

    m_menu->addSeparator();
    m_menu->addAction(actions->value(APP_QUIT));

    setContextMenuPolicy( Qt::CustomContextMenu );
    connect( this, SIGNAL( clicked ( bool ) ), this, SLOT( slot_showMenu() ) );
}


void MainToolButton::slot_showMenu()
{
    Debug::debug() << "-- MainToolButton --> showMenu";
    updateMultiDbMenu();

    QPoint pos( 0, height() );
    m_menu->exec( mapToGlobal( pos ) );
}

void MainToolButton::updateMultiDbMenu()
{
    //Debug::debug() << "-- MainToolButton --> updateMultiDbMenu";
    if( !DatabaseManager::instance()->multiDb )
    {
        if(m_menuChooseDbAction != 0)
          m_menu->removeAction ( m_menuChooseDbAction );

        m_menuChooseDbAction = 0;
        return;
    }

    //! Multi Db activated --> show choose database menu
    QActionGroup* group = new QActionGroup(this);

    foreach (const QString& name, DatabaseManager::instance()->DB_NAME_LIST())
    {
        QAction *a = new QAction(QIcon(), name, this);
        a->setCheckable(true);

        group->addAction(a);

        if(name == DatabaseManager::instance()->DB_NAME)
          a->setChecked(true);

        connect(a, SIGNAL(triggered()), this, SLOT(slot_dbNameClicked()));
    }

    m_menuChooseDb->clear();
    m_menuChooseDb->addActions(group->actions());

    if(!m_menuChooseDbAction)
       m_menuChooseDbAction = m_menu->insertMenu ( ACTIONS()->value(DIALOG_DB_OPERATION), m_menuChooseDb );

    m_menuChooseDb->setEnabled(!ThreadManager::instance()->isDbRunning());
}


void MainToolButton::slot_dbNameClicked()
{
    QAction *action = qobject_cast<QAction *>(sender());

    DatabaseManager::instance()->DB_NAME = action->text();
    //Debug::debug() << "-- MainToolButton --> dbNameClicked = " << action->text();

    emit dbNameChanged();
}
