#ifndef EJCONVERTORPDF_H
#define EJCONVERTORPDF_H

#include <QString>
#include <QDomDocument>

#include "../convertor.h"
#include "../include/ejcommon.h"
#include "ejpdfdocument.h"
#include "ejpdfdocumenteditor.h"

#include <QPainter>
#include <QPdfWriter>


class EjConvertorPdf: public Convertor
{
	EjPdfDocument document;
	QMap<quint8, EjPdfDocumentWriter*> editors;
	bool write(EjDocument *doc,  QString file_name, QPdfWriter *writer = nullptr);
	void setSettings(EjDocument *doc, QPdfWriter *writer);
public:
	EjConvertorPdf();
	~EjConvertorPdf();
	bool convert(EjDocument *doc, QString const& file_name);
	bool print(EjDocument *doc, QPdfWriter *writer);
};

#endif // EJCONVERTORPDF_H
