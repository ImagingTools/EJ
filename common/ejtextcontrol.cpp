/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#include "ejtextcontrol.h"
#include "calculatorhelper.h"
#include "ejtableblocks.h"
// #include "labelplug.h"
#include "difft.h"
#include <ejcalculator.h>
#include "docprops.h"
#include "labelblock.h"

#include <QDebug>
// #include <QQmlFile>
#include <QMimeData>
#include <QClipboard>
#include <QGuiApplication>
#include <QStaticText>
#include <QGlyphRun>
#include <QTextLayout>
#include <QTextLine>

template<>
struct StreamReader<EjBlock>
{
    static EjBlock* read(QDataStream &is);
};

EjTextControl::EjTextControl(QObject *parent) :
    QObject(parent),
    anchor(0),
    position(0),
    leftColontitul(0),
    rightColontitul(0),
    topColontitul(0),
    bottomColontitul(0),
    m_statusMode(READ_ONLY),
    m_selectMode(NO_SELECTED),
    doc(nullptr),
    docPrev(nullptr),
    m_viewMode(RICH_TEXT)
{
    SurroundingText = "";
    CurrentSelection = "";
    position =  anchor = 0;
    activeIndex = id_inputSelBlock = -1;
    m_width = 100;
    m_interval = 10;
    m_painter = NULL;
    m_deltapos = 0;
    m_inputSelectMode = false;
    m_heightCursor = 0;
    m_contentX = m_contentY = 0;
    m_posCursorX = leftColontitul;
    m_posCursorY = topColontitul;
    m_height = 0;//metric.height() * 110;
    m_isViewDoc = false;
    m_startCursor = true;
    m_showCell = false;
    m_inputSelBlock = NULL;
    is_startInputMode = false;
    m_defaultPageWidth = 21000;
    m_defaultPageHeight = 29700;
    m_defaultOrientation = EjDocLayout::ORN_PORTRAIT;
    docPrev = nullptr;
    m_currentPatch = -1;
    m_createPatchEnabled = false;
    m_clipboardDoc = nullptr;
}

EjTextControl::~EjTextControl()
{
    //   clear();
}



void EjTextControl::calcInputMethodParams()
{
    int start = 0, end = 0;

    QString str;
    SurroundingText = TextAfterCursor = TextBeforeCursor = "";
    if(doc->lBlocks->count() == 0 || activeIndex < 0 || activeIndex > doc->lBlocks->count() - 1)
        return;
    if(activeIndex >= 0)
        start = end = activeIndex;
    if(!isTable(activeIndex))
    {
        start++;
        while(start > 0 && doc->lBlocks->at(start-1)->type != ENTER && doc->lBlocks->at(start-1)->type != BASECELL
              && doc->lBlocks->at(start-1)->type != EXT_TABLE)
            start--;
        if(start > end)
            start = end;
        while(end < doc->lBlocks->count()-1 && doc->lBlocks->at(end+1)->type != ENTER && doc->lBlocks->at(end+1)->type != BASECELL
              && doc->lBlocks->at(end+1)->type != EXT_TABLE)
            end++;
    }
    else
    {
        while(start > 0 && doc->lBlocks->at(start-1)->type != ENTER && doc->lBlocks->at(start)->type != BASECELL
              && doc->lBlocks->at(start-1)->type != EXT_TABLE)
            start--;
        if(start > end)
            start = end;
        while(end < doc->lBlocks->count()-1 && doc->lBlocks->at(end+1)->type != ENTER && doc->lBlocks->at(end+1)->type != BASECELL
              && doc->lBlocks->at(end+1)->type != EXT_TABLE)
            end++;
    }



    while(start <= end)
    {
        switch (doc->lBlocks->at(start)->type) {
        case TEXT: case BASECELL:
            str = static_cast<EjTextBlock*>(doc->lBlocks->at(start))->text;
            if(start < activeIndex)
                TextBeforeCursor += str;
            if(start > activeIndex)
                TextAfterCursor += str;
            break;
        case SPACE: // case IMAGE_D:
            str = " ";
            if(start <= activeIndex)
                TextBeforeCursor += str;
            if(start > activeIndex)
                TextAfterCursor += str;
            break;
        default:
            break;
        }
        start++;
    }
    {
        if(activeIndex >= 0 && (doc->lBlocks->at(activeIndex)->type == TEXT || doc->lBlocks->at(activeIndex)->type == BASECELL))
        {
            str = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text;

            TextBeforeCursor += str.left(position);
            TextAfterCursor = str.right(str.count() - position) + TextAfterCursor;
        }
    }
    SurroundingText = TextBeforeCursor + TextAfterCursor;

}

int EjTextControl::delText(int delta, int count)
{
    if(activeIndex < 0)
        return 0;
    int start = count + delta;
    int row = -1, colum = -1, row_new = -1, colum_new = -1;
    int activeBlock_back = activeIndex;
    int position_back;
    bool is_rem = false;
    while(start > 0)
    {
        position_back = position;
        cursorRight();
        if(activeBlock_back != activeIndex
                //                && (doc->lBlocks->at(activeBlock_back)->type == TEXT || doc->lBlocks->at(activeBlock_back)->type == BASECELL ))
                && isTable(activeBlock_back) && doc->lBlocks->at(activeIndex)->type == BASECELL)
        {
            activeIndex = activeBlock_back;
            position = position_back;
            count--;
            return 0;
        }
        else
        {
            activeBlock_back = activeIndex;
            position_back = position;
        }
        start--;
    }
    for(int i = 0; i < count; i++)
    {
        inputBackSpace();
        is_rem = true;
    }
    if(is_rem)
        calc(0);
    calcInputMethodParams();
    return 1;
}

QVariant EjTextControl::inputMethodQuery(Qt::InputMethodQuery property, QVariant argument) const
{
    Q_UNUSED(argument)
    int n;
    QString str;
    QRect rect(0,0,m_width,m_height);
    QString blockTextAfterCursor, blockTextBeforeCursor;
    int maxLength = 1024;

    int start = 0, end = 0, startBlock = 0, endBlock = 0;
    int length;
    EjBlock *block;

    QString SurroundingText, TextAfterCursor, TextBeforeCursor;
    if(property == Qt::ImTextBeforeCursor || property == Qt::ImTextAfterCursor)
    {
        maxLength = argument.isValid() ? argument.toInt() : 1024;
    }

    if(doc->lBlocks->count() > 0 && activeIndex > -1 && activeIndex < doc->lBlocks->count())
    {
        start = end = startBlock = endBlock = activeIndex;
        start--;
        end++;
        length = 0;
        while(start > -1)
        {
            block = doc->lBlocks->at(start);
            str = "";
            if(block->type == TEXT)
            {
                str = (dynamic_cast<EjTextBlock*>(block))->text;
            }
            else if(block->type == SPACE)
            {
                str = " ";
            }
            if(doc->lBlocks->at(start)->type == ENTER ||
                    doc->lBlocks->at(start)->type == BASECELL )
            {
                blockTextBeforeCursor = TextBeforeCursor;
                if(length > maxLength)
                    break;
                str = "\n";
            }
            length += str.length();
            TextBeforeCursor = str + TextBeforeCursor;
            start--;
        }
        length = 0;
        while(end < doc->lBlocks->count())
        {
            block = doc->lBlocks->at(end);
            str = "";
            if(block->type == TEXT)
            {
                str = (dynamic_cast<EjTextBlock*>(block))->text;
            }
            else if(block->type == SPACE)
            {
                str = " ";
            }
            if(block->type == ENTER || block->type == BASECELL )
            {
                blockTextAfterCursor = TextAfterCursor;
                if(length > maxLength)
                    break;
                str = "\n";
            }
            length += str.length();
            TextAfterCursor = TextAfterCursor + str;
            end++;
        }
        if(blockTextBeforeCursor == "")
            blockTextBeforeCursor = TextBeforeCursor;
        if(blockTextAfterCursor == "")
            blockTextAfterCursor = TextAfterCursor;
        if(doc->lBlocks->at(activeIndex)->type == TEXT)
        {
            str = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text;

            TextBeforeCursor += str.left(position);
            TextAfterCursor = str.right(str.count() - position) + TextAfterCursor;
            blockTextBeforeCursor += str.left(position);
            blockTextAfterCursor = str.right(str.count() - position) + blockTextAfterCursor;
        }
        else if(doc->lBlocks->at(activeIndex)->type == SPACE)
        {
           TextBeforeCursor += " ";
           blockTextBeforeCursor += " ";
        }
        else if(doc->lBlocks->at(activeIndex)->type == ENTER)
        {
           TextBeforeCursor += "\n";
           blockTextBeforeCursor = "";
        }
        //        SurroundingText = TextBeforeCursor + TextAfterCursor;

    }
    switch (property) {
    case Qt::ImCursorRectangle:
        return QRectF(0,0,0,0);
    case Qt::ImFont:
        return getTextStyle(0)->m_font;
    case Qt::ImCursorPosition:
        qWarning() << "ImCursorPosition: " << TextBeforeCursor.count();
        return blockTextBeforeCursor.count();
    case Qt::ImAnchorPosition:
        qWarning() << "ImCursorPosition: " << TextBeforeCursor.count();
        return blockTextBeforeCursor.count();
    case Qt::ImSurroundingText:
        return QVariant(blockTextBeforeCursor + blockTextAfterCursor);
    case Qt::ImTextBeforeCursor:
        return QVariant(TextBeforeCursor);

    case Qt::ImTextAfterCursor:
        return QVariant(TextAfterCursor);

    case Qt::ImCurrentSelection:

        return QVariant();

    case Qt::ImMaximumTextLength:
        return QVariant(); // No limit.

    case Qt::ImAbsolutePosition:
        return TextBeforeCursor.count();
    default:
        return QVariant();
    }
}

void EjTextControl::inputText(QString text)
{
    if(text.isEmpty())
        return;
    EjTextBlock *cur_txtBlock;

    if(activeIndex > doc->lBlocks->count() -1)
    {
        doc->lBlocks->insert(doc->lBlocks->count(), new EjTextBlock());
        activeIndex = doc->lBlocks->count() -1;
        position = 0;
    }
    if(activeIndex > -1 && doc->lBlocks->at(activeIndex)->type == EXT_TABLE)
        return;
    if(activeIndex > -1 && doc->lBlocks->at(activeIndex)->type > GROUP_BLOCK)
    {
        activeIndex += ((EjGroupBlock *)doc->lBlocks->at(activeIndex))->m_counts;
        inputText(text);
        return;
    }
    if(m_createPatchEnabled)
    {
        killTimer(m_timerId);
        m_timerId = startTimer(1000);
    }
    EjTableBlock *table = isTable(activeIndex);

	if(!m_startCursor && activeIndex > -1 && doc->lBlocks->at(activeIndex)->type == ENTER)
    {
        {
            activeIndex++;
            doc->lBlocks->insert(activeIndex, new EjTextBlock());
            if(table)
                table->m_counts++;
            position = 0;
        }
    }

    else if(activeIndex == -1 || (doc->lBlocks->at(activeIndex)->type != TEXT))
    {
        if(activeIndex <= 0){
            if (activeIndex < 0) activeIndex++;;
            while (activeIndex < doc->lBlocks->count()){
                if (doc->lBlocks->at(activeIndex)->type != NUM_STYLE){
                    break;
                }
                activeIndex++;
            }
        }
        else if (!m_startCursor && doc->lBlocks->at(activeIndex)->type != ENTER
        && doc->lBlocks->at(activeIndex)->type != NUM_STYLE) {
            activeIndex++;
        }
        doc->lBlocks->insert(activeIndex, new EjTextBlock());
        if(table)
            table->m_counts++;
        position = 0;
    }
    m_startCursor = false;
    cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(activeIndex);
    if(m_statusMode == EDIT_CELL || ((m_statusMode == EDIT_TEXT) && cur_txtBlock->type == TEXT))
    {
        if(m_inputSelectMode == false)
        {
            cur_txtBlock->text.insert(position, text);
            cur_txtBlock->flag_redraw = true;
        }
        else
        {
            cur_txtBlock->text = text;
            cur_txtBlock->flag_redraw = true;
            position = 0;
        }
        position += text.size();

        QFontMetrics drawMetric = getDrawMetrics(activeIndex);

        int d = 0;
        d = drawMetric.horizontalAdvance(cur_txtBlock->text) * 100 * 0.347;
        if(doc->lBlocks->at(activeIndex)->type == BASECELL)
        {
            if(cur_txtBlock->width < d)
            {
                cur_txtBlock->width = d;
                calcData();
            }
        }
        else
            cur_txtBlock->width = d;
    }
    else if (cur_txtBlock->type == BASECELL && m_statusMode == EDIT_TEXT)
    {
        EjTableBlock *table = isTable(activeIndex);
        if(table)
        {
            int center = (table->startCell() + table->m_index + table->m_counts) / 2;
            if(activeIndex < center)
            {
                activeIndex = table->m_index - 1;
            }
            else
            {
                activeIndex = table->m_index + table->m_counts;
                if(activeIndex > doc->lBlocks->count() - 1)
                {
                    doc->lBlocks->insert(activeIndex,new EjBlock(ENTER));
                    updateFragments(activeIndex, true);
                    activeIndex++;
                    position = 0;
                }
            }
            inputText(text);
        }

    }
}

void EjTextControl::addImage(QString path)
{
    QByteArray full_key;

//    splitText(activeIndex,position);
//    //    updateFragments(activeBlock,true);
//    doc->lBlocks->insert(activeIndex,new ImageBlock_old());
//    updateFragments(activeIndex, true);
//    if(activeIndex == -1)
//        activeIndex++;
//    ImageBlock_old *curImageBlock = static_cast<ImageBlock_old*>(doc->lBlocks->at(activeIndex));
//    //    curImageBlock->height = metric.height()*4;
//    //    curImageBlock->width = metric.height()*6;
//    ext_storage->addImage(path,full_key);
//    ext_storage->loadSmallImage(&curImageBlock->small_image,full_key,0);
//    curImageBlock->name = full_key;   //.toString().remove(QChar('-'));
//    curImageBlock->width = curImageBlock->small_image.width()  * 100 * 0.347;
//    curImageBlock->ascent = curImageBlock->small_image.height() * 100 * 0.347;
//    //    curImageBlock->small_image = image.scaled(curImageBlock->width,curImageBlock->height);
//    //    curImageBlock->lData = block.lData;
//    m_inputSelectMode = false;
//    calc(activeIndex);

}

void EjTextControl::addExtBlock(EjBlock *block)
{
    splitText(activeIndex,position);
    doc->lBlocks->insert(activeIndex,block);
    updateFragments(activeIndex, true);
}

void EjTextControl::addShopList()
{

}

void EjTextControl::addClearTable()
{

}

void EjTextControl::inputContacts(ContactBlock &block)
{
    // splitText(activeIndex,position);
    // //    updateFragments(activeBlock,true);
    // doc->lBlocks->insert(activeIndex,new ContactBlock());
    // updateFragments(activeIndex, true);
    // //    activeBlock++;
    // ContactBlock *curContactBlock = static_cast<ContactBlock*>(doc->lBlocks->at(activeIndex));
    // QFontMetrics drawMetric = getDrawMetrics(activeIndex);
    // curContactBlock->ascent = drawMetric.ascent();
    // curContactBlock->name = block.name;
    // curContactBlock->lData = block.lData;
    // m_inputSelectMode = false;
    // calc(activeIndex);

}

bool EjTextControl::splitText(int &block,int &pos)
{
    QString str1;
    QString str2;
    EjTextBlock *curTextBlock;
    QString text;
    bool res = false;
    if(block < 0 || block > doc->lBlocks->count() - 1) return false;
    if(doc->lBlocks->at(block)->type > GROUP_BLOCK)
    {
        block += ((EjGroupBlock *)doc->lBlocks->at(block))->m_counts;
        return splitText(block,pos);
    }
    if(doc->lBlocks->at(block)->type == TEXT || doc->lBlocks->at(block)->type == BASECELL)
    {
        if(pos > 0)
        {
            QFontMetrics drawMetric = getDrawMetrics(block);
            curTextBlock = static_cast<EjTextBlock*>(doc->lBlocks->at(block));
            text = curTextBlock->text;
            if(pos < text.size())
            {
                //                updateFragments(block,true,true);
                str1 = text.left(pos);
                str2 = text.right(text.size() - pos);
                curTextBlock->text = str1;
                curTextBlock->flag_redraw = true;
                curTextBlock->ascent = drawMetric.ascent();
                curTextBlock->width = 0;
                block++;
                doc->lBlocks->insert(block,new EjTextBlock());
                curTextBlock = static_cast<EjTextBlock*>(doc->lBlocks->at(block));
                curTextBlock->text = str2;
                curTextBlock->ascent = drawMetric.ascent();
                curTextBlock->width = 0;
                pos = 0;
                res = true;

                EjTableBlock *table = isTable(pos);
                if(table)
                {
                    table->m_counts++;
                }
            }
            else
                block++;
        }

    }
    else
    {
        if(!doc->lBlocks->at(block)->isProperty())
            block++;
    }
    return res;

}

quint32 EjTextControl::inputEnter(bool force)
{
    quint32 res = 0;
    EjBlock *curBlock;

    EjTableBlock *table = isTable(activeIndex);
	if(activeIndex < 0 || m_startCursor)
	{
        doc->lBlocks->insert(0,new EjBlock(ENTER));
		activeIndex = 0;
		m_startCursor = false;
		position = 0;
	}
	else if(table)
    {
        if(force)
        {
            splitText(activeIndex,position);
            doc->lBlocks->insert(activeIndex,new EjBlock(ENTER));
            table->m_counts++;
        }
        else
        {
            int center = (table->startCell() + table->endBlock()) / 2;
            EjBlock *block = table;
            while(block->parent)
            {
                block = block->parent;
            }

            if(activeIndex < center)
            {
                activeIndex = doc->lBlocks->indexOf(block) - 1;
                if(activeIndex < 0)
                    activeIndex = 0;
            }
            else
            {
                if(block->type >= GROUP_BLOCK)
                    activeIndex = ((EjGroupBlock*)block)->m_index + ((EjGroupBlock*)block)->m_counts + 1;
            }
            if(activeIndex <= doc->lBlocks->count() )
            {
                doc->lBlocks->insert(activeIndex,new EjBlock(ENTER));
                position = 0;
            }
            else activeIndex--;
        }
    }
    else
	{

        m_startCursor = false;
        int index = activeIndex;
        if(index > doc->lBlocks->count() - 1)
            index = doc->lBlocks->count() - 1;
        while(index > -1 && doc->lBlocks->at(index)->isProperty())
            index--;
        if(index > -1 && index < doc->lBlocks->count()) {
            curBlock = doc->lBlocks->at(index);
            res = curBlock->ascent + curBlock->descent;
        }
        splitText(activeIndex,position);
        doc->lBlocks->insert(activeIndex,new EjBlock(ENTER));
        curBlock = doc->lBlocks->at(activeIndex);
        QFontMetrics drawMetric = getDrawMetrics(activeIndex);
        curBlock->ascent = drawMetric.ascent();
        m_inputSelectMode = false;
    }
    createPatch();

    return res;
}

void EjTextControl::inputSpace()
{
    EjTextBlock *curTextBlock;
    int back_activeBlock = activeIndex;
    {
        if(activeIndex > -1 && activeIndex < doc->lBlocks->count() && doc->lBlocks->at(activeIndex)->type != SPACE)
            createPatch();
        if(m_createPatchEnabled)
        {
            killTimer(m_timerId);
            m_timerId = startTimer(1000);
        }

        m_startCursor = false;
        EjTableBlock *table = isTable(activeIndex);
        if(splitText(activeIndex,position))
        {
        }
        if(back_activeBlock == activeIndex && activeIndex > -1 && activeIndex < doc->lBlocks->count()
                && doc->lBlocks->at(activeIndex)->type == BASECELL)
        {
            curTextBlock = new EjTextBlock();
            activeIndex++;
            doc->lBlocks->insert(activeIndex,curTextBlock);
            if(table)
                table->m_counts++;
        }
        doc->lBlocks->insert(activeIndex,new EjSpaceBlock());
        if(table)
            table->m_counts++;
        if(activeIndex < 0)
            activeIndex = 0;
        QFontMetrics drawMetric = getDrawMetrics(activeIndex);
        doc->lBlocks->at(activeIndex)->ascent = drawMetric.ascent();
    }
}

void EjTextControl::inputBackSpace()
{
    QString str;
    if(activeIndex < 0 || m_startCursor)
    {
        if(m_selectMode == SELECTED && m_selectStart == true && m_endSelectBlock > -1)
        {
            activeIndex = m_endSelectBlock;
            position = m_endSelectPos;
			m_startCursor = false;
            if(m_startSelectBlock < 0)
            {
                m_startSelectBlock = 0;
                m_startSelectPos = 0;
            }
        }
        else
            return;
    }
    if(doc->lBlocks->at(activeIndex)->type == EXT_TABLE)
        return;
    EjBlock *curBlock;
    int count_groups;
    if(m_createPatchEnabled)
    {
        killTimer(m_timerId);
        m_timerId = startTimer(1000);
    }

    if(m_selectMode == SELECTED && m_selectStart == true)
    {
        m_selectStart = false;
        activeIndex = m_endSelectBlock;
        position = m_endSelectPos;
    }

    if(doc->lBlocks->at(activeIndex)->isProperty()) {
        while(doc->lBlocks->at(activeIndex)->isProperty())
        {
            if(activeIndex > 0)
                activeIndex--;
            if(activeIndex == 0)
                return;
        }
        if(doc->lBlocks->at(activeIndex)->type == TEXT)
        {
            position = (dynamic_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex)))->text.count();
        }

    }

    if(doc->lBlocks->at(activeIndex)->type == END_GROUP)
    {
        curBlock = doc->lBlocks->at(activeIndex);
            bool isTable = false;
            count_groups = 1;
            delete doc->lBlocks->takeAt(activeIndex);
            while(count_groups > 0 && activeIndex > 0)
            {
                activeIndex--;
                curBlock = doc->lBlocks->at(activeIndex);
                if(curBlock->type == END_GROUP)
                    count_groups++;
                if(curBlock->type > GROUP_BLOCK)
                {
                    count_groups--;
                    if(!isTable)
                        isTable = curBlock->type == EXT_TABLE;
                }
                delete doc->lBlocks->takeAt(activeIndex);
            }
            activeIndex--;
            if(isTable)
                updateTables(doc);
        return;
    }



    if(doc->lBlocks->at(activeIndex)->type == BASECELL)
    {
        if(position > 0)
        {
            EjTextBlock *cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(activeIndex);
            cur_txtBlock->text = cur_txtBlock->text.remove(position-1,1);
            cur_txtBlock->flag_redraw = true;
            position--;
            if(position > 0)
            {
                str = cur_txtBlock->text.left(position);
            }
            else
                m_deltapos = 0;
        }
    }
    else if(doc->lBlocks->at(activeIndex)->type == TEXT && position > 0)
    {
        EjTextBlock *cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(activeIndex);
        cur_txtBlock->text = cur_txtBlock->text.remove(position-1,1);
        cur_txtBlock->flag_redraw = true;
        if(cur_txtBlock->text.isEmpty())
        {
            delete doc->lBlocks->at(activeIndex);
            doc->lBlocks->removeAt(activeIndex);
            updateFragments(activeIndex,false);
            activeIndex--;
            position = 0;
            if(activeIndex >= 0 && doc->lBlocks->at(activeIndex)->type == TEXT)
            {
                EjTextBlock *cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(activeIndex);
                position = cur_txtBlock->text.size();
            }
            if(activeIndex < 0)
            {
                m_startCursor = true;
            }
        }
        else
        {
            position--;
            cur_txtBlock->width = 0;
            str = cur_txtBlock->text.left(position);
        }
    }
    else if(doc->lBlocks->at(activeIndex)->type == TEXT)
    {
        if(activeIndex > 0)
        {
            activeIndex--;
            position = 0;
            if(activeIndex >= 0 && (doc->lBlocks->at(activeIndex)->type == TEXT || doc->lBlocks->at(activeIndex)->type == BASECELL))
            {
                position = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text.size();
            }
            inputBackSpace();
        }
        else
            m_startCursor = true;
    }
    else if(activeIndex >= 0)
    {
        if(doc->lBlocks->at(activeIndex)->type == ENTER)
        {
            if(activeIndex + 1 < doc->lBlocks->count() && doc->lBlocks->at(activeIndex + 1)->type == EXT_TABLE)
            {
                return;
            }
            if(activeIndex > 0 && doc->lBlocks->at(activeIndex - 1)->type == BASECELL)
            {
                return;
            }
        }
        if(doc->lBlocks->at(activeIndex)->type >= GROUP_BLOCK)
        {
            EjGroupBlock *curGroupBlock = dynamic_cast<EjGroupBlock*>(doc->lBlocks->at(activeIndex));
            curGroupBlock->remFromBlocks(doc->lBlocks);
            bool isTable = (curGroupBlock->type == EXT_TABLE || curGroupBlock->type == EXT_TASKS_GROUP);
            delete curGroupBlock;
            activeIndex--;
            if(isTable)
                updateTables(doc);
        }
        else
        {
            curBlock = doc->lBlocks->at(activeIndex);
            while(curBlock->parent)
                curBlock = curBlock->parent;
            if(curBlock->type >= GROUP_BLOCK)
            {
                EjGroupBlock *curGroupBlock = dynamic_cast<EjGroupBlock*>(curBlock);
                curGroupBlock->remBlock(doc->lBlocks,doc->lBlocks->at(activeIndex));
            }
            else if(doc->lBlocks->at(activeIndex)->type != END_GROUP)
            {
                delete doc->lBlocks->at(activeIndex);
                doc->lBlocks->removeAt(activeIndex);
            }
            activeIndex--;
        }
        position = 0;
        if(activeIndex >= 0 && (doc->lBlocks->at(activeIndex)->type == TEXT || doc->lBlocks->at(activeIndex)->type == BASECELL))
        {
            EjTextBlock *cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(activeIndex);
            position = cur_txtBlock->text.size();
        }
        if(activeIndex < 0)
        {
            activeIndex = 0;
            m_startCursor = true;
        }
    }
    if(activeIndex > doc->lBlocks->count() - 1)
        activeIndex = doc->lBlocks->count() - 1;

    if(m_selectMode == SELECTED && m_startSelectBlock > -1 && m_endSelectBlock > -1)
    {
        if(activeIndex > m_startSelectBlock)
            inputBackSpace();
        else if(position > m_startSelectPos)
            inputBackSpace();
        else setSelectMode(NO_SELECTED);
    }
}

//void EjTextControl::draw(QPainter *painter, int dx, int dy, int w, int h, bool select_area)
//{
//    EjBlock *cur_Block;
//    //    EjTextBlock *cur_txtBlock;
//    //    ContactBlock *cur_cntBlock;
//    //    ImageBlock *cur_imageBlock;
//    //    EjTableBlock *cur_TableBlock;
//    //    QString txt;
//    //    int delta_start = 0;
//    //    int delta_end = 0;
//    //    QImage image_check("://Style1/check.png");
//    static QImage image("://Style1/image6418.jpg");
//    //    QMap<quint8,Param> mActualParams;
//    //    QList<quint8> lKeys;
//    //    QFont drawFont = currentFont;
//    //    int h_align = 0;
//    //    int v_align = 0;
//    //    bool bOk;

//    QBrush brush;
//    //    brush = QBrush(QColor("#9f000000"));
//    brush.setTextureImage(image);

//    painter->setBrush(brush);
//    painter->setPen(Qt::NoPen);
//    //    painter->drawRoundedRect(dx + 5, dy + 5, width() - 10, height() - 10,5,5);
//    //    if(height() > 30)
//    //        painter->drawRect(dx + 5, dy + 5, width() - 10, height() - 10);
//    if(height() > 30)
//        painter->drawRect(dx, dy, width(), height());
//    //    painter->fillRect(0,0,w,h,brush);

//    //        painter->setBrush(QColor("#bbdcec"));
//    //        painter->setPen(Qt::NoPen);
//    //        painter->fillRect(0,m_viewH - dy,m_viewW,dy,brush);
//    //        painter->drawRect(cur_Block->x + dx,cur_Block->y + dy + d,cur_Block->width,-cur_Block->height + d1);

//    //    if(dy > 0)
//    //    {
//    //        painter->setClipRect(0,m_viewH - dy,m_viewW,dy);
//    ////        painter->fillRect(0,m_viewH - dy,m_viewW,dy,QColor(0,0,0,0));
//    //    }
//    //    else if(dy < 0)
//    //    {
//    //        painter->setClipRect(0,0,m_viewW,-dy);
//    ////        painter->fillRect(0,0,m_viewW,-dy,QColor(0,0,0,0));

//    //    }
//    //    else
//    //    {
//    //        painter->setClipRect(0,0,m_viewW,m_viewH);
//    ////        painter->fillRect(0,0,m_viewW,m_viewH,Qt::NoBrush);

//    //    }
//    bool firstDraw;
//    int k;
//    bool bExit = false;
//    k = wichBlock(-dx,-dy);
//    if(k < 0) k = 0;
//    firstDraw = true;
//    k = 0;
//    EjString *curString, *curString2 = 0;
//    //    if(m_statusMode == EDIT_CELL)
//    //    {
//    //        cur_Block = doc->lBlocks->at(activeBlock);
//    //        QBrush brush(QColor("#1f000000"));
//    //        QRegion r1(QRect(0,0,w,h));
//    //        QRegion r2(QRect(cur_Block->x,cur_Block->y + dy - cur_Block->height,cur_Block->width,cur_Block->height));
//    //        r1-=r2;
//    //        painter->setClipRegion(r1);
//    //        painter->fillRect(0,0,w,h,brush);
//    //        painter->setClipping(false);
//    //        painter->setBrush(Qt::NoBrush);
//    //    }

//    for(int row = 0 ; row < doc->lStrings->count(); row++)
//    {
//        if(bExit)
//            break;
//        curString = doc->lStrings->at(row);
//        if(k < curString->startBlock)
//            k = curString->startBlock;
//        for(int i = k; i <= curString->endBlock; i++)
//        {
//            if(i == activeBlock && m_statusMode == EDIT_CELL)
//            {
//                curString2 = curString;
//                //                    break;
//            }
//            //                else if(m_statusMode == EDIT_CELL)
//            //                    continue;
//            else if(!drawCell(i,w,h,dx,dy,select_area, firstDraw,painter, curString))
//            {
//                bExit = true;
//                break;
//            }

//        }
//    }

//    if(m_statusMode == EDIT_CELL)
//    {
//        //        painter->setBrush(QColor("#0000000f"));
//        BaseCellBlock *curCell = 0;
//        cur_Block = doc->lBlocks->at(activeBlock);
//        k = activeBlock;
//        while(doc->lBlocks->at(k)->type != BASECELL)
//            k--;
//        if(k > -1)
//            curCell = (BaseCellBlock*)doc->lBlocks->at(k);
//        if(curCell)
//        {
//            if(curString2)
//            {
//                //                painter->fillRect(curCell->x,curCell->y + dy - curCell->height,curCell->width,curCell->height,QColor("#ffffffff"));
//                k = activeBlock;
//                drawCell(k,w,h,dx,dy,select_area, firstDraw,painter, curString2);
//            }
//            QBrush brush(QColor("#1f000000"));
//            QRegion r1(QRect(0,0,w,h));
//            QRegion r2(QRect(curCell->x,curCell->y + dy - curCell->height,curCell->width,curCell->height));
//            r1-=r2;
//            painter->setClipRegion(r1);
//            if(m_showCell)
//                brush = QBrush(QColor("#9f000000"));
//            painter->fillRect(0,0,w,h,brush);
//            painter->setClipping(false);
//            painter->setBrush(Qt::NoBrush);

//            if(m_showCell)
//            {
//                firstDraw = true;
//                bExit = false;
//                k = 0;
//                for(int row = 0 ; row < doc->lStrings->count(); row++)
//                {
//                    if(bExit)
//                        break;
//                    curString = doc->lStrings->at(row);
//                    if(k < curString->startBlock)
//                        k = curString->startBlock;
//                    for(int i = k; i <= curString->endBlock; i++)
//                    {
//                        if(i == activeBlock && m_statusMode == EDIT_CELL)
//                        {
//                            curString2 = curString;
//                        }
//                        else  if(!drawCell(i,w,h,dx,dy,select_area, firstDraw,painter, curString, true))
//                        {
//                            bExit = true;
//                            break;
//                        }
//                    }
//                }
//            }
//        }

//    }
//    //    if(activeBlock > -1)
//    //    {
//    //        cur_Block = doc->lBlocks->at(activeBlock);
//    //        if(cur_Block->type == BASECELL && (m_statusMode == EDIT_TEXT || m_statusMode == EDIT_CELL))
//    //        {
//    //            painter->setPen(QColor("red"));
//    //            painter->setBrush(Qt::NoBrush);

//    //            painter->drawRect(cur_Block->x,cur_Block->y + dy,cur_Block->width,-cur_Block->height);
//    //            painter->setPen(Qt::black);
//    ////            painter->setBrush(brush);
//    //        }
//    //    }
//}

