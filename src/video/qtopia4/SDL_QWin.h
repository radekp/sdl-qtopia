/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#ifndef _SDL_QWin_h
#define _SDL_QWin_h

#include "SDL_config.h"

#include <QImage>
#include <QWidget>
#include <QMainWindow>
#include <QDirectPainter>
#include <QMatrix>
#include <QtScript/QScriptEngine>

#include "SDL_events.h"

extern "C" {
#include "../../events/SDL_events_c.h"
};

class SDL_QWin : public QMainWindow {
  Q_OBJECT
public:
  enum Rotation
  {
    NoRotation = 0,
    Clockwise = 1,
    CounterClockwise = 2
  };

  SDL_QWin(QWidget *parent = 0, Qt::WindowFlags f = 0);
  virtual ~SDL_QWin();
  
  /**
   * Instruct window to use \a buffer as framebuffer and assume 
   * that screen is rotated according to \a rotation
   */
  void setBackBuffer(Rotation rotation, QImage *buffer);
  
  /**
   * Update screen contents from SDL buffer.
   * NOTE: \a region contains coordinates in SDL system (no rotation applied)
   */
  void flushRegion(const QRegion& region);

  inline QPoint getMousePosition() const {
    return mousePosition;
  }

public slots:
   void showOnFullScreen();
   void toggleKeyboard();
   void enableRedraw();
   void disableRedraw();

protected:
  /* Handle resizing of the window */
  bool event(QEvent *event);
  void showEvent(QShowEvent *e);
  void closeEvent(QCloseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void mousePressEvent(QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);
  void paintEvent(QPaintEvent *ev);
  void keyPressEvent(QKeyEvent *e) { keyEvent(true, e); }
  void keyReleaseEvent(QKeyEvent *e) { keyEvent(false, e); }
private:
  void keyEvent(bool pressed, QKeyEvent *e);
  void init();
  void suspend();
  void resume();

  QImage *backBuffer;
  Rotation rotationMode;

  /**
   * When modifier key is pressed, treat left mouse button as right.
   * This way, we need to remember which key was pressed while moving pointer.
   */
  bool useRightMouseButton;
  Qt::MouseButton pressedButton;

  /**
   * SDL needs to know current mouse position sometimes
   */
  QPoint mousePosition;
public:
  bool debug;
  bool keyboardShown;
  bool redrawEnabled;
  bool windowDeactivated;
  SDL_keysym pressedKey;
  QScriptEngine scriptEngine;
  QScriptValue scriptFun;
  };

#endif /* _SDL_QWin_h */
