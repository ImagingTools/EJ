#ifndef PARAGRAPHNODEDOCX_H
#define PARAGRAPHNODEDOCX_H

#include "paragraphnode.h"
#include "ejcommon.h"

class ParagraphNodeDocx: public ParagraphNode
{
public:
    ParagraphNodeDocx();
    QDomElement addParagraph(QList<QDomElement> nodesList);
    QDomElement addParagraphWithStyles(QList<QDomElement> nodesList, QMap<QString, int> tumbler);
    QDomElement addParagraphOne(QDomElement node);
    QDomElement addParagraphEmpty();
};

#endif // PARAGRAPHNODEDOCX_H