//bool EjTextControl::drawCell(int &index, int w, int h, int dx, int dy, bool select_area, bool &firstDraw, QPainter *painter, EjString *curString, bool showCell)
//{
//    EjBlock *cur_Block;
//    EjTextBlock *cur_txtBlock;
//    ContactBlock *cur_cntBlock;
//    ImageBlock_old *cur_imageBlock;
//    EjTableBlock *cur_TableBlock;
//    QString txt;
//    int cellHeight;

//    int delta_start = 0;
//    int delta_end = 0;
//    static QImage image_check("://Style1/check@4x.png");
//    //    static QImage image("://Style1/image6422.jpg");
//    QList<EjFragmentBlock*> lActualFragments;
//    QList<quint8> lKeys;
//    QFont drawFont = currentFont;
//    int h_align = 0;
//    int v_align = 0;
//    //    bool bOk;
//    if(index > doc->lBlocks->count()-1)
//        return false;
//    cur_Block = doc->lBlocks->at(index);
//    if(!cur_Block)
//        return false;
//    if(cur_Block->width == 0 || cur_Block->height == 0)
//        return true;
//    if(showCell && cur_Block->type != BASECELL)
//        return true;
//    //        if(dy > 0)
//    //        {
//    //            if(!(cur_Block->x + m_contentX <= m_viewW &&
//    //                 cur_Block->x + cur_Block->width + m_contentX >=  0 &&
//    //                 cur_Block->y - cur_Block->height + m_contentY <= m_viewH &&
//    //                 cur_Block->y + m_contentY >=  m_viewH - dy ) )
//    //                continue;
//    //        }
//    //        else if(dy < 0)
//    //        {
//    //            if(!(cur_Block->x + m_contentX <= m_viewW &&
//    //                 cur_Block->x + cur_Block->width + m_contentX >=  0 &&
//    //                 cur_Block->y - cur_Block->height + m_contentY <= -dy &&
//    //                 cur_Block->y + m_contentY >=  0 ) )
//    //                continue;
//    //        }
//    //        else
//    //        {
//    //            if(!(cur_Block->x + m_contentX <= m_viewW &&
//    //                 cur_Block->x + cur_Block->width + m_contentX >=  0 &&
//    //                 cur_Block->y - cur_Block->height + m_contentY <= m_viewH &&
//    //                 cur_Block->y + m_contentY >=  0 ) )
//    //                continue;
//    //        }

//    //        if(!(cur_Block->x + dx <= w &&
//    //             cur_Block->x + cur_Block->width + dx >=  0 &&
//    //             cur_Block->y - cur_Block->height + dy <= h &&
//    //             cur_Block->y + dy >=  0 ) )
//    //            continue;
//    //        if(!(cur_Block->y - cur_Block->height + dy <= h &&
//    //             cur_Block->y + dy >=  -5 ) )
//    //            if(!(cur_Block->x + m_contentX > - m_viewW && cur_Block->x + m_contentX <  m_viewW * 6 &&
//    //            cur_Block->y + m_contentY > - m_viewH * 3 && cur_Block->y + m_contentY <  m_viewH * 6  ))
//    //           continue;



//    //            if(cur_Block->y + dy < 0 ||
//    //                    cur_Block->y - curString->height + dy >  h)

//    if(cur_Block->type == BASECELL)
//        cellHeight = cur_Block->height;
//    else
//        cellHeight = curString->height;
//    //    if(cur_Block->y + dy < 0 ||
//    //            cur_Block->y - cellHeight + dy >  h)
//    //    {
//    //        //            continue;
//    //        if(firstDraw)
//    //            return true;
//    //        //            else if (cur_Block->type != BASECELL && cur_Block->type != CHECK)
//    //        else
//    //        {
//    //            //                if(cur_Block->type != IMAGE)
//    ////            bExit = true;
//    //            return false;
//    //        }
//    //    }
//    if(cur_Block->type != EXT_TABLE)
//        firstDraw = false;

//    drawFont = currentFont;
//    h_align = 0;
//    v_align = 0;
//    lActualFragments.clear();
//    lKeys.clear();
//    //        if(cur_Block->type != BASECELL)
//    {
//        lActualFragments = getActualFragments(index);
//        //        lKeys = lActualFragments.keys();
//        for(int jj=0; jj<lActualFragments.count(); jj++)
//        {
//            switch (lActualFragments[jj]->vid) {
//            case EjFragment::Bold:
//                drawFont.setBold(true);
//                break;
//            case EjFragment::Italic:
//                drawFont.setItalic(true);
//                break;
//            case EjFragment::Underline:
//                drawFont.setUnderline(true);
//                break;
//                //            case EjFragment::AlignHAuto:
//                //                h_align = 0;
//                //                break;
//                //            case EjFragment::AlignLeft:
//                //                h_align = Qt::AlignLeft;
//                //                break;
//                //            case EjFragment::AlignRight:
//                //                h_align = Qt::AlignRight;
//                //                break;
//                //            case EjFragment::AlignHCenter:
//                //                h_align = Qt::AlignHCenter;
//                //            case EjFragment::AlignVAuto:
//                //                v_align = 0;
//                //                break;
//                //            case EjFragment::AlignTop:
//                //                v_align = Qt::AlignTop;
//                //                break;
//                //            case EjFragment::AlignBottom:
//                //                v_align = Qt::AlignBottom;
//                //                break;
//                //            case EjFragment::AlignVCenter:
//                //                v_align = Qt::AlignVCenter;
//            default:
//                break;
//            }

//        }
//    }
//    //       return QFontMetrics(drawFont);

//    QFontMetrics drawMetrics(drawFont);

//    int d = drawMetrics.descent(), d1 = -2;
//    if(cur_Block->type == BASECELL)
//    {
//        d = -2; d1 = 4;
//        if(static_cast<BaseCellBlock*>(cur_Block)->vid == BaseCellBlock::ENDTABLE)
//            return true;
//    }

//    if(select_area)
//    {
//        //index >= m_startSelectBlock &&    index <= m_endSelectBlock && cur_Block->type != EXT_TABLE )
//        //cur_Block->type != EXT_TABLE && cur_Block->type != BASECELL
//        if(m_statusMode == SELECTED && index >= m_startSelectBlock && index <= m_endSelectBlock && !isTable(index) )
//        {
//            painter->setBrush(QColor("#bbdcec"));
//            painter->setPen(Qt::NoPen);
//            //                qDebug() << "m_startSelectBlock " << m_startSelectBlock;
//            //                qDebug() << "m_startSelectPos " << m_startSelectPos;
//            //                qDebug() << "m_endSelectBlock " << m_endSelectBlock;
//            //                qDebug() << "m_endSelectPos " << m_endSelectPos;
//            if(index == m_startSelectBlock && cur_Block->type == TEXT)
//            {
//                txt = static_cast<EjTextBlock*>(cur_Block)->text;
//                txt = txt.right(txt.size() - m_startSelectPos);
//                delta_start = drawMetrics.horizontalAdvance(txt);
//                delta_end = 0;
//                if(index == m_endSelectBlock)
//                {
//                    txt = static_cast<EjTextBlock*>(cur_Block)->text;
//                    txt = txt.right(txt.size() - m_endSelectPos);
//                    delta_end = drawMetrics.horizontalAdvance(txt);
//                }
//                painter->drawRect(cur_Block->x + dx + cur_Block->width - delta_start,
//                                  cur_Block->y + dy + d,delta_start-delta_end,-cur_Block->height + d1);
//            }
//            else if(index == m_endSelectBlock && cur_Block->type == TEXT)
//            {
//                txt = static_cast<EjTextBlock*>(cur_Block)->text;
//                txt = txt.left(m_endSelectPos);
//                delta_start = drawMetrics.horizontalAdvance(txt);
//                painter->drawRect(cur_Block->x + dx,
//                                  cur_Block->y + dy + d,delta_start,-cur_Block->height + d1);
//            }
//            else painter->drawRect(cur_Block->x + dx,cur_Block->y + dy + d,cur_Block->width,-cur_Block->height + d1);
//        }
//    }

//    //        EjFragment *curFragment;
//    //        QList<quint8>lKeys;

//    //        for(int j = 0; j < lFragments.size(); j++)
//    //        {
//    //            curFragment = lFragments.at(j);
//    //            if(i >= curFragment->startBlock && i <= curFragment->endBlock)
//    //            {
//    //                lKeys = curFragment->mParams.keys();
//    //                for(int jj=0; jj<lKeys.size(); jj++)
//    //                {
//    //                    mActualParams.insert(lKeys[jj],curFragment->mParams.value(lKeys[jj]));
//    //                }
//    //            }
//    //        }
//    //    static int table_index;
//    //        else
//    {
//        switch(doc->lBlocks->at(index)->type)
//        {
//        case EXT_TABLE:
//            painter->setPen(QColor("#d2d0d0"));
//            //                    painter->setPen(QColor(Qt::red));
//            painter->setBrush(Qt::NoBrush);
//            //                painter->setBrush(QColor("#F3F0F0"));
//            cur_Block = doc->lBlocks->at(index);
//            painter->drawRect(cur_Block->x,cur_Block->y + dy,cur_Block->width,-cur_Block->height);
//            painter->setPen(Qt::black);
//            //            table_index = 0;
//            break;
//        case BASECELL: {
//            BaseCellBlock *cur_cell = (BaseCellBlock*)doc->lBlocks->at(index);
//            int row = 0;
//            int colum = 0;

//            cur_TableBlock = cur_cell->parent;
//            if(cur_TableBlock)
//            {
//                //                int start = doc->lBlocks->indexOf(cur_TableBlock) + 1;
//                cellParams(cur_TableBlock,index,row,colum);
//                if(showCell)
//                {
//                    //                    painter->setPen(QColor("#934d50"));
//                    //                    int table_index = -1;
//                    //                    for(int i = cur_TableBlock->startBlock+1; i <= cur_TableBlock->endBlock; i++ )
//                    //                    {
//                    //                        if(doc->lBlocks->at(i)->type == BASECELL)
//                    //                            table_index++;
//                    //                        if(i == index)
//                    //                        {
//                    //                            if(cur_TableBlock->nColums() > 0)
//                    //                                row = (table_index) / cur_TableBlock->nColums();
//                    //                            colum = table_index - row*cur_TableBlock->nColums();
//                    //                            break;
//                    //                        }
//                    //                    }
//                    painter->setPen(Qt::white);
//                    txt = QString::number(cur_TableBlock->num) + QString('A' + colum) + QString::number(row+1);
//                    painter->drawText(cur_cell->x + dx + 3,cur_cell->y + dy,cur_cell->width,-cur_cell->height, Qt::AlignCenter,txt);
//                    painter->setPen(Qt::black);
//                    break;
//                }


//                if(row % 2 == 0 && !(m_statusMode == EDIT_CELL && index == activeBlock) && cur_TableBlock->evenRowsColor.alpha() > 0)
//                {

//                    //                        painter->drawRect(doc->lBlocks->at(index)->x + dx,doc->lBlocks->at(index)->y + dy,doc->lBlocks->at(i)->width,-doc->lBlocks->at(index)->height);
//                    //                            if(m_statusMode != SELECTED || i < m_startSelectBlock || i > m_endSelectBlock)
//                    {
//                        QBrush brush(cur_TableBlock->evenRowsColor);
//                        //                                BaseCellBlock *cur_cell;
//                        //                EjBlock *cur_Block;

//                        //                                int index;
//                        //                        brush.setTextureImage(image);
//                        painter->setBrush(brush);

//                        painter->setPen(Qt::NoPen);
//                        //                                painter->drawRect(0,doc->lBlocks->at(index)->y + dy,m_width,-doc->lBlocks->at(index)->height);
//                        painter->drawRect(cur_cell->x,cur_cell->y + dy,cur_cell->width,-cur_cell->height);
//                        painter->setPen(Qt::black);
//                    }
//                }
//                else
//                {
//                    painter->setPen(cur_TableBlock->borderColor);
//                    painter->setBrush(Qt::NoBrush);
//                    //                    cur_Block = doc->lBlocks->at(index);
//                    //                    painter->drawRect(cur_Block->x,cur_Block->y + dy,cur_Block->width,-cur_Block->height);
//                    painter->drawRect(cur_cell->x,cur_cell->y + dy,cur_cell->width,-cur_cell->height);
//                    painter->setPen(Qt::black);
//                }

//            }

//            if(m_statusMode == SELECTED && isCellSelected(index) )
//            {
//                painter->setBrush(QColor("#bbdcec"));
//                painter->setPen(Qt::NoPen);
//                //                                painter->drawRect(0,doc->lBlocks->at(index)->y + dy,m_width,-doc->lBlocks->at(index)->height);
//                painter->drawRect(cur_cell->x,cur_cell->y + dy - 2,cur_cell->width,-cur_cell->height + 4);
//                painter->setPen(Qt::black);
//            }

//            if(cur_cell)
//            {
//                if(m_inputSelectMode == true && index == activeBlock)
//                {
//                    drawFont.setUnderline(true);
//                }
//                painter->setFont(drawFont);
//                painter->setPen(Qt::black);
//                if(index == activeBlock && m_statusMode == EDIT_CELL)
//                    painter->drawText(cur_cell->txt_x + dx,cur_cell->txt_y + dy, cur_cell->text);
//                //                painter->drawText(cur_cell->x + dx+3,cur_cell->y + dy,cur_cell->width,-cur_cell->height, Qt::AlignLeft,cur_cell->text);
//                //                painter->drawText(cur_cell->x + dx+3,cur_cell->y + dy,cur_cell->width,-cur_cell->height, Qt::AlignLeft | Qt::AlignVCenter,cur_cell->text);
//                else
//                {
//                    if(h_align == 0)
//                    {
//                        if(cur_cell->vid == BaseCellBlock::NUMBER || cur_cell->vid == BaseCellBlock::FORMULA)
//                            h_align = Qt::AlignRight;
//                        else
//                            h_align = Qt::AlignLeft;
//                    }
//                    //                    if(v_align == 0)
//                    //                        v_align = Qt::AlignVCenter;
//                    //                    painter->drawText(cur_cell->x + dx + 3,cur_cell->y + dy,cur_cell->width - 6,-cur_cell->height, h_align | v_align,cur_cell->text);
//                    painter->drawText(cur_cell->txt_x + dx,cur_cell->txt_y + dy,cur_cell->text);
//                }
//                if(cur_cell->vid == BaseCellBlock::CHECK && cur_cell->width > 0 && cur_cell->height > 0)
//                {

//                    painter->setBrush(Qt::NoBrush);
//                    int cell_y = cur_cell->y + dy + m_interval * 0.5 - cur_cell->height;
//                    int cell_h = 16*scaleSize;
//                    painter->drawRoundedRect(cur_cell->x+dx+5*scaleSize,cell_y + 2*scaleSize,cell_h,cell_h, 2*scaleSize, 2*scaleSize);
//                    //                    painter->drawRoundedRect(cur_cell->x+dx+5*scaleSize,cur_cell->y +dy -4*scaleSize,16*scaleSize,-16*scaleSize, 2*scaleSize, 2*scaleSize);
//                    cell_h = 10.5*scaleSize;
//                    if(((BaseCellBlock*)cur_cell)->value > 0 )
//                        painter->drawImage(QRectF(cur_cell->x+dx+3*scaleSize,cell_y + 0*scaleSize,cell_h*2,cell_h*2),image_check); //0,0,200*scaleSize,200*scaleSize
//                    //                        painter->drawImage(QRectF(cur_cell->x+dx+9*scaleSize,cell_y + 6.5*scaleSize,cell_h,cell_h),image_check); //0,0,200*scaleSize,200*scaleSize
//                    //                    painter->drawImage(QRectF(cur_cell->x+dx+9*scaleSize,cur_cell->y +dy -17*scaleSize,10.5*scaleSize,10.5*scaleSize),image_check); //0,0,200*scaleSize,200*scaleSize
//                }

//                if(m_inputSelectMode == true && index == activeBlock)
//                {
//                    drawFont.setUnderline(false);
//                }
//                while(index < doc->lBlocks->count() - 1 && doc->lBlocks->at(index + 1)->type != BASECELL && doc->lBlocks->at(index + 1)->type != EXT_TABLE)
//                {
//                    index++;
//                    drawCell(index,w,h,dx,dy,select_area,firstDraw,painter,curString,showCell);
//                }
//            }
//        }
//            break;

//        case TEXT:
//            //            case TABLECELL:
//            cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(index);
//            if(cur_txtBlock)
//            {
//                if(m_inputSelectMode == true && index == activeBlock)
//                {
//                    drawFont.setUnderline(true);
//                }
//                painter->setFont(drawFont);
//                painter->setPen(Qt::black);
//                //                 painter->setPen(Qt::NoPen);
//                //                 painter->setBrush(Qt::black);
//                //                 QPainterPath p;
//                //                 p.addText(0,0, painter->font(), cur_txtBlock->text);
//                //                 QList<QPolygonF> poly = p.toFillPolygons();
//                //                for(int j = poly.size()-1; j != -1; j--)
//                //                        painter->drawPolygon(poly[j]);
//                //                QStaticText stText(cur_txtBlock->text);
//                //                painter->drawText(cur_txtBlock->x + dx,cur_txtBlock->y + dy - curString->descent,cur_txtBlock->text);
//                //                painter->drawStaticText(cur_txtBlock->x + dx,cur_txtBlock->y + dy - curString->descent,stText);
//                QGlyphRun glyphrun;
//                QRawFont raw_font = QRawFont::fromFont(drawFont, QFontDatabase::Latin);
//                glyphrun.setRawFont(raw_font);
//                glyphrun.setGlyphIndexes(raw_font.glyphIndexesForString(cur_txtBlock->text));

//                painter->drawGlyphRun(QPoint(cur_txtBlock->x + dx,cur_txtBlock->y + dy - curString->ascent),glyphrun);
//                //                QRawFont raw_font = QRawFont::fromFont(drawFont, QFontDatabase::Latin);

//                //                qreal line_width = raw_font.averageCharWidth() * cur_txtBlock->text.size();
//                ////                QSGRenderContext *sgr = QQuickItemPrivate::get(m_owner)->sceneGraphRenderContext();
//                //                QTextLayout layout(cur_txtBlock->text,drawFont);
//                //                layout.beginLayout();
//                //                QTextLine line = layout.createLine();
//                //                line.setLineWidth(line_width);
//                //                //Q_ASSERT(!layout.createLine().isValid());
//                //                layout.endLayout();
//                //                QList<QGlyphRun> glyphRuns = line.glyphRuns();
//                //                qreal xpos = cur_txtBlock->x + dx;
//                //                for (int i = 0; i < glyphRuns.size(); i++) {
//                //                    painter->drawGlyphRun(QPoint(xpos,cur_txtBlock->y + dy - curString->descent),glyphRuns.at(i));
//                ////                    node->setGlyphs(QPointF(xpos, y + raw_font.ascent()), glyphRuns.at(i));
//                //                    xpos += raw_font.averageCharWidth() * glyphRuns.at(i).positions().size();
//                //                }

//                if(m_inputSelectMode == true && index == activeBlock)
//                {
//                    drawFont.setUnderline(false);
//                }
//            }
//            break;
//        case CONTACT:
//            cur_cntBlock = (ContactBlock*)doc->lBlocks->at(index);
//            if(cur_cntBlock)
//            {
//                currentFont.setUnderline(true);
//                painter->setFont(currentFont);
//                painter->setPen(Qt::blue);
//                painter->drawText(cur_cntBlock->x + dx,cur_cntBlock->y + dy,cur_cntBlock->name);
//                currentFont.setUnderline(false);
//            }
//            break;
//        case IMAGE:
//            cur_imageBlock = (ImageBlock_old*)doc->lBlocks->at(index);
//            if(cur_imageBlock)
//            {
//                int _x = cur_imageBlock->x + dx + 4*scaleSize;
//                int _y = cur_imageBlock->y - cur_imageBlock->height + dy+ 4*scaleSize;
//                //                    int _width = cur_imageBlock->small_image.width() - 8*scaleSize;
//                //                    int _height = cur_imageBlock->small_image.height() - 8*scaleSize;
//                int _width = cur_imageBlock->width - 8*scaleSize;
//                int _height = cur_imageBlock->height - 8*scaleSize;
//                painter->setPen(QColor("#9d9bad"));
//                painter->drawLine(cur_imageBlock->x+dx,cur_imageBlock->y+dy,cur_imageBlock->x+dx+8*scaleSize,cur_imageBlock->y+dy);
//                painter->drawLine(cur_imageBlock->x+dx,cur_imageBlock->y+dy,cur_imageBlock->x+dx,cur_imageBlock->y+dy-8*scaleSize);
//                painter->drawLine(cur_imageBlock->x+dx + cur_imageBlock->width,cur_imageBlock->y+dy,cur_imageBlock->x+dx + cur_imageBlock->width-8*scaleSize,cur_imageBlock->y+dy);
//                painter->drawLine(cur_imageBlock->x+dx + cur_imageBlock->width,cur_imageBlock->y+dy,cur_imageBlock->x+dx + cur_imageBlock->width,cur_imageBlock->y+dy-8*scaleSize);
//                painter->drawLine(cur_imageBlock->x+dx + cur_imageBlock->width,cur_imageBlock->y+dy-cur_imageBlock->height,cur_imageBlock->x+dx + cur_imageBlock->width-8*scaleSize,cur_imageBlock->y+dy-cur_imageBlock->height);
//                painter->drawLine(cur_imageBlock->x+dx + cur_imageBlock->width,cur_imageBlock->y+dy-cur_imageBlock->height,cur_imageBlock->x+dx + cur_imageBlock->width,cur_imageBlock->y+dy-cur_imageBlock->height+8*scaleSize);
//                painter->drawLine(cur_imageBlock->x+dx,cur_imageBlock->y+dy-cur_imageBlock->height,cur_imageBlock->x+dx+8*scaleSize,cur_imageBlock->y+dy-cur_imageBlock->height);
//                painter->drawLine(cur_imageBlock->x+dx,cur_imageBlock->y+dy-cur_imageBlock->height,cur_imageBlock->x+dx,cur_imageBlock->y-cur_imageBlock->height+dy+8*scaleSize);
//                //                    painter->drawRect(cur_imageBlock->x + dx,cur_imageBlock->y - cur_imageBlock->height + dy,cur_imageBlock->width,cur_imageBlock->height);

//                painter->drawImage(QRectF(_x,_y,_width,_height), cur_imageBlock->small_image);
//            }
//        default:
//            break;

//        }
//    }

//    return true;

//}

//int EjTextControl::drawLens(QPainter *painter, int w, int h, float scale, int *_delta)
//{
//    int cur1 = 0;
//    int cur2 = 0;
//    int cur3;
//    int delta = 0;
//    int delta_start;
//    int delta_end;
//    EjTextBlock *cur_txtBlock;
//    ContactBlock *cur_cntBlock;
//    EjBlock *cur_Block;
//    //    int ind_string;
//    //    int ind_block;
//    QString txt;
//    painter->setClipRect(0,0,w,h);
//    if(doc->lBlocks->isEmpty()) return 0;
//    cur2 = doc->lStrings->size() -1;
//    while(1)
//    {
//        if(cur2 - cur1 <= 1)
//        {
//            break;
//        }
//        cur3 = (cur1 + cur2) / 2;
//        if(m_posCursorY < doc->lStrings->at(cur3)->y + doc->lStrings->at(cur3)->height)
//        {
//            cur2 = cur3;
//        }
//        else cur1 = cur3;
//    }
//    //+ doc->lStrings[cur2]->height
//    if(m_posCursorY < (doc->lStrings->at(cur1)->y + m_interval))
//        cur3 = cur1;
//    else cur3 = cur2;
//    cur1 = doc->lStrings->at(cur3)->startBlock;
//    cur2 = doc->lStrings->at(cur3)->endBlock;

//    painter->save();
//    painter->scale(scale,scale);
//    float k = w / scale / m_width;
//    //    float k = scale / m_width;
//    delta = m_posCursorX * (1 - k);
//    if(delta < 0) delta = 0;
//    else if(m_posCursorX > m_width - rightColontitul)
//    {
//        delta = m_posCursorX - (w - rightColontitul ) / scale;
//    }

//    for(int i = cur1; i <= cur2; i++)
//    {
//        //        QMap<quint8,Param> mActualParams = getActualParams(i);
//        QFont drawFont = currentFont;
//        //        QList<quint8> lKeys = mActualParams.keys();
//        //        for(int jj=0; jj<lKeys.size(); jj++)
//        //        {
//        //            switch (lKeys[jj]) {
//        //            case EjFragment::Bold:
//        //                drawFont.setBold(true);
//        //                break;
//        //            case EjFragment::Italic:
//        //                drawFont.setItalic(true);
//        //                break;
//        //            case EjFragment::Underline:
//        //                drawFont.setUnderline(true);
//        //                break;
//        //            default:
//        //                break;
//        //            }

//        //        }

//        QFontMetrics drawMetrics(drawFont);


//        if(m_statusMode == SELECTED && i >= m_startSelectBlock && i <= m_endSelectBlock)
//        {
//            painter->setBrush(QColor("#bbdcec"));
//            painter->setPen(Qt::NoPen);
//            cur_Block = doc->lBlocks->at(i);
//            if(i == m_startSelectBlock && cur_Block->type == TEXT)
//            {
//                txt = static_cast<EjTextBlock*>(cur_Block)->text;
//                txt = txt.right(txt.size() - m_startSelectPos);
//                delta_start = drawMetrics.horizontalAdvance(txt);
//                delta_end = 0;
//                if(i == m_endSelectBlock)
//                {
//                    txt = static_cast<EjTextBlock*>(cur_Block)->text;
//                    txt = txt.right(txt.size() - m_endSelectPos);
//                    delta_end = drawMetrics.horizontalAdvance(txt);
//                }
//                painter->drawRect(cur_Block->x - delta + cur_Block->width - delta_start,
//                                  h / 2 / scale + cur_Block->height / scale / 2 + 2,delta_start-delta_end,-cur_Block->height);
//            }
//            else if(i == m_endSelectBlock && cur_Block->type == TEXT)
//            {
//                txt = static_cast<EjTextBlock*>(cur_Block)->text;
//                txt = txt.left(m_endSelectPos);
//                delta_start = metric.width(txt);
//                painter->drawRect(cur_Block->x - delta,
//                                  h / 2 / scale + cur_Block->height / scale / 2 + 2,delta_start,-cur_Block->height);
//            }
//            else painter->drawRect(cur_Block->x - delta,h / 2 / scale + cur_Block->height / scale / 2 + 2,cur_Block->width,-cur_Block->height);
//        }

//        switch(doc->lBlocks->at(i)->type)
//        {
//        case TEXT:
//            cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(i);
//            if(cur_txtBlock)
//            {
//                if(m_inputSelectMode == true && i == activeBlock)
//                {
//                    drawFont.setUnderline(true);
//                }
//                painter->setFont(drawFont);
//                painter->setPen(Qt::black);
//                //+ cur_txtBlock->height * scale / 4
//                painter->drawText(cur_txtBlock->x - delta,h / 2 / scale + cur_txtBlock->height / scale / 2,cur_txtBlock->text);
//                //                if(m_inputSelectMode == true && i == activeBlock)
//                //                {
//                //                    drawFont.setUnderline(false);
//                //                }
//            }
//            break;
//        case CONTACT:
//            cur_cntBlock = (ContactBlock*)doc->lBlocks->at(i);
//            if(cur_cntBlock)
//            {
//                currentFont.setUnderline(true);
//                painter->setFont(currentFont);
//                painter->setPen(Qt::blue);
//                //                painter->drawText(cur_cntBlock->x + dx,cur_cntBlock->y + dy,cur_cntBlock->name);
//                painter->drawText(cur_cntBlock->x - delta,h / 2 / scale + cur_cntBlock->height / scale / 2,cur_cntBlock->name);
//                currentFont.setUnderline(false);
//            }
//            break;
//        default:
//            break;

//        }


//    }
//    painter->restore();
//    if(_delta) *_delta = delta;
//    return (m_posCursorX - delta) * scale;


//}

void EjTextControl::setWidth(int width)
{
    if(m_viewMode == RICH_TEXT)
    {
        m_width = width;
    }
}

void EjTextControl::setHeight(int height)
{
    if(m_viewMode == RICH_TEXT)
    {
        calc();
        if(m_height < height)
        {
            m_height = height;
            emit controlHeightChanged();
        }
    }
}

void EjTextControl::calcCursor(bool force)
{
    if (doc == nullptr){
        return;
    }
    int x,y;
    QString str;
    int leftControl;
    EjBlock *curBlock;
    bool isPositionChanged = false;
    bool isHeightChanged = false;
    if(m_isViewDoc && !doc->lPages->isEmpty())
    {
        leftControl = leftColontitul + doc->lPages->at(0)->leftMarging;
        y = doc->lPages->at(0)->topMarging;;
    }
    else
    {
        leftControl = leftColontitul;
        y = 0;
    }

    if(m_startCursor || activeIndex < 0)
    {
        x = leftControl;
        emit cursorPositionChanged(x,y);
        m_posCursorX = x; m_posCursorY = y;
        int h = 0;
        if(!doc->lBlocks->isEmpty())
        {
            int index = 0;
            while (index < doc->lBlocks->count()) {
                if(doc->lBlocks->at(index)->type == TEXT)
                {
                    h = doc->lBlocks->at(index)->height();
                    break;
                }
                index++;
            }
        }
        if(h == 0)
        {
            EjTextStyle *textStyle = getTextStyle(0);
            if(textStyle)
            {
                h = textStyle->fontSize()  * 100 * 0.347;
            }
            else
                h = 400;
        }

        if(force || h != m_heightCursor)
            emit cursorHeightChanged(h);
        m_heightCursor = h;
        return;
    }
    if(doc->lBlocks->empty()) return;

    if(activeIndex >= 0)
    {
        curBlock = doc->lBlocks->at(activeIndex);
        x = curBlock->x;
        if(curBlock->isProperty())
            y = curBlock->y;
        else
            y = curBlock->y + curBlock->interval_top;
        if(curBlock->type == TEXT || curBlock->type == BASECELL)
        {
            QFontMetrics drawMetrics = getDrawMetrics(activeIndex);

            str = static_cast<EjTextBlock*>(curBlock)->text.left(position);
            m_deltapos = drawMetrics.horizontalAdvance(str) * 100 * 0.347;
            x += m_deltapos;
        }
        else if(curBlock->type == ENTER)
        {
        }
        else
        {
            x += curBlock->width;
        }
        if(force || m_posCursorX != x || m_posCursorY != y)
        {
            isPositionChanged = true;
            m_posCursorX = x; m_posCursorY = y;
        }
        if( doc->lBlocks->at(activeIndex)->type == BASECELL)
        {
            QFontMetrics drawMetrics = getDrawMetrics(activeIndex);
            int h = drawMetrics.height() * 100 * 0.347 + m_interval*1.5;
            EjTableBlock *table = isTable(activeIndex);
            x = curBlock->x + table->spacing;
            isPositionChanged = true;

            if(force || h != m_heightCursor)
                isHeightChanged = true;
            m_heightCursor = h;
        }
        else
        {
            int index = activeIndex;
            curBlock = doc->lBlocks->at(index);
            while(curBlock->isProperty() && index > 0)
            {
                index--;
                curBlock = doc->lBlocks->at(index);
            }
            int h = curBlock->ascent + curBlock->descent; // * (curBlock->type == TEXT || curBlock->type == ENTER || curBlock->type == SPACE ? 0.7 : 1.0);
            if(index != activeIndex)
            {
            }
            if(isTable(activeIndex))
                h += m_interval*1.5;
            if(force || h != m_heightCursor)
                isHeightChanged = true;
            m_heightCursor = h;

        }
        if(isPositionChanged)
            emit cursorPositionChanged(x,y);
        if(isHeightChanged)
            emit cursorHeightChanged(m_heightCursor);
    }

}

void EjTextControl::calcData(bool force)
{
    if (doc == nullptr){
        return;
    }
    m_calcIndex -= 5000;
    if(m_calcIndex < 0)
        m_calcIndex = 0;
    calcNext(force);
    if(activeIndex > doc->lBlocks->count() - 1)
        activeIndex = doc->lBlocks->count() - 1;
}

void EjTextControl::calcNext(bool force)
{
    if (doc == nullptr){
        return;
    }
    if(doc->lBlocks->count() == 0)
        return;
    int index = m_calcIndex;
    if(m_calcIndex > 0 && (!force && m_calcIndex == doc->lBlocks->count() - 1))
        return;
    if(m_calcIndex + 5000 > doc->lBlocks->count() - 1) {
        m_calcIndex = doc->lBlocks->count() - 1;
    }
    else
        m_calcIndex += 5000;
    calc(index, force);
}

