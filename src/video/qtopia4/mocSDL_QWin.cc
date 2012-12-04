/****************************************************************************
** Meta object code from reading C++ file 'SDL_QWin.h'
**
** Created: Tue Dec 4 21:05:02 2012
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "SDL_QWin.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SDL_QWin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SDL_QWin[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
       9,   28,   28,   28, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SDL_QWin[] = {
    "SDL_QWin\0showOnFullScreen()\0\0"
};

const QMetaObject SDL_QWin::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_SDL_QWin,
      qt_meta_data_SDL_QWin, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SDL_QWin::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SDL_QWin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SDL_QWin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SDL_QWin))
        return static_cast<void*>(const_cast< SDL_QWin*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int SDL_QWin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: showOnFullScreen(); break;
        default: ;
        }
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
