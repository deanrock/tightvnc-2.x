// Copyright (C) 2009,2010,2011,2012 GlavSoft LLC.
// All rights reserved.
//
//-------------------------------------------------------------------------
// This file is part of the TightVNC software.  Please visit our Web site:
//
//                       http://www.tightvnc.com/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//-------------------------------------------------------------------------
//

#include "TvnForegroundServerApplication.h"
#include "ServerCommandLine.h"
#include "TvnServerHelp.h"

#include "thread/GlobalMutex.h"

#include "util/ResourceLoader.h"
#include "util/StringTable.h"
#include "tvnserver-app/NamingDefs.h"
#include "win-system/WinCommandLineArgs.h"

#include "tvnserver/resource.h"

TvnForegroundServerApplication::TvnForegroundServerApplication(HINSTANCE hInstance,
                                           const TCHAR *windowClassName,
                                           const TCHAR *commandLine,
                                           NewConnectionEvents *newConnectionEvents)
: WindowsApplication(hInstance, windowClassName),
  m_fileLogger(true),
  m_tvnServer(0),
  m_commandLine(commandLine),
  m_newConnectionEvents(newConnectionEvents)
{
}

TvnForegroundServerApplication::~TvnForegroundServerApplication()
{
}

int TvnForegroundServerApplication::run()
{
  // FIXME: May be an unhandled exception.
  // Check wrong command line and situation when we need to show help.

  ServerCommandLine parser;

  try {
    WinCommandLineArgs cmdArgs(m_commandLine.getString());
    if (!parser.parse(&cmdArgs) || parser.showHelp()) {
      throw Exception(_T("Wrong command line argument"));
    }
  } catch (...) {
    TvnServerHelp::showUsage();
    return 0;
  }

  StringStorage password = StringStorage();
  parser.getPassword(&password);

  StringStorage portStr = StringStorage();
  parser.getPort(&portStr);
  int port = _tstoi(portStr.getString());

  // Reject 2 instances of TightVNC server application.

  GlobalMutex *appInstanceMutex;

  try {
    appInstanceMutex = new GlobalMutex(
      ServerApplicationNames::SERVER_INSTANCE_MUTEX_NAME, false, true);
  } catch (...) {
    MessageBox(0,
               StringTable::getString(IDS_SERVER_ALREADY_RUNNING),
               StringTable::getString(IDS_MBC_TVNSERVER), MB_OK | MB_ICONEXCLAMATION);
    return 1;
  }

  // Start TightVNC server and TightVNC control application.
  try {
	m_tvnServer = new TvnForegroundServer(false, m_newConnectionEvents, this, &m_fileLogger, password.getString(), port);
    m_tvnServer->addListener(this);
    m_tvnControlRunner = new WsConfigRunner(&m_fileLogger);

    int exitCode = WindowsApplication::run();

    delete m_tvnControlRunner;
    m_tvnServer->removeListener(this);
    delete m_tvnServer;
    delete appInstanceMutex;
    return exitCode;
  } catch (Exception &e) {
    // FIXME: Move string to resource
    StringStorage message;
    message.format(_T("Couldn't run the server: %s"), e.getMessage());
    MessageBox(0,
               message.getString(),
               _T("Server error"), MB_OK | MB_ICONEXCLAMATION);
    return 1;
  }
}

void TvnForegroundServerApplication::onTvnServerShutdown()
{
  WindowsApplication::shutdown();
}

void TvnForegroundServerApplication::onLogInit(const TCHAR *logDir, const TCHAR *fileName,
                                     unsigned char logLevel)
{
  m_fileLogger.init(logDir, fileName, logLevel);
  m_fileLogger.storeHeader();
}

void TvnForegroundServerApplication::onChangeLogProps(const TCHAR *newLogDir, unsigned char newLevel)
{
  m_fileLogger.changeLogProps(newLogDir, newLevel);
}