void EjTextControl::setCursor(int x, int y)
{
    if (doc == nullptr){
        return;
    }
    int cur1 = 0;
    int cur2 = 0;
    int cur3;
    int newActiveBlock;
    m_startCursor = false;
    QString txt;
    newActiveBlock = wichBlock(x,y);
    EjTableBlock *table = isTable(newActiveBlock);

    if(m_statusMode == EDIT_CELL && newActiveBlock != activeIndex)
    {
        if(!table)
            return;
        cur1 = newActiveBlock;
        while(cur1 > -1 && cur1 < doc->lBlocks->count() - 1 && doc->lBlocks->at(cur1)->type != BASECELL)
            cur1--;
        cur2 = activeIndex;
        while(cur2 > -1 && cur2 < doc->lBlocks->count() - 1 && doc->lBlocks->at(cur2)->type != BASECELL)
            cur2--;
        if(cur1 != cur2)
            return;
    }
    if(table && m_statusMode == EDIT_TEXT && doc->lBlocks->at(newActiveBlock)->type != BASECELL)
    {
        newActiveBlock = table->prevCell(newActiveBlock);
    }
    activeIndex = newActiveBlock;
    position = 0;
    if(activeIndex < 0)
    {
        if(doc->lBlocks->count() > 0)
        {
            m_startCursor = false;
            activeIndex = 0;
            position = 0;
            while(activeIndex < doc->lBlocks->count() - 1 &&
                  doc->lBlocks->at(activeIndex)->isProperty())
                activeIndex++;
        }
        else m_startCursor = true;
        calcCursor();
        return;
    }
    if(doc->lBlocks->at(activeIndex)->type == BASECELL && m_statusMode != EDIT_CELL)
    {
        calcCursor();
        return;
    }

    if(doc->lBlocks->at(activeIndex)->type == TEXT || doc->lBlocks->at(activeIndex)->type == BASECELL)
    {
        bool is_remove = false;
        EjTextBlock *cur_txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex));

        QFontMetrics drawMetrics = getDrawMetrics(activeIndex);
        QString str3;
        position = 0;

        txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text;
        cur1 = 0;
        cur2 = txt.size();
        int d1 = doc->lBlocks->at(activeIndex)->x;
        str3 = txt;
        int d3 = drawMetrics.horizontalAdvance(str3) * 100 * 0.347;
        while(1)
        {
            if(cur2 - cur1 <= 1)
            {
                break;
            }
            cur3 = (cur1 + cur2) / 2;
            str3 = txt.left(cur3);
            d3 = drawMetrics.horizontalAdvance(str3) * 100 * 0.347 + doc->lBlocks->at(activeIndex)->x;
            if(x < d3)
            {
                cur2 = cur3;
            }
            else {
                cur1 = cur3;
                d1 = d3;
            }
        }
        str3 = txt.left(cur2);
        position_w = drawMetrics.horizontalAdvance(str3) * 100 * 0.347;
        d3 = position_w + doc->lBlocks->at(activeIndex)->x;
        if(abs(x-d1) <= abs(d3-x)) position = cur1;
        else position = cur2;
        if(doc->lBlocks->at(activeIndex)->type == BASECELL && doc->lBlocks->at(activeIndex + 1)->type != BASECELL)
        {
            EjCellBlock *curCell = (EjCellBlock*)doc->lBlocks->at(activeIndex);
                while(activeIndex <  doc->lBlocks->count() - 1
                      && (doc->lBlocks->at(activeIndex + 1)->type != BASECELL
                          && doc->lBlocks->at(activeIndex + 1)->type != END_GROUP))
                    activeIndex++;
                if(doc->lBlocks->at(activeIndex)->type == TEXT)
                {
                    txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text;
                    position = txt.count();
                }
                else
                    position = 0;
        }
        str3 = txt.left(position);
        position_w = drawMetrics.horizontalAdvance(str3) * 100 * 0.347;

    }
    else if(activeIndex == 0 && doc->lBlocks->at(activeIndex)->type == ENTER)
        m_startCursor = true;

    if(m_statusMode == EDIT_TEXT || m_statusMode == EDIT_CELL)
    {
        calcCursor();
    }
}

void EjTextControl::startSelected(int x, int y)
{
    if(m_selectMode != SELECTED)
        return;
    m_selectStart = true;
    if(doc->lBlocks->isEmpty() || activeIndex < 0) return;
    if(activeIndex < (m_startSelectBlock + m_endSelectBlock) / 2 )
    {
    }
    else if(activeIndex > (m_startSelectBlock + m_endSelectBlock) / 2)
    {
        m_selectStart = false;
    }
    else
    {
        if(doc->lBlocks->at(activeIndex)->type == TEXT && m_startSelectBlock == m_endSelectBlock)
        {
            if(position <= (m_startSelectPos + m_endSelectPos) / 2)
            {
            }
            else
            {
                m_selectStart = false;
            }
        }

    }

}

void EjTextControl::updateSelected()
{
    if(activeIndex < 0 || m_selectMode != SELECTED)
        return;
    doc->lBlocks->at(activeIndex)->flag_redraw = true;
    if(m_selectStart)
    {
        if(activeIndex < m_endSelectBlock || (activeIndex == m_endSelectBlock && position < m_endSelectPos))
        {
            m_startSelectBlock = activeIndex;
            m_startSelectPos = position;
            m_startSelectWidth = position_w;
        }
        else
        {
            m_startSelectBlock = m_endSelectBlock;
            m_startSelectPos = m_endSelectPos;
            m_startSelectWidth = m_endSelectWidth;
            m_endSelectBlock = activeIndex;
            m_endSelectPos = position;
            m_endSelectWidth = position_w;
            m_selectStart = false;
        }
    }
    else
    {
        if(activeIndex > m_startSelectBlock || (activeIndex == m_startSelectBlock && position > m_startSelectPos))
        {
            m_endSelectBlock = activeIndex;
            m_endSelectPos = position;
            m_endSelectWidth = position_w;
        }
        else
        {
            m_endSelectBlock = m_startSelectBlock;
            m_endSelectPos = m_startSelectPos;
            m_endSelectWidth = m_startSelectWidth;;
            m_startSelectBlock = activeIndex;
            m_startSelectPos = position;
            m_startSelectWidth = position_w;
            m_selectStart = true;
        }
    }

}

void EjTextControl::clearSelected()
{
    m_startSelectBlock = m_endSelectBlock = -1;
    m_startSelectPos = m_endSelectPos = 0;
}

int EjTextControl::wichBlock(int x, int y)
{
    int cur1 = 0;
    int cur2 = 0;
    int cur3;
    int cur4;
    int res = -1;
    EjGroupBlock *cur_block;
    EjCellBlock *curCell;
    EjString *curString;
    EjBlock *block;
    if(doc->lBlocks->isEmpty() || doc->lStrings->isEmpty())
        return res;
    cur2 = doc->lStrings->count() -1;
    while(1)
    {
        if(cur2 - cur1 <= 1)
        {
            break;
        }
        cur3 = (cur1 + cur2) / 2;
        if(y < doc->lStrings->at(cur3)->y + doc->lStrings->at(cur3)->height)
        {
            cur2 = cur3;
        }
        else cur1 = cur3;
        qDebug() << "while 1" << cur1 << cur2;
    }
    if(y < (doc->lStrings->at(cur1)->y + doc->lStrings->at(cur1)->height + m_interval))
        cur3 = cur1;
    else cur3 = cur2;
    curString = doc->lStrings->at(cur3);
    cur1 = curString->startBlock;
    cur2 = curString->endBlock;
    if(cur2 > doc->lBlocks->count() - 1)
        cur2 = doc->lBlocks->count() - 1;
    if(cur1 < 0)
        cur1 = 0;
    if(cur2 < 0)
        cur2 = 0;
    while(1)
    {
        qDebug() << "while 2" << cur1 << cur2 << cur3;
        if(doc->lBlocks->at(cur1)->parent != NULL && !doc->lBlocks->at(cur1)->parent->isGlassy())
        {
            block = doc->lBlocks->at(cur1);
            block = block->rootBlock();
            cur1 = (dynamic_cast<EjGroupBlock*>(block))->m_index;
            if(doc->lBlocks->at(cur1) != block)
            {
                (dynamic_cast<EjGroupBlock*>(block))->calcLenght(cur1,doc->lBlocks);
            }
        }
        cur3 = cur2;
        if(doc->lBlocks->at(cur2)->parent != NULL && !doc->lBlocks->at(cur2)->parent->isGlassy())
        {
            block = doc->lBlocks->at(cur2);
            block = block->rootBlock();
            cur_block = dynamic_cast<EjGroupBlock*>(block);
            cur3 = cur_block->m_index;
        }
        if(doc->lBlocks->at(cur1)->x > x)
        {
            cur2 = cur1;
            break;
        }

        if(cur3 != cur2 && doc->lBlocks->at(cur3)->x < x)
        {
            cur1 = cur2;
            break;
        }
        cur2 = cur3;
        if(cur2 - cur1 <= 1)
        {
            break;
        }
        cur3 = (cur1 + cur2) / 2;
        if(doc->lBlocks->at(cur3)->isGlassy())
            cur3++;
        while(doc->lBlocks->at(cur3)->isProperty() && cur3 < doc->lBlocks->count() - 1)
            cur3++;
        if(doc->lBlocks->at(cur3)->parent != NULL && !doc->lBlocks->at(cur3)->parent->isGlassy())
        {
            block = doc->lBlocks->at(cur3);
            block = block->rootBlock();
            cur3 = (dynamic_cast<EjGroupBlock*>(block))->m_index;
        }
        if(cur3 == cur1)
        {
            cur2 = cur1;
            break;
        }
        if(cur3 == cur2)
        {
            cur1 = cur2;
            break;
        }
        if((cur3 > 0 && x < doc->lBlocks->at(cur3)->x + doc->lBlocks->at(cur3)->width))
        {
            cur2 = cur3;
        }
        else cur1 = cur3;

    }
    if(cur1 >= 0 && x <= (doc->lBlocks->at(cur1)->x + doc->lBlocks->at(cur1)->width))
        res = cur1;
    else res = cur2;
    if(res == 0 && x < doc->lBlocks->at(res)->x)
    {
        res--;
    }
    if(res < 0)
        return res;
    EjTableBlock *table = 0;
    foreach(EjTableBlock *curTable, *doc->lTables)
    {
        if(res >= curTable->m_index && res <= (curTable->m_index + curTable->m_counts))
        {
            table = curTable;
            break;
        }
    }

    if(table)
    {
        double d1;
        double d2;
        curCell = nullptr;
        bool first = true;

        for(int i = table->startCell(); i <= table->endBlock(); i++)
        {
            if(i < doc->lBlocks->count() - 1 && doc->lBlocks->at(i)->type == BASECELL) // && doc->lBlocks->at(i+1)->type != BASECELL)
            {
                if(curCell)
                    break;
                if(x >= doc->lBlocks->at(i)->x && x <= doc->lBlocks->at(i)->x + doc->lBlocks->at(i)->width
                        && y >= doc->lBlocks->at(i)->y && y <= doc->lBlocks->at(i)->y + doc->lBlocks->at(i)->ascent)
                {
                        curCell = (EjCellBlock*)doc->lBlocks->at(i);
                        res = i;
                        d1 = (curCell->x - x) * (curCell->x - x) + (curCell->y - y) * (curCell->y - y);
                        d2 = (curCell->x + curCell->width - x) * (curCell->x + curCell->width - x) + (curCell->y + curCell->height() - y) * (curCell->y + curCell->height() - y);
                        if(curCell->visible == false)
                        {
                            curCell = (EjCellBlock*)curCell->parent;
                            res = doc->lBlocks->indexOf(curCell);
                            d1 = (curCell->x - x) * (curCell->x - x) + (curCell->y - y) * (curCell->y - y);
                            d2 = (curCell->x + curCell->width - x) * (curCell->x + curCell->width - x) + (curCell->y + curCell->height() - y) * (curCell->y + curCell->height() - y);
                        }
                }

            }
            else
            {
                EjBlock *curBlock = doc->lBlocks->at(i);
                if(x >= curBlock->x && x <= curBlock->x + curBlock->width
                        && y >= curBlock->y && y <= curBlock->y + curBlock->interval_top + curBlock->ascent + curBlock->descent + curBlock->interval_bottom)
                {
                    res = i;
                    curCell = table->currentCell(i);
                    if(curCell->visible == false)
                    {
                        curCell = (EjCellBlock*)curCell->parent;
                        res = doc->lBlocks->indexOf(curCell);
                    }
                    break;
                }
                else
                {
                    double dd1 = (curBlock->x - x) * (curBlock->x - x) + (curBlock->y - y) * (curBlock->y - y);
                    double dd2 = (curBlock->x + curBlock->width - x) * (curBlock->x + curBlock->width - x) + (curBlock->y + curBlock->height() - y) * (curBlock->y + curBlock->height() - y);
                    if(curCell && (dd1 < d1 || dd2 < d2 || first))
                    {
                        res = i;
                        d1 = dd1;
                        d2 = dd2;
                        first = false;
                    }
                }
            }

        }
    }
    if(res > -1)
    {
        block = doc->lBlocks->at(res);
        while(block->isProperty() && res >= 0)
        {
            res--;
            if(res > -1)
                block = doc->lBlocks->at(res);
        }
    }
    return res;
}

EjBlock* EjTextControl::getBlock(int x, int y)
{
    EjBlock *block = 0;
    int res = wichBlock(x,y);
    if(res > -1)
        block = doc->lBlocks->at(res);
    return block;
}

EjBlock *EjTextControl::currentBlock()
{
    if(activeIndex > -1 && activeIndex < doc->lBlocks->count() )
        return doc->lBlocks->at(activeIndex);
    return NULL;
}

void EjTextControl::selectBlock(int x, int y)
{
    int sel_block = wichBlock(x,y);
    if(sel_block < 0) return;
    EjTableBlock *table = isTable(sel_block);
    m_startSelectBlock = m_endSelectBlock = sel_block;
    m_startSelectPos = m_endSelectPos = 0;
    if(table && m_statusMode != EDIT_CELL)
    {
        if(doc->lBlocks->at(sel_block)->type != BASECELL)
            m_startSelectBlock = m_endSelectBlock = table->prevCell(sel_block);
    }
    else if(doc->lBlocks->at(sel_block)->type == TEXT || doc->lBlocks->at(sel_block)->type == BASECELL)
    {
        while(m_startSelectBlock > 0)
        {
            if(doc->lBlocks->at(m_startSelectBlock-1)->type != TEXT)
                break;
            m_startSelectBlock--;
        }
        while(m_endSelectBlock < doc->lBlocks->count() - 1)
        {
            if(doc->lBlocks->at(m_endSelectBlock+1)->type != TEXT)
                break;
            m_endSelectBlock++;
        }
        m_endSelectPos = static_cast<EjTextBlock*>(doc->lBlocks->at(m_endSelectBlock))->text.size();
    }
}

void EjTextControl::cursorLeft()
{
    bool changeActive = false;
    int activeBlock_back;
    EjBlock *curBlock;
    int count_groups;
    resetUpDown();
    if(doc->lBlocks->count() == 0 || activeIndex < 0)
        return;
    int index = activeIndex;
    while(doc->lBlocks->at(index)->isProperty())
    {
        position = 0;
        index--;
        if(index < 0)
        {
            break;
        }
    }
    if(index < 0)
        return;
    else
        activeIndex = index;
    if(m_statusMode == EDIT_TEXT)
    {
        EjTableBlock *table = isTable(activeIndex);
        if(table)
        {
            activeIndex = table->currCellIndex(activeIndex);
            if(activeIndex < 0)
                activeIndex = 0;
        }
    }
    if(doc->lBlocks->at(activeIndex)->type == TEXT)
    {

        if(position > 0)
        {
            position--;
            if(position == 0 && activeIndex > 0 && doc->lBlocks->at(activeIndex-1)->type == END_GROUP)
                changeActive = true;
        }
        else if(activeIndex > 0)
        {
            changeActive = true;
        }
    }
    else
    {
        changeActive = true;
    }
	if(changeActive && activeIndex > 0)
    {
        activeBlock_back = index = activeIndex;
        position = 0;
        index--;
        while(index > 0 && doc->lBlocks->at(index)->type != BASECELL && doc->lBlocks->at(index)->isProperty())
        {
            index--;
        }
		if(index > -1 && doc->lBlocks->at(index)->type != BASECELL && !doc->lBlocks->at(index)->isProperty())
                activeIndex = index;

		if(index > -1 && m_statusMode == EDIT_TEXT && doc->lBlocks->at(index)->type == BASECELL)
                activeIndex = index;

        if(doc->lBlocks->at(activeIndex)->type == SPACE || doc->lBlocks->at(activeIndex)->type == ENTER )
        {
            if(activeIndex > 0 && (doc->lBlocks->at(activeBlock_back)->type == TEXT ||
                                   doc->lBlocks->at(activeBlock_back)->type == BASECELL))
                //            if(activeBlock > 0)
            {
                activeBlock_back = activeIndex;
                activeIndex--;
                position = 0;
            }
			else
				m_startCursor = true;
        }
        if(activeBlock_back != activeIndex && doc->lBlocks->at(activeIndex)->type == TEXT) // || doc->lBlocks->at(activeIndex)->type == BASECELL)
        {
            position = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text.size();
            if(doc->lBlocks->at(activeBlock_back)->type == TEXT)
            {
                position--;
                if(position < 0)
                    position = 0;
            }
        }
        curBlock = doc->lBlocks->at(activeIndex);
        if(curBlock->type == END_GROUP)
        {
            count_groups = 1;
            while(count_groups > 0 && activeIndex > 0)
            {
                activeIndex--;
                curBlock = doc->lBlocks->at(activeIndex);
                if(curBlock->type == END_GROUP)
                    count_groups++;
                if(curBlock->type > GROUP_BLOCK)
                    count_groups--;
            }
        }

    }
    else if(changeActive && activeIndex == 0)
    {
		m_startCursor = true;
    }
    qDebug() << "position= " << position;
}

void EjTextControl::cursorRight()
{
    bool changeActive = false;
    resetUpDown();
    if(doc->lBlocks->count()==0)
        return;
    if(m_startCursor)
    {
        m_startCursor = false;
        if(activeIndex > -1 && doc->lBlocks->at(activeIndex)->type != TEXT && doc->lBlocks->at(activeIndex)->type != BASECELL)
            return;
    }
    if(m_statusMode == EDIT_TEXT)
    {
        EjTableBlock *table = isTable(activeIndex);
        if(table)
        {
            activeIndex = table->currCellIndex(activeIndex);
            if(activeIndex < 0)
                activeIndex = 0;
        }
    }
    if(activeIndex > -1 && (doc->lBlocks->at(activeIndex)->type == TEXT)) // || (doc->lBlocks->at(activeIndex)->type == BASECELL && m_statusMode == EDIT_CELL)))
    {
        if(position < static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text.size()) position++;
        else if(activeIndex < doc->lBlocks->count() - 1)
        {
            if(m_statusMode == EDIT_CELL)
            {
                if(doc->lBlocks->at(activeIndex + 1)->type != BASECELL)
                    changeActive = true;
            }
            else
            {
                changeActive = true;
            }
        }
    }
    else if(m_statusMode == EDIT_TEXT || doc->lBlocks->at(activeIndex + 1)->type != BASECELL)
    {
        if(m_statusMode == EDIT_CELL)
        {
            if(doc->lBlocks->at(activeIndex + 1)->type != BASECELL)
                changeActive = true;
        }
        else
        {
            changeActive = true;
        }
    }
    if(changeActive && activeIndex < doc->lBlocks->count() - 1 && doc->lBlocks->count() > 0)
    {
        if(activeIndex > -1 && m_statusMode == EDIT_TEXT && doc->lBlocks->at(activeIndex)->type == BASECELL )
        {
            EjTableBlock *table = isTable(activeIndex);
            if(table)
                activeIndex = table->nextCell(activeIndex);
        }
        else
        {
            int index = activeIndex;
            if(index > -1 && doc->lBlocks->at(index)->type > GROUP_BLOCK)
            {
                index += ((EjGroupBlock*)doc->lBlocks->at(index))->m_counts + 1;
                if(index > doc->lBlocks->count() - 1)
                    index = doc->lBlocks->count() - 1;
                position = 0;
            }
            else
                index++;
            bool isAllProperty = true;
            while(doc->lBlocks->at(index)->isProperty() && index < doc->lBlocks->count() - 1)
            {
                isAllProperty = false;

                if(m_statusMode == EDIT_CELL)
                {
                    if(doc->lBlocks->at(index + 1)->type != BASECELL)
                        index++;
                    else
                        break;
                }
                else
                {
                    index++;
                }
            }
            if(activeIndex != index && !doc->lBlocks->at(index)->isProperty())
            {
                activeIndex = index;
                position = 0;
            }
        }
        if(doc->lBlocks->at(activeIndex)->type == TEXT && static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text.size() > position)
            position++;

    }
    qDebug() << "position= " << position;
}

void EjTextControl::worldLeft()
{
    EjTableBlock *table = isTable(activeIndex);
    if(table && m_statusMode == EDIT_TEXT)
        return;
    if(activeIndex < 0)
    {
        m_startCursor = true;
        return;
    }
    if(activeIndex > 0 && doc->lBlocks->at(activeIndex)->type == TEXT && position == 0)
    {
        int index = activeIndex - 1;
        while(index > 0 &&  doc->lBlocks->at(index)->isProperty() && doc->lBlocks->at(index)->type != BASECELL)
            index--;
        if(!doc->lBlocks->at(index)->isProperty() &&  doc->lBlocks->at(index)->type != BASECELL)
            activeIndex = index;
    }
    while(activeIndex > 0 && doc->lBlocks->at(activeIndex)->type != TEXT)
        cursorLeft();
    if(activeIndex < 0)
    {
        m_startCursor = true;
        return;
    }
    activeIndex = startText(activeIndex);
    if(activeIndex < 0)
    {
        m_startCursor = true;
    }
    position = 0;
}

void EjTextControl::worldRight()
{
    EjTableBlock *table = isTable(activeIndex);
    if(table && m_statusMode == EDIT_TEXT)
        return;
    if(activeIndex < 0)
        activeIndex = 0;
    while(activeIndex < doc->lBlocks->count() - 1 && doc->lBlocks->at(activeIndex)->type != TEXT)
        cursorRight();
    if(activeIndex > doc->lBlocks->count() - 1)
        activeIndex = doc->lBlocks->count() - 1;
    int index = endText(activeIndex);
    while(index < doc->lBlocks->count() - 1 &&  doc->lBlocks->at(index)->isProperty() && doc->lBlocks->at(index)->type != BASECELL)
        index++;
    if(!doc->lBlocks->at(index)->isProperty() && doc->lBlocks->at(index)->type != BASECELL)
        activeIndex = index;
    if(activeIndex > doc->lBlocks->count() - 1)
        activeIndex = doc->lBlocks->count() - 1;
    position = 0;
    if(doc->lBlocks->at(activeIndex)->type == TEXT)
        position = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text.size();

}

void EjTextControl::resetUpDown()
{
    m_upDown_x = m_upDown_y = -1;
}


void EjTextControl::cursorUp()
{
    if(activeIndex < 1)
        return;
    int x_s;
    int y_s;
    if( m_upDown_x == -1 && m_upDown_y == -1)
    {
        x_s = m_upDown_x = m_posCursorX;
    }
    else
    {
        x_s = m_upDown_x;
    }

    int index_string = getIndexString(activeIndex);
    EjString *cur_string = doc->lStrings->at(0);
    index_string--;
    if(index_string > -1)
        cur_string = doc->lStrings->at(index_string);
    y_s = m_upDown_y = cur_string->y; //m_posCursorY +
    setCursor(x_s,y_s);
}

void EjTextControl::cursorDown()
{
    if(m_startCursor && doc->lBlocks->count() > 0)
    {
        m_startCursor = false;
        activeIndex = 0;
        position = 0;
    }
    if(activeIndex < 0)
        return;
    int x_s;
    int y_s;
    if( m_upDown_x == -1 && m_upDown_y == -1)
    {
        x_s = m_upDown_x = m_posCursorX;
    }
    else
    {
        x_s = m_upDown_x;
    }
    int index_string = getIndexString(activeIndex);
    EjString *cur_string = doc->lStrings->at(index_string);

    int delta = m_posCursorY - cur_string->y;
    index_string++;
    if(index_string < doc->lStrings->count())
        cur_string = doc->lStrings->at(index_string);
    y_s = m_upDown_y =  cur_string->y;

    qDebug() << __FILE__ << __LINE__ << "m_posCursorY:" << m_posCursorY << "y_s:" << y_s;
    setCursor(x_s,y_s);
}

void EjTextControl::cursorFirst()
{
    if(doc->lBlocks->count() > 0)
        activeIndex = 0;
    else
        activeIndex = -1;
    position = 0;
}

void EjTextControl::cursorLast()
{
    activeIndex = doc->lBlocks->count() - 1;
    position = 0;
}

int EjTextControl::setInputMode(bool mode, QString &text, int preeditCursor)
{
    EjTextBlock *cur_txt = nullptr;
    int res = 0;

    if(text.contains(" "))
    {
        QStringList ltext = text.split(" ");
        int pos = 0;
        int activeIndexBack = activeIndex;
        if(id_inputSelBlock > -1)
            activeIndex = id_inputSelBlock;
        else
            id_inputSelBlock = activeIndex;
        bool bSendCursor = true;
        for(int i = 0; i < ltext.count(); i++)
        {

            if(ltext[i].count() > 0)
            {
                if(i > 0)
                {
                    pos++;
                    if(mode == true)
                    {
                        inputSpace();
                    }
                }
                if(mode)
                {
                    cur_txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex));
                    if(cur_txt->type != TEXT || i > 0)
                    {
                        activeIndex++;
                        doc->lBlocks->insert(activeIndex, new EjTextBlock());
                        cur_txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex));
                    }
                    cur_txt->text = ltext[i];
                    cur_txt->flag_redraw = true;
                    position = cur_txt->text.length();
                    cur_txt->width = 0;
                }
                if(pos + ltext[i].count() >= preeditCursor && bSendCursor)
                {
                    if(getBlocks()->at(activeIndex)->type == TEXT)
                    {
                        if(mode == true)
                            position = preeditCursor - pos;
                        EjTextBlock *block = dynamic_cast<EjTextBlock*>(getBlocks()->at(activeIndex));
                        if(block && (block->text.length() < position ) && mode == true) //|| mode == false
                        {
                            position = block->text.length();
                        }
                    }
                    else
                    {
                        position = 0;
                    }
                    bSendCursor = false;
                }
                pos += ltext[i].count();
            }
            else if(i > 0){
                pos++;
                if(mode == true)
                {
                    inputSpace();
                    position = 0;
                }
            }
        }
        if(!mode)
        {
            activeIndex = activeIndexBack;
        }
    }
    else if(text.contains("\n"))
    {
        EjTableBlock *table = isTable(activeIndex);
        if(!table || m_statusMode == EDIT_TEXT)
        {
            res = inputEnter();
        }
        else if(m_statusMode == EDIT_CELL)
        {
            setStatusMode(EDIT_TEXT);
            calcTables();
        }
    }
    else
    {
        if(mode == false)
        {
        }
        else
        {
            bool isInsert = false;
            if(activeIndex > -1)
                cur_txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex));
            if(mode != m_inputSelectMode)
                isInsert = true;
            if(!cur_txt || cur_txt->type != TEXT)
            {
                activeIndex++;
                isInsert = true;
            }
            else if(isInsert && position == cur_txt->text.length())
            {
                activeIndex++;
            }
            if(isInsert)
            {
                doc->lBlocks->insert(activeIndex, new EjTextBlock());
                id_inputSelBlock = activeIndex;
            }
            cur_txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex));

            cur_txt->text = text;
            cur_txt->flag_redraw = true;
            cur_txt->width = 0;
            position = preeditCursor;
        }
    }


    if(activeIndex > -1 && !mode && activeIndex > -1 && doc->lBlocks->at(activeIndex)->type == TEXT)
    {
        while(activeIndex > 0 && (doc->lBlocks->at(activeIndex-1)->type == TEXT))
        {
            activeIndex--;
            cur_txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex));
            cur_txt->text += static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex+1))->text;
            position = cur_txt->text.length();
            cur_txt->flag_redraw = true;
            cur_txt->width = 0;
            delete doc->lBlocks->at(activeIndex+1);
            doc->lBlocks->removeAt(activeIndex+1);
        }
        while(activeIndex < doc->lBlocks->count() - 1 && (doc->lBlocks->at(activeIndex+1)->type == TEXT))
        {
            cur_txt = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex));
            cur_txt->text += static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex+1))->text;
            position = cur_txt->text.length();
            cur_txt->flag_redraw = true;
            cur_txt->width = 0;
            delete doc->lBlocks->at(activeIndex+1);
            doc->lBlocks->removeAt(activeIndex+1);
        }
    }
    if(!mode)
        id_inputSelBlock = activeIndex;

    m_inputSelectMode = mode;
    calcInputMethodParams();
    return res;
}


void EjTextControl::setStatusMode(e_statusMode mode)
{
    EjTableBlock *table;
    EjTextBlock *cur_txtBlock;
    QString str;
    bool bOk;
    EjBlock *curBlock = 0;
    EjCellBlock *curCell = 0;
    int k = activeIndex;
    int row;
    int colum;
    double res, d;
    EjCalculator calculator(doc);

    if(mode != READ_ONLY && !docPrev && doc != nullptr)
    {
        docPrev = new EjDocument();
        doc->copyData(docPrev);
        createPatch();
    }

    table = isTable(activeIndex);

    if(table)
    {
        cellParams(table,activeIndex,row,colum);
        curBlock = doc->lBlocks->at(activeIndex);
        while(doc->lBlocks->at(k)->type != BASECELL && k > table->m_index)
            k--;
        if(k == table->m_index)
            return;
        if(k > -1)
            curCell = (EjCellBlock*)doc->lBlocks->at(k);
    }


    if(m_statusMode == EDIT_CELL && mode != m_statusMode &&  table && curCell)
    {
        //        QLocale locale;
        QString str = curCell->getText();
        if(!str.isEmpty() && str.at(0).toLatin1() == '=' && (curCell->vid == EjCellBlock::CELL_AUTO
                              || curCell->vid == EjCellBlock::CELL_FORMULA))
        {
            curCell->setFormula(str);
        }
        else if(curCell->vid == EjCellBlock::CELL_AUTO
                || curCell->vid == EjCellBlock::CELL_FORMULA)
        {
            curCell->value = getDValue(str,&bOk);
            if(bOk)
                curCell->text = getDText(curCell->value,12);
            else curCell->text.clear();
            curCell->vid = EjCellBlock::CELL_AUTO;
        }
        activeIndex = doc->lBlocks->indexOf(curCell);

        if(colum == table->nColums() - 1 && row > 0 && row < table->nRows() - 1 && table->vid == EjTableBlock::SHOP_LIST)
        {

        }

        position = 0;
        updateTables(doc);
        calcTables();
    }
    else   if(mode == EDIT_CELL && mode != m_statusMode && table && curCell)
    {
        if(table->vid != EjTableBlock::SHOP_LIST)
        {
            QString formula;
            if(curCell->vid == EjCellBlock::CELL_FORMULA )
                formula = curCell->formula();
            if(curCell->vid == EjCellBlock::CELL_AUTO)
                formula = curCell->text;
            if(!formula.isEmpty())
                curCell->setText(formula,this);

        }
        while(activeIndex < doc->lBlocks->count() - 1 && doc->lBlocks->at(activeIndex+1)->type != BASECELL && doc->lBlocks->at(activeIndex+1)->type != END_GROUP)
            activeIndex++;
        if(doc->lBlocks->at(activeIndex)->type == TEXT)
        {
            position = ((EjTextBlock*)doc->lBlocks->at(activeIndex))->text.count();
            //            position = curCell->text.count();
        }
        else
            position = 0;
    }
    m_statusMode = mode;
    calcData();
    calcCursor();
}

void EjTextControl::setSelectMode(e_selectMode mode)
{
    m_selectMode = mode;
    if(mode == NO_SELECTED)
        m_startSelectBlock = m_endSelectBlock = -1;
    else
    {
        m_selectStart = false;
        m_startSelectBlock = m_endSelectBlock = activeIndex;
        m_startSelectPos = m_endSelectPos = position;
        m_startSelectWidth = m_endSelectWidth = position_w;
        updateSelected();

    }

}

QString EjTextControl::getText() const
{
    QString res;
    for(int i = 0; i < doc->lBlocks->count(); i++)
    {
        switch (doc->lBlocks->at(i)->type) {
        case TEXT:
            res += static_cast<EjTextBlock*>(doc->lBlocks->at(i))->text;
            break;
        case SPACE:
            res += ' ';
            break;
        case ENTER:
            res += '\n';
        default:
            break;
        }

    }
    return res;
}

void EjTextControl::setText(const QString &source)
{
    m_startCursor = false;
    QString txt = source;
    QString left;

    int i  = txt.indexOf(' ');
    while(i > -1)
    {
        left = txt.left(i);
        txt = txt.right(txt.size() - i - 1);
        i = txt.indexOf(' ');
        inputText(left);
        inputSpace();
    }
    if(txt == " ")
    {
        inputSpace();
    }
    else
    {
        inputText(txt);
    }
}

void EjTextControl::groupBlocksIndexes()
{
    EjBlock *curBlock;
	for(int i = 0; i < doc->lBlocks->count(); i++)
	{
		curBlock = (*doc->lBlocks)[i];
		if(curBlock->type >= GROUP_BLOCK)
		{
            dynamic_cast<EjGroupBlock*>(curBlock)->m_index = i;
		}
	}
}

