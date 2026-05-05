/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef CONVERTOR_H
#define CONVERTOR_H
#include <QString>
#include "ejcommon.h"


class Convertor {
    //virtual bool write(EjDocument *doc) = 0;
public:
    virtual bool convert(EjDocument *doc, QString const& fileName) = 0;
    virtual ~Convertor() {}
};

#endif // CONVERTOR_H
