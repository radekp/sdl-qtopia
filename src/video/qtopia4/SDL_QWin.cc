/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga
    Copyright (C) 2012 Radek Polak

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
#include "SDL_config.h"

#include "SDL_QWin.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <QObject>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>
#include <QMenu>
#include <QSoftMenuBar>
#include <QDesktopWidget>
#include <QApplication>
#include <QtopiaApplication>

#include <QtDebug>

SDL_QWin::SDL_QWin(QWidget * parent, Qt::WindowFlags f)
  : QMainWindow(parent, f), 
  rotationMode(NoRotation), backBuffer(NULL), useRightMouseButton(false),
  keyboardShown(false), redrawEnabled(true), scriptEngine(this), scriptFun(),
  windowDeactivated(false)
{
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_OpaquePaintEvent);

  debug = getenv("SDL_QT_DEBUG");
  
  const char *scriptFile = getenv("SDL_QT_SCRIPT");
  if(scriptFile) {
      if(debug)
        qDebug() << "using script file" << scriptFile;

      QFile f(scriptFile);
      if(f.open(QFile::ReadOnly)) {
          QByteArray scriptText = f.readAll();
          f.close();
          scriptFun = scriptEngine.evaluate(scriptText);
          if(!scriptFun.isValid())
              qWarning() << "script is not valid function" << scriptFun.toString();
      } else {
          qWarning() << "failed to open script file" << f.errorString();
      }
  }
  
  pressedKey.sym = SDLK_UNKNOWN;
}

SDL_QWin::~SDL_QWin() {
  delete backBuffer;
}

void SDL_QWin::setBackBuffer(SDL_QWin::Rotation new_rotation, QImage *new_buffer) {
  rotationMode = new_rotation;
  delete backBuffer;
  backBuffer = new_buffer;
}

void SDL_QWin::toggleKeyboard()
{
    keyboardShown = !keyboardShown;
    if(keyboardShown)
        QtopiaApplication::showInputMethod();
    else
        QtopiaApplication::hideInputMethod();
}

void SDL_QWin::enableRedraw()
{
    redrawEnabled = true;
}

void SDL_QWin::disableRedraw()
{
    redrawEnabled = false;
}

// Show widget in full screen
void SDL_QWin::showOnFullScreen()
{
    showMaximized();
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowState(Qt::WindowFullScreen);
    raise();
}

bool SDL_QWin::event(QEvent *event)
{
    // Needed for QtMoko fullscreen
    if(event->type() == QEvent::WindowDeactivate)
    {
        windowDeactivated = true;
        lower();
        QMenu *menu = QSoftMenuBar::menuFor(this);
        menu->show();
        menu->hide();
    }
    else if(event->type() == QEvent::WindowActivate)
    {
        QString title = windowTitle();
        setWindowTitle(QLatin1String("_allow_on_top_"));
        raise();
        setWindowTitle(title);
    }
    return QWidget::event(event);
}

/**
 * According to Qt documentation, widget must be visible 
 * when grabbing keyboard and mouse.
 **/ 
void SDL_QWin::showEvent(QShowEvent *) {
  grabKeyboard();
  grabMouse();
}

void SDL_QWin::suspend() { // FIXME: not used
  printf("suspend\n");
  releaseKeyboard();
  releaseMouse();
  SDL_PrivateAppActive(false, SDL_APPINPUTFOCUS);
  hide();
}

void SDL_QWin::resume() { // FIXME: not used
  printf("resume\n");
  show();
  SDL_PrivateAppActive(true, SDL_APPINPUTFOCUS);
}

void SDL_QWin::closeEvent(QCloseEvent *e) {
  SDL_PrivateQuit();
}

void SDL_QWin::mouseMoveEvent(QMouseEvent *e) {
  int sdlstate = 0;
  if (pressedButton == Qt::LeftButton) {
    sdlstate |= SDL_BUTTON_LMASK;
  } else {
    sdlstate |= SDL_BUTTON_RMASK;
  }
  
  mousePosition = e->globalPos();
  SDL_PrivateMouseMotion(sdlstate, 0, mousePosition.x(), mousePosition.y());
}

void SDL_QWin::mousePressEvent(QMouseEvent *e) {
   
  if(windowDeactivated) {
      windowDeactivated = false;
      return;
  }
    
  // Emulate keys using mouse press and javascipt
  if(scriptFun.isValid()) {
    QScriptValueList args;
    args << e->x() << e->y() << width() << height();
    if(debug)
        qDebug() << "calling script x=" << e->x() << ", y=" << e->y() << ", w=" << width() << ", h=" << height();
    
    QScriptValue scriptRes = scriptFun.call(QScriptValue(), args);
    if(scriptRes.toBool()) {
        
        pressedKey.sym = (SDLKey) scriptEngine.globalObject().property("sym").toInt32();
        pressedKey.scancode = (Uint8)(scriptEngine.globalObject().property("scancode").toInt32());
        pressedKey.unicode = 0;
        pressedKey.mod = (SDLMod)(scriptEngine.globalObject().property("mod").toInt32());
        
        if(debug)
            qDebug() << "emualting key sym=" << pressedKey.sym << ", scancode=" << pressedKey.scancode << ", mod=" << pressedKey.mod;
       
        SDL_PrivateKeyboard(SDL_PRESSED, &pressedKey);
    }
  }
    
  mouseMoveEvent(e);
  if (useRightMouseButton)
    pressedButton = Qt::RightButton;
  else
    pressedButton = e->button();

  mousePosition = e->globalPos();
  SDL_PrivateMouseButton(SDL_PRESSED, (pressedButton==Qt::LeftButton)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT,
     mousePosition.x(), mousePosition.y());
}

