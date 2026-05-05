/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef TABLENODE_H
#define TABLENODE_H

#include "QDomDocument"
#include "ejcommon.h"
#include "tableignorcellstruct.h"

class TableNode{
public:
    virtual QDomElement addTable(int row, int column, QList<QDomElement> cellList, Empty emptyCell)=0;
};
#endif // TABLENODE_H
