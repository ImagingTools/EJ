/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef OBSERVER_H
#define OBSERVER_H

#include "ejcommon.h"
#include "QDomDocument"
class Observer
{
public:
    virtual QList<QDomElement> observe(EjDocument *doc)=0;
    virtual QDomDocument glue(QList<QDomElement> list)=0;
    virtual void write(QDomDocument doc)=0;
};

#endif // OBSERVER_H
