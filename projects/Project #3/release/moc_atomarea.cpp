/****************************************************************************
** Meta object code from reading C++ file 'atomarea.h'
**
** Created: Sun 8. Jun 22:53:51 2014
**      by: The Qt Meta Object Compiler version 61 (Qt 4.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../atomarea.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'atomarea.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 61
#error "This file was generated using the moc from 4.5.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_AtomArea[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

       0        // eod
};

static const char qt_meta_stringdata_AtomArea[] = {
    "AtomArea\0"
};

const QMetaObject AtomArea::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_AtomArea,
      qt_meta_data_AtomArea, 0 }
};

const QMetaObject *AtomArea::metaObject() const
{
    return &staticMetaObject;
}

void *AtomArea::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_AtomArea))
        return static_cast<void*>(const_cast< AtomArea*>(this));
    if (!strcmp(_clname, "QThread"))
        return static_cast< QThread*>(const_cast< AtomArea*>(this));
    return QWidget::qt_metacast(_clname);
}

int AtomArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