void SDL_QWin::mouseReleaseEvent(QMouseEvent *e) {

    if(pressedKey.sym != SDLK_UNKNOWN)
    {
        SDL_PrivateKeyboard(SDL_RELEASED, &pressedKey);
        return;
    }
    
  mousePosition = e->globalPos();
  SDL_PrivateMouseButton(SDL_RELEASED, (pressedButton==Qt::LeftButton)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT,
     mousePosition.x(), mousePosition.y());
}

void SDL_QWin::flushRegion(const QRegion &region) {

    if(backBuffer == NULL)
        return;
    
    if(keyboardShown || windowDeactivated) {
        QPainter p(this);
        p.drawImage(geometry(), *backBuffer, backBuffer->rect());
        return;
    }
    
    if(redrawEnabled)
        QScreen::instance()->blit(*backBuffer, pos(), QRegion(geometry()));
}

// This paints the current buffer to the screen, when desired.
void SDL_QWin::paintEvent(QPaintEvent *ev)
{
    flushRegion(QRegion(ev->rect()));
}

static SDLMod qToSDLMod(Qt::KeyboardModifiers qmod)
{
    switch(qmod)
    {
        case Qt::ShiftModifier:
            return (SDLMod)(KMOD_SHIFT);
        case Qt::ControlModifier:
            return (SDLMod)(KMOD_CTRL);
        case Qt::AltModifier:
            return (SDLMod)(KMOD_ALT);
        case Qt::MetaModifier:
            return (SDLMod)(KMOD_META);
        case Qt::KeypadModifier:
            return KMOD_NUM;
        default:
            return KMOD_NONE;
    }
}

