#ifndef EJPDFDOCUMENT_H
#define EJPDFDOCUMENT_H
#include <QPdfWriter>
#include <QPainter>
#include <QPrinter>

#include "../include/ejcommon.h"

class EjPdfDocument
{
    QPen pen;
    QColor m_backgroundColor = QColor("white");
public:
    bool m_fontStyleExist = false;
	EjPdfDocument();
	~EjPdfDocument();
    void setFont(QPainter* painter, QColor color, QFont font);
    void setColorBackground(QColor color);
	void addText(QPainter* painter, EjTextBlock* textBlock, int x, int y, double scale = 1);
    void drawFillRect(QPainter* painter, int x, int y, int width, int height);
    void drawBorder(QPainter* painter, int x, int y, int widthCell, int heightCell, int widthLeftBorder, int widthRightBorder,
                    int widthTopBorder, int widthBottomBorder);
    void drawImage(QPainter *painter, QImage image, int x, int y);
};
#endif // EJPDFDOCUMENT_H
