#include "ejpdfdocumenteditor.h"
#include "ejtableblocks.h"
#include "export_global.h"
#include "standart_function.h"
#include <QDebug>

EjPdfDocumentWriter::~EjPdfDocumentWriter() {}

void TextWriterPdf::edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale)
{
	EjTextBlock* text_block = (EjTextBlock*)block;
	int x = (int)text_block->x;
	// document.drawBorder(painter, x, pagePositionY, text_block->width, text_block->height(),
	// 					100, 100,
	// 					100, 100);
	document.drawFillRect(painter, x, pagePositionY, (int)(text_block->width), (int)(text_block->height()));
	document.addText(painter, text_block, x, pagePositionY, scale);
}

void CellWriterPdf::edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter* painter, double scale)
{
	EjCellBlock* cell_block = (EjCellBlock*)block;
	EjCellStyle* style = cell_block->cellStyle;
	int width = (int)(cell_block->width), height = (int)(cell_block->height());
	int x = (int)(cell_block->x);

	document.setColorBackground(style->m_brushColor);
	document.drawFillRect(painter, x, pagePositionY, width, height);
	if(cell_block->visible)
	{
		document.drawBorder(painter, x, pagePositionY, width, height,
							style->leftBorder()->width(), style->rightBorder()->width(),
							style->topBorder()->width(), style->bottomBorder()->width());
	}
}

void StyleWriterPdf::edit(EjPdfDocument &document, EjBlock *block, double& pagePositionY, QPainter* painter, double scale)
{
	EjNumStyleBlock *block_style = (EjNumStyleBlock*)block;
	if(block_style->style->m_vid == e_PropDoc::TEXT_STYLE)
	{
	   document.m_fontStyleExist = true;
	   EjTextStyle* text_style = (EjTextStyle*)block_style->style;
	   document.setFont(painter, text_style->fontColor(), text_style->m_font);
	   document.setColorBackground(text_style->brushColor());
	}
}

void ImageWriterPdf::edit(EjPdfDocument &document, EjBlock *block, double& pagePositionY, QPainter *painter, double scale)
{
	ImageBlock *image_block = (ImageBlock*)block;
	int x = (int)image_block->x;
	int y = (int)image_block->y;
	document.drawImage(painter, image_block->m_smallImage, x, y);
}

void ByteArrayWriterPdf::edit(EjPdfDocument& document, EjBlock* block, double& pagePositionY, QPainter *painter, double scale) {
	EjPropByteArrayBlock *prop = ((EjPropByteArrayBlock*)block);
}

void SpaceWriterPdf::edit(EjPdfDocument &document, EjBlock *block, double& pagePositionY, QPainter *painter, double scale)
{
}
