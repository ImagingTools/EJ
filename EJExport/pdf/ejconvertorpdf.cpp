#include "ejconvertorpdf.h"

#include "imageblock.h"
#include "ejtableblocks.h"
#include "export_global.h"
#include "ejstoragehelper.h"

#include <QtCore>


bool EjConvertorPdf::convert(EjDocument *doc, const QString &file_name)
{
	bool result = write(doc, file_name);
	return result;
}

bool EjConvertorPdf::print(EjDocument *doc, QPdfWriter *writer)
{
	bool result = true;
	result &= write(doc, "", writer);
	return result;
}

bool EjConvertorPdf::write(EjDocument *doc, QString file_name, QPdfWriter* writer)
{
	QPdfWriter* pdfWriter = writer;
	if (pdfWriter == nullptr)
	{
		pdfWriter = new QPdfWriter(file_name);
	}

	setSettings(doc, pdfWriter);

	QPainter painter(pdfWriter);
	painter.begin(pdfWriter);

	// у PDF размеры изначально должны быть заданы в книжной ориентации (ширина < высоты)
	EjDocLayout::Orientation orientation = doc->attributes()->getDocLayout()->docOrientation();
	double docWidth = doc->attributes()->getDocLayout()->docWidth();
	double docHeight = doc->attributes()->getDocLayout()->docHeight();
	if ((orientation == EjDocLayout::ORN_PORTRAIT && docWidth > docHeight) ||  (orientation == EjDocLayout::ORN_LANDSCAPE && docWidth < docHeight)){
		std::swap(docWidth, docHeight);
	}

	// масштабирование по размеру страницы с учетом разрешения DPI
	QRect pageRect = pdfWriter->pageLayout().paintRectPixels(pdfWriter->resolution());
	double scale = (double)pageRect.width() / docWidth;
	painter.scale(scale, scale);
	double pageHeightLimit = pageRect.height() / scale;

	bool result = true;
	const QList<EjBlock*> &list = *doc->lBlocks;
	double pagePositionY = 0;
	int pagesCount = 1;
	int docPositionY = 0;

	for (int i = 0; i < list.size() && result; i++)
	{
		EjBlock* block = list[i];
		int blockY = block->y;
		int blockHeight = block->height();
		EjPdfDocumentWriter *editor = editors.value(block->type, nullptr);
		if (editor != nullptr)
		{
			if (pageHeightLimit * pagesCount < blockY+blockHeight){
				pdfWriter->newPage();
				pagesCount++;
				pagePositionY = 0;
				docPositionY = blockY;
			}
			else{
				pagePositionY = blockY - docPositionY;
			}
			editor->edit(document, block, pagePositionY, &painter, scale);
		}
	}
	painter.end();
	if (writer == nullptr){
		delete pdfWriter;
	}
	return result;
}

void EjConvertorPdf::setSettings(EjDocument *doc, QPdfWriter *writer)
{
	// у PDF размеры изначально должны быть заданы в книжной ориентации (ширина < высоты)
	EjDocLayout::Orientation orientation = doc->attributes()->getDocLayout()->docOrientation();
	double docWidth = doc->attributes()->getDocLayout()->docWidth()/100;
	double docHeight = doc->attributes()->getDocLayout()->docHeight()/100;
	if (docWidth > docHeight){
		std::swap(docWidth, docHeight);
	}
	// границы страницы
	EjDocMargings* margins = doc->attributes()->getDocMargings();
	double topMargin = margins->top()/100;
	double bottomMargin = margins->bottom()/100;
	double leftMargin = margins->left()/100;
	double rightMargin = margins->right()/100;
	QPageSize pageSize(QSizeF(docWidth, docHeight), QPageSize::Millimeter);
	QPageLayout pageLayout = writer->pageLayout();
	QMarginsF pageMargins(leftMargin, topMargin, rightMargin, bottomMargin);

	pageLayout.setPageSize(pageSize);
	pageLayout.setOrientation(orientation == EjDocLayout::ORN_PORTRAIT ? QPageLayout::Portrait : QPageLayout::Landscape);
	pageLayout.setUnits(QPageLayout::Millimeter);
	pageLayout.setMargins(pageMargins);
	writer->setPageLayout(pageLayout);
}

EjConvertorPdf::EjConvertorPdf()
{
	editors.insert(e_typeBlocks::TEXT, new TextWriterPdf());
	editors.insert(e_typeBlocks::SPACE, new SpaceWriterPdf());
	editors.insert(e_typeBlocks::BASECELL, new CellWriterPdf());
	editors.insert(e_typeBlocks::EXT_IMAGE, new ImageWriterPdf());
	editors.insert(e_typeBlocks::NUM_STYLE, new StyleWriterPdf());
	editors.insert(e_typeBlocks::PROP_BA, new ByteArrayWriterPdf());
}

EjConvertorPdf::~EjConvertorPdf()
{
	for (QMap<quint8, EjPdfDocumentWriter*>::iterator i = editors.begin(); i != editors.end(); ++i)
	{
		delete *i;
	}
}
