/***************************************************************************
 *   Copyright (C) 2008-2009 by Dominik Kapusta       <d@ayoy.net>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of      *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this program; if not, write to                     *
 *   the Free Software Foundation, Inc.,                                   *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#ifndef QTWITTER_H
#define QTWITTER_H

#include "mainwindow.h"
#include <twitterapi/twitterapi_global.h>
#include "settings.h"

class Core;
class TwitPicView;
class Settings;

class Qtwitter : public MainWindow
{
  Q_OBJECT
public:
  static Qtwitter* instance();
  Qtwitter( QWidget *parent = 0 );

private slots:
  void setCurrentModel( const QString &serviceUrl, const QString &login );
  void openTwitPic();

private:
  Core *core;
  TwitPicView *twitpic;
  Settings *settingsDialog;

  static Qtwitter *m_instance;

};

#endif // QTWITTER_H