void EjTextControl::setFirstTextStyle()
{
	int i = 0;
    EjBlock *cur_block;
	bool bFind = false;
	while(i < doc->lBlocks->count())
	{
		cur_block = doc->lBlocks->at(i);
		if(cur_block->type == NUM_STYLE)
		{
            if(((EjNumStyleBlock *)cur_block)->style)
			{
                if(((EjNumStyleBlock *)cur_block)->style->m_vid == TEXT_STYLE)
				{
					bFind = true;
					break;
				}
			}
			else if(cur_block->type == TEXT)
			{
				break;
			}
		}
		i++;
	}
	if(bFind)
		return;

    EjTextStyle *curTextStyle = getTextStyle(0);
    EjNumStyleBlock *curNumStyle = new EjNumStyleBlock();
	curNumStyle->num = doc->lStyles->indexOf(curTextStyle);
	curNumStyle->style = curTextStyle;

	doc->lBlocks->insert(0,curNumStyle);
	activeIndex++;
	if(m_startSelectBlock > -1)
		m_startSelectBlock++;
	if(m_endSelectBlock > -1)
		m_endSelectBlock++;
	groupBlocksIndexes();
}

void EjTextControl::setTextStyle(EjTextStyle *textStyle, bool force, bool autoClose)
{
    //    EjBlock *curBlock;
    if (doc == nullptr){
        return;
    }
    int index;
    //    int startIndex;
    EjTextStyle *oldTextStyle;
    EjTextStyle *curTextStyle;
    EjNumStyleBlock *curNumStyle;
    //    quint8 int_align = align | EjParagraphStyle::AlignBaseline;
    EjTextStyle *newTextStyle = NULL;
    EjTableBlock *table;
    int type;
    int count;
    int max_count = -1;
    int endIndex;

	if(activeIndex > 0)
	{
		setFirstTextStyle();
	}

    if(m_startSelectBlock == -1 || m_endSelectBlock == -1)
    {
        endIndex = activeIndex;

        {
            if(autoClose)
            {
                index = activeIndex;
                if(position > 0 || activeIndex > startText(activeIndex))
                    index = endText(activeIndex);
                oldTextStyle = getTextStyle(index);
                curNumStyle = new EjNumStyleBlock();
                curNumStyle->num = doc->lStyles->indexOf(oldTextStyle);
                curNumStyle->style = oldTextStyle;
                doc->lBlocks->insert(index,curNumStyle);

                table = isTable(index);
                if(table)
                {
                    table->m_counts++;
                }
                endIndex = index;
            }

            index = activeIndex;

            if(isEndText(index))
            {
                index++;
                activeIndex++;
            }
            else if(activeIndex < endText(activeIndex))
                index = startText(activeIndex);
            if(index == endIndex)
                newTextStyle = dynamic_cast<EjTextStyle*>(getTextStyle(index - 1)->makeCopy());
            else
                newTextStyle = dynamic_cast<EjTextStyle*>(getTextStyle(index)->makeCopy());
            newTextStyle->normalizeStyle(textStyle, force);
            curTextStyle = doc->fromTextStyles(newTextStyle);
            curNumStyle = new EjNumStyleBlock();
            curNumStyle->num = doc->lStyles->indexOf(curTextStyle);
            curNumStyle->style = curTextStyle;

            if (index == -1) index = 0;
            doc->lBlocks->insert(index,curNumStyle);

            table = isTable(index);
            if(table)
            {
                table->m_counts++;
            }
            activeIndex++;
            oldTextStyle = curTextStyle;
            delete newTextStyle;
            newTextStyle = nullptr;

        }
    }
    else
    {
        int pos = m_endSelectPos;
        index = m_endSelectBlock;
        if(splitText(index,pos)) {
            if(index <= activeIndex)
                activeIndex++;
        }

        oldTextStyle = getTextStyle(index);
        curNumStyle = new EjNumStyleBlock();
        curNumStyle->num = doc->lStyles->indexOf(oldTextStyle);
        curNumStyle->style = oldTextStyle;
        doc->lBlocks->insert(index,curNumStyle);
        table = isTable(index);
        if(table)
        {
            table->m_counts++;
        }
        if(index <= activeIndex)
            activeIndex++;
        endIndex = index;

        if(splitText(m_startSelectBlock,m_startSelectPos) )
        {
            if(m_startSelectBlock <= activeIndex)
                activeIndex++;
            if(m_startSelectBlock <= m_endSelectBlock)
                m_endSelectBlock++;
            m_startSelectPos = 0;
            m_startSelectWidth = 0;
        }
        index = m_startSelectBlock;

		while(index > 0)
		{
			if(doc->lBlocks->at(index)->type == TEXT)
				break;
			index--;
		}
		if(index > 0 && index < m_startSelectBlock)
			index++;

        newTextStyle = dynamic_cast<EjTextStyle*>(getTextStyle(index)->makeCopy());
        newTextStyle->normalizeStyle(textStyle, force);
        curTextStyle = doc->fromTextStyles(newTextStyle);
        curNumStyle = new EjNumStyleBlock();
        curNumStyle->num = doc->lStyles->indexOf(curTextStyle);
        curNumStyle->style = curTextStyle;

        doc->lBlocks->insert(index,curNumStyle);

        table = isTable(index);
        if(table)
        {
            table->m_counts++;
        }
        if(index <= activeIndex)
            activeIndex++;
        if(index <= m_endSelectBlock)
            m_endSelectBlock++;
        oldTextStyle = curTextStyle;
        delete newTextStyle;
        newTextStyle = nullptr;
    }

    for(int i = index + 1; i < endIndex; i++)
    {
        if(doc->lBlocks->at(i)->type == NUM_STYLE) {
            curNumStyle = (EjNumStyleBlock*)doc->lBlocks->at(i);
            if(curNumStyle->style && curNumStyle->style->m_vid == TEXT_STYLE)
            {
                curTextStyle = dynamic_cast<EjTextStyle*>(curNumStyle->style);
                if(curTextStyle)
                {
                    bool isSame = false;
                    if(!force)
                    {
                        curTextStyle = dynamic_cast<EjTextStyle*>(curTextStyle->makeCopy());
                        curTextStyle->normalizeStyle(textStyle, false);
                        if(curTextStyle->fullCompare(oldTextStyle) == true)
                        {
                            isSame = true;
                        }
                        else
                        {
                            oldTextStyle = doc->fromTextStyles(curTextStyle);
                            curNumStyle->style = oldTextStyle;
                            curNumStyle->num = doc->lStyles->indexOf(curNumStyle->style);
                        }
                        delete curTextStyle;
                    }
                    if(isSame)
                    {
                        table = isTable(i);
                        if(table)
                        {
                            table->m_counts--;
                        }
                        delete doc->lBlocks->takeAt(i);
                        if(activeIndex >= i)
                            activeIndex--;
                        if(m_endSelectBlock >= i)
                            m_endSelectBlock--;
                        i--;
                        endIndex--;
                    }

                }
            }
        }

    }

    textStyleOptimize();
    createPatch();
}

bool EjTextControl::textStyleOptimize()
{

    bool res = false;
    EjNumStyleBlock *curNumStyle;
    EjTableBlock *table;
    bool textEnabled = false;
    int type;
    int start = doc->lBlocks->count() - 1;
    int end = -1;
    while(end < start)
    {
        end++;
        type = doc->lBlocks->at(end)->type;
        if(type == NUM_STYLE)
        {
            curNumStyle = (EjNumStyleBlock*)doc->lBlocks->at(end);
            if(curNumStyle->style && curNumStyle->style->m_vid == TEXT_STYLE)
                break;
        }
    }

    for(int i = start; i > end; i--)
    {
        type = doc->lBlocks->at(i)->type;
        if(type == NUM_STYLE && !textEnabled && i != activeIndex - 1)
        {
            curNumStyle = (EjNumStyleBlock*)doc->lBlocks->at(i);
            if(curNumStyle->style && curNumStyle->style->m_vid == TEXT_STYLE)
            {
                table = isTable(i);
                if(table)
                {
                    table->m_counts--;
                }
                delete doc->lBlocks->takeAt(i);
                if(activeIndex >= i)
                    activeIndex--;
                if(m_endSelectBlock >= i)
                    m_endSelectBlock--;
				if(m_startSelectBlock > i)
                    m_startSelectBlock--;
                i--;
                res = true;
            }
        }
        else if(type == TEXT)
        {
            textEnabled = true;
           // oldTextStyle = nullptr;
        }
        else if(type == NUM_STYLE)
        {
            curNumStyle = (EjNumStyleBlock*)doc->lBlocks->at(i);
            if(curNumStyle->style && curNumStyle->style->m_vid == TEXT_STYLE)
            {
                textEnabled = false;
            }
        }

    }
	groupBlocksIndexes();
    return res;
}

void EjTextControl::setParagraphTextStyle(EjTextStyle *textStyle)
{
    int index = activeIndex;
    if(isTable(index))
    {
        while(index > 0)
        {
            if(doc->lBlocks->at(index)->type == BASECELL || doc->lBlocks->at(index)->type == EXT_TABLE )
                break;
            index--;
        }
        m_startSelectBlock = index;
        index = activeIndex;
        if(isTable(index))
        {
            while(index < doc->lBlocks->count() - 1)
            {
                if(doc->lBlocks->at(index)->type == BASECELL || doc->lBlocks->at(index)->type == END_GROUP)
                    break;
                index++;
            }
        }
        else
        {
            while(index < doc->lBlocks->count())
            {
                if(doc->lBlocks->at(index)->type == ENTER)
                    break;
                index++;
            }

        }
        m_endSelectBlock = index;
    }
    else
    {
        while(index > 0)
        {
            if(doc->lBlocks->at(index)->type == ENTER)
                break;
            index--;

        }
        m_startSelectBlock = index;
        index = activeIndex;
        if(isTable(index))
        {
            while(index < doc->lBlocks->count() - 1)
            {
                if(doc->lBlocks->at(index)->type == BASECELL || doc->lBlocks->at(index)->type == END_GROUP)
                    break;
                index++;
            }
        }
        else
        {
            while(index < doc->lBlocks->count())
            {
                if(doc->lBlocks->at(index)->type == ENTER)
                    break;
                index++;
            }

        }
        m_endSelectBlock = index;
    }
    setTextStyle(textStyle);
}

void EjTextControl::setParagraphStyle(EjParagraphStyle *paragraphStyle)
{
    int index;
    int startIndex;
    if(activeIndex > doc->lBlocks->count() - 1)
        activeIndex = doc->lBlocks->count() - 1;
    EjBaseStyle *curStylePrg = NULL;
    EjBaseStyle *oldStylePrg = getParagraphStyle(activeIndex);
    EjNumStyleBlock *curNumStyle;
    //    quint8 int_align = align | EjParagraphStyle::AlignBaseline;
    EjBaseStyle *curStyle = NULL;
    EjTableBlock *tableStart, *tableEnd, *table;

    if(activeIndex < 0)
        activeIndex = 0;

    index = startIndex = activeIndex;
    tableStart = isTable(m_startSelectBlock);
    tableEnd = isTable(m_endSelectBlock);
    while(index < doc->lBlocks->count() )
    {

        if(doc->lBlocks->at(index)->type == NUM_STYLE && index != startIndex)
        {
            curNumStyle = (EjNumStyleBlock*)doc->lBlocks->at(index);
            if(curNumStyle->style && curNumStyle->style->m_vid == PARAGRAPH_STYLE)
            {
                oldStylePrg = curNumStyle->style;
                table = isTable(index);
                if(table)
                {
                    table->m_counts--;
                }
                delete doc->lBlocks->takeAt(index);
                index--;
                if(index < 0) index = 0;
            }
        }
        if(index >= m_endSelectBlock || m_endSelectBlock < 0)
        {
            table = isTable(index);

            if(table && (doc->lBlocks->at(index)->type == e_typeBlocks::END_GROUP ||
                         doc->lBlocks->at(index)->type == e_typeBlocks::BASECELL))
            {
                index++;
                break;
            }
            else if(doc->lBlocks->at(index)->type == e_typeBlocks::ENTER)
            {
                index++;
                break;
            }
        }
        doc->lBlocks->at(index)->flag_redraw = true;
        index++;
    }
    if(oldStylePrg && index < doc->lBlocks->count() - 1)
    {
        curNumStyle = new EjNumStyleBlock();
        curNumStyle->num = doc->lStyles->indexOf(oldStylePrg);
        curNumStyle->style = oldStylePrg;
        table = isTable(index);
        doc->lBlocks->insert(index,curNumStyle);
        if(table)
        {
            table->m_counts++;
        }
    }
    index = activeIndex;
    while(index > -1 && doc->lBlocks->count() > 0)
    {
        if(doc->lBlocks->at(index)->type == NUM_STYLE) {
            curNumStyle = (EjNumStyleBlock*)doc->lBlocks->at(index);
            if(curNumStyle->style && curNumStyle->style->m_vid == PARAGRAPH_STYLE)
            {
                table = isTable(index);
                if(table)
                {
                    table->m_counts--;
                }
                delete doc->lBlocks->takeAt(index);
                activeIndex--;
                if(index < 0) index = 0;
            }
        }
        if(doc->lBlocks->count() == 0)
            break;
        if(index <= m_startSelectBlock || m_startSelectBlock < 0)
        {
            table = isTable(index);

            if(table && (doc->lBlocks->at(index)->type == e_typeBlocks::END_GROUP ||
                         doc->lBlocks->at(index)->type == e_typeBlocks::BASECELL))
            {
                break;
            }
            else if(index < doc->lBlocks->count() && doc->lBlocks->at(index)->type == e_typeBlocks::ENTER)
            {
                break;
            }
        }
        doc->lBlocks->at(index)->flag_redraw = true;
        index--;
    }
    if(index > 0 && index < doc->lBlocks->count()
            && (doc->lBlocks->at(index)->isProperty()
                || doc->lBlocks->at(index)->type == e_typeBlocks::ENTER
                || doc->lBlocks->at(index)->type == e_typeBlocks::BASECELL ) )
        index++;

    if (index < 0) index = 0;
    curStylePrg = doc->fromParagraphStyles(paragraphStyle);
    curNumStyle = new EjNumStyleBlock();
    curNumStyle->num = doc->lStyles->indexOf(curStylePrg);
    curNumStyle->style = curStylePrg;
    table = isTable(index);
    doc->lBlocks->insert(index,curNumStyle);
    if(table)
    {
        table->m_counts++;
    }
    activeIndex++;

    createPatch();
}



void EjTextControl::clear()
{
    if (doc == nullptr){
        return;
    }
    doc->clear();
    activeIndex = doc->lBlocks->count();
    position = 0;
    m_height = 0;
    emit controlHeightChanged();
}

void EjTextControl::inputTable(int rows, int colums)
{

}

void EjTextControl::addTableString()
{

}

void EjTextControl::addTableColum()
{


}

void EjTextControl::delTableString(QList<EjBlock*> *l_blocks, int &active_block)
{
    EjCellBlock *curCell;
    EjTableBlock *curTable = 0;
    TableFragment *curFragment;
    EjSizeProp *sizeProp;
    int row = 0;
    int colum = 0;
    int frgs_row, frge_row;
    int d = 0;
    //    int start;
    int index;

    if(active_block < 0 || active_block > l_blocks->count() - 2)
        return;

    while(l_blocks->at(active_block)->type != BASECELL)
        active_block--;

    curCell = (EjCellBlock*)l_blocks->at(active_block);
    curTable = ((EjTableBlock*)(curCell->parent));
    //    else return;
    //    curTable = isTable(activeBlock);

    if(curTable)
    {
        if(curTable->vid == EjTableBlock::SHOP_LIST)
            d = 1;
        //        start = l_blocks->indexOf(curTable) + 1;
        //       row = (active_block-start) / curTable->nColums();
        cellParams(curTable,active_block,row,colum, l_blocks);

        if(d && curTable->nRows() == 2)
        {
            while(curTable->endBlock() > curTable->startCell())
            {
                delete l_blocks->takeAt(curTable->startCell() );
                if(l_blocks == doc->lBlocks)
                    updateFragments(curTable->startCell(),false);
                else
                    curTable->m_counts--;
            }
            if(l_blocks == doc->lBlocks)
                updateFragments(curTable->startCell(),false);
            delete l_blocks->takeAt(curTable->startCell());
            curTable = 0;
            if(l_blocks == doc->lBlocks)
            {
                updateTables(doc);
                calcTables();
            }
            return;
        }

        if(d && (row == 0 || row > curTable->nRows() - 2))
            return;
        updateFormulas(row, 0, true, false, false, curTable);
        index = tableCellIndex(curTable,row,0, l_blocks);
        for(int i = 0; i < curTable->nColums(); i++)
        {
            delete l_blocks->takeAt(index);
            if(l_blocks == doc->lBlocks)
                updateFragments(index,false);
            else
                curTable->m_counts--;

            while(l_blocks->at(index)->type != BASECELL)
            {
                delete l_blocks->takeAt(index);
                if(l_blocks == doc->lBlocks)
                    updateFragments(index,false);
                else
                    curTable->m_counts--;
            }
            //            index = start+row*curTable->nColums();
            //            curCell = static_cast<BaseCellBlock*>(l_blocks->takeAt(index));
            //            delete curCell;
        }
        //        active_block -= curTable->nColums();
        active_block = index;
        if(active_block < 0)
        {
            active_block = -1;
            m_startCursor = true;
        }
        sizeProp = curTable->lRows.takeLast();
        delete sizeProp;
        if(d && l_blocks == doc->lBlocks)
        {
            updateShopList(curTable);
        }

        if(curTable->lColums.isEmpty() || curTable->lRows.isEmpty())
        {
            if(l_blocks->at(curTable->startCell())->type == BASECELL)
            {
                delete l_blocks->takeAt(curTable->startCell());
                if(l_blocks == doc->lBlocks)
                {
                    updateFragments(curTable->startCell(),false);
                }
                l_blocks->takeAt(curTable->startCell());
            }

            if(l_blocks == doc->lBlocks)
            {
                activeIndex = curTable->m_index - 1;
                if(activeIndex < 0)
                {
                    activeIndex = -1;
                    m_startCursor = true;
                }
                updateTables(doc);
            }
            delete curTable;
        }

        if(l_blocks == doc->lBlocks)
            calcTables();


    }

}

void EjTextControl::delTableColum(QList<EjBlock *> *l_blocks, int &active_block)
{
    EjCellBlock *curCell;
    EjTableBlock *curTable = 0;
    TableFragment *curFragment;
    EjSizeProp *sizeProp;
    EjTableBlock::ColumProp *columProp;
    int row = 0;
    int colum = 0;
    int frgs_colum, frge_colum;
    int d = 0;
    int index;

    if(active_block < 0 || active_block > l_blocks->count() - 2)
        return;

    while(l_blocks->at(active_block)->type != BASECELL)
        active_block--;

    curCell = (EjCellBlock*)l_blocks->at(active_block);
    curTable = ((EjTableBlock*)(curCell->parent));

    if(curTable)
    {
        if(curTable->vid == EjTableBlock::SHOP_LIST)
            return;

        cellParams(curTable,active_block,row,colum, l_blocks);

        active_block -= row;
        updateFormulas(colum, 0, false, false, false, curTable);

        for(int i = 0; i < curTable->nRows(); i++)
        {
            index = tableCellIndex(curTable,i,colum - i, l_blocks);
            delete l_blocks->takeAt(index);
            if(l_blocks == doc->lBlocks)
                updateFragments(index,false);
            else
                curTable->m_counts--;

            while(l_blocks->at(index)->type != BASECELL)
            {
                delete l_blocks->takeAt(index);
                if(l_blocks == doc->lBlocks)
                    updateFragments(index,false);
                else
                    curTable->m_counts--;
            }

        }
        columProp = curTable->lColums.takeLast();
        delete columProp;

        if(curTable->lColums.isEmpty() || curTable->lRows.isEmpty())
        {
            doc->lTables->removeOne(curTable);
            l_blocks->removeOne(curTable);
            //            active_block = start - 2;
            active_block = curTable->m_index - 1;
            delete curTable;
            if(active_block < 0)
            {
                active_block = -1;
                m_startCursor = true;
            }
            if(l_blocks == doc->lBlocks)
                updateTables(doc);
        }

        if(l_blocks == doc->lBlocks)
            calcTables();


    }

}

void EjTextControl::moveTableString(bool isUp)
{


}

void EjTextControl::moveTableColum(bool isLeft)
{


}

void EjTextControl::updateFormulas(int fromVal, int toVal, bool isRow, bool isAdd,bool isMove, EjTableBlock *curTable)
{
    return;
    QStringList list;
    EjTableBlock *curTable2;
    EjCellBlock *curCell;
    //    int start;
    int numRow;
    int numColum;
    int numTable;
    int index;
    bool isEdit;
    QString txt;
    bool isFormula;
    EjCalculator calculator(doc);
    for(int i_table = 0; i_table < doc->lTables->count(); i_table++)
    {
        curTable2 = doc->lTables->at(i_table);
        //        start = doc->lBlocks->indexOf(curTable2) + 1;
        for(int i_row = 0; i_row < curTable2->nRows(); i_row++)
        {
            for(int i_colum = 0; i_colum < curTable2->nColums(); i_colum++)
            {
                //                index = start + i_row*curTable2->nColums() + i_colum;
                index = tableCellIndex(curTable2,i_row,i_colum,doc->lBlocks);
                if(index < 0)
                    continue;
                curCell = static_cast<EjCellBlock*>(doc->lBlocks->at(index));
                if(curCell->vid == EjCellBlock::CELL_FORMULA)
                {
                    list.clear();
                    txt.clear();
                    QString formula = curCell->formula();
                    for(int i = 0; i < formula.count(); i++)
                    {

                        if(calculator.is_split(formula[i].toLatin1()) )
                        {
                            if(!txt.isEmpty())
                                list << txt;
                            txt = formula[i];
                            list << txt;
                            txt.clear();
                        }
                        else txt += formula[i];
                    }
                    if(!txt.isEmpty())
                        list << txt;
                    for(int i2 = 0; i2 < list.count(); i2++)
                    {
                        txt = list.at(i2);
                        isEdit = false;
                        if(txt == "SUMM" || txt == "MAX" || txt == "MIN")
                            continue;
                        isFormula = tableLinkParams(txt, curTable, curTable2, numTable, numRow, numColum);

                        if(isFormula && numTable == curTable->key)
                        {
                            if(isMove)
                            {
                                if(isRow)
                                {
                                    if(numRow == fromVal+1)
                                    {
                                        numRow = toVal+1;
                                        isEdit = true;
                                    }
                                    else if(numRow == toVal+1)
                                    {
                                        numRow = fromVal+1;
                                        isEdit = true;
                                    }

                                }
                                else
                                {
                                    if(numColum == fromVal)
                                    {
                                        numColum = toVal;
                                        isEdit = true;
                                    }
                                    else if(numColum == toVal)
                                    {
                                        numColum = fromVal;
                                        isEdit = true;
                                    }
                                }

                            }
                            else if(isRow)
                            {
                                if(numRow > fromVal)
                                {

                                    if(isAdd) numRow++;
                                    else
                                        numRow--;
                                    if(numRow < 0)
                                        numRow = 0;
                                    isEdit = true;
                                }
                            }
                            else
                            {
                                if(numColum > fromVal)
                                {

                                    if(isAdd) numColum++;
                                    else
                                        numColum--;
                                    if(numColum < 0)
                                        numColum = 0;
                                    isEdit = true;
                                }
                            }
                        }
                        if(isEdit)
                        {
                            txt = "";
                            if(curTable2 != curTable)
                                txt += QString::number(numTable);
                            txt += (QString::number(numColum) + 'A');
                            txt += QString::number(numRow);
                            list[i2] = txt;
                        }
                    }
                    txt = "";
                    for(int i = 0; i < list.count(); i++)
                    {
                        txt += list.at(i);
                    }
                    curCell->setFormula(txt);
                }

            }
        }

    }

}

bool EjTextControl::tableLinkParams(QString txt, EjTableBlock *curTable, EjTableBlock *curTable2, int &numTable, int &numRow, int &numColum)
{
    bool isFormula = true;

    if(txt.count() > 1 && curTable == curTable2 && is_value(txt.toLatin1().at(0)))
    {
        numTable = curTable->key;
        numRow = txt.right(txt.count()-1).toInt();
        numColum = txt.toLatin1().at(0) - 'A';
    }
    else if(txt.count() > 2 && is_value(txt.toLatin1().at(1)))
    {
        numTable = txt.left(1).toInt();
        numRow = txt.right(txt.count()-2).toInt();
        numColum = txt.toLatin1().at(1) - 'A';
    }
    else if(txt.count() > 3 && is_value(txt.toLatin1().at(2)))
    {
        numTable = txt.left(2).toInt();
        numRow = txt.right(txt.count()-3).toInt();
        numColum = txt.toLatin1().at(2) - 'A';
    }
    else
    {
        isFormula = false;
    }

    return isFormula;

}




void EjTextControl::updateShopList(EjTableBlock *curTableBlock)
{
    int index = curTableBlock->startCell();
    EjCellBlock *curCell;


    while(doc->lBlocks->at(index)->type != BASECELL)
        index++;
    index--;
    for(int row = 0; row < curTableBlock->nRows() - 1; row++)
    {
        index++;

        if(row > 0)
        {
            curCell = (EjCellBlock*)doc->lBlocks->at(index);
            curCell->value = row;
            curCell->vid = EjCellBlock::CELL_NUMBER;
        }
        index++;
        while(doc->lBlocks->at(index+1)->type != BASECELL)
            index++;
        if(row > 0)
        {
            curCell = (EjCellBlock*)doc->lBlocks->at(index);
            curCell->vid = EjCellBlock::CELL_CHECK;
        }
        index++;
        while(doc->lBlocks->at(index+1)->type != BASECELL)
            index++;
        index++;
        while(doc->lBlocks->at(index+1)->type != BASECELL)
            index++;
        if(row > 0)
        {
            curCell = (EjCellBlock*)doc->lBlocks->at(index);
            if(curCell->vid != EjCellBlock::CELL_NUMBER)
            {
                curCell->vid = EjCellBlock::CELL_NUMBER;
                curCell->setText("0");
                curCell->value = 0;
            }
        }
        index++;
        while(doc->lBlocks->at(index+1)->type != BASECELL)
            index++;
        if(row > 0)
        {
            curCell = (EjCellBlock*)doc->lBlocks->at(index);
            if(curCell->vid != EjCellBlock::CELL_NUMBER)
            {
                curCell->vid = EjCellBlock::CELL_NUMBER;
                curCell->setText("1");
                curCell->value = 1;
            }
        }

        index++;
        while(doc->lBlocks->at(index+1)->type != BASECELL)
            index++;
        if(row > 0)
        {
            curCell = (EjCellBlock*)doc->lBlocks->at(index);
            curCell->setFormula(QString("=D%1*E%1").arg(row+1));
            curCell->vid = EjCellBlock::CELL_FORMULA;
        }

    }

    index = tableCellIndex(curTableBlock,curTableBlock->nRows()-1,curTableBlock->nColums()-1, doc->lBlocks);
    curCell = NULL;
    if(index > -1)
        curCell = (EjCellBlock*)doc->lBlocks->at(index);
    if(curCell)
    {
        curCell->setFormula(QString("=SUMM(F2:F%1)").arg(curTableBlock->nRows()-1));
        curCell->vid = EjCellBlock::CELL_FORMULA;
    }

}

void EjTextControl::cellParams(EjTableBlock *EjTableBlock, int index, int &row, int &colum, QList<EjBlock *> *l_blocks)
{
    int table_index = -1;
    if(!l_blocks)
        l_blocks = doc->lBlocks;
    for(int i = EjTableBlock->startCell(); i <= EjTableBlock->endBlock(); i++ )
    {
        if(l_blocks->at(i)->type == BASECELL)
            table_index++;
        if(i == index)
        {
            if(EjTableBlock->nColums() > 0)
                row = (table_index) / EjTableBlock->nColums();
            colum = table_index - row*EjTableBlock->nColums();
            break;
        }
    }


}

void EjTextControl::cellParams(EjTableBlock *tableBlock, EjBlock *block, int &row, int &colum, QList<EjBlock *> *l_blocks)
{
    int table_index = -1;
    if(!l_blocks)
        l_blocks = doc->lBlocks;
    EjBlock *curBlock;
    for(int i = tableBlock->startCell(); i <= tableBlock->endBlock(); i++ )
    {
        curBlock = l_blocks->at(i);
        if(curBlock->type == BASECELL)
            table_index++;
        if(curBlock == block)
        {
            if(tableBlock->nColums() > 0)
                row = (table_index) / tableBlock->nColums();
            colum = table_index - row * tableBlock->nColums();
            break;
        }
    }
}


int EjTextControl::tableCellIndex(EjTableBlock *tableBlock, int row, int colum, QList<EjBlock *> *l_blocks)
{
    int table_index = -1;
    //    int cur_row, cur_colum;
    int res = -1;
    int index = row * tableBlock->nColums() + colum;

    if(!l_blocks)
        return res;
//        l_blocks = doc->lBlocks;
    int i = tableBlock->startCell();
    while(l_blocks->at(i)->type != BASECELL)
        i++;
    for(i; i <= tableBlock->endBlock(); i++ )
    {
        if(l_blocks->at(i)->type == BASECELL)
        {
            table_index++;
            //            cur_row = 0;
            //            if(tableBlock->nColums() > 0)
            //                cur_row = (table_index) / tableBlock->nColums();
            //            cur_colum = table_index - cur_row*tableBlock->nColums();
            //            if(row == cur_row && colum == cur_colum)
            if(table_index >= index)
            {
                res = i;
                break;
            }
        }
    }
    return res;

}

