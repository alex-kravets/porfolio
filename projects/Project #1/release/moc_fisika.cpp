/****************************************************************************
** Meta object code from reading C++ file 'fisika.h'
**
** Created: Tue 14. Feb 20:45:16 2012
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../fisika.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fisika.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Fisika[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,    8,    7,    7, 0x05,

 // slots: signature, parameters, type, tag, flags
      29,    7,    7,    7, 0x0a,
      40,    7,    7,    7, 0x0a,
      48,    7,    7,    7, 0x0a,
      60,    7,    7,    7, 0x0a,
      77,    7,   70,    7, 0x0a,
      96,    7,   70,    7, 0x0a,
     118,    7,    7,    7, 0x0a,
     130,    7,    7,    7, 0x0a,
     141,    7,    7,    7, 0x0a,
     152,    7,    7,    7, 0x0a,
     159,    7,    7,    7, 0x0a,
     167,    7,    7,    7, 0x0a,
     177,    7,    7,    7, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Fisika[] = {
    "Fisika\0\0pos\0graphDrawed(int)\0viewHelp()\0"
    "clear()\0drawGraph()\0getParm()\0double\0"
    "addAnimPos(double)\0setSliderAnimPos(int)\0"
    "animation()\0stepPrev()\0stepNext()\0"
    "stop()\0pause()\0aboutQt()\0"
    "viewGraphInfo(QString)\0"
};

const QMetaObject Fisika::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Fisika,
      qt_meta_data_Fisika, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Fisika::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Fisika::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Fisika::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Fisika))
        return static_cast<void*>(const_cast< Fisika*>(this));
    return QWidget::qt_metacast(_clname);
}

int Fisika::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: graphDrawed((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: viewHelp(); break;
        case 2: clear(); break;
        case 3: drawGraph(); break;
        case 4: getParm(); break;
        case 5: { double _r = addAnimPos((*reinterpret_cast< double(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< double*>(_a[0]) = _r; }  break;
        case 6: { double _r = setSliderAnimPos((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< double*>(_a[0]) = _r; }  break;
        case 7: animation(); break;
        case 8: stepPrev(); break;
        case 9: stepNext(); break;
        case 10: stop(); break;
        case 11: pause(); break;
        case 12: aboutQt(); break;
        case 13: viewGraphInfo((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void Fisika::graphDrawed(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
