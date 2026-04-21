#ifndef PDFDOCUMENTEDITOR_H
#define PDFDOCUMENTEDITOR_H
#include "ejpdfdocument.h"
#include "imageblock.h"


class COMMONSHARED_EXPORT EjPdfDocumentWriter
{
public:
	virtual void edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter = 0, double scale = 1) = 0;
	~EjPdfDocumentWriter();
};

class TextWriterPdf: public EjPdfDocumentWriter {
public:
	void edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale);
};

class SpaceWriterPdf: public EjPdfDocumentWriter {
public:
	void edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale);
};

class CellWriterPdf: public EjPdfDocumentWriter {
public:
	void edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale);
};

class ImageWriterPdf: public EjPdfDocumentWriter {
public:
	void edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale);
};

class ByteArrayWriterPdf: public EjPdfDocumentWriter {
public:
	void edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale);
};

class EndGroupWriterPdf: public EjPdfDocumentWriter {
public:
	void edit(EjPdfDocument& document, EjBlock*, double& pagePositionY, QPainter* painter, double scale);
};

class StyleWriterPdf: public EjPdfDocumentWriter {
public:
	void edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale);
};

#endif // PDFDOCUMENTEDITOR_H