void EjTextControl::updateTables(EjDocument *fdoc)
{
    EjCalcParams calcParams;
//    calcParams.control = this;
    EjTableBlock *curTable = 0;
    EjPropIntBlock *propInt;
    EjBlock *curBlock;
    EjCellBlock *curCell;
    QString text;
    bool bOk;

//    if(doc->lStrings->isEmpty())
//        doc->lStrings->append(new EjString());
    fdoc->lTables->clear();
    fdoc->lLabels_bak->clear();
    fdoc->lLabels->clear();
    for(int i = 0; i < fdoc->lBlocks->count(); i++)
    {
        curBlock = fdoc->lBlocks->at(i);
        if(curBlock->type == EXT_TABLE)
        {
//            calcParams.textStyle = getTextStyle(i);
//            calcParams.paragraphStyle = getParagraphStyle(i);

            curTable = dynamic_cast<EjTableBlock*>(curBlock);
            curTable->m_index = i;
            curTable->m_doc = fdoc;
            if(i < fdoc->lBlocks->count() - 1)
                i++;
            else
                break;
//            curTable->calcBlock(i,&calcParams);
            fdoc->lTables->append(curTable);
            while(i < fdoc->lBlocks->count() - 1 && curBlock->type != BASECELL && curBlock->type != END_GROUP) {
                curBlock = fdoc->lBlocks->at(i);
                curBlock->parent = curTable;
                if(curBlock->type == PROP_INT) {
                    propInt = dynamic_cast<EjPropIntBlock*>(curBlock);
                    if(propInt->num == EjTableBlock::TBL_ROWS)
                    {
                        while(curTable->lRows.count() > propInt->value)
                            delete curTable->lRows.takeLast();
                        while(curTable->lRows.count() < propInt->value)
                            curTable->lRows.append(new EjSizeProp());
                    }
                    if(propInt->num == EjTableBlock::TBL_COLUMS)
                    {
                        while(curTable->lColums.count() > propInt->value)
                            delete curTable->lColums.takeLast();
                        while(curTable->lColums.count() < propInt->value)
                            curTable->lColums.append(new EjTableBlock::ColumProp());
                    }
                }
                if(curBlock->type == PROP_PNT) {
                    EjPropPntBlock *propPnt = dynamic_cast<EjPropPntBlock*>(curBlock);
                    if(propPnt && propPnt->y_value < curTable->lColums.count())
                    {
                        if(propPnt->num == EjTableBlock::TBL_COLUM_MAX_WIDTH)
                            curTable->lColums.at(propPnt->y_value)->sizeProp.max = propPnt->x_value;
                        if(propPnt->num == EjTableBlock::TBL_COLUM_MIN_WIDTH)
                            curTable->lColums.at(propPnt->y_value)->sizeProp.min = propPnt->x_value;
                    }
//                    if(propPnt && propPnt->num == EjTableBlock::TBL_COLUM_MAX_WIDTH && propPnt->y < curTable->lColums.count())
//                    {
//                        curTable->lColums.at(propPnt->y)->sizeProp.max = static_cast<quint16>(propPnt->x);
//                    }
                }

                i++;
            }
            i--;
            curTable->deltaProps = i - curTable->m_index;
            curBlock = fdoc->lBlocks->at(i);
            while(i < fdoc->lBlocks->count() - 1 && curBlock->type != END_GROUP) {
                curBlock = fdoc->lBlocks->at(i);
                curBlock->parent = curTable;
                if(curBlock->type >= GROUP_BLOCK)
                {
                    EjGroupBlock *groupBlock = dynamic_cast<EjGroupBlock*>(curBlock);
                    if(groupBlock)
                    {
                        groupBlock->calcLenght(i,fdoc->lBlocks);
                        i += groupBlock->m_counts;
                    }
                }
                if(curBlock->type == BASECELL)
                {
                    curCell = dynamic_cast<EjCellBlock*>(curBlock);
                    if(curCell->vid == EjCellBlock::CELL_AUTO || curCell->vid == EjCellBlock::CELL_NUMBER)
                    {
                        if(!curCell->text.isEmpty() && curCell->text.at(0).toLatin1() != '=')
                            curCell->value = getDValue(curCell->text, &bOk);
                        else if(curCell->vid == EjCellBlock::CELL_AUTO)
                        {
                            text = curCell->getText();
                            curCell->value = getDValue(text, &bOk);
                        }
                    }
                }
                i++;
            }
            curTable->m_counts = i - curTable->m_index;
            if(curTable->m_counts < 0)
                curTable->m_counts = 0;
        }
        if(fdoc->lBlocks->at(i)->type == EXT_LARGETEXT_BAK)
        {
            fdoc->lLabels_bak->append((LargeTextBlock*)fdoc->lBlocks->at(i));
        }
        if(fdoc->lBlocks->at(i)->type == EXT_LABEL)
        {
            LabelBlock *label = (LabelBlock*)fdoc->lBlocks->at(i);
            fdoc->lLabels->append(label);
        }
    }
//    doc->lTables->clear();
//    doc->lFragments.clear();
//    EjFragmentBlock *curFragment;
//    EjTableBlock *curTable = 0;
//    BaseCellBlock *curCell;
//    int startTable;
//    for(int i = 0; i < doc->lBlocks->count(); i++)
//    {
//        if(doc->lBlocks->at(i)->type == FRAGMENT)
//        {
//            curFragment = (EjFragmentBlock*)doc->lBlocks->at(i);
//            curFragment->startBlock = i;
//            curFragment->endBlock = i + curFragment->countBlocks;
//            doc->lFragments.append(curFragment);
//        }
//        else if(doc->lBlocks->at(i)->type == EXT_TABLE)
//        {
//            if(i==0 || (i>0 && doc->lBlocks->at(i-1)->type != ENTER) || (i>1 && doc->lBlocks->at(i-2)->type == BASECELL) )
//            {
//                doc->lBlocks->insert(i,new EjBlock(ENTER));
//                updateFragments(i,true);
//                i++;
//            }
//            curTable = (EjTableBlock*)doc->lBlocks->at(i);
//            doc->lTables->append(curTable);
////            startTable = i;
////            curTable->startBlock = i;
//            while(doc->lBlocks->at(i+1)->type != BASECELL)
//                i++;
//            for(int row = 0; row < curTable->nRows(); row++)
//            {
//                for(int colum = 0; colum < curTable->nColums(); colum++)
//                {
//                    i++;
////                    if(doc->lBlocks->at(i)->type == FRAGMENT)
////                    {
////                        curFragment = (EjFragmentBlock*)doc->lBlocks->at(i);
////                        curFragment->startBlock = i;
////                        curFragment->endBlock = i + curFragment->countBlocks;
////                        doc->lFragments.append(curFragment);
////                    }
////                    else
//                    if(doc->lBlocks->at(i)->type == BASECELL)
//                    {
//                        curCell = (BaseCellBlock*)doc->lBlocks->at(i);
//                        curCell->parent = curTable;
//                        if(curCell->vid == BaseCellBlock::NUMBER)
//                            curCell->text = getDText(curCell->value, curCell->parent->accuracy);

//                        if(i > doc->lBlocks->count() - 2 || doc->lBlocks->at(i+1)->type == EXT_TABLE)
//                        {
//                            curCell = new BaseCellBlock();
//                            curCell->vid = BaseCellBlock::ENDTABLE;
//                            doc->lBlocks->insert(i + 1, curCell);
//                            updateFragments(i + 1,true);
//                        }
//                        while(doc->lBlocks->at(i + 1)->type != BASECELL)
//                        {
//                            i++;
////                            if(doc->lBlocks->at(i)->type == FRAGMENT)
////                            {
////                                curFragment = (EjFragmentBlock*)doc->lBlocks->at(i);
////                                curFragment->startBlock = i;
////                                curFragment->endBlock = i + curFragment->countBlocks;
////                                doc->lFragments.append(curFragment);
////                            }
////                            else
////                            if(i > doc->lBlocks->count() - 2 || doc->lBlocks->at(i + 1)->type == EXT_TABLE)
////                            {
////                                curCell = new BaseCellBlock();
////                                curCell->vid = BaseCellBlock::ENDTABLE;
////                                doc->lBlocks->insert(i + 1, curCell);
//////                                updateFragments(i+1,true);
////                                break;
////                            }
//                        }
//                    }
//                }
//            }
//            i++;
//            if(doc->lBlocks->at(i)->type == BASECELL)
//            {
//                curCell = (BaseCellBlock*)doc->lBlocks->at(i);
//                curCell->parent = curTable;
//            }
////            curTable->endBlock() = i;
////            curTable->countBlocks = curTable->endBlock - curTable->startBlock;
//            //            if(i < doc->lBlocks->count() && doc->lBlocks->at(i)->type != BASECELL)
//            //            {
//            //                curCell = new BaseCellBlock();
//            //                curCell->vid = BaseCellBlock::ENDTABLE;
//            //                doc->lBlocks->insert(i, curCell);
//            //                updateFragments(i,true);
//            //            }
//            if(i >= doc->lBlocks->count() - 1 || (i < doc->lBlocks->count()-1 && doc->lBlocks->at(i+1)->type != ENTER))
//            {
//                doc->lBlocks->insert(i+1,new EjBlock(ENTER));
//                updateFragments(i+1,true);
//            }
//            if(curTable->vid == EjTableBlock::SHOPLIST)
//                updateShopList(curTable);
//        }
//        //        if(doc->lBlocks->at(i)->type == BASECELL)
//        //        {
//        //            ((BaseCellBlock*)doc->lBlocks->at(i))->parent = curTable;
//        //        }

//    }
//    //    for(int i = 0; i < doc->lTables->count(); i++)
//    //    {
//    //        lastTable = (EjTableBlock*)doc->lBlocks->at(i);
//    //        if(lastTable->vid == EjTableBlock::SHOPLIST)

//    //    }
}

//void EjTextControl::updateFragments(int index, bool isAdd)
//{
//    foreach(EjTableBlock *curTable, *doc->lTables)
//    {
//        if(index > curTable->startBlock && index <= curTable->endBlock)
//        {
//            if(!isAdd)
//            {
//                curTable->endBlock--;
//                curTable->countBlocks--;
//            }
//            else
//            {
//                curTable->endBlock++;
//                curTable->countBlocks++;
//            }

//        }
//        else if(index <= curTable->startBlock)
//        {
//            if(!isAdd)
//            {
//                if(index < curTable->startBlock)
//                    curTable->startBlock--;
//                curTable->endBlock--;
//            }
//            else
//            {
//                curTable->endBlock++;
//                curTable->startBlock++;

//            }
//        }
//    }
//    updateFragments(index,isAdd);


//}

EjTableBlock *EjTextControl::isTable(int index)
{
    EjTableBlock *res = 0;
    //    bool res = false;
    if (doc == nullptr){
        return nullptr;
    }
    foreach(EjTableBlock *curTable, *doc->lTables)
    {
        if(index >= curTable->m_index && index <= curTable->endBlock())
        {
            res = curTable;
            break;
        }
    }
    return res;
}

void EjTextControl::checkFormula()
{
    if(activeIndex < 0 || doc->lBlocks->at(activeIndex)->type != BASECELL)
        return;
    EjCellBlock *curCell = dynamic_cast<EjCellBlock*>(doc->lBlocks->at(activeIndex));
    QString text = curCell->getText();
    if(text.isEmpty())
    {
        text = "=";
        position = 1;
    }
    else if(text[0] != '=')
    {
        text.insert(0,"=");
        position++;
    }
    curCell->setText(text);
}

bool EjTextControl::isNumber()
{
    if(activeIndex < 0 || doc->lBlocks->at(activeIndex)->type != BASECELL)
        return false;
    EjCellBlock *curBlock = (EjCellBlock*)doc->lBlocks->at(activeIndex);
    if(((EjTableBlock*)(curBlock->parent))->vid == EjTableBlock::SHOP_LIST)
    {
        EjTableBlock *curTable = (EjTableBlock*)curBlock->parent;
        //        int index = doc->lBlocks->indexOf(curTable) + 1;
        int row;   // = (activeBlock - index) / curTable->nColums();
        int colum; // = activeBlock - index - row * curTable->nColums();
        cellParams(curTable,activeIndex,row,colum);
        if(colum > 2)
            return true;
    }
    return false;
}

bool EjTextControl::isActiveText()
{
    bool res = false;
    if(activeIndex >= 0 && activeIndex < doc->lBlocks->count() && doc->lBlocks->at(activeIndex)->type == TEXT)
        res = true;
    return res;
}

bool EjTextControl::isCellEditFormula()
{
    if(activeIndex < 0 || doc->lBlocks->at(activeIndex)->type != BASECELL)
        return false;
    EjCellBlock *curBlock = (EjCellBlock*)doc->lBlocks->at(activeIndex);
    if(((EjTableBlock*)(curBlock->parent))->vid != EjTableBlock::SHOP_LIST && curBlock->vid == EjCellBlock::CELL_FORMULA)
        return true;
    return false;

}

bool EjTextControl::isCellSelected(int index)
{
    bool res = false;
    int row_index, colum_index;
    int row_start, colum_start;
    int row_end, colum_end;
    int n;
    EjTableBlock *startTable;
    EjTableBlock *endTable;
    startTable = isTable(m_startSelectBlock);
    endTable = isTable(m_endSelectBlock);
    //        if(doc->lBlocks->at(m_startSelectBlock)->type == BASECELL && doc->lBlocks->at(m_endSelectBlock)->type == BASECELL)
    if(startTable && endTable && startTable == endTable)
    {
        //            BaseCellBlock *curBlockStart;
        //            BaseCellBlock *curBlockEnd;
        //            curBlockStart = (BaseCellBlock*)doc->lBlocks->at(m_startSelectBlock);
        //            curBlockEnd = (BaseCellBlock*)doc->lBlocks->at(m_endSelectBlock);
        //            if(curBlockStart->parent == curBlockEnd->parent)
        //            {
        //                getBaseCellParams(index, row_index, colum_index);
        //                getBaseCellParams(m_startSelectBlock, row_start, colum_start);
        //                getBaseCellParams(m_endSelectBlock, row_end, colum_end);
        cellParams(startTable, index, row_index, colum_index);
        cellParams(startTable, m_startSelectBlock, row_start, colum_start);
        cellParams(startTable, m_endSelectBlock, row_end, colum_end);
        if(row_start > row_end)
        {
            n = row_start;
            row_start = row_end;
            row_end = n;
        }
        if(colum_start > colum_end)
        {
            n = colum_start;
            colum_start = colum_end;
            colum_end = n;
        }
        if( row_index >= row_start && row_index <= row_end
                && colum_index >= colum_start && colum_index <= colum_end)
            res = true;
        //            }
        //            else
        //                res = true;
    }
    else  if(index >= m_startSelectBlock && index <= m_endSelectBlock)
        res = true;
    //        res = true;
    return res;
}

bool EjTextControl::getBaseCellParams(int index, int &row, int &colum)
{
    bool res = false;
    EjTableBlock *curTable = isTable(index);
    if(curTable)
    {
        cellParams(curTable,index,row,colum);
        res = true;
    }

    //    if(index < 0 || !(doc->lBlocks->at(index)->type == BASECELL) )
    //            return false;
    //    BaseCellBlock *curBlock = (BaseCellBlock *)doc->lBlocks->at(index);
    //   EjTableBlock *curTable = curBlock->parent;
    //   if(curTable)
    //   {
    //      int start = doc->lBlocks->indexOf(curTable) + 1;
    //      row = (index - start) / curTable->nColums();
    //      colum = index - start - row*curTable->nColums();
    //      res = true;
    //   }
    return res;
}

int EjTextControl::getIndexString(int index)
{
    int index_string = 0;
    for(int i = 0; i < doc->lStrings->count(); i++)
    {
        if(index >= doc->lStrings->at(i)->startBlock) {
            index_string = i;
        }
        else
            break;
    }
    return index_string;
}

int EjTextControl::tableVid()
{
    int res = -1;
    EjTableBlock *table = isTable(activeIndex);
    if(table)
        res = table->vid;
    return res;
    //    if(activeBlock < 0 || doc->lBlocks->at(activeBlock)->type != BASECELL)
    //        return -1;
    //    BaseCellBlock *curBlock = (BaseCellBlock*)doc->lBlocks->at(activeBlock);
    //    return curBlock->parent->vid;
}

void EjTextControl::loadLinks()
{
    quint32 lastOffset;
    qint16 lastKey;
    quint16 lastVer;
    EjLinkProp *curLink;

    if(!doc->m_attrProp)
        return;
    for(int i = 0; i < doc->m_attrProp->m_lLinks.count(); i++) {
        curLink = doc->m_attrProp->m_lLinks.at(i);
  //      if(i == 0)
  //      {
  //          curLink->m_doc = this->doc;
  //      }
  //      else
        {
            if(!curLink->m_extDoc)
                curLink->m_extDoc = new EjDocument(curLink->keyUuid());
         }
    }
}

void EjTextControl::signLinks(qint32 sign_id,qint32 group_id, int status, QString comment)
{
    quint32 lastOffset;
    qint16 lastKey;
    quint16 lastVer;
    EjLinkProp *curLink;
    bool usl;

    if(!doc->m_attrProp)
        return;
    for(int i = 0; i < doc->m_attrProp->m_lLinks.count(); i++) {
        curLink = doc->m_attrProp->m_lLinks.at(i);
  //      if(i == 0)
  //      {
  //          curLink->m_doc = this->doc;
  //      }
  //      else
        {
            if(!curLink->m_extDoc)
                curLink->m_extDoc = new EjDocument(curLink->keyUuid());
        }
    }

}

void EjTextControl::updateLinks()
{
//  Task task;
//  quint32 lastOffset;
//  qint16 lastKey;
//  quint16 lastVer;
  EjLinkProp *curLink;
//  PropsDoc::Link *curLink;
  EjCalculator calculator(doc);

//  delete task.m_doc;
//  task.m_doc = NULL;

//  PropsDoc *propDoc = doc->getPropDoc();
  if(!doc->m_attrProp)
      return;
  for(int i = 0; i < doc->m_attrProp->m_lLinks.count(); i++) {
      curLink = doc->m_attrProp->m_lLinks.at(i);
//      if(i == 0)
//      {
//          curLink->m_doc = this->doc;
//      }
//      else
//      {
//          if(!curLink->m_extDoc)
//              curLink->m_extDoc = new EjDocument(curLink->keyUuid());
//          task.key = curLink->keyUuid();
//          task.m_doc = curLink->m_extDoc;
//          if(ext_storage->getId(&task))
//          {
//              task.getTask();
//              ext_storage->loadTasksBody(&task, lastOffset, lastKey, lastVer);
//          }
//          task.m_doc = NULL;
//      }
      if(curLink->m_extDoc)
          updateTables(curLink->m_extDoc);
//      calculator.m_doc = curLink->m_extDoc;
//      calculator.updateFormulas(true);
  }
  calculator.calcTables(true);
//  propDoc = doc->getPropDoc();
  calculator.m_doc = doc;
  calculator.updateFormulas(true);

}

void EjTextControl::createPatch()
{
    if(!doc || !m_createPatchEnabled)
        return;
    if(!docPrev)
    {
//        docPrev = new EjDocument();
//        doc->copy(docPrev);
    }
    else
    {
        while(m_currentPatch < m_lPatches.count() - 1)
            delete m_lPatches.takeLast();

        PatchUndoRedo *patch = new PatchUndoRedo();
        patch->m_body = diff(*docPrev->lBlocks,*doc->lBlocks,true);
        if(patch->m_body.count() < 3)
            patch->m_body.clear();
        patch->m_attributes = diff(*docPrev->lPropBlocks,*doc->lPropBlocks,true);
        if(patch->m_attributes.count() < 3)
            patch->m_attributes.clear();
        if(!patch->m_body.isEmpty() || !patch->m_attributes.isEmpty())
        {
            patch->m_activeIndex = activeIndex;
            patch->m_position = position;
            patch->m_startSelectBlock = m_startSelectBlock;
            patch->m_startSelectPos = m_startSelectPos;
            patch->m_startSelectWidth = m_startSelectWidth;
            patch->m_endSelectBlock = m_endSelectBlock;
            patch->m_endSelectPos = m_endSelectPos;
            patch->m_endSelectWidth = m_endSelectWidth;
            patch->m_selectMode = m_selectMode;

            m_lPatches.append(patch);
            doc->copyData(docPrev);
        }
        else{
            delete patch;
        }
        m_currentPatch = m_lPatches.count() - 1;
    }
}

void EjTextControl::undo()
{
    bool isError;
    QList<EjBlock*> unused;
    if(m_currentPatch == m_lPatches.count() - 1)
        createPatch();
   if(m_currentPatch > -1)
   {
       PatchUndoRedo *patch = m_lPatches[m_currentPatch];
       if(!patch->m_body.isEmpty())
       {
           isError = false;
           try {
               *doc->lBlocks  = patch2(*doc->lBlocks, unused, patch->m_body, true);
           }
           catch(...)
           {
               isError = true;
               qDebug() << "Error for load " << __FILE__ << __LINE__ ;

//                break;
           }
           if(!isError)
               qDeleteAll(unused);
           unused.clear();
       }
       if(!patch->m_attributes.isEmpty())
       {
           isError = false;
           try {
               *doc->lPropBlocks  = patch2(*doc->lPropBlocks, unused, patch->m_attributes, true);
               doc->m_attrProp = nullptr;
           }
           catch(...)
           {
               isError = true;
               qDebug() << "Error for load " << __FILE__ << __LINE__ ;
           }
           if(!isError)
               qDeleteAll(unused);
           unused.clear();
       }
       if(!patch->m_body.isEmpty() || !patch->m_attributes.isEmpty())
       {
           activeIndex = patch->m_activeIndex;
           position = patch->m_position;
           if(m_currentPatch > 0)
           {
               patch = m_lPatches[m_currentPatch - 1];
               m_startSelectBlock = patch->m_startSelectBlock;
               m_startSelectPos = patch->m_startSelectPos;
               m_startSelectWidth = patch->m_startSelectWidth;
               m_endSelectBlock = patch->m_endSelectBlock;
               m_endSelectPos = patch->m_endSelectPos;
               m_endSelectWidth = patch->m_endSelectWidth;
               m_selectMode = (e_selectMode)patch->m_selectMode;
           }
           else
           {
               m_startSelectBlock = -1;
               m_startSelectPos = 0;
               m_startSelectWidth = 0;
               m_endSelectBlock = -1;
               m_endSelectPos = 0;
               m_endSelectWidth = 0;
               m_selectMode = NO_SELECTED;
           }
           doc->copyData(docPrev);
           doc->calcProps();
           updateTables(doc);
           calcTables();
           calc(0,true);
           calcData(true);
           calcCursor(true);

           //    if(m_currentPatch > -1)
           m_currentPatch--;
       }
   }
}

void EjTextControl::redo()
{
    bool isError;
    QList<EjBlock*> unused;
    if(m_currentPatch > -2 && m_currentPatch < m_lPatches.count() - 1)
    {
        m_currentPatch++;
        PatchUndoRedo *patch = m_lPatches[m_currentPatch];
        if(!patch->m_attributes.isEmpty())
        {
            isError = false;
            try {
                *doc->lPropBlocks  = patch2(*doc->lPropBlocks, unused, patch->m_attributes, false);
                doc->m_attrProp = nullptr;
            }
            catch(...)
            {
                isError = true;
                qDebug() << "Error for load " << __FILE__ << __LINE__ ;
            }
            if(!isError)
                qDeleteAll(unused);
            unused.clear();
        }

        if(!patch->m_body.isEmpty())
        {
            isError = false;
            try {
                *doc->lBlocks  = patch2(*doc->lBlocks, unused, patch->m_body, false);
            }
            catch(...)
            {
                isError = true;
                qDebug() << "Error for load " << __FILE__ << __LINE__ ;
            }
            if(!isError)
                qDeleteAll(unused);
            unused.clear();
        }

        if(!patch->m_body.isEmpty() || !patch->m_attributes.isEmpty())
        {
            activeIndex = patch->m_activeIndex;
            position = patch->m_position;
            m_startSelectBlock = patch->m_startSelectBlock;
            m_startSelectPos = patch->m_startSelectPos;
            m_startSelectWidth = patch->m_startSelectWidth;
            m_endSelectBlock = patch->m_endSelectBlock;
            m_endSelectPos = patch->m_endSelectPos;
            m_endSelectWidth = patch->m_endSelectWidth;
            m_selectMode = (e_selectMode)patch->m_selectMode;
            doc->copyData(docPrev);
            doc->calcProps();
            updateTables(doc);
            calcTables();
            calc(0,true);
            calcData(true);
            calcCursor(true);
        }
    }

}


int EjTextControl::GetNormalPageHeight()
{
    if (m_defaultOrientation == EjDocLayout::ORN_PORTRAIT){
        return m_defaultPageHeight;
    }

    return m_defaultPageWidth;
}


int EjTextControl::GetNormalPageWidth()
{
    if (m_defaultOrientation == EjDocLayout::ORN_PORTRAIT){
        return m_defaultPageWidth;
    }

    return m_defaultPageHeight;
}


void EjTextControl::setIsViewDoc(bool source)
{
    m_isViewDoc = source;

    if (doc == nullptr){
        return;
    }
    qDeleteAll(*doc->lStrings);
    doc->lStrings->clear();
    qDeleteAll(*doc->lPages);
    doc->lPages->clear();
}

void EjTextControl::setDocument(EjDocument *document)
{
    //    doc->lBlocks = doc->doc->lBlocks;
    //    doc->lStrings = doc->doc->lStrings;
    //    doc->lPages = doc->doc->lPages;
    ////    lFragments = doc->lFragments;
    //    doc->lTables = doc->doc->lTables;
    //    lStyles-> = doc->lStyles->;
    //    m_Title = doc->title;
    //    m_Tags = doc->lTags;
    EjCalcParams calcParams;
    LabelBlock *curLabel;
    int index;

    doc = document;
    EjCalculator calculator(doc);

    if(docPrev)
    {
        delete docPrev;
        docPrev = nullptr;
    }
    qDeleteAll(m_lPatches);
    m_lPatches.clear();


    if(activeIndex > doc->lBlocks->count() - 1)
    {
        activeIndex = doc->lBlocks->count() - 1;
        position = 0;
    }
    if(activeIndex > -1 && (doc->lBlocks->at(activeIndex)->type == TEXT || doc->lBlocks->at(activeIndex)->type == BASECELL))
    {
        int len = ((EjTextBlock*)(doc->lBlocks->at(activeIndex)))->text.length();
        if(position > len)
            position = len;
    }
    else
        position = 0;
    m_startSelectBlock = -1;
    m_endSelectBlock = -1;
    m_startSelectPos = 0;
    m_endSelectPos = 0;
    //    calculator.doc.lBlocks = doc->lBlocks;
    //    calculator.doc.lStrings = doc->lStrings;
    //    calculator.doc.lPages = doc->lPages;
    //    calculator.doc.lFragments = doc->lFragments;
    //    calculator.doc.lTables = doc->lTables;
    m_calcIndex = 0;
    //    if(activeBlock >= 0 && doc->lBlocks->at(activeBlock)->type == TEXT)
    //    {
    //        position = static_cast<EjTextBlock*>(doc->lBlocks->at(activeBlock))->text.size();
    //    }
    updateTables(doc);

    calcParams.textStyle = getTextStyle(0);
    calcParams.control = this;
    calcParams.contentX = m_contentX;
    calcParams.contentY = m_contentY;
    calcParams.viewScale = scaleSize;
    calcParams.statusMode = m_statusMode;
    if(doc->lLabels->count() > 0)
    {
        index = 0;
        doc->lBlocks->at(0)->calcBlock(index, &calcParams);
    }
    for(int i = 0; i < doc->lLabels->count(); i++)
    {
        curLabel = doc->lLabels->at(i);
        index = doc->lBlocks->indexOf(curLabel);
        curLabel->calcBlock(index, &calcParams);
    }

//    calculator.calcTables();
//    updateTables(doc);
//    calcTables();

    calcNext();
    calcCursor();
}

bool EjTextControl::pasteCells()
{

    int index = activeIndex;
    EjBlock* curBlock;

    int count_cells = 0;

    for (int i = 0; i < m_clipboardDoc->lBlocks->count(); i++)
    {
        if (m_clipboardDoc->lBlocks->at(i)->type == e_typeBlocks::BASECELL)
        {
            count_cells++;
        }
    }

//    while (doc->lBlocks->at(index)->type != e_typeBlocks::BASECELL)
//    {
//        index--;
//    }

//    for (int i = 0; i < m_clipboardDoc->lBlocks->count(); i++)
//    {
//        curBlock = m_clipboardDoc->lBlocks->at(i);
//        if (curBlock->type == e_typeBlocks::BASECELL)
//        {

//        }
//    }

    int temp_index = 1;
    for (int i = 0; i < count_cells; i++)
    {
        while (doc->lBlocks->at(index)->type != e_typeBlocks::BASECELL && doc->lBlocks->at(index)->type != e_typeBlocks::END_GROUP)
        {
           doc->lBlocks->removeAt(index);
        }


        for (int i = temp_index; i < m_clipboardDoc->lBlocks->count() && m_clipboardDoc->lBlocks->at(i)->type != e_typeBlocks::BASECELL ; i++)
        {
            doc->lBlocks->insert(index, m_clipboardDoc->lBlocks->at(i));
            index++;
            temp_index++;
        }
        index++;
        temp_index++;
    }


    return true;
}

bool EjTextControl::copyCells()
{
    if(m_clipboardDoc)
        delete m_clipboardDoc;
    m_clipboardDoc = new EjDocument();
    EjBlock* curBlock;
    for(int i = 0; i < doc->lPropBlocks->count(); i++)
    {
        curBlock = doc->lPropBlocks->at(i);
        curBlock = curBlock->makeCopy();
        m_clipboardDoc->lPropBlocks->append(curBlock);
    }
    m_clipboardDoc->calcProps();
    int startBlock = m_startSelectBlock;
    int endBlock = m_endSelectBlock;
    //EjTableBlock* tableBlock;
    if (m_startSelectBlock > -1 && m_endSelectBlock > -1)
    {

        if (doc->lBlocks->at(endBlock + 1)->type != e_typeBlocks::BASECELL)
        {
           while (doc->lBlocks->at(endBlock + 1)->type != e_typeBlocks::BASECELL && doc->lBlocks->at(endBlock + 1)->type != e_typeBlocks::END_GROUP)
           {
                endBlock++;
           }
        }

        for (int i = startBlock; i <= endBlock; i++)
        {
            curBlock = doc->lBlocks->at(i)->makeCopy();
            m_clipboardDoc->lBlocks->append(curBlock);
        }
    }

    return true;
}

bool EjTextControl::menuActivate(QString command, QString data)
{
    //m_startSelectBlock
    EjFragmentBlock *fragment;
    Param param;
    EjBlock *cur_Block;
    EjTextBlock *cur_textBlock;
    bool res = true;
    int n_blockEnd = m_endSelectBlock;
    int n_pos = m_startSelectPos;
    int n_blockStart = m_startSelectBlock;
    QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    QString str;


    if(m_endSelectBlock > 0 && doc->lBlocks->at(m_endSelectBlock)->type == TEXT && m_endSelectPos == 0){
        m_endSelectBlock--;
        if(doc->lBlocks->at(m_endSelectBlock)->type == TEXT)
            m_endSelectPos = static_cast<EjTextBlock*>(doc->lBlocks->at(m_endSelectBlock))->text.size();

    }

    fragment = new EjFragmentBlock();
    if(command == "B" || command == "I" || command == "U" || command == "dB" || command == "dI" || command == "dU" || command == "Cut" || command == "Del")
    {
//        if(m_selectMode == SELECTED)
//        {
//            activeIndex = m_startSelectBlock;
//            if(activeIndex > -1 && (doc->lBlocks->at(activeIndex)->type == TEXT || doc->lBlocks->at(activeIndex)->type == BASECELL) )
//            {
//                res = splitText(n_blockStart,n_pos);
//                if(res) {
//                    if(m_startSelectBlock == m_endSelectBlock)
//                        m_endSelectPos -= m_startSelectPos;
//                    m_startSelectBlock++;
//                    m_endSelectBlock++;
//                    n_blockEnd++;
//                }
//                else
//                    m_startSelectBlock = n_blockStart;
//            }
//            m_startSelectPos = 0;
//            n_pos = m_endSelectPos;
//            res = splitText(n_blockEnd,n_pos);

//            if(command == "B" || command == "I" || command == "U")
//            {
//                doc->lBlocks->insert(m_startSelectBlock, fragment);
//                fragment->startBlock = m_startSelectBlock;
//                fragment->endBlock = m_endSelectBlock+1;
//                fragment->countBlocks = fragment->endBlock - fragment->startBlock;
//                updateFragments(m_startSelectBlock,true);
//                //                doc->lFragments.append(fragment);
//                m_endSelectBlock++;
//                m_startSelectBlock++;
//            }

//        }
//        else // if(doc->lBlocks->at(activeBlock)->type != BASECELL)
//        {
//            res = splitText(activeIndex,position);
//            position = 0;
//            //            updateFragments(activeBlock,true);
//            doc->lBlocks->insert(activeIndex,new EjTextBlock());
////            updateFragments(activeIndex, true);

//            if(command == "B" || command == "I" || command == "U")
//            {
//                doc->lBlocks->insert(activeIndex, fragment);
//                fragment->startBlock = activeIndex;
//                fragment->endBlock = activeIndex + 1;
//                fragment->countBlocks = 1;
//                updateFragments(activeIndex,true);
//                //                m_lFragments.append(fragment);
//            }
//        }
    }
    if(command == "B")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
		textStyle->setFontBold(true);
		setTextStyle(textStyle,false,true);
        delete textStyle;
    }
    else if(command == "I")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
		textStyle->setFontItalic(true);
		setTextStyle(textStyle,false,true);
        delete textStyle;
    }
    else if(command == "U")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
		textStyle->setFontUnderline(true);
		setTextStyle(textStyle,false,true);
        delete textStyle;
    }
    else if(command == "dB")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
		textStyle->setFontBold(false);
		setTextStyle(textStyle,false,true);
        delete textStyle;
    }
    else if(command == "dI")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
		textStyle->setFontItalic(false);
		setTextStyle(textStyle,false,true);
        delete textStyle;
    }
    else if(command == "dU")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
		textStyle->setFontUnderline(false);
		setTextStyle(textStyle,false,true);
		delete textStyle;
    }
    else if(command == "FontStyle" && data != "")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
        int fontStyle = data.toInt();
        if(textStyle->fontStyle() != fontStyle)
        {
            textStyle->setFontStyle(EjTextStyle::e_fontStyle(fontStyle));
            if(fontStyle == 0) {
                textStyle->setFontSize(12); textStyle->setFontBold(false); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 1) {
//                textStyle.fontSize = 10; textStyle.fontBold = textStyle.fontItalic = textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(10); textStyle->setFontBold(false); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 2) {
//                textStyle.fontSize = 8; textStyle.fontBold = textStyle.fontItalic = textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(8); textStyle->setFontBold(false); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 3) {
//                textStyle.fontSize = 16; textStyle.fontBold = textStyle.fontItalic = textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(16); textStyle->setFontBold(false); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 4) {
//                textStyle.fontSize = 18; textStyle.fontBold = textStyle.fontItalic = textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(18); textStyle->setFontBold(false); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 10) {
//                textStyle.fontSize = 20; textStyle.fontBold = true; textStyle.fontItalic = textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(20); textStyle->setFontBold(true); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 11) {
//                textStyle.fontSize = 18; textStyle.fontBold = true; textStyle.fontItalic = textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(18); textStyle->setFontBold(true); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 12) {
//                textStyle.fontSize = 14; textStyle.fontBold = true; textStyle.fontItalic = textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(14); textStyle->setFontBold(true); textStyle->setFontItalic(false); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 13) {
//                textStyle.fontSize = 13; textStyle.fontBold = textStyle.fontItalic = true; textStyle.fontUnderline = textStyle.fontStrikeout = false;
                textStyle->setFontSize(13); textStyle->setFontBold(true); textStyle->setFontItalic(true); textStyle->setFontUnderline(false); textStyle->setFontStrikeOut(false);
            }
            if(fontStyle == 14) {
//                textStyle.fontSize = 13; textStyle.fontBold = textStyle.fontItalic = textStyle.fontUnderline = true; textStyle.fontStrikeout = false;
                textStyle->setFontSize(13); textStyle->setFontBold(true); textStyle->setFontItalic(true); textStyle->setFontUnderline(true); textStyle->setFontStrikeOut(false);
            }

            setTextStyle(textStyle,false,true);
        }
        else
            res = false;
        delete textStyle;
    }
    else if(command == "TextColor" && data != "")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
        QColor textColor = QColor(data);
        if(textStyle->fontColor() != textColor)
        {
            textStyle->setFontColor(textColor);
            setTextStyle(textStyle,false,true);
        }
        delete textStyle;
    }
    else if(command == "FillColor" && data != "")
    {
        EjTextStyle *textStyle = dynamic_cast<EjTextStyle*>(getSelectedTextStyle(activeIndex)->makeCopy());
        QColor fillColor = QColor(data);
        if(textStyle->brushColor() != fillColor)
        {
            textStyle->setBrushColor(fillColor);
            setTextStyle(textStyle,false,true);
        }
        delete textStyle;
    }
    else if(command == "AlignLeft")
    {
        EjParagraphStyle *prgStyle = dynamic_cast<EjParagraphStyle*>(getParagraphStyle(activeIndex)->makeCopy());
        int align = prgStyle->align();
        if(!(align & EjParagraphStyle::AlignLeft))
        {
            align |= EjParagraphStyle::AlignLeft;
            align &= ~EjParagraphStyle::AlignHCenter;
            align &= ~EjParagraphStyle::AlignRight;
            prgStyle->setAlign(align);
            setParagraphStyle(prgStyle);
        }
        else
            res = false;
        delete prgStyle;
    }
    else if(command == "AlignHCenter")
    {
        EjParagraphStyle *prgStyle = dynamic_cast<EjParagraphStyle*>(getParagraphStyle(activeIndex)->makeCopy());
        int align = prgStyle->align();
        if(!(align & EjParagraphStyle::AlignHCenter))
        {
            align &= ~EjParagraphStyle::AlignLeft;
            align |= EjParagraphStyle::AlignHCenter;
            align &= ~EjParagraphStyle::AlignRight;
            prgStyle->setAlign(align);
            setParagraphStyle(prgStyle);
        }
        else
            res = false;
        delete prgStyle;
    }
    else if(command == "AlignRight")
    {
        EjParagraphStyle *prgStyle = dynamic_cast<EjParagraphStyle*>(getParagraphStyle(activeIndex)->makeCopy());
        int align = prgStyle->align();
        if(!(align & EjParagraphStyle::AlignRight))
        {
            align &= ~EjParagraphStyle::AlignLeft;
            align &= ~EjParagraphStyle::AlignHCenter;
            align |= EjParagraphStyle::AlignRight;
            prgStyle->setAlign(align);
            setParagraphStyle(prgStyle);
        }
        else
            res = false;
        delete prgStyle;
    }
    else if(command == "Call")
    {
        QString tel = isTell();
//        for (int i = 0; i < str.size(); ++i) {
//            if ((str.at(i) >= QChar('0') && str.at(i) <= QChar('9')) || str.at(i) == QChar('-') || str.at(i) == QChar('+'))
//                tel += str.at(i);
//            if(tel.count() > 50)
//                break;
//        }
        if(tel.count() > 3)
            emit ring(tel);
    }
