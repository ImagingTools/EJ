/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef STORAGEHELPER_H
#define STORAGEHELPER_H

#include "ejdocument.h"

class COMMONSHARED_EXPORT EjStorageHelper
{
public:
	EjStorageHelper();
	static EjStorageHelper *getInstance();
    static void loadSmallImage(QImage *image, QByteArray &name, bool isSmall = true);
	static void addImage(QString path, QByteArray &name);
	static void addImage(QImage &image, QByteArray &name);
    static QString pathPic();
    static QByteArray getPatch(EjDocument *oldDoc, EjDocument *newDoc, quint16 newVer, QString user);
	// static bool isCrypted();
	// static bool encryptFile(const QString &name, QByteArray &data);
	// static bool decryptFile(const QString &name, QByteArray &data);

    static void loadData(EjDocument *doc, QList<QByteArray *> &lData, bool isSaveHistory = false);
    static void updateDoc(EjDocument *doc, quint16 patchVer, quint32 patchTime, bool isSetKey = true);
};
#endif // STORAGEHELPER_H
