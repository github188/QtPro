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

#ifndef _DB_OPERATION_DIALOG_H_
#define _DB_OPERATION_DIALOG_H_

#include "dialog_base.h"

#include <QRadioButton>

/*
********************************************************************************
*                                                                              *
*    Class DbOperationDialog                                                   *
*                                                                              *
********************************************************************************
*/
class DbOperationDialog : public DialogBase
{
  Q_OBJECT
  Q_DISABLE_COPY(DbOperationDialog)

  public:
    explicit DbOperationDialog(QWidget *parent = 0);
    bool isFullRescan();

  private:
    QRadioButton        *ui_check_fullScan;
    QRadioButton        *ui_check_updateScan;

  private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();
};

#endif // _DB_OPERATION_DIALOG_H_