//    else if(command == "Copy" || command == "Cut" || command == "Del" || command == "Call")
    else if(command == "Copy" || command == "Cut" || command == "Del")
    {
        //        if(m_endSelectBlock > doc->lBlocks->count() - 1)
        //            m_endSelectBlock = doc->lBlocks->count() - 1;

        //return copyCells();

        int startSelectBlock = m_startSelectBlock;
        int endSelectBlock = m_endSelectBlock;
        if(startSelectBlock == -1)
            startSelectBlock = activeIndex;
        if(endSelectBlock == -1)
            endSelectBlock = activeIndex;
        if(startSelectBlock == -1 || endSelectBlock == -1)
            return false;
        if(doc->lBlocks->at(endSelectBlock)->type >= GROUP_BLOCK)
        {
            EjGroupBlock *block = dynamic_cast<EjGroupBlock*>(doc->lBlocks->at(endSelectBlock));
            if(block)
            {
                endSelectBlock = block->m_index + block->m_counts;
            }
        }
        if(command != "Del")
        {
            bool res = false;
            int row_index, colum_index;
            int row_start, colum_start;
            int row_end, colum_end;
            EjCellBlock *curCell;
            EjTableBlock *curTable;
            EjBlock *curBlock;
            TableFragment *curFragment;
            int indexStart;
            int indexEnd;
            int startSelect = startSelectBlock;
            int endSelect = endSelectBlock;

//            qDeleteAll(m_lClipboardBlocks);
//            m_lClipboardBlocks.clear();
            if(m_clipboardDoc)
                delete m_clipboardDoc;
            m_clipboardDoc = new EjDocument();
            for(int i = 0; i < doc->lPropBlocks->count(); i++)
            {
                curBlock = doc->lPropBlocks->at(i);
                curBlock = curBlock->makeCopy();
                m_clipboardDoc->lPropBlocks->append(curBlock);
            }
            m_clipboardDoc->calcProps();
            curTable = isTable(startSelectBlock);
            if(curTable && curTable == isTable(endSelectBlock))
            {
                getBaseCellParams(startSelectBlock, row_start, colum_start);
                getBaseCellParams(endSelectBlock, row_end, colum_end);
                if(row_start != row_end || colum_start != colum_end || startSelect == endSelect)
                {
                    while(doc->lBlocks->at(startSelect)->type != BASECELL)
                        startSelect--;
                    while(doc->lBlocks->at(endSelect+1)->type != END_GROUP && doc->lBlocks->at(endSelect+1)->type != BASECELL)
                        endSelect++;
                }
            }
            for(int i = startSelect; i <= endSelect; i++)
            {
                //                if(isCellSelected(i))
                if(doc->lBlocks->at(i)->type == EXT_TABLE)
                {
                    curTable = (EjTableBlock *)doc->lBlocks->at(i)->makeCopy();
                    curTable->m_doc = m_clipboardDoc;

                    m_clipboardDoc->lBlocks->append(curTable);
                }
                else if(doc->lBlocks->at(i)->type == BASECELL)
                {
                    //                    isCellSelected(i)
                    //                    BaseCellBlock *curBlockStart;
                    //                    BaseCellBlock *curBlockEnd;
                    //                    curBlockStart = (BaseCellBlock*)doc->lBlocks->at(m_startSelectBlock);
                    //                    curBlockEnd = (BaseCellBlock*)doc->lBlocks->at(m_endSelectBlock);
                    curCell = (EjCellBlock*)doc->lBlocks->at(i);
//                    curTable = (EjTableBlock *)curCell->parent->makeCopy();
//                    curTable->m_doc = m_clipboardDoc;
                    //                    indexStart = doc->lBlocks->indexOf(curBlockCurrent->parent) + 1;
                    //                    indexEnd = indexStart + curTable->nRows()*curTable->nColums();
                    indexStart = ((EjTableBlock*)(curCell->parent))->m_index + 1;
                    indexEnd = ((EjTableBlock*)(curCell->parent))->endBlock();


                    //curTable->m_doc = m_clipboardDoc;//custom
                   // m_clipboardDoc->lBlocks->append(curTable);
                    curTable = (EjTableBlock*)curTable->makeCopy();
                    curTable->m_doc = m_clipboardDoc;
                    m_clipboardDoc->lBlocks->append(curTable);
                    curTable->m_index = m_clipboardDoc->lBlocks->indexOf(curTable);
                    curTable->m_counts = indexEnd - indexStart + 1;
                    if(curTable->m_counts < 0)
                        curTable->m_counts = 0;
//                    curTable->endBlock = curTable->startBlock + curTable->countBlocks;

                    EjCellBlock *cell_block;
                    for(int j = indexStart; j <= indexEnd; j++)
                    {
                        //                        if(j == startSelectBlock || isCellSelected(j))
                        //                            curBlock = (BaseCellBlock*)doc->lBlocks->at(j)->makeCopy();
                        //                        else
                        //                            curBlock = new BaseCellBlock();
                        //                        curBlock->parent = curTable;
                        curBlock = doc->lBlocks->at(j)->makeCopy();

                        if(curBlock->type == BASECELL)
                        {
                            cell_block = (EjCellBlock *)curBlock;
                            ((EjCellBlock *)curBlock)->parent = curTable;
                            int index = doc->lPropBlocks->indexOf(((EjCellBlock *)doc->lBlocks->at(j))->cellStyle);
                            ((EjCellBlock *)curBlock)->cellStyle = (EjCellStyle *)(m_clipboardDoc->lPropBlocks->at(index));
                        }/*
                        else if (curBlock->type == NUM_STYLE && ((EjNumStyleBlock*)curBlock)->style->m_vid == e_PropDoc::CELL_STYLE)
                        {
                            ((EjNumStyleBlock*)curBlock)->style = cell_block->cellStyle;
                        }*/
                        m_clipboardDoc->lBlocks->append(curBlock);
                        i = j;
                    }

                    if(startSelect > indexStart)
                        indexStart = startSelectBlock;
                    if(endSelect < indexEnd)
                        indexEnd = endSelectBlock;
                    getBaseCellParams(indexStart, row_start, colum_start);
                    getBaseCellParams(indexEnd, row_end, colum_end);
                    curTable->start_column = colum_start;
                    curTable->start_row = row_start;
                    if(curTable->vid == EjTableBlock::SHOP_LIST)
                    {
//                        curFragment = new TableFragment();
//                        curFragment->setStartColum(colum_start);
//                        curFragment->setEndColum(colum_end);
//                        curFragment->setStartRow(row_start);
//                        curFragment->setEndRow(row_end);
//                        curFragment->type = EjFragment::Select;
//                        curTable->lFragments.append(curFragment);
                        colum_start = 0;
                        colum_end = ((EjTableBlock*)(curCell->parent))->nColums();
                        //                        colum_start = doc->lBlocks->indexOf(curBlockCurrent->parent) + 1 + row_start*curBlockCurrent->parent->nColums();
                        //                        colum_end = doc->lBlocks->indexOf(curBlockCurrent->parent) + 1 + row_end*curBlockCurrent->parent->nColums();
                    }
                    row_end = ((EjTableBlock*)(curCell->parent))->nRows() - row_end - 1;
                    if(row_end < 0)
                        row_end = 0;
                    colum_end = ((EjTableBlock*)(curCell->parent))->nColums() - colum_end - 1;
                    if(colum_end < 0)
                        colum_end = 0;
//                    indexStart = m_lClipboardBlocks.indexOf(curTable) + 1;
                    indexStart = curTable->cellIndex(0,0,m_clipboardDoc->lBlocks);
                    if(curTable->vid == EjTableBlock::SHOP_LIST)
                    {
                        row_start--;
                        if(row_start < 0)
                            row_start = 0;
                        row_end--;
                        if(row_end < 0)
                            row_end = 0;
                        indexStart += curTable->nColums();
                    }
                    for(int j = 0; j < row_start; j++)
                    {
                        curTable->delString(m_clipboardDoc->lBlocks,indexStart);
                    }
                    for(int j = 0; j < row_end; j++)
                    {
                        indexEnd = m_clipboardDoc->lBlocks->count() - 2;
                        if(curTable->vid == EjTableBlock::SHOP_LIST)
                            indexEnd -= curTable->nColums();
                        curTable->delString(m_clipboardDoc->lBlocks,indexEnd);
                    }
                    for(int j = 0; j < colum_start; j++)
                    {
//                        delTableColum(&m_lClipboardBlocks,indexStart);
                        curTable->delColum(m_clipboardDoc->lBlocks,indexStart);
                    }
                    for(int j = 0; j < colum_end; j++)
                    {
                        indexEnd = m_clipboardDoc->lBlocks->count() - 2;
                        if(curTable->vid == EjTableBlock::SHOP_LIST)
                            indexEnd -= curTable->nColums();
//                        delTableColum(&m_lClipboardBlocks,indexEnd);
                        curTable->delColum(m_clipboardDoc->lBlocks,indexEnd);
                    }
//                    curTable->endBlock = curTable->startBlock + curTable->countBlocks;


                }
                else if(doc->lBlocks->at(i)->type != EXT_TABLE)
                    m_clipboardDoc->lBlocks->append(doc->lBlocks->at(i)->makeCopy());
            }
            if(m_startSelectPos > 0 && m_clipboardDoc->lBlocks->count() > 0 && m_clipboardDoc->lBlocks->at(0)->type == TEXT)
            {
                cur_textBlock = (EjTextBlock*)m_clipboardDoc->lBlocks->at(0);
                cur_textBlock->text = cur_textBlock->text.right(cur_textBlock->text.size() - m_startSelectPos);
            }
            if(m_endSelectPos > 0 && m_clipboardDoc->lBlocks->count() > 0 && m_clipboardDoc->lBlocks->at(m_clipboardDoc->lBlocks->count()-1)->type == TEXT)
            {
                cur_textBlock = (EjTextBlock*)m_clipboardDoc->lBlocks->at(m_clipboardDoc->lBlocks->count()-1);
                cur_textBlock->text = cur_textBlock->text.left(m_endSelectPos);
            }
            str.clear();
            curTable = 0;
            for(int i = 0; i < m_clipboardDoc->lBlocks->count(); i++)
            {
                switch (m_clipboardDoc->lBlocks->at(i)->type) {
                case EXT_IMAGE:
                    str += " Image ";
                    break;
                case TEXT:
                    str += static_cast<EjTextBlock*>(m_clipboardDoc->lBlocks->at(i))->text;
                    break;
                case EXT_TABLE:
                    curTable = (EjTableBlock*)m_clipboardDoc->lBlocks->at(i);
                    break;
                case BASECELL:
                    if(curTable)
                    {
                        EjCellBlock *curBlock = (EjCellBlock*)m_clipboardDoc->lBlocks->at(i);
                        //                    EjTableBlock *curTable = (EjTableBlock*)curBlock->parent;
                        //                        int start = m_lClipboardBlocks.indexOf(curTable)+1;
                        //                        int row = (i - start)/ curTable->nColums();
                        //                        int colum = i - start - row*curTable->nColums();
                        int row;
                        int colum;
                        cellParams(curTable,i,row,colum,m_clipboardDoc->lBlocks);
                        if(colum > 0 || curBlock->vid == EjCellBlock::CELL_CHECK)
                            str += QChar(QChar::Tabulation);
                        //                        if(!curBlock->text.isEmpty())
                        str += curBlock->getText();
                        //                        else
                        //                            str += QChar(QChar::Tabulation);
                        while(i < m_clipboardDoc->lBlocks->count() - 1 && m_clipboardDoc->lBlocks->at(i + 1)->type != BASECELL)
                        {
                            i++;
                            if(m_clipboardDoc->lBlocks->at(i)->type == SPACE)
                                str += ' ';
                            else if(m_clipboardDoc->lBlocks->at(i)->type == TEXT)
                                str += static_cast<EjTextBlock*>(m_clipboardDoc->lBlocks->at(i))->text;
                        }
                        if(colum == curTable->nColums() - 1)
                            str += "\r\n";
                    }
                    break;

                    //                case CHECK:
                    //                    str += QChar(QChar::Tabulation);
                    //                    break;
                case SPACE:
                    str += ' ';
                    break;
                case ENTER:
                    str += "\r\n";
                default:
                    break;
                }
            }
            QMimeData mimeData;
//            mimeData.setText(str);
            clipboard->setText(str);

            m_htmlBuffer = str;
            if(command == "Call")
            {
                QString tel;
                for (int i = 0; i < str.size(); ++i) {
                    if ((str.at(i) >= QChar('0') && str.at(i) <= QChar('9')) || str.at(i) == QChar('-') || str.at(i) == QChar('+'))
                        tel += str.at(i);
                    if(tel.count() > 50)
                        break;
                }
                if(tel.count() > 3)
                    emit ring(tel);
            }
            str.clear();
            str += QString(" <!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"> \n"
                           " <html> \n"
                           " <head> \n"
                           " <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"> \n"
                           " <meta http-equiv=\"Content-Style-Type\" content=\"text/css\"> \n"
                           " <title></title> \n"
                           " <meta name=\"Generator\" content=\"Jotter\"> \n"
                           " <meta name=\"JotterVersion\" content=\"1265.21\"> \n"
                           " <style type=\"text/css\"> \n"
                           " p { "
                           " font-size: 12.0px; font-family: Arial; color: black "
                           " } \n"
                           " table { "
                           " border-style: solid; "
                           " border-collapse: collapse "
                           " } \n"
                           " td { "
                           //                           " background-color: #bfbfbf; "
                           " border-style: solid; "
                           " border-width: 1.0px; "
                           " border-color: #323333; padding: 4.0px "
                           " } \n"
                           " </style> \n"
                           " </head> \n"
                           " <body> \n"
                           " <p> \n" );
            //            EjTableBlock *curTable = 0;
            bool isTable = false;
            for(int i = 0; i < m_clipboardDoc->lBlocks->count(); i++)
            {
                if(isTable && !(m_clipboardDoc->lBlocks->at(i)->type == BASECELL ) )
                {
                    str += " </table> \n";
                    str += " <p> \n";
                    isTable = false;
                }
                switch (m_clipboardDoc->lBlocks->at(i)->type) {
                case TEXT:
                    str += static_cast<EjTextBlock*>(m_clipboardDoc->lBlocks->at(i))->text;
                    break;
                case EXT_TABLE:
                    str += " </p> \n";
                    str += " <table cellspacing=\"0\" cellpadding=\"4\" > \n";
                    str += " <tr> \n";
                    curTable = (EjTableBlock*)m_clipboardDoc->lBlocks->at(i);
                    isTable = true;
                    break;
                case BASECELL:
                    if(curTable)
                    {
                        EjCellBlock *curBlock = (EjCellBlock*)m_clipboardDoc->lBlocks->at(i);
//                        if(curBlock->vid == EjCellBlock::ENDTABLE)
//                            break;
                        //                    EjTableBlock *curTable = (EjTableBlock*)curBlock->parent;
                        //                        int start = m_lClipboardBlocks.indexOf(curTable)+1;
                        //                        int row = (i - start)/ curTable->nColums();
                        //                        int colum = i - start - row*curTable->nColums();
                        //                        if(colum > 0)
                        //                            str += QChar(QChar::Tabulation);
                        //                        if(!curBlock->text.isEmpty())
                        int row;
                        int colum;
                        cellParams(curTable,i,row,colum,m_clipboardDoc->lBlocks);

                        str += "<td><p>";
//                        str += curBlock->text;
                        //                        else
                        //                            str += QChar(QChar::Tabulation);
                        while(i < m_clipboardDoc->lBlocks->count() - 1 &&  m_clipboardDoc->lBlocks->at(i + 1)->type != BASECELL)
                        {
                            i++;
                            if(m_clipboardDoc->lBlocks->at(i)->type == SPACE)
                                str += ' ';
                            else if(m_clipboardDoc->lBlocks->at(i)->type == TEXT)
                                str += static_cast<EjTextBlock*>(m_clipboardDoc->lBlocks->at(i))->text;
                        }
                        str += "</p></td>";

                        if(colum == curTable->nColums() - 1)
                        {
                            str += "\n </tr> \n ";
                            if(row < curTable->nRows() -1)
                                str += "<tr> \n";
                        }
                    }
                    break;

                    //                case CHECK:
                    //                    str += " <td><p>";
                    //                    str += "</p></td>";
                    //                    break;
                case SPACE:
                    str += ' ';
                    break;
                case ENTER:
                    str += " <br> \n";
                default:
                    break;
                }
            }

            if(isTable)
            {
                str += " </table> \n";
            }
            else
                str += " </p> \n";
            str += " </body> \n  </html> ";
            //            clipboard->setText(str);
            //            QMimeData mimeData;
//            mimeData.setHtml(str);
//            clipboard->setMimeData(&mimeData);
            //            m_htmlBuffer = str;


        }
        res = false;
        if(command == "Cut")
        {
            if(m_startSelectBlock > -1 || m_endSelectBlock > -1)
            {
                inputBackSpace();
                calc(0);
            }
        }
        else if(command == "Del")
        {
            inputBackSpace();
            calc(0);
//            //            int n_blockEnd = m_endSelectBlock;
//            //            int n_blockStart = m_startSelectBlock;
//            //            activeBlock = m_startSelectBlock;
//            //            res = splitText(n_blockStart,m_startSelectPos);
//            //            if(res) {
//            //                m_startSelectBlock++;
//            //                m_endSelectBlock++;
//            //                n_blockEnd++;
//            //            }
//            //            res = splitText(n_blockEnd,m_endSelectPos);
//            for(int i = startSelectBlock; i <= endSelectBlock; i++)
//            {
//                if(doc->lBlocks->at(i)->type == EXT_TABLE)
//                    continue;
//                if(doc->lBlocks->at(i)->type == BASECELL)
//                {
////                    EjCellBlock *curCell = (EjCellBlock*)doc->lBlocks->at(i);
////                    curCell->vid = EjCellBlock::CELL_AUTO;
////                    curCell->text = "";
////                    curCell->formula = "";
//                }
//                else
//                {
//                    cur_Block = doc->lBlocks->takeAt(i);
//                    updateFragments(i,false);
//                    //                    updateFragments(i,false);
//                    m_endSelectBlock--;
//                    endSelectBlock--;
//                    i--;
//                    delete cur_Block;
//                    cur_Block = 0;
//                }
//            }
//            position = 0;
//            if(activeIndex > doc->lBlocks->count()-1) activeIndex = doc->lBlocks->count()-1;
//            if(doc->lBlocks->count() > 0 && doc->lBlocks->at(activeIndex)->type == TEXT)
//            {
//                cur_textBlock = (EjTextBlock*)doc->lBlocks->at(activeIndex);
//                position = cur_textBlock->text.size();
//            }
//            res = true;
        }
    }
    else if(command == "Past")
    {
        if(activeIndex > -1 && doc->lBlocks->at(activeIndex)->type == EXT_TABLE)
            return false;



       // return pasteCells();
        m_createPatchEnabled = false;
        EjCellBlock *curCell;
        int count;
        e_statusMode statusMode_bak;
        QString txt;
        QStringList list;
        int  numTable, numRow, numColum;
        bool isFormula;
        EjCalculator calculator(doc);


        if(activeIndex > -1 && isTable(activeIndex))
        {
//            while(doc->lBlocks->at(activeIndex)->type != BASECELL)
//                activeIndex--;
//            curCell = (EjCellBlock*)doc->lBlocks->at(activeIndex);
//            if(doc->lBlocks->at(activeIndex)->type == TEXT)
//                activeIndex++;
//            position = 0;

            if (m_clipboardDoc->lBlocks->at(0)->type != EXT_TABLE && m_clipboardDoc->lBlocks->size() > 1)
            {
                return false;
            }

            int index = activeIndex;
//            doc->lPropBlocks->clear();
//            for(int i = 0; i < m_clipboardDoc->lPropBlocks->count(); i++)
//            {
//                EjBlock *curBlock = m_clipboardDoc->lPropBlocks->at(i);
//                curBlock = curBlock->makeCopy();
//                doc->lPropBlocks->append(curBlock);
//            }
//            doc->calcProps();
            while(doc->lBlocks->at(index)->type != BASECELL)
                index--;
            curCell = (EjCellBlock*)doc->lBlocks->at(index);
//            curCell->text = "";
            curCell->vid = EjCellBlock::CELL_AUTO;
            statusMode_bak = m_statusMode;
            m_statusMode = EDIT_CELL;
            //            if(mimeData->html() == m_htmlBuffer)
            if(mimeData && mimeData->text() == m_htmlBuffer)
            {
                for(int i = 0; i < m_clipboardDoc->lBlocks->count(); i++)
                {
                    cur_Block = m_clipboardDoc->lBlocks->at(i);
                    if(cur_Block->type == EXT_TABLE)
                    {
                        EjTableBlock *curTable = (EjTableBlock*)m_clipboardDoc->lBlocks->at(i);
                       // curTable->calc();
                        EjTableBlock *curActiveTable = ((EjTableBlock*)(curCell->parent));
                        EjCellBlock *curCell2;
                        TableFragment *curFragment = NULL;
                        int active_row, active_colum;
                        int activeBlockStart;
                        int nColums = 0;
                        int nRows = 0;
                        int startRow = 0, endRow = 0, startColum = 0, endColum = 0;
                        if(curTable->vid == EjTableBlock::SHOP_LIST)
                        {
//                            foreach(TableFragment *fragment, *curTable->lFragments)
//                            {
//                                if(fragment->type == EjFragment::Select)
//                                {
//                                    startRow = fragment->startRow();
//                                    endRow = fragment->endRow();
//                                    startColum = fragment->startColum();
//                                    endColum = fragment->endColum();
//                                    nColums = endColum - startColum + 1;
//                                    nRows = endRow - startRow + 1;
//                                    curFragment = fragment;
//                                }
//                            }

                        }
                        else
                        {
                            nColums = curTable->nColums();
                            nRows = curTable->nRows();
                            startRow = startColum = 0;
                            endRow = nRows - 1;
                            endColum = nColums - 1;
                        }
                        getBaseCellParams(activeIndex, active_row, active_colum);
                        while(active_colum + nColums > curActiveTable->nColums())
                        {
                            curActiveTable->addColum(this, curCell);
                        }
                        while(active_row + nRows > curActiveTable->nRows())
                        {
                            curActiveTable->addString(this, curCell);
                        }
                        activeBlockStart = activeIndex;
                        for(int row = 0; row < curTable->nRows(); row++)
                        {

                            for(int colum = 0; colum < curTable->nColums(); colum++)
                            {
                                //                                i++;
                                if(colum < startColum || colum > endColum || row < startRow || row > endRow)
                                    continue;
                                activeIndex = tableCellIndex(curActiveTable,row + active_row,colum + active_colum,doc->lBlocks);
                                i = tableCellIndex(curTable,row,colum, m_clipboardDoc->lBlocks);
                                curCell = (EjCellBlock*)doc->lBlocks->at(activeIndex);
                                curCell2 = (EjCellBlock*)m_clipboardDoc->lBlocks->at(i);
                                //curCell2->copyCell(curCell);
                                while(doc->lBlocks->at(activeIndex+1)->type != END_GROUP && doc->lBlocks->at(activeIndex+1)->type != BASECELL)
                                {
                                    delete doc->lBlocks->takeAt(activeIndex + 1);
                                    //doc->lBlocks->removeAt(activeIndex + 1);
                                    updateFragments(activeIndex,false);
                                }
                                if(curCell->vid == EjCellBlock::CELL_FORMULA)
                                {
                                    list.clear();
                                    txt.clear();
                                    QString formula = curCell->formula();
                                    for(int i = 0; i < formula.count(); i++)
                                    {

                                        if(calculator.is_split(formula[i].toLatin1()) )
                                        {
                                            if(!txt.isEmpty())
                                                list << txt;
                                            txt = formula[i];
                                            list << txt;
                                            txt.clear();
                                        }
                                        else txt += formula[i];
                                    }
                                    if(!txt.isEmpty())
                                        list << txt;
                                    for(int i2 = 0; i2 < list.count(); i2++)
                                    {
                                        txt = list.at(i2);
                                        //                                        isEdit = false;
                                        if(txt == "SUMM" || txt == "MAX" || txt == "MIN")
                                            continue;
                                        isFormula = tableLinkParams(txt, curActiveTable, curActiveTable, numTable, numRow, numColum);
                                        if(isFormula && numTable == curActiveTable->key)
                                        {
                                            numRow = numRow - curTable->start_row + active_row;
                                            if(numRow < 1)
                                                numRow = 1;
                                            numColum = numColum - curTable->start_column + active_colum;
                                            if(numColum < 0)
                                                numColum = 0;
                                            txt = "";
                                            txt += (QString::number(numColum) + 'A');
                                            txt += QString::number(numRow);
                                            list[i2] = txt;
                                        }
                                    }
                                    txt = "";
                                    for(int i2 = 0; i2 < list.count(); i2++)
                                    {
                                        txt += list.at(i2);
                                    }
                                    curCell->setFormula(txt);
                                }

                                if(row == curTable->nRows() - 1 && colum == curTable->nColums() - 1 && statusMode_bak == EDIT_CELL
                                        && curCell->vid == EjCellBlock::CELL_FORMULA)
                                    curCell->setText(curCell->formula());
                                while(m_clipboardDoc->lBlocks->at(i+1)->type != e_typeBlocks::END_GROUP && m_clipboardDoc->lBlocks->at(i+1)->type != BASECELL)
                                {
                                    i++;
                                    cur_Block = m_clipboardDoc->lBlocks->at(i)->makeCopy();
                                    if(cur_Block->type == TEXT)
                                    {
                                        cur_textBlock = (EjTextBlock*)cur_Block;
                                        //                                        activeBlock++;
                                        //                                        doc->lBlocks->insert(activeBlock,new EjTextBlock(cur_textBlock->text));
                                        //                                        updateTablesParams(activeBlock);
                                        inputText(cur_textBlock->text);
                                    }
                                    else if(cur_Block->type == SPACE)
                                    {
                                        activeIndex++;
                                        doc->lBlocks->insert(activeIndex, new EjSpaceBlock());
                                        updateFragments(activeIndex, true);
                                        //                                        inputSpace();
                                    }
                                    else if (cur_Block->type == NUM_STYLE)
                                    {
                                        EjNumStyleBlock *block_style = (EjNumStyleBlock*)cur_Block;
                                        if (block_style->style->m_vid == e_PropDoc::CELL_STYLE)
                                        {
                                            EjCellStyle *cell_style = new EjCellStyle();
                                            cell_style->setTopBorder(curCell->cellStyle->topBorder());
                                            cell_style->setLeftBorder(curCell->cellStyle->leftBorder());
                                            cell_style->setBottomBorder(curCell->cellStyle->bottomBorder());
                                            cell_style->setRightBorder(curCell->cellStyle->rightBorder());
                                            cell_style->setBrushColor(curCell2->cellStyle->m_brushColor);
                                            block_style->style = cell_style;
                                            activeIndex++;
                                            doc->lBlocks->insert(activeIndex, block_style);

                                            doc->lPropBlocks->append(cell_style);
                                            doc->lStyles->append(cell_style);

                                        }else if (block_style->style->m_vid == e_PropDoc::TEXT_STYLE)
                                        {
                                            EjTextStyle *text_style = (EjTextStyle*)block_style;
                                            block_style->style = text_style;
                                            //setTextStyle(text_style);
//                                            activeIndex++;
//                                            doc->lBlocks->insert(activeIndex, block_style);
                                        }else if (block_style->style->m_vid == e_PropDoc::PARAGRAPH_STYLE)
                                        {
                                            EjParagraphStyle *p_style = (EjParagraphStyle*)block_style;
                                            //setParagraphStyle(p_style);

//                                            activeIndex++;
//                                            doc->lBlocks->insert(activeIndex, block_style);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if(cur_Block->type == ENTER)
                        {
                            //                            count = curCell->parent->nRows() * curCell->parent->nColums();
                            count = ((EjTableBlock*)(curCell->parent))->m_counts;
                            //                            activeBlock += curCell->parent->nColums();
                            if(activeIndex + ((EjTableBlock*)(curCell->parent))->nColums() > doc->lBlocks->indexOf(curCell->parent) + 1 + count)
                            {
                                addTableString();
                            }
                            activeIndex += ((EjTableBlock*)(curCell->parent))->nColums();
                            curCell = (EjCellBlock*)doc->lBlocks->at(activeIndex);
//                            curCell->text = "";
                            curCell->vid = EjCellBlock::CELL_AUTO;
                            position = 0;
                        }
                        else if(cur_Block->type == TEXT)
                        {

                            cur_textBlock = (EjTextBlock*)m_clipboardDoc->lBlocks->at(i);
                            inputText(cur_textBlock->text);
                        }
                        else if(cur_Block->type == SPACE)
                        {
                            inputSpace();
                        }
                    }
                }
            }
            else
            {
                if(mimeData && (mimeData->hasHtml() || mimeData->hasText()))
                {
                    //                    QStringList lStr;  //=  mimeData->text().split(QChar::Tabulation);
                    //                    QStringList ltext = mimeData->text().split(" ");
                    //                    for(int i = 0; i < ltext.count(); i++)
                    //                    {
                    //                        if(i > 0)
                    //                            inputSpace();
                    //                        lStr =  ltext[i].split(QChar::Tabulation);
                    //                        for(int j = 0; j < lStr.count(); j++)
                    //                        {
                    //                            if(j > 0)
                    //                            {
                    //                                activeBlock++;
                    //                                while(doc->lBlocks->at(activeBlock)->type != BASECELL)
                    //                                    activeBlock++;
                    //                                curCell = (BaseCellBlock*)doc->lBlocks->at(activeBlock);
                    //                            }
                    //                            inputText(lStr[j]);
                    //                        }
                    //                    }
                    //                    curCell->text = mimeData->text();
//                    int index = activeIndex;
//                    while(doc->lBlocks->at(index)->type != BASECELL)
//                        index--;
//                    curCell = (EjCellBlock*)doc->lBlocks->at(index);
//                    EjTableBlock *curTable = ((EjTableBlock*)(curCell->parent));
                    int row, colum;
                    int base_colum;
                    EjTableBlock *curTable = ((EjTableBlock*)(curCell->parent));

                    cellParams(curTable,activeIndex,row,base_colum);

                    txt = mimeData->text();
                    QChar chr;
//                    curCell->clearData(doc);
//                    while(doc->lBlocks->at(activeIndex + 1)->type != BASECELL)
//                    {
//                        delete doc->lBlocks->takeAt(activeIndex + 1);
//                        updateFragments(activeIndex,false);
//                    }
//                    position = 0;

                    for(int i = 0; i < txt.count(); i++)
                    {
                        chr = txt[i];
                        if(chr == QChar::Space)
                        {
                            inputSpace();

                        }
                        else if(chr == QChar::Tabulation)
                        {
                            cellParams(curTable,activeIndex,row,colum);
                            colum++;
                            if(colum > curTable->nColums() - 1)
                                addTableColum();
                            activeIndex = tableCellIndex(curTable,row,colum,doc->lBlocks);
                            while(doc->lBlocks->at(activeIndex + 1)->type != END_GROUP && doc->lBlocks->at(activeIndex + 1)->type != BASECELL)
                            {
                                delete doc->lBlocks->takeAt(activeIndex + 1);
                                updateFragments(activeIndex,false);
                            }
                            position = 0;
                        }
                        else if(chr == QChar::CarriageReturn)
                        {
                            cellParams(curTable,activeIndex,row,colum);
                            row++;
                            if(row > curTable->nRows() - 1)
                                addTableString();
                            activeIndex = tableCellIndex(curTable,row,base_colum,doc->lBlocks);
//                            while(doc->lBlocks->at(activeIndex + 1)->type != BASECELL)
//                            {
//                                delete doc->lBlocks->takeAt(activeIndex + 1);
//                                updateFragments(activeIndex,false);
//                            }
                            position = 0;

                        }
                        else if(chr != QChar::LineFeed)
                            inputText(chr);
                    }
                }

            }

            m_statusMode = statusMode_bak;

        }
        else
        {
            if(activeIndex > -1)
                splitText(activeIndex,position);
            int i = 0;
            //        str.clear();
            //        for(i = 0; i < m_lClipboardBlocks.size(); i++)
            //        {
            //            cur_Block = m_lClipboardBlocks[i]->makeCopy();
            //            switch (cur_Block->type) {
            //            case TEXT:
            //                str += static_cast<EjTextBlock*>(cur_Block)->text;
            //                break;
            //            case SPACE:
            //                str += ' ';
            //                break;
            //            case ENTER:
            //                str += '\n';
            //            default:
            //                break;
            //            }

            //        }
            //        QString mimeStr = mimeData->text();
            //            if(mimeData->html() == m_htmlBuffer)
            if(mimeData && mimeData->text() == m_htmlBuffer && !0)
            {
                TableFragment *curFragment;
                for(i = 0; i < m_clipboardDoc->lBlocks->count(); i++)
                {
                    cur_Block = m_clipboardDoc->lBlocks->at(i)->makeCopy();
                    if(cur_Block->type == EXT_TABLE)
                    {
//                        int select_index = -1;
//                        for(int i = 0; i < ((EjTableBlock*)cur_Block)->lFragments.count(); i++)
//                        {
//                            if(((EjTableBlock*)cur_Block)->lFragments[i]->type == EjFragment::Select)
//                            {
//                                select_index = i;
//                            }
//                        }
//                        if(select_index > -1)
//                        {
//                            curFragment = ((EjTableBlock*)cur_Block)->lFragments.takeAt(select_index);
//                            delete curFragment;
//                            curFragment = NULL;
//                        }

                    }
                    doc->lBlocks->insert(activeIndex,cur_Block);
                    updateFragments(activeIndex,true);
                    activeIndex++;
                    position = 0;
                }
                updateTables(doc);
                calcTables();

                if(i > 0)
                {
                    activeIndex--;
                    if(activeIndex > -1 && doc->lBlocks->at(activeIndex)->type==TEXT)
                    {
                        position = static_cast<EjTextBlock*>(doc->lBlocks->at(activeIndex))->text.size();
                    }
                }
            }
            else
            {
                if(activeIndex > -1)
                    activeIndex--;
                if(mimeData && ( mimeData->hasHtml() || mimeData->hasText()))
                {
                    str = mimeData->text();
                }
                else
                    str.clear();
                QStringList ltext = str.split(" ");
                for(int i = 0; i < ltext.count(); i++)
                {
                    if(i > 0)
                        inputSpace();
                    str = ltext[i];
                    QStringList ltext2 = str.split('\n');
                    for(int j = 0; j < ltext2.count(); j++)
                    {
                        if(j > 0)
                            inputEnter();
                        str = ltext2[j];
                        if(str != "")
                            inputText(str);
                    }
//
                }
                //            inputText(str);
            }
        }
        m_createPatchEnabled = true;

    }
    else
    {
        delete fragment;
        fragment = 0;
        res = false;
    }
    if(fragment) {
        updateTables(doc);
        //        m_lFragments.append(fragment);
    }
    return res;
}

QString EjTextControl::isTell()
{
    bool res = false;
    int j;
    int start;
    int end;
    bool exit = false;
    QString str;
    int startSelectBlock = m_startSelectBlock;
    int endSelectBlock = m_endSelectBlock;
    int startSelectPos = m_startSelectPos;
    int endSelectPos = m_endSelectPos;
    if( startSelectBlock < 0 || endSelectBlock < 0 )
    {
        if(activeIndex < 0 || doc->lBlocks->isEmpty())
            return str;
        EjTableBlock *table = isTable(activeIndex);
        if(table)
        {
            startSelectBlock = activeIndex;
            if(getBlocks()->at(startSelectBlock)->type != BASECELL)
            {
                startSelectBlock = table->prevCell(startSelectBlock);
            }
            endSelectBlock = table->nextCell(startSelectBlock);
            if(endSelectBlock < 0)
                endSelectBlock = startSelectBlock;
            startSelectPos = 0;
            if(doc->lBlocks->at(endSelectBlock)->type == TEXT)
                endSelectPos = dynamic_cast<EjTextBlock*>(doc->lBlocks->at(endSelectBlock))->text.count();
        }
        else
        {
            if(doc->lBlocks->at(activeIndex)->type != TEXT)
                return str;
            startSelectBlock = activeIndex;
            while(startSelectBlock - 1 > 0 && doc->lBlocks->at(startSelectBlock - 1)->type == TEXT)
                startSelectBlock--;
            startSelectPos = 0;
            endSelectBlock = startSelectBlock;
            while(endSelectBlock + 1 < doc->lBlocks->count() && doc->lBlocks->at(endSelectBlock + 1)->type == TEXT)
                endSelectBlock++;
            endSelectPos = dynamic_cast<EjTextBlock*>(doc->lBlocks->at(endSelectBlock))->text.count();
        }

    }
    str = "";
    for(int i = startSelectBlock; i <= endSelectBlock; i++)
    {
        if(exit)
            break;
        if(doc->lBlocks->at(i)->type == TEXT || doc->lBlocks->at(i)->type == BASECELL)
            str += ((EjTextBlock*)doc->lBlocks->at(i))->text;
        else
        {
            res = false;
            if(doc->lBlocks->at(i)->type == SPACE || doc->lBlocks->at(i)->isProperty())
            {}
//                str = "";
            else
                break;
        }
        if(str.count() > 50)
        {
            res = false;
            break;
        }
        start = 0;
        end = str.count();
        if(i == startSelectBlock && startSelectPos < start)
            start = startSelectPos;
        if(i == endSelectBlock && endSelectPos < end)
            end = endSelectPos;
        for(j = start; j < end; j++)
        {

            if ((str.at(j) >= QChar('0') && str.at(j) <= QChar('9')) || str.at(j) == QChar('(') || str.at(j) == QChar(')') || str.at(j) == QChar('-') || str.at(j) == QChar('+')  || str.at(j) == QChar(' '))
                res = true;
            else
            {
                res = false;
                exit = true;
                break;
            }
        }
    }
//    if(res && m_selectMode == NO_SELECTED)
//    {
////        setSelectMode(SELECTED);
//        m_startSelectBlock = startSelectBlock;
//        m_endSelectBlock = endSelectBlock;
//        m_startSelectPos = startSelectPos;
//        m_endSelectPos = endSelectPos;
//    }
    if(!res)
        str = "";
    return str;
}

QList<EjFragmentBlock *> EjTextControl::getActualFragments(int block, EjTableBlock *cur_Table, int row, int colum)
{
    EjFragmentBlock *curFragment;
    TableFragment *curTableFragment;
    QList<EjFragmentBlock*> lActualFragments;
    //    Param param;
    QList<EjFragmentBlock*> *cur_lFragments = 0;
    //    QList<quint8>lKeys;
    //    BaseCellBlock *cur_baseCell;

    bool bInsert;
    bool bIsBaseCell = false;
    ////    int start = 0;
    ////    int row = 0;
    ////    int colum = 0;
    //    if(cur_Table)
    //    {
    //        cur_lFragments = (QList<EjFragmentBlock*> *)&cur_Table->lFragments;
    //        bIsBaseCell = true;
    ////        cellParams(cur_Table,block,row,colum);
    ////        start = doc->lBlocks->indexOf(cur_shopList) +1;
    ////        if(cur_shopList->nColums() > 0)
    ////            row = (block - start) / cur_shopList->nColums();
    ////        colum = block - start - row*cur_shopList->nColums();
    //    }
    //    else
    //    {
    //        cur_lFragments = &m_lFragments;
    //    }
    ////    if(!cur_lFragments)
    ////        return mActualParams;

    //    for(int j = 0; j < cur_lFragments.count(); j++)
    //    {
    //        curFragment = cur_lFragments.at(j);
    //        bInsert = false;
    //        if(bIsBaseCell)
    //        {
    //            curTableFragment = (TableFragment*)curFragment;
    //           if(row >= curTableFragment->startRow() && row <= curTableFragment->endRow()
    //              && colum >= curTableFragment->startColum() && colum <= curTableFragment->endColum() )
    //           {
    //               bInsert = true;
    //           }
    //        }
    //        else if(block >= curFragment->startBlock && block <= curFragment->endBlock)
    //        {
    //            bInsert = true;
    //        }
    //        if(bInsert)
    //        {
    ////            lKeys = curFragment->mParams.keys();
    ////            for(int jj=0; jj<lActualFragments.size(); jj++)
    ////            {
    ////                switch (lKeys[jj])
    //                switch(curFragment->type) {
    //                case EjFragment::DBold:
    //                    removeFragments(&lActualFragments, EjFragment::Bold);
    //                    break;
    //                case EjFragment::DItalic:
    //                    removeFragments(&lActualFragments, EjFragment::Italic);
    ////                    mActualParams.remove(EjFragment::Italic);
    //                    break;
    //                case EjFragment::DUnderline:
    //                    removeFragments(&lActualFragments, EjFragment::Underline);
    ////                    mActualParams.remove(EjFragment::Underline);
    //                    break;
    //                default:
    //                    removeFragments(&lActualFragments, curFragment->vid);
    //                    lActualFragments.append(curFragment);
    ////                    mActualParams.insert(lKeys[jj],curFragment->mParams.value(lKeys[jj]));
    //                    break;
    ////                }
    //            }
    //        }
    //    }
    return lActualFragments;

}

QList<EjFragmentBlock *> EjTextControl::getSelectFragments()
{
    EjFragmentBlock *curFragment;
    QList<EjFragmentBlock*> lActualFragments;
    ////    QMap<quint8,Param> mActualParams;
    ////    Param param;
    //    //    QList<quint8>lKeys;

    //    for(int j = 0; j < m_lFragments.count(); j++)
    //    {
    //        curFragment = m_lFragments.at(j);
    //        if(m_startSelectBlock >= curFragment->startBlock && m_endSelectBlock <= curFragment->endBlock)
    //        {
    //            lActualFragments.append(curFragment);

    ////            switch(curFragment->vid) {
    ////                case EjFragment::DBold:
    ////                    removeFragments(&lActualFragments, EjFragment::Bold);
    ////                    break;
    ////                case EjFragment::DItalic:
    ////                    removeFragments(&lActualFragments, EjFragment::Italic);
    ////                    break;
    ////                case EjFragment::DUnderline:
    ////                    removeFragments(&lActualFragments, EjFragment::Underline);
    ////                    break;
    ////                default:
    ////                    removeFragments(&lActualFragments, curFragment->vid);
    ////                    lActualFragments.append(curFragment);
    ////                    break;
    ////                }
    //        }
    //    }
    return lActualFragments;

}

void EjTextControl::removeFragments(QList<EjFragmentBlock *> *l_fragments, int vid)
{
    for(int i = 0; i < l_fragments->count(); i++)
    {
        if(l_fragments->at(i)->vid == vid)
        {
            l_fragments->removeAt(i);
            i--;
        }
    }

}

//QMap<quint8, Param> EjTextControl::getActualParams(int block)
//{
//    EjFragment *curFragment;
//    TableFragment *curTableFragment;
//    QMap<quint8,Param> mActualParams;
//    Param param;
//    QList<EjFragment*> *cur_lFragments = 0;
////    QList<quint8>lKeys;
//    BaseCellBlock *cur_baseCell;
//    EjTableBlock *cur_Table;
//    bool bInsert;
//    bool bIsBaseCell = false;
//    int start = 0;
//    int row = 0;
//    int colum = 0;
//    if(doc->lBlocks->at(block)->type == BASECELL)
//    {
//        cur_baseCell = (BaseCellBlock *)doc->lBlocks->at(block);
//        cur_Table = cur_baseCell->parent;
//        if(!cur_Table)
//            return mActualParams;
//        cur_lFragments = (QList<EjFragment*> *)&cur_Table->lFragments;
//        bIsBaseCell = true;
//        cellParams(cur_Table,block,row,colum);
////        start = doc->lBlocks->indexOf(cur_shopList) +1;
////        if(cur_shopList->nColums() > 0)
////            row = (block - start) / cur_shopList->nColums();
////        colum = block - start - row*cur_shopList->nColums();
//    }
//    else
//    {
//        cur_lFragments = m_lFragments;
//    }
////    if(!cur_lFragments)
////        return mActualParams;

//    for(int j = 0; j < cur_lFragments.size(); j++)
//    {
//        curFragment = cur_lFragments.at(j);
//        bInsert = false;
//        if(bIsBaseCell)
//        {
//            curTableFragment = (TableFragment*)curFragment;
//           if(row >= curTableFragment->startRow() && row <= curTableFragment->endRow()
//              && colum >= curTableFragment->startColum() && colum <= curTableFragment->endColum() )
//           {
//               bInsert = true;
//           }
//        }
//        else if(block >= curFragment->startBlock && block <= curFragment->endBlock)
//        {
//            bInsert = true;
//        }
//        if(bInsert)
//        {
////            lKeys = curFragment->mParams.keys();
////            for(int jj=0; jj<lKeys.size(); jj++)
////            {
////                switch (lKeys[jj])
//                switch(curFragment->type) {
//                case EjFragment::DBold:
//                    mActualParams.remove(EjFragment::Bold);
//                    break;
//                case EjFragment::DItalic:
//                    mActualParams.remove(EjFragment::Italic);
//                    break;
//                case EjFragment::DUnderline:
//                    mActualParams.remove(EjFragment::Underline);
//                    break;
//                default:
//                    mActualParams.insert(curFragment->type,param);
////                    mActualParams.insert(lKeys[jj],curFragment->mParams.value(lKeys[jj]));
//                    break;
//                }
////            }
//        }
//    }
//    return mActualParams;
//}

//QMap<quint8, Param> EjTextControl::getSelectParams()
//{
//    EjFragment *curFragment;
//    QMap<quint8,Param> mActualParams;
//    Param param;
////    QList<quint8>lKeys;

//    for(int j = 0; j < lFragments.size(); j++)
//    {
//        curFragment = lFragments.at(j);
//        if(m_startSelectBlock >= curFragment->startBlock && m_endSelectBlock <= curFragment->endBlock)
//        {
////            lKeys = curFragment->mParams.keys();
////            for(int jj=0; jj<lKeys.size(); jj++)
////            {
////                switch (lKeys[jj])
//                switch(curFragment->type){
//                case EjFragment::DBold:
//                    mActualParams.remove(EjFragment::Bold);
//                    break;
//                case EjFragment::DItalic:
//                    mActualParams.remove(EjFragment::Italic);
//                    break;
//                case EjFragment::DUnderline:
//                    mActualParams.remove(EjFragment::Underline);
//                    break;
//                default:
//                    mActualParams.insert(curFragment->type,param);
////                    mActualParams.insert(lKeys[jj],curFragment->mParams.value(lKeys[jj]));
//                    break;
//                }
////            }
//        }
//    }
//    return mActualParams;

//}

QFontMetrics EjTextControl::getDrawMetrics(int block)
{
    //    QMap<quint8,Param>

    //    QFont drawFont = currentFont;
    ////    m_isViewDoc = true;
    ////    if(m_isViewDoc)
    ////        drawFont.setPixelSize(12* 0.32);
    //    QList<EjFragmentBlock*> lActualFragments = getActualFragments(block);
    ////    QList<quint8> lKeys = mActualParams.keys();
    //    for(int jj=0; jj<lActualFragments.count(); jj++)
    //    {
    //        switch (lActualFragments[jj]->vid) {
    //        case EjFragment::Bold:
    //            drawFont.setBold(true);
    //            break;
    //        case EjFragment::Italic:
    //            drawFont.setItalic(true);
    //            break;
    //        case EjFragment::Underline:
    //            drawFont.setUnderline(true);
    //            break;
    //        default:
    //            break;
    //        }

    //    }
    //    return QFontMetrics(drawFont);
    return QFontMetrics(getTextStyle(block)->m_font);
}

EjTextStyle *EjTextControl::getTextStyle(int block) const
{
    if (doc == nullptr){
        return nullptr;
    }

    return doc->currentTextStyle(block);
}

EjTextStyle *EjTextControl::getSelectedTextStyle(int block) const
{
    int resIndex;
    if(m_startSelectBlock > -1 && m_endSelectBlock > -1)
    {
        resIndex = m_startSelectBlock;
        bool bFindfText = false;
        while(resIndex < doc->lBlocks->count() && resIndex < m_endSelectBlock)
        {
            if(doc->lBlocks->at(resIndex)->type == TEXT)
            {
                bFindfText = true;
                break;
            }
            resIndex++;
        }
        if(!bFindfText)
            resIndex = m_startSelectBlock;
    }
    else
    {
        resIndex = block;
//        bool isEndText = false;
//        if(resIndex > -1 && resIndex < doc->lBlocks->count() && doc->lBlocks->at(resIndex)->type == TEXT)
//        {
//            EjTextBlock *block = dynamic_cast<EjTextBlock*>(doc->lBlocks->at(resIndex));
//            if(block && block->text.count() <= position)
//            {
//                isEndText = true;
//            }
//        }
//        if(!isEndText || block < endText(block))
//            resIndex = startText(block);
    }
    return doc->currentTextStyle(resIndex);
}

EjParagraphStyle *EjTextControl::getParagraphStyle(int block)
{
    return doc->currentParagraphStyle(block);
}



QRect EjTextControl::selectArea()
{
    int x1,y1,x2,y2;
    EjBlock *cur_Block;
    EjString *cur_String = 0;
    int index;
    x1=x2=y1=y2=0;
    if(m_startSelectBlock >= 0 && m_startSelectBlock < doc->lBlocks->count())
    {
        cur_Block = doc->lBlocks->at(m_startSelectBlock);
        x1 = cur_Block->x + m_contentX;
        y1 = cur_Block->y + m_contentY - cur_Block->ascent - cur_Block->descent;
        index = doc->lBlocks->indexOf(cur_Block);
        cur_String = wichString(index);
        if(cur_String)
        {
            y1 = cur_String->y - cur_String->height;
        }
        if(cur_Block->type == TEXT)
        {
            QString txt = static_cast<EjTextBlock*>(cur_Block)->text;
            QFontMetrics drawMetrics = getDrawMetrics(m_startSelectBlock);
            if(m_startSelectPos > 0 && cur_Block->type == TEXT)
            {
                txt = txt.left(m_startSelectPos);
                x1 = x1 + drawMetrics.horizontalAdvance(txt);
            }
        }
    }
    if(m_endSelectBlock >= 0 && m_endSelectBlock < doc->lBlocks->count())
    {
        cur_Block = doc->lBlocks->at(m_endSelectBlock);
        x2 = cur_Block->x + m_contentX;
        y2 = cur_Block->y + m_contentY;
        if(cur_Block->type == TEXT)
        {
            QString txt = static_cast<EjTextBlock*>(cur_Block)->text;
            QFontMetrics drawMetrics = getDrawMetrics(m_endSelectBlock);
            if(m_endSelectPos > 0 && cur_Block->type == TEXT)
            {
                txt = txt.left(m_endSelectPos);
                x2 = x2 + drawMetrics.horizontalAdvance(txt);
            }
        }
    }
    QRect res;
    res.setCoords(x1,y1,x2,y2);
    return res;

}

void EjTextControl::updateFragments(int index, bool is_add, bool posNotNul)
{
    EjFragmentBlock *curFragment;
    int del_index;




    //    foreach(EjTableBlock *curTable, *doc->lTables)
    //    {
    //        if(index > curTable->startBlock && index <= curTable->endBlock)
    //        {
    //            if(!is_add)
    //            {
    //                curTable->endBlock--;
    //                curTable->countBlocks--;
    //            }
    //            else
    //            {
    //                curTable->endBlock++;
    //                curTable->countBlocks++;
    //            }

    //        }
    //        else if(index <= curTable->startBlock)
    //        {
    //            if(!is_add)
    //            {
    //                if(index < curTable->startBlock)
    //                    curTable->startBlock--;
    //                curTable->endBlock--;
    //            }
    //            else
    //            {
    //                curTable->endBlock++;
    //                curTable->startBlock++;

    //            }
    //        }
    //    }


    //    for (int ii = 0; ii < m_lFragments.count(); ii++) {
    //        curFragment = m_lFragments.at(ii);
    //        if(!posNotNul)
    //        {
    //            if(curFragment->endBlock >= index)
    //            {
    //                if(is_add) curFragment->endBlock++;
    //                else curFragment->endBlock--;
    //            }
    //            if(curFragment->startBlock >= index)
    //            {
    //                if(is_add) curFragment->startBlock++;
    //                else curFragment->startBlock--;
    //            }
    //        }
    //        else
    //        {
    //            if(curFragment->endBlock >= index)
    //            {
    //                if(is_add) curFragment->endBlock++;
    //                //                    else curFragment->endBlock--;
    //            }
    //            if(curFragment->startBlock > index)
    //            {
    //                if(is_add) curFragment->startBlock++;
    //                //                    else curFragment->startBlock--;
    //            }
    //        }
    //        if(curFragment->endBlock == index && curFragment->startBlock == index)
    //        {
    //            curFragment = m_lFragments.takeAt(ii);
    ////            del_index = doc->lBlocks->indexOf(curFragment);
    //            if(doc->lBlocks->at(curFragment->startBlock)->type == FRAGMENT)
    //                doc->lBlocks->takeAt(curFragment->startBlock);
    //            else
    //                qDebug()  << __FILE__ << __LINE__ << ": " << "Error for EjFragment!!!";
    //            updateFragments(curFragment->startBlock,false);
    ////            doc->lBlocks->removeOne(curFragment);
    //            delete curFragment;
    //            curFragment = 0;
    //            ii = -1;
    ////            ii--;
    //        }
    //        else curFragment->countBlocks = curFragment->endBlock - curFragment->startBlock;
    //    }

}

void EjTextControl::blockOptimize()
{
    EjTextBlock *cur_textBlock;
    //    QMap<quint8,Param> mActualParams;
    //    QMap<quint8,Param> mActualParams2;
    QList<EjFragmentBlock*> lActualFragments;
    QList<EjFragmentBlock*> lActualFragments2;

    for(int i = 0; i < doc->lBlocks->count(); i++)
    {
        if(doc->lBlocks->at(i)->type == TEXT)
        {
            cur_textBlock = (EjTextBlock*)doc->lBlocks->at(i);
            if(i != activeIndex && cur_textBlock->text == "")
            {
                updateFragments(i,false);
                if(m_startSelectBlock >= i) m_startSelectBlock--;
                if(m_endSelectBlock >= i) m_endSelectBlock--;
                doc->lBlocks->removeAt(i);
                if(activeIndex > i) activeIndex--;
                i--;
                continue;
            }
            lActualFragments = getActualFragments(i);
            if(i+1 < doc->lBlocks->count()) lActualFragments2 = getActualFragments(i+1);
            if(i+1 < doc->lBlocks->count() && doc->lBlocks->at(i+1)->type == TEXT && lActualFragments == lActualFragments2)
            {
                if(m_startSelectBlock >= i+1) {
                    if(m_startSelectBlock == i+1) m_startSelectPos += cur_textBlock->text.size();
                    m_startSelectBlock--;
                }

                if(m_endSelectBlock >= i+1) {
                    if(m_endSelectBlock == i+1) m_endSelectPos += cur_textBlock->text.size();
                    m_endSelectBlock--;
                    //                    m_endSelectPos += cur_textBlock->text.size();
                }
                cur_textBlock->text += static_cast<EjTextBlock*>(doc->lBlocks->at(i+1))->text;
                updateFragments(i+1,false);
                doc->lBlocks->removeAt(i+1);
                if(activeIndex > i+1) activeIndex--;
                if(i+1 < doc->lBlocks->count()) lActualFragments2 = getActualFragments(i+1);
                i--;
            }
        }
    }
    if(activeIndex > doc->lBlocks->count() - 1) activeIndex = doc->lBlocks->count() - 1;

}

void EjTextControl::fragmentOptimize(int vid)
{
    //    EjFragmentBlock *curFragment;
    //    EjFragmentBlock *curFragment2;
    ////    EjBlock *cur_Block;
    ////    int vid2 = vid;
    //    int index;
    //    for(int i = 0; i < m_lFragments.count(); i++)
    //    {
    //        curFragment = m_lFragments.at(i);
    //        if(curFragment->vid == vid)
    //        {
    ////            index = curFragment->startBlock - 1;
    ////            while(index > -1 && doc->lBlocks->at(index))
    //            for(int j = i + 1; j < m_lFragments.count(); j++)
    //            {
    //                curFragment2 = m_lFragments.at(j);
    //                if(curFragment2->vid == vid)
    //                {
    //                    if(curFragment2->endBlock >= curFragment->startBlock && curFragment2->startBlock <= curFragment->endBlock)
    //                    {
    //                        if(curFragment->startBlock > curFragment2->startBlock)
    //                        {
    //                            curFragment = curFragment2;
    //                            curFragment2 = m_lFragments.at(i);
    //                            j = i;
    ////                            curFragment->endBlock = curFragment2->endBlock;
    //                        }
    //                        if(curFragment->endBlock < curFragment2->endBlock)
    //                            curFragment->endBlock = curFragment2->endBlock;
    //                        curFragment->countBlocks = curFragment->endBlock - curFragment->startBlock;
    //                        if(doc->lBlocks->at(curFragment2->startBlock)->type == FRAGMENT)
    //                            doc->lBlocks->takeAt(curFragment2->startBlock);
    //                        else
    //                            qDebug()  << __FILE__ << __LINE__ << ": " << "Error for EjFragment!!!";
    //                        updateFragments(curFragment2->startBlock,false);

    //                        if(curFragment2->startBlock < m_startSelectBlock)
    //                            m_startSelectBlock--;
    //                        if(curFragment2->startBlock < m_endSelectBlock)
    //                            m_endSelectBlock--;
    //                        delete m_lFragments.takeAt(j);



    //                        curFragment2 = 0;
    //                        i--;
    //                        break;
    //                    }
    //                }
    //            }
    //        }
    //    }

}

void EjTextControl::fragmentDOptimize(int vid, int startBlock, int endBlock)
{
    //    EjFragmentBlock *curFragment;
    //    EjFragmentBlock *curFragment2;
    ////    EjBlock *cur_Block;
    ////    int vid2 = vid;
    //    int index;
    //    for(int i = 0; i < m_lFragments.count(); i++)
    //    {
    //        curFragment = m_lFragments.at(i);
    //        if(curFragment->vid == vid)
    //        {
    //            if(curFragment->startBlock >= startBlock && curFragment->startBlock <= endBlock)
    //            {
    //                if(doc->lBlocks->at(curFragment->startBlock)->type == FRAGMENT)
    //                    doc->lBlocks->takeAt(curFragment->startBlock);
    //                else
    //                    qDebug()  << __FILE__ << __LINE__ << ": " << "Error for EjFragment!!!";
    //                updateFragments(curFragment->startBlock,false);
    //                if(curFragment->endBlock > endBlock)
    //                {
    //                  doc->lBlocks->insert(endBlock-1, curFragment);
    //                  curFragment->startBlock = endBlock - 1;
    //                  updateFragments(curFragment->startBlock,true);
    //                }
    //                else
    //                {
    //                    if(curFragment->startBlock < m_startSelectBlock)
    //                        m_startSelectBlock--;
    //                    if(curFragment->startBlock < m_endSelectBlock)
    //                        m_endSelectBlock--;
    //                    delete m_lFragments.takeAt(i);
    //                }
    //            }
    //            else if(curFragment->startBlock < startBlock && curFragment->endBlock >= startBlock)
    //            {
    //                if(curFragment->endBlock > endBlock)
    //                {
    //                    curFragment2 = new EjFragmentBlock();
    //                    curFragment2->vid = vid;
    //                    doc->lBlocks->insert(endBlock + 1, curFragment2);
    //                    updateFragments(endBlock + 1, true);
    //                    curFragment2->startBlock = endBlock + 1;
    //                    curFragment2->endBlock = curFragment->endBlock;
    //                    curFragment2->countBlocks = curFragment2->endBlock - curFragment2->startBlock;
    //                    m_lFragments.append(curFragment2);
    //                }
    //                curFragment->endBlock = startBlock-1;
    //                curFragment->countBlocks = curFragment->endBlock - curFragment->startBlock;
    //                if(curFragment->endBlock - curFragment->startBlock < 1)
    //                {
    //                    if(doc->lBlocks->at(curFragment->startBlock)->type == FRAGMENT)
    //                        doc->lBlocks->takeAt(curFragment->startBlock);
    //                    else
    //                        qDebug()  << __FILE__ << __LINE__ << ": " << "Error for EjFragment!!!";
    //                    updateFragments(curFragment->startBlock,false);

    //                    if(curFragment->startBlock < m_startSelectBlock)
    //                        m_startSelectBlock--;
    //                    if(curFragment->startBlock < m_endSelectBlock)
    //                        m_endSelectBlock--;
    //                    delete m_lFragments.takeAt(i);
    //                }

    //            }
    //        }
    //    }
    //    blockOptimize();

}

void EjTextControl::calcTables()
{
    EjCalculator calculator(doc);
    calculator.calcTables();
//    calculator.updateFormulas(false);
}

void EjTextControl::moveTable(int dX)
{
    if(m_isViewDoc)
        return;
    EjTableBlock *table = isTable(activeIndex);
    EjCellBlock *curCellBlock;


    if(table)
    {
        //        int start = doc->lBlocks->indexOf(table) + 1;
        int start = table->startCell();
        int x_bak = doc->lBlocks->at(start)->x;
        int x = x_bak + dX / scaleSize;
        float k_scale = 0.0423333 / scaleSize * 0.236;

        if(x > 0 && x > leftColontitul * k_scale)
            x = leftColontitul * k_scale;
        else if(x < 0 && x + table->width < (m_width - rightColontitul) * k_scale) {
            x = (m_width - rightColontitul) * k_scale - table->width ;
        }
        qDebug() << "x= " << x << " " << doc->lBlocks->at(start)->x;
        //         doc->lBlocks->at(start)->x  = x;
        dX = (x - x_bak);
        table->x += dX;
        for(int j = start; j <= table->endBlock(); j++)
        {
            doc->lBlocks->at(j)->x += dX;
            if(doc->lBlocks->at(j)->type == BASECELL)
            {
                curCellBlock = static_cast<EjCellBlock*>(doc->lBlocks->at(j));
//                curCellBlock->txt_x += dX;
            }
        }
        //        calcStrings();
    }
}


QList<EjBlock *> *EjTextControl::getBlocks()
{
    if (doc == nullptr){
        static QList<EjBlock *> empty;
        return &empty;
    }
    return doc->lBlocks;
}


QList<EjString *> *EjTextControl::getStrings()
{
    if (doc == nullptr){
        static QList<EjString *> empty;

        return &empty;
    }

    return doc->lStrings;
}


QList<EjPage *> *EjTextControl::getPages()
{
    if (doc == nullptr){
        static QList<EjPage *> empty;

        return &empty;
    }

    return doc->lPages;
}

QList<EjBaseStyle *> *EjTextControl::getStyles()
{
    if (doc == nullptr){
        static QList<EjBaseStyle *> empty;

        return &empty;
    }

    return doc->lStyles;
}


QList<EjTableBlock *> *EjTextControl::getTables()
{
    if (doc == nullptr){
        static QList<EjTableBlock *> empty;

        return &empty;
    }

    return doc->lTables;
}


int EjTextControl::getBaseWidth(int index, QFontMetrics &drawMetric)
{
    int res = 0;
    EjTextBlock *cur_txtBlock;
    ContactBlock *cur_cntBlock;
//    ImageBlock_old *cur_imgBlock;

    switch(doc->lBlocks->at(index)->type)
    {
    case TEXT:
        drawMetric = getDrawMetrics(index);
        cur_txtBlock = (EjTextBlock*)doc->lBlocks->at(index);
        if(cur_txtBlock)
        {
            res = drawMetric.horizontalAdvance(cur_txtBlock->text);
        }
        break;
    case CONTACT:
        drawMetric = getDrawMetrics(index);
        cur_cntBlock = (ContactBlock*)doc->lBlocks->at(index);
        if(cur_cntBlock)
        {
            res = drawMetric.horizontalAdvance(cur_cntBlock->name);

        }
        break;
    case SPACE:
        res = 5;
        break;
    }
    return res * 100 * 0.347;

}


void EjTextControl::calc(int index, bool force)
{

    if (doc == nullptr){
        return;
    }

    int index_string = 0;
    int index_page = 0;
    EjBlock *cur_Block;
    EjPage *cur_page;
    EjString *cur_string;

    EjCalcParams calcParams;
    calcParams.viewScale = scaleSize;

    int right_pos = 0;
    int left_pos = 0;
    int back_type = 0;

    bool new_string = false;
    double k_scale;
    int deltaX = 0;
    int x; // + metric.height() / 1.2;

    m_defaultOrientation = doc->attributes()->getDocLayout()->docOrientation();
    m_defaultPageWidth = doc->attributes()->getDocLayout()->docWidth();
    m_defaultPageHeight = doc->attributes()->getDocLayout()->docHeight();
    m_defaultLeftMarging = doc->attributes()->getDocMargings()->left();
    m_defaultTopMarging = doc->attributes()->getDocMargings()->top();
    m_defaultRightMarging = doc->attributes()->getDocMargings()->right();
    m_defaultBottomMarging = doc->attributes()->getDocMargings()->bottom();

    for (EjPage* page: *doc->lPages){
        page->width = m_defaultPageWidth;
        page->height = m_defaultPageHeight;
        page->orientation = m_defaultOrientation;
        page->leftMarging = m_defaultLeftMarging;
        page->topMarging = m_defaultTopMarging;
        page->rightMarging = m_defaultRightMarging;
        page->bottomMarging = m_defaultBottomMarging;
    }

    if(m_calcIndex > doc->lBlocks->count() - 1)
        m_calcIndex = doc->lBlocks->count() - 1;
    if(doc->lPages->isEmpty())
    {
        cur_page = new EjPage;
        cur_page->width = m_defaultPageWidth;
        cur_page->height = m_defaultPageHeight;
        cur_page->orientation = m_defaultOrientation;
        //        cur_page->y = 50;
        doc->lPages->append(cur_page);
    }
    else cur_page = doc->lPages->at(0);

    if(doc->lStrings->isEmpty())
    {
        cur_string = new EjString;
        if(m_isViewDoc){
            if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
                cur_string->y = cur_page->topMarging;
            }
            else{
                cur_string->y = cur_page->leftMarging;
            }
        }
        else{
            //            cur_string->y = topColontitul  * k_scale / scaleSize * 0.236; // + metric.height() / 1.2;
            cur_string->y = 0; // + metric.height() / 1.2;
        }
        doc->lStrings->append(cur_string);
    }
    else {
        index_string = 0;
        cur_string = doc->lStrings->at(0);
        for(int i = 0; i < doc->lStrings->count(); i++)
        {
            if(index >= doc->lStrings->at(i)->startBlock) {
                cur_string = doc->lStrings->at(i);
                index_string = i;
				break;
            }
        }
    }

//    foreach (EjPage *page, doc->lPages) {
    for (int i = 0; i < doc->lPages->count(); i++) {
        EjPage *page = doc->lPages->at(i);
        if(cur_string->y > page->y && cur_string->y < page->y + page->height) {
            cur_page = page;
//            index_page = doc->lPages->indexOf(cur_page);
            index_page = i;

        }
        else if(cur_string->y < page->y && !force)
            break;
        if(force)
            page->flag_redraw = true;
        //        cur_page->width = 20500;
    }

    int baseY = 0;

    if(m_isViewDoc) {
        k_scale = scaleSize;
         right_pos = 0;
        if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
            right_pos = (cur_page->width - cur_page->rightMarging + leftColontitul );
            left_pos = leftColontitul + cur_page->leftMarging;
            baseY = m_defaultTopMarging;
        }
        else{
            right_pos = (cur_page->height - cur_page->topMarging + leftColontitul );
            left_pos = leftColontitul + cur_page->bottomMarging;
            baseY = m_defaultLeftMarging;
        }

    }
    else {
        k_scale = 0.0423333;  //23.622; // 1/4.23333*100
         right_pos = m_width * 0.236  * k_scale / scaleSize - rightColontitul;
        left_pos = leftColontitul;
    }
    if(force) {
        //        fragmentOptimize();
        //        blockOptimize();
    }

    x = left_pos;
    cur_page->x = leftColontitul;
    if(force)
        cur_page->flag_redraw = true;
    //    cur_page->x =  cur_page->x = leftColontitul * scaleSize * (m_width - leftColontitul / 100 - rightColontitul / 100) / m_width;
    //    qDebug() << "baseY: " << baseY << doc->lBlocks->count();

    if(cur_string == doc->lStrings->at(0)) {
        index = 0;
        if(doc->lBlocks->count() > 0)
        {
            cur_Block = doc->lBlocks->at(0);
        }
    }
    else
    {
        index = cur_string->startBlock;
        if(doc->lBlocks->at(index)->type == ENTER)
        {
            index++;
            if(index > doc->lBlocks->count() - 1)
                index = doc->lBlocks->count() - 1;
        }
    }

    calcParams.viewScale = k_scale;
    calcParams.leftColontitul = x; //leftColontitul;
    calcParams.rightPosition = right_pos;
    calcParams.index_page = index_page;
    calcParams.index_string = index_string;
    calcParams.textStyle = getTextStyle(index);
    calcParams.paragraphStyle = getParagraphStyle(index);
    calcParams.control = this;
    calcParams.baseY = baseY;
    calcParams.interval = calcParams.textStyle->m_fontMetrics.height() * 100 * 0.347 * 0.5;
    calcParams.isViewDoc = m_isViewDoc;
    calcParams.statusMode = m_statusMode;
    calcParams.force = force;

    cur_string =  doc->lStrings->at(index_string);
    cur_string->width = 0;

    m_interval = 0;

    int indexBack = index;

    for(int i = index; i <= m_calcIndex; i++)
    {
        cur_Block = doc->lBlocks->at(i);
        if(force)
            cur_Block->flag_redraw = true;
        //        continue;

        calcParams.baseX = x;
        if(cur_Block->type == ENTER)
        {
            cur_Block->descent = calcParams.textStyle->m_fontMetrics.descent() * 100 * 0.347;
            cur_Block->ascent = calcParams.textStyle->m_fontMetrics.ascent() * 100 * 0.347;
            x = left_pos;
            cur_Block->x = x;
//            if(indexBack != i)
            {
                new_string = true;
                indexBack = i;
//                i--;
            }
        }
        else
        {
            indexBack = i;
            cur_Block->calcBlock(i, &calcParams);
        }

        if(x > left_pos && x + cur_Block->width > right_pos)
         {
            new_string = true;
        }
        else
        {
            cur_Block->x = x;
            x += cur_Block->width;
            cur_string->width += cur_Block->width;
        }
        if(new_string)
        {

            calcParams.control->calcString(cur_string, cur_page, &calcParams);
            calcParams.control->addString(&calcParams, indexBack);
            cur_page = doc->lPages->at(calcParams.index_page);
            if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
                right_pos = (cur_page->width - cur_page->rightMarging + leftColontitul );
                left_pos = leftColontitul + cur_page->leftMarging;
            }
            else{
                right_pos = (cur_page->height - cur_page->topMarging + leftColontitul );
                left_pos = leftColontitul + cur_page->bottomMarging;
            }
            index_string = calcParams.index_string;
            cur_string =  doc->lStrings->at(index_string);
            baseY = calcParams.baseY;
            new_string = false;
            x = calcParams.baseX;
            cur_Block->x = x;
            cur_string->width = cur_Block->width;
            x += cur_Block->width;
        }

        cur_string->endBlock = i;
        if(cur_string->endBlock - cur_string->startBlock > 1000)
        {
            qWarning() << "Error for string" << __FILE__ << __LINE__ << ": " << doc->lBlocks->count() << i << doc->lStrings->count() << index_string << cur_string->startBlock << cur_string->endBlock;

        }

        if(i < doc->lBlocks->count())
            back_type = cur_Block->type;

    }
    //    qWarning() << "Time for calc 1" << __FILE__ << __LINE__ << ": " << QDateTime::currentDateTime().msecsTo(dt) << doc->lBlocks->count() << index << force;

    cur_string = doc->lStrings->at(index_string);
    calcString(cur_string, cur_page, &calcParams);
    //            cur_string->y = baseY;
    if(m_isViewDoc)
    {
        int pageWorkHeight = 0;
        if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
            pageWorkHeight = cur_page->GetNormalHeight() - cur_page->bottomMarging;
        }
        else {
            pageWorkHeight = cur_page->GetNormalHeight() - cur_page->rightMarging;
        }

        if (cur_string->y > cur_page->y + pageWorkHeight){
            cur_page = new EjPage;
            cur_page->width = m_defaultPageWidth;
            cur_page->height = m_defaultPageHeight;
            cur_page->orientation = m_defaultOrientation;

            if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
                right_pos = (cur_page->width - cur_page->rightMarging + leftColontitul );
                left_pos = leftColontitul + cur_page->leftMarging;
                baseY += (cur_page->bottomMarging) + 1500;
            }
            else{
                right_pos = (cur_page->height - cur_page->topMarging + leftColontitul );
                left_pos = leftColontitul + cur_page->bottomMarging;
                baseY += (cur_page->rightMarging) + 1500;
            }


            cur_page->y = baseY;
            doc->lPages->append(cur_page);
            if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
                baseY += (cur_page->topMarging );
            }
            else{
                baseY += (cur_page->leftMarging );
            }
            for(int i = cur_string->startBlock; i <= cur_string->endBlock; i++)
            {
                //            doc->lBlocks->at(i)->y = baseY + cur_string->height - doc->lBlocks->at(i)->height;
                doc->lBlocks->at(i)->y = baseY;
            }
        }
    }
    index_string++;
    while (index_string < doc->lStrings->count()) {
        delete doc->lStrings->at(index_string);
        doc->lStrings->removeAt(index_string);
    }
    //    calcStrings();
    if(m_isViewDoc)
    {
        cur_page = doc->lPages->last();
        if(m_height != cur_page->y + cur_page->GetNormalHeight())
        {
            m_height = cur_page->y + cur_page->GetNormalHeight();
            emit controlHeightChanged();
        }
    }
    else
        if(m_height != baseY + cur_string->height + bottomColontitul * k_scale / scaleSize * 0.236)
        {
            m_height = baseY + cur_string->height + bottomColontitul * k_scale / scaleSize * 0.236;
            if(!doc->lBlocks->isEmpty() && doc->lBlocks->last()->type == ENTER)
            {
                m_height += cur_string->height;//calcParams.textStyle->m_fontMetrics.height() * 100 * 0.347 * 1.5;
            }
            //        m_height = baseY + bottomColontitul * k_scale / scaleSize * 23.6;
            emit controlHeightChanged();
        }

}


