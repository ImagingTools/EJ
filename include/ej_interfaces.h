#ifndef EJ_INTERFACES_H
#define EJ_INTERFACES_H
#include <QtPlugin>
#include <QMouseEvent>

#include "ejcommon.h"
#include "ejtextcontrol.h"

QT_BEGIN_NAMESPACE
class QImage;
class QString;
class QDataStream;
class QSGNode;
class UpdatePaintNodeData;
class QColor;
class PopupMenuModel;
QT_END_NAMESPACE


class INTERFACESHARED_EXPORT ItemBlock : public QObject
{
    Q_OBJECT
    public:
	ItemBlock(QObject *parent=0) : QObject(parent), pBlock(0), m_backGround(QColor(Qt::transparent)), m_viewScale(1.0), m_snapString(false) { }
    virtual ~ItemBlock() {}
	EjBlock *pBlock;
    virtual void createPreview() {}
    QColor m_backGround;
    qreal m_viewScale;
    bool m_snapString;
};


class INTERFACESHARED_EXPORT JotInterface : public QObject
{
    Q_OBJECT
public:
    JotInterface() : QObject() { m_doc = NULL; m_activeBlock = NULL; m_control = NULL; userKey = 0; }
    virtual ~JotInterface() {}
    enum ResultMenuActivate {
        RMA_NOT,
        RMA_ITEM,
        RMA_MENU
    };

    Q_INVOKABLE virtual quint8 type() const = 0;
    Q_INVOKABLE virtual QString name() const = 0;
    Q_INVOKABLE virtual bool addEnabled(quint8 type) { Q_UNUSED(type) return true; }
    Q_INVOKABLE virtual QQuickItem* getActiveViewItem(int statusMode, QQuickItem *parent) = 0; // vid == 0 for view, vid == 1 for edit
    Q_INVOKABLE virtual QQuickItem* getActivePropItem(int statusMode, QQuickItem *parent, QString command, QString data);
	virtual EjBlock* newBlock(int tid = -1) = 0;
	virtual ItemBlock* newItem(EjBlock *block, EjCalcParams *calcParams, QQuickItem *parent)  = 0 ;
	virtual QQuickItem* createDefault(int index, EjDocument *doc, QQuickItem *parent);
    virtual ResultMenuActivate menuActivate(QString command, QString data, PopupMenuModel *popupModel, e_statusMode statusMode); // 0 - not active, 1 - item active, 2 -  menu active

    virtual void setActiveBlock(EjTextControl *control, int index, QMouseEvent *event);

#ifdef USE_QML
    virtual void registerQMLTypes(QQmlContext *ctxt) { Q_UNUSED(ctxt) }
#endif

	virtual void linksChanged(EjDocument *doc) { Q_UNUSED(doc) }
	virtual void docSigned(EjDocument *doc, int status, QString comment) { Q_UNUSED(doc) Q_UNUSED(status) Q_UNUSED(comment)}

    virtual void setTextControl(EjTextControl *textControl)
    {
        m_control = textControl;
        m_doc = textControl->doc;
    }
    quint32 userKey;

protected:
	EjDocument *m_doc;
	EjBlock* m_activeBlock;
//    ItemBlock *m_activeItem;
    EjTextControl *m_control;
};
//! [0]
extern COMMONSHARED_EXPORT QMap<int,JotInterface *> ext_plugins;


#endif // EJ_INTERFACES_H
