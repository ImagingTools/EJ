#include "ejpdfdocument.h"
#include "export_global.h"

EjPdfDocument::EjPdfDocument(){}

EjPdfDocument::~EjPdfDocument(){}

void EjPdfDocument::setFont(QPainter* painter, QColor color, QFont font)
{
	int fontSize = font.pixelSize();
	double pointSize = fontSize * 1.75;
	font.setPointSizeF(pointSize);
    painter->setFont(font);
    pen.setColor(color);
}

void EjPdfDocument::setColorBackground(QColor color)
{
    m_backgroundColor = color;
}

void EjPdfDocument::addText(QPainter* painter, EjTextBlock* textBlock, int x, int y, double scale)
{
	int textWidth = textBlock->width;
	int textHeight = textBlock->height();
    if (!m_fontStyleExist)
    {
        QFont font;
		font.setPointSizeF(textHeight*scale);
        painter->setFont(font);
    }
    painter->setPen(pen);
	int flags = Qt::AlignCenter | Qt::TextWordWrap;
	QRect rect(x, y, textWidth, textHeight);
	QString text = textBlock->text;
	text = text.remove("\n");
	painter->drawText(rect, flags,text);
}

void EjPdfDocument::drawFillRect(QPainter *painter, int x, int y, int width, int height)
{
	painter->fillRect(x, y, width, height, m_backgroundColor);
}

void EjPdfDocument::drawBorder(QPainter* painter, int x, int y, int widthCell, int heightCell, int widthLeftBorder, int widthRightBorder,
                             int widthTopBorder, int widthBottomBorder)
{
    QPoint point1(x, y), point2(x + widthCell, y), point3(x, y + heightCell/* - 1*/);
    QPen pen;
	if(widthTopBorder != -1)
    {
		pen.setWidthF(widthTopBorder / 100.0);
        painter->setPen(pen);
        painter->drawLine(point1, point2);
    }

	if(widthLeftBorder != -1)
    {
		pen.setWidthF(widthLeftBorder / 100.0);
        painter->setPen(pen);
        painter->drawLine(point1, point3);
    }

	QPoint point(x + widthCell, y + heightCell);
	if(widthBottomBorder != -1)
    {
		pen.setWidthF(widthBottomBorder / 100.0);
        painter->setPen(pen);
        painter->drawLine(point3, point);
    }

	if(widthRightBorder != -1)
    {
		pen.setWidthF(widthRightBorder / 100.0);
        painter->setPen(pen);
        painter->drawLine(point2, point);
    }
}


void EjPdfDocument::drawImage(QPainter *painter, QImage image, int x, int y)
{
	painter->drawImage(x, y, image);
}