QQuickItem *EjTextControl::getItem(EjBlock *cur_Block, EjCalcParams *calcParams, QQuickItem *parent)
{
    return NULL;
//    QQuickItem *curItem = NULL;
//    switch(cur_Block->type)
//    {
//    case NUM_STYLE:
//        //                    if(((EjNumStyleBlock *)cur_block)->style && ((EjNumStyleBlock *)cur_block)->style->type == TEXT_STYLE)
//        //                    {
//        //                        curTextStyle = (EjTextStyle*)((EjNumStyleBlock *)cur_block)->style;
//        //                    }
//        //                    else
//        //                    {
//        //                        foreach(EjBaseStyle *style,*plStyles)
//        //                        {
//        //                            if(style->num == ((EjNumStyleBlock *)cur_block)->num)
//        //                            {
//        //                                if(style->type == TEXT_STYLE)
//        //                                {
//        //                                    curTextStyle = (EjTextStyle*)style;
//        //                                    break;
//        //                                }

//        //                            }

//        //                        }
//        //                    }
//        if(((EjNumStyleBlock *)cur_Block)->style->type == TEXT_STYLE)
//            calcParams->textStyle = getTextStyle(doc->lBlocks->indexOf(cur_Block));
//        break;
//    case TEXT:
//    {
//        //                    if(m_statusMode == SELECTED)
//        //                    if(curTextStyle->m_brushColor.rgba() != 0 || (m_statusMode == SELECTED && i >= m_startSelectBlock && i <= m_endSelectBlock))
//        {

//            curItem = new ItemSelectedBlock(parent);
//            EjTextBlock *cur_txtBlock = (EjTextBlock*)cur_Block;
//            int x_select = 0;
//            int x_end_select = cur_Block->width * m_scaleSize + 1;

//            //                        curItem->setY(cur_block->string->y * m_scaleSize + m_contentY);
//            curItem->setY(cur_Block->y * m_scaleSize + m_contentY);
//            //                        curItem->setWidth(cur_block->width * m_scaleSize);
//            //                        curItem->setX(cur_block->x * m_scaleSize + m_contentX);
//            if(i == m_startSelectBlock && (cur_Block->type == TEXT || cur_Block->type == BASECELL) && m_startSelectPos > 0)
//            {
//                x_select = calcParams->textStyle->m_fontMetrics.width(cur_txtBlock->text.left(m_startSelectPos));
//                x_select *= 100 * 0.347 * m_scaleSize;
//                //                            curItem->setWidth(cur_block->width * m_scaleSize - x_select);
//            }
//            if(i == m_endSelectBlock && (cur_Block->type == TEXT || cur_Block->type == BASECELL) )
//            {
//                //                            QFontMetrics fm(drawFont);
//                x_end_select = cur_Block.width(cur_txtBlock->text.left(m_endSelectPos));
//                x_end_select *= 100 * 0.347 * m_scaleSize;
//            }
//            curItem->setX(cur_Block->x * m_scaleSize + x_select + m_contentX);
//            curItem->setWidth(x_end_select - x_select);

//            ((ItemSelectedBlock*)curItem)->pBlock = cur_block;
//            if(i >= m_startSelectBlock && i <= m_endSelectBlock) {
//                curItem->setHeight(cur_block->string->height * m_scaleSize);
//                ((ItemBlock*)curItem)->m_backGround = QColor("#bbdcec");
//            }
//            else {
//                curItem->setHeight(cur_Block.height() * 100 * 0.347 * m_scaleSize);
//                ((ItemBlock*)curItem)->m_backGround = curTextStyle->m_brushColor;
//            }
//            ((ItemSelectedBlock*)curItem)->m_strikeOut = curTextStyle->m_font.strikeOut();
//            ((ItemSelectedBlock*)curItem)->m_underLine = curTextStyle->m_font.underline();
//            ((ItemSelectedBlock*)curItem)->m_lineColor = curTextStyle->m_fontColor;
//            //                        qWarning() << "ItemSelectedBlock" << cur_block->height << cur_block->width;
//            //                        ((ItemSelectedBlock*)curItem)->m_backGround = QColor("#bbdcec");
//        }
//        curItem = new ItemText(((EjTextBlock*)cur_Block)->text, curTextStyle->m_font, curTextStyle->m_fontColor, cur_Block->width / (100 * 0.347),this);
//        //                    curItem = new ItemText(((EjTextBlock*)cur_block)->text, drawFont, Qt::black, cur_block->width / (100 * 0.347),this);
//        //                    if(m_statusMode == SELECTED)
//        //                    {
//        //                       ((ItemText*)curItem)->m_backGround = QColor("#bbdcec");
//        //                    }
//        ((ItemText*)curItem)->pBlock = cur_Block;
//        //                    curItem->setScale(35.3 * m_scaleSize);
//        curItem->setX(cur_Block->x * m_scaleSize + m_contentX);
//        curItem->setY(cur_Block->y * m_scaleSize + m_contentY);
//        //                    curItem->setHeight(cur_block->height);
//        //                    curItem->setWidth(cur_block->width);
//        curItem->setHeight(0);
//        curItem->setWidth(0);
//        curItem->setScale(m_scaleSize*100*0.347);
//        //                    curItem->setClip(true);
//        //                    k = sizeof(*curItem);
//        //                    k = sizeof(*testItem);
//    }
//        break;
//    case BASECELL:
//    {
//        BaseCellBlock *cur_cell = (BaseCellBlock*)cur_block;
//        if(cur_cell->parent)
//        {
//            cellParams(cur_cell->parent,i,row,colum);
//        }
//        else continue;

//        curItem = NULL;
//        if(cur_cell->vid == BaseCellBlock::CHECK && cur_cell->width > 0 && cur_Block->height > 0)
//        {
//            if(!m_check_texture)
//            {
//                m_check_texture = window()->createTextureFromImage(image_check);

//            }
//            curItem = new ItemCheck(((BaseCellBlock*)cur_Block)->parent->spacing, m_check_texture, this);
//            ((ItemCheck*)curItem)->pBlock = cur_Block;
//            ((ItemCheck*)curItem)->m_viewScale = m_scaleSize;
//            curItem->setX(cur_Block->x * m_scaleSize + m_contentX);
//            curItem->setY(cur_Block->y * m_scaleSize + m_contentY);
//            curItem->setHeight(cur_Block->height * m_scaleSize);
//            curItem->setWidth(cur_Block->width * m_scaleSize);
//            if(cur_cell->value > 0 )
//            {
//                ((ItemCheck*)curItem)->setIsChecked(true);
//            }
//            else
//                ((ItemCheck*)curItem)->setIsChecked(false);
//            //                        curItem->update();
//            if(cur_cell->parent->vid == EjTableBlock::CLEANTABLE)
//                ((ItemCheck*)curItem)->isAllBorders = true;
//            else {
//                if(row == 0)
//                    ((ItemCheck*)curItem)->isTopBorder = true;
//                ((ItemCheck*)curItem)->isBottomBorder = true;
//            }
//        }
//        else
//        {
//            curItem = new ItemCell(((BaseCellBlock*)cur_Block)->parent->spacing, ((BaseCellBlock*)cur_Block)->text, drawFont, Qt::black, ((BaseCellBlock*)cur_Block)->txtWidth / (100 * 0.347), this);
//            //                        curItem = new ItemCell(0, ((BaseCellBlock*)cur_block)->text, drawFont, Qt::black, ((BaseCellBlock*)cur_block)->txtWidth / (100 * 0.347), this);
//            ((ItemCell*)curItem)->pBlock = cur_Block;
//            ((ItemCell*)curItem)->m_viewScale = m_scaleSize;

//            //                        curItem->setX(cur_block->x * m_scaleSize + m_contentX);
//            //                        curItem->setY(cur_block->y * m_scaleSize + m_contentY);
//            curItem->setHeight(cur_Block->height * m_scaleSize);
//            curItem->setWidth(cur_Block->width * m_scaleSize);
//            //                        curItem->setHeight(0);
//            //                        curItem->setWidth(0);
//            //                        curItem->setScale(m_scaleSize*100*0.347);
//            curItem->setX(cur_Block->x * m_scaleSize + m_contentX);
//            curItem->setY(cur_Block->y * m_scaleSize + m_contentY);
//            //                        curItem->setHeight(cur_block->height);
//            //                        curItem->setWidth(cur_block->width);
//            //                        curItem->setHeight(0);
//            //                        curItem->setWidth(0);
//            //                        curItem->setScale(m_scaleSize*100*0.347);

//            if(cur_cell->parent->vid == EjTableBlock::CLEANTABLE)
//                ((ItemCell*)curItem)->isAllBorders = true;
//            else {
//                if(row == 0)
//                    ((ItemCell*)curItem)->isTopBorder = true;
//                ((ItemCell*)curItem)->isBottomBorder = true;
//            }
//        }
//        //                    if(row % 2 == 0 && m_statusMode != EDIT_CELL)
//        //                    {
//        //                        ((ItemBlock *)curItem)->m_backGround = QColor(0,0,0,15);
//        //                        //                        curItem->update();
//        //                    }
//    }
//        break;
//        //    case IMAGE:
//        //    {
//        //        ImageBlock_old *cur_imageBlock = (ImageBlock_old*)cur_Block;
//        //        if(cur_imageBlock)
//        //        {
//        ////                        QString path = ext_storage->pathPic + ext_storage->m_login + "/";
//        ////                        QString picname = QString(cur_imageBlock->name.toHex()); //name.toString();  //.remove(QChar('-')).remove(QChar('{')).remove(QChar('}')) + ".jpeg";
//        ////                        QUrl url = path+picname + "s" + ".jpeg";



//        ////                        curItem = new ItemImage(((ImageBlock_old*)cur_block)->small_image,this);
//        ////                        ((ItemImage*)curItem)->pBlock = cur_block;
//        ////                        ((ItemImage*)curItem)->m_viewScale = m_scaleSize;
//        ////                        curItem->setX(cur_block->x * m_scaleSize + m_contentX);
//        ////                        curItem->setY(cur_block->y * m_scaleSize + m_contentY);
//        ////                        curItem->setHeight(cur_block->height * m_scaleSize);
//        ////                        curItem->setWidth(cur_block->width * m_scaleSize);

//        //        }
//        //    }
//        //        break;
//    case SPACE:
//        if(curTextStyle->m_brushColor.rgba() != 0 || (m_statusMode == SELECTED && i >= m_startSelectBlock && i <= m_endSelectBlock) || curTextStyle->m_font.underline() || curTextStyle->m_font.strikeOut() )
//        {
//            curItem = new ItemSelectedBlock(parent);
//            if(i >= m_startSelectBlock && i <= m_endSelectBlock) {
//                curItem->setHeight(cur_Block->string->height * m_scaleSize);
//                //                            ((ItemBlock*)curItem)->m_backGround = QColor("#bbdcec");
//            }
//            else {
//                curItem->setHeight(fm.height() * 100 * 0.347 * m_scaleSize);
//                //                            ((ItemBlock*)curItem)->m_backGround = curTextStyle->m_brushColor;
//            }
//            curItem->setY(cur_Block->y * m_scaleSize + m_contentY);
//            curItem->setWidth(cur_Block->width * m_scaleSize);
//            curItem->setX(cur_Block->x * m_scaleSize + m_contentX);
//            ((ItemSelectedBlock*)curItem)->pBlock = cur_Block;
//            qWarning() << "ItemSelectedBlock" << cur_Block->height << cur_Block->width;
//            ((ItemSelectedBlock*)curItem)->m_strikeOut = curTextStyle->m_font.strikeOut();
//            ((ItemSelectedBlock*)curItem)->m_underLine = curTextStyle->m_font.underline();
//            ((ItemSelectedBlock*)curItem)->m_lineColor = curTextStyle->m_fontColor;
//            //                        ((ItemSelectedBlock*)curItem)->m_backGround = QColor("#bbdcec");
//        }
//        break;
//    default: {
//        bool bFind = false;

//        if(ext_plugins.contains(cur_Block->type)) {
//            JotInterface *iPlug = ext_plugins.value(cur_Block->type);
//            curItem = iPlug->newItem(m_scaleSize, cur_Block, parent);
//            //                        ((ItemImage*)curItem)->pBlock = cur_block;
//            curItem->setX(cur_Block->x * m_scaleSize + m_contentX);
//            curItem->setY(cur_Block->y * m_scaleSize + m_contentY);
//            //                            curItem->setHeight(0);
//            //                            curItem->setWidth(0);

//            //                            curItem->setHeight(cur_block->height);
//            //                            curItem->setWidth(cur_block->width);
//            //                            curItem->setScale(0.99);
//            //                            curItem->setScale(m_scaleSize*100*0.347);
//            //                            curItem->setV(m_scaleSize);

//            bFind = true;


//        }
//        //                    if(!bFind && m_statusMode == SELECTED && i >= m_startSelectBlock && i <= m_endSelectBlock)
//        //                    if(!bFind && (m_statusMode == SELECTED || curTextStyle->m_fontColor.rgba() != 0 ))
//        //                    {
//        //                        curItem = new ItemSelectedBlock(this);
//        //                        if(m_statusMode == SELECTED) {
//        //                            curItem->setHeight(cur_block->string->height * m_scaleSize);
//        //                            ((ItemBlock*)curItem)->m_backGround = QColor("#bbdcec");
//        //                        }
//        //                        else {
//        ////                            curItem->setHeight(fm.height() * 100 * 0.347 * m_scaleSize);
//        ////                            ((ItemBlock*)curItem)->m_backGround = curTextStyle->m_brushColor;
//        //                            curItem->setHeight(cur_block->string->height * m_scaleSize);
//        //                            ((ItemBlock*)curItem)->m_backGround = QColor("#bbdcec");
//        //                        }
//        //                        curItem->setY(cur_block->y * m_scaleSize + m_contentY);
//        //                        curItem->setWidth(cur_block->width * m_scaleSize);
//        //                        curItem->setX(cur_block->x * m_scaleSize + m_contentX);
//        //                        ((ItemSelectedBlock*)curItem)->pBlock = cur_block;
//        //                        qWarning() << "ItemSelectedBlock" << cur_block->height << cur_block->width;
//        ////                        ((ItemSelectedBlock*)curItem)->m_backGround = QColor("#bbdcec");
//        //                    }

//    }
//        break;

//    }
//    return curItem;

}