static SDLKey qToSDLKey(int qkey, QString qtext)
{
    switch(qkey) {
        case Qt::Key_Backspace:
            return SDLK_BACKSPACE;
        case Qt::Key_Tab:
            return SDLK_TAB;
        case Qt::Key_Clear:
            return SDLK_CLEAR;
        case Qt::Key_Return:
            return SDLK_RETURN;
        case Qt::Key_Pause:
            return SDLK_PAUSE;
        case Qt::Key_Escape:
            return SDLK_ESCAPE;
        case Qt::Key_Space:
            return SDLK_SPACE;
        case Qt::Key_Exclam:
            return SDLK_EXCLAIM;
        case Qt::Key_QuoteDbl:
            return SDLK_QUOTEDBL;
        case Qt::Key_Dollar:
            return SDLK_DOLLAR;
        case Qt::Key_Ampersand:
            return SDLK_AMPERSAND;
        case Qt::Key_QuoteLeft:
            return SDLK_QUOTE;
        case Qt::Key_ParenLeft:
            return SDLK_LEFTPAREN;
        case Qt::Key_ParenRight:
            return SDLK_RIGHTPAREN;
        case Qt::Key_Asterisk:
            return SDLK_ASTERISK;
        case Qt::Key_Plus:
            return SDLK_PLUS;
        case Qt::Key_Comma:
            return SDLK_COMMA;
        case Qt::Key_Minus:
            return SDLK_MINUS;
        case Qt::Key_Period:
            return SDLK_PERIOD;
        case Qt::Key_Slash:
            return SDLK_SLASH;
        case Qt::Key_0:
            return SDLK_0;
        case Qt::Key_1:
            return SDLK_1;
        case Qt::Key_2:
            return SDLK_2;
        case Qt::Key_3:
            return SDLK_3;
        case Qt::Key_4:
            return SDLK_4;
        case Qt::Key_5:
            return SDLK_5;
        case Qt::Key_6:
            return SDLK_6;
        case Qt::Key_7:
            return SDLK_7;
        case Qt::Key_8:
            return SDLK_8;
        case Qt::Key_9:
            return SDLK_9;
        case Qt::Key_Colon:
            return SDLK_COLON;
        case Qt::Key_Semicolon:
            return SDLK_SEMICOLON;
        case Qt::Key_Less:
            return SDLK_LESS;
        case Qt::Key_Equal:
            return SDLK_EQUALS;
        case Qt::Key_Greater:
            return SDLK_GREATER;
        case Qt::Key_Question:
            return SDLK_QUESTION;
        case Qt::Key_At:
            return SDLK_AT;
        case Qt::Key_BracketLeft:
            return SDLK_LEFTBRACKET;
        case Qt::Key_Backslash:
            return SDLK_BACKSLASH;
        case Qt::Key_BracketRight:
            return SDLK_RIGHTBRACKET;
        case Qt::Key_Underscore:
            return SDLK_UNDERSCORE;
        case Qt::Key_A:
            return SDLK_a;
        case Qt::Key_B:
            return SDLK_b;
        case Qt::Key_C:
            return SDLK_c;
        case Qt::Key_D:
            return SDLK_d;
        case Qt::Key_E:
            return SDLK_e;
        case Qt::Key_F:
            return SDLK_f;
        case Qt::Key_G:
            return SDLK_g;
        case Qt::Key_H:
            return SDLK_h;
        case Qt::Key_I:
            return SDLK_i;
        case Qt::Key_J:
            return SDLK_j;
        case Qt::Key_K:
            return SDLK_k;
        case Qt::Key_L:
            return SDLK_l;
        case Qt::Key_M:
            return SDLK_m;
        case Qt::Key_N:
            return SDLK_n;
        case Qt::Key_O:
            return SDLK_o;
        case Qt::Key_P:
            return SDLK_p;
        case Qt::Key_Q:
            return SDLK_q;
        case Qt::Key_R:
            return SDLK_r;
        case Qt::Key_S:
            return SDLK_s;
        case Qt::Key_T:
            return SDLK_t;
        case Qt::Key_U:
            return SDLK_u;
        case Qt::Key_V:
            return SDLK_v;
        case Qt::Key_W:
            return SDLK_w;
        case Qt::Key_X:
            return SDLK_x;
        case Qt::Key_Y:
            return SDLK_y;
        case Qt::Key_Z:
            return SDLK_z;
        case Qt::Key_Delete:
            return SDLK_DELETE;
        case Qt::Key_division:
            return SDLK_KP_DIVIDE;
        case Qt::Key_multiply:
            return SDLK_KP_MULTIPLY;
        case Qt::Key_Up:
            return SDLK_UP;
        case Qt::Key_Down:
            return SDLK_DOWN;
        case Qt::Key_Right:
            return SDLK_RIGHT;
        case Qt::Key_Left:
            return SDLK_LEFT;
        case Qt::Key_Insert:
            return SDLK_INSERT;
        case Qt::Key_Home:
            return SDLK_HOME;
        case Qt::Key_End:
            return SDLK_END;
        case Qt::Key_PageUp:
            return SDLK_PAGEUP;
        case Qt::Key_PageDown:
            return SDLK_PAGEDOWN;
        case Qt::Key_F1:
            return SDLK_F1;
        case Qt::Key_F2:
            return SDLK_F2;
        case Qt::Key_F3:
            return SDLK_F3;
        case Qt::Key_F4:
            return SDLK_F4;
        case Qt::Key_F5:
            return SDLK_F5;
        case Qt::Key_F6:
            return SDLK_F6;
        case Qt::Key_F7:
            return SDLK_F7;
        case Qt::Key_F8:
            return SDLK_F8;
        case Qt::Key_F9:
            return SDLK_F9;
        case Qt::Key_F10:
            return SDLK_F10;
        case Qt::Key_F11:
            return SDLK_F11;
        case Qt::Key_F12:
            return SDLK_F12;
        case Qt::Key_F13:
            return SDLK_F13;
        case Qt::Key_F14:
            return SDLK_F14;
        case Qt::Key_F15:
            return SDLK_F15;
        case Qt::Key_NumLock:
            return SDLK_NUMLOCK;
        case Qt::Key_CapsLock:
            return SDLK_CAPSLOCK;
        case Qt::Key_ScrollLock:
            return SDLK_SCROLLOCK;
        case Qt::Key_Shift:
            return SDLK_LSHIFT;
        case Qt::Key_Control:
            return SDLK_LCTRL;
        case Qt::Key_Alt:
            return SDLK_LALT;
        case Qt::Key_Meta:
            return SDLK_LMETA;
        case Qt::Key_Super_L:
            return SDLK_LSUPER;
        case Qt::Key_Super_R:
            return SDLK_RSUPER;
        case Qt::Key_Mode_switch:
            return SDLK_MODE;
      case Qt::Key_Help:
            return SDLK_HELP;
        case Qt::Key_Print:
            return SDLK_PRINT;
        case Qt::Key_SysReq:
            return SDLK_SYSREQ;
        case Qt::Key_nobreakspace:
            return SDLK_BREAK;
        case Qt::Key_Menu:
            return SDLK_MENU;
        default:
            return SDLK_UNKNOWN;
    }   
}

void SDL_QWin::keyEvent(bool pressed, QKeyEvent *e) {
          
  SDL_keysym k;
  k.sym = qToSDLKey(e->key(), e->text());
  k.scancode = (Uint8)(e->nativeScanCode());
  k.unicode = 0;
  k.mod = qToSDLMod(e->modifiers());
  
  if(debug)
    qDebug() << "SDL_QWin::keyEvent pressed=" << pressed << ", e->key()=" << e->key() << ",e->text()=" << e->text() << "k.sym=" << k.sym << ", k.scancode=" << k.scancode << ", k.mod=" << k.mod;
  
  if (pressed)
    SDL_PrivateKeyboard(SDL_PRESSED, &k);
  else
    SDL_PrivateKeyboard(SDL_RELEASED, &k);
}
