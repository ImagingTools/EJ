/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef EJDOCXTOUJFCONVERTOR_H
#define EJDOCXTOUJFCONVERTOR_H
#include <QString>
#include <QDomDocument>

#include "docxtoujf/docxtoujfdocumenteditors.h"
#include "ejbaseujfconvertor.h"


class EjDocxToUjfConvertor: public EjBaseUjfConvertor
{
    Zipper zipper;
    QString temp_folder;
    QString subfolder;
    QMap<QString, WordDocumentReader*> editors;
	EjDocxToUjfDocument document;

public:
	EjDocxToUjfConvertor();
    bool readDoc(EjDocument *doc, QString fileName);

};

#endif // EJDOCXTOUJFCONVERTOR_H