void EjTextControl::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)

    killTimer(m_timerId);
    createPatch();

}

//int &baseY, int interval, int deltaX

void EjTextControl::calcString(EjString *string, EjPage *page, EjCalcParams *calcParams)
{
    if(doc->lBlocks->isEmpty()) return;
    int deltaX = 0;
    if(calcParams->paragraphStyle->m_align & EjParagraphStyle::AlignLeft)
        deltaX = 0;
    if(calcParams->paragraphStyle->m_align & EjParagraphStyle::AlignRight)
    {
        if(calcParams->isViewDoc){
            if (page->orientation == EjDocLayout::ORN_PORTRAIT){
                // deltaX = calcParams->rightPosition; // - string->width - page->leftMarging; // - calcParams->leftColontitul;
                deltaX = calcParams->rightPosition - string->width - calcParams->leftColontitul; // - page->rightMarging;
            }
            else{
                deltaX = calcParams->rightPosition - string->width - calcParams->leftColontitul; // - calcParams->leftColontitul;
            }
        }
        else
            deltaX = calcParams->rightPosition - string->width - calcParams->leftColontitul;
        if(deltaX < 0)
            deltaX = 0;
    }
    if(calcParams->paragraphStyle->m_align & EjParagraphStyle::AlignHCenter)
    {
        //                deltaX = (m_width) * 0.236 * k_scale / scaleSize - cur_string->width - (leftColontitul + rightColontitul) * 0.236;
        if(calcParams->isViewDoc){
            if (page->orientation == EjDocLayout::ORN_PORTRAIT){
                deltaX = calcParams->rightPosition - string->width - page->leftMarging - calcParams->leftColontitul;
            }
            else{
                deltaX = calcParams->rightPosition - string->width - page->bottomMarging - calcParams->leftColontitul;
            }
        }
        else
            deltaX = calcParams->rightPosition - string->width - calcParams->leftColontitul;
        deltaX *= 0.5;
        if(deltaX < 0)
            deltaX = 0;
    }

    if(string->startBlock < 0)
    {
        QFontMetrics drawMetric = getDrawMetrics(activeIndex);
        string->height = drawMetric.height();
        string->width = 0;

        return;
    }
    EjBlock *cur_Block = doc->lBlocks->at(string->startBlock);
    string->height = cur_Block->ascent + cur_Block->descent;
    string->width = 0;

    {
        string->height = 0;
        string->ascent = 0;
        string->width = 0;
        string->interval = 0;
        for(int i = string->startBlock; i <= string->endBlock; i++)
        {
            cur_Block = doc->lBlocks->at(i);
            cur_Block->x += deltaX;
            string->width += cur_Block->width;
            if(string->height < cur_Block->ascent + cur_Block->descent)
                string->height = cur_Block->ascent + cur_Block->descent;
            if(string->ascent < cur_Block->ascent) string->ascent = cur_Block->ascent;
			if((cur_Block->type == TEXT || cur_Block->type == ENTER) && string->interval < (cur_Block->ascent + cur_Block->descent) * 0.5)
                string->interval = (cur_Block->ascent + cur_Block->descent) * 0.5;
            if(cur_Block->type >= GROUP_BLOCK && !((EjGroupBlock*)cur_Block)->isGlassy())
            {
                i += ((EjGroupBlock*)cur_Block)->m_counts;
            }
        }
        string->height += string->interval;
        for(int i = string->startBlock; i <= string->endBlock; i++)
        {
            cur_Block = doc->lBlocks->at(i);
            if(cur_Block->type == SPACE)
                cur_Block->ascent = string->ascent;
            cur_Block->y = calcParams->baseY;
            cur_Block->interval_top = string->ascent - cur_Block->ascent;
            cur_Block->interval_bottom = string->interval;
            if(cur_Block->type >= GROUP_BLOCK && !((EjGroupBlock*)cur_Block)->isGlassy())
            {
                i += ((EjGroupBlock*)cur_Block)->m_counts;
                doc->lBlocks->at(i)->y = calcParams->baseY;
            }
        }
        string->x = doc->lBlocks->at(string->startBlock)->x;
        string->y = calcParams->baseY;
        calcParams->baseY += string->height;
    }
    //    baseY += height;
}

void EjTextControl::addString(EjCalcParams *calcParams, int indexBlock)
{
    EjPage *cur_page = calcParams->control->doc->lPages->at(calcParams->index_page);
    EjString *cur_string = calcParams->control->doc->lStrings->at(calcParams->index_string);

    int pageWorkHeight = 0;
    if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
        pageWorkHeight = cur_page->GetNormalHeight() - cur_page->bottomMarging;
    }
    else{
         pageWorkHeight = cur_page->GetNormalHeight() - cur_page->rightMarging;
    }

    if(calcParams->isViewDoc && cur_string->y > cur_page->y + pageWorkHeight)
    {
        //                baseY += (cur_page->bottomMarging)/100 + 25;
        calcParams->baseY = cur_page->y + cur_page->GetNormalHeight() + 1500;
        {
            cur_page = new EjPage;
            cur_page->width = m_defaultPageWidth;
            cur_page->height = m_defaultPageHeight;
            cur_page->orientation = m_defaultOrientation;
            cur_page->y = calcParams->baseY;
            cur_page->x = leftColontitul;
            doc->lPages->append(cur_page);
        }
        calcParams->index_page++;
        if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
             calcParams->baseY += (cur_page->topMarging);
        }
        else{
            calcParams->baseY += (cur_page->leftMarging);
        }

        for(int i = cur_string->startBlock; i <= cur_string->endBlock; i++)
        {
//            doc->lBlocks->at(i)->y = calcParams->baseY + cur_string->height - doc->lBlocks->at(i)->height;
            doc->lBlocks->at(i)->y = calcParams->baseY;
        }
        cur_string->y = calcParams->baseY;
        calcParams->baseY += cur_string->height;
    }
//    calcParams->baseY += calcParams->interval;

    calcParams->index_string++;
    if(calcParams->index_string > doc->lStrings->count() - 1)
    {
        doc->lStrings->append(new EjString());
    }
    doc->lStrings->at(calcParams->index_string)->startBlock = indexBlock;
    doc->lStrings->at(calcParams->index_string)->endBlock = indexBlock;
    if(indexBlock > doc->lBlocks->count() - 1)
    {
        doc->lStrings->at(calcParams->index_string)->startBlock = doc->lBlocks->count() - 1;
        doc->lStrings->at(calcParams->index_string)->endBlock = doc->lBlocks->count() - 1;
        //                continue;
    }
    cur_string = doc->lStrings->at(calcParams->index_string);
    cur_string->width = 0;

    if(m_isViewDoc){
        if (cur_page->orientation == EjDocLayout::ORN_PORTRAIT){
            calcParams->baseX = leftColontitul + cur_page->leftMarging;
        }
        else{
            calcParams->baseX = leftColontitul + cur_page->bottomMarging;
        }
    }
    //                x =  cur_page->leftMarging;
    else
        calcParams->baseX = leftColontitul;

}

int EjTextControl::startText(int index) const
{
   int res = index;
//   int lastText = index;
   for(int i = index; i > -1; i--)
   {
       int type = doc->lBlocks->at(i)->type;
       if(!doc->lBlocks->at(i)->isProperty() && type != TEXT)
           break;
       if(type == TEXT)
           res = i;
   }
   return res;
}

int EjTextControl::endText(int index) const
{
    int res = index;
    if(index > -1 && index < doc->lBlocks->count())
    {
        for(int i = index; i < doc->lBlocks->count(); i++)
        {
            int type = doc->lBlocks->at(i)->type;
            if(type == TEXT)
                res = i;
            if(!doc->lBlocks->at(i)->isProperty() && type != TEXT)
                break;
        }
        if(doc->lBlocks->at(res)->type == TEXT)
            res++;
    }
    return res;
}

bool EjTextControl::isEndText(int index) const
{
    bool res = false;
    if(index > -1 && index < doc->lBlocks->count() && doc->lBlocks->at(index)->type == TEXT)
    {
        EjTextBlock *block = dynamic_cast<EjTextBlock*>(doc->lBlocks->at(index));
        if(index == activeIndex && block && block->text.count() <= position)
        {
            res = true;
            index++;
        }
    }
    return res;
}

EjString *EjTextControl::wichString(int index)
{
    EjString *resString = 0;
    foreach(EjString *curString, *doc->lStrings)
    {
        if(index >= curString->startBlock && index <= curString->endBlock)
            resString = curString;
        break;
    }
    return resString;

}
