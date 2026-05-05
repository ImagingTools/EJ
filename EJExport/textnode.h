/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef TEXTNODE_H
#define TEXTNODE_H

#include "QDomDocument"
#include "ejcommon.h"

class TextNode{
public:
    virtual QDomElement addText(QString text)=0;
};
#endif // TEXTNODE_H
