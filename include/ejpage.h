#pragma once

#include "common_global.h"
#include "docprops.h"

class EjPage
{
public:
    EjPage();
//    virtual ~EjPage(){}
	// enum Orientation{
	// 	Portrait,
	// 	Landscape
	// };

    qint32 startBlock;
    qint32 endBlock;
    qint32 width;
    qint32 height;
    quint16 leftMarging;
    quint16 rightMarging;
    quint16 topMarging;
    quint16 bottomMarging;
    qint16 x;
    qint32 y;
	EjDocLayout::Orientation orientation;
    quint16 num;
    bool flag_redraw;
	int GetNormalHeight();
	int GetNormalWidth();
//    void calcHeight(QList<EjString*> &l_block);
};
