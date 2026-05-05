#ifndef OBSERVERXSLX_H
#define OBSERVERXSLX_H


#include "ejcommon.h"
#include "QDomDocument"
#include "QtXlsxWriter-master/src/xlsx/xlsxdocument.h"

//class ObserverXslx: public Observer
//{
//public:
//    ObserverXslx();
//    QList<QDomElement> observe(EjDocument *doc);
//    QDomDocument glue(QList<QDomElement> nodesList);
//    void write(QDomDocument doc);
//    void addImageRelationship(QString path,int id);
//    QDomElement createSectPr();

//};

class ObserverXlsx
{
  public:
    ObserverXlsx();
    void observe (EjDocument *doc, const QString fileName);
    QMap<QString, int> changeStyle(QMap<QString, int> styles, EjNumStyleBlock *nsb);
    void writeXlsx(QDomDocument doc, const QString fileName);
    QXlsx::Format chgFormat(QXlsx::Format format, QMap<QString, int> styles);

};

#endif // OBSERVERXLSX_H
