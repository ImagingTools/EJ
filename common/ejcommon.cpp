/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#include "ejcommon.h"
#include "ejtextcontrol.h"
#include "ejtableblocks.h"
// #include "labelplug.h"
// #include "difft.h"
// #include "storage.h"
// #include "itemtext.h"
#include "IntVarLen2.h"
#include <QLocale>
#include <QGuiApplication>

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (0x01<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(0x01<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (0x01<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (0x01<<(b))))

int writeSmallString(QDataStream &stream, const QString &source)
{
    QByteArray ba = source.toUtf8();
    return  writeBA(stream,ba);
}


void readSmallString(QDataStream &stream, QString &source)
{
    QByteArray ba;
    readBA(stream,ba);
    source = QString::fromUtf8(ba);
}

int writeBA(QDataStream &stream, const QByteArray &source)
{
    quint32 size = (quint32)source.size();
	IntVarLen2 t(size);
    stream << t;
    return stream.device()->write(source.data(),size);
}


void readBA(QDataStream &stream, QByteArray &source)
{
    quint32 size;
	IntVarLen2 t(size);
    QByteArray ba;
    stream >> t;
    source = stream.device()->read(size);
}


double getDValue(QString s_value, bool *bOk)
{
    QLocale locale;
    s_value.remove(locale.groupSeparator());
    double res = s_value.toDouble(bOk);
    if(!*bOk)
        res = locale.toDouble(s_value, bOk);
    return res;
}


QString getDText(double value, int accuracy)
{
    QLocale locale;
    QString res = locale.toString(value,'f',accuracy);
     if(accuracy == 0)
        return res;
    int n = res.count() - 1;
    int i = 0;
    for(i = 0; i< accuracy; i++)
    {
        if(res[n - i] == '0')
            continue;
        else
        {
            break;
        }
    }
    if(i == accuracy)
        i++;
    res.resize(n - i + 1);
    return res;
}




EjBlock *EjPointBlock::makeCopy()
{
    EjPointBlock *res = new EjPointBlock();
    copyData(res);
    res->type = type;
    res->x = x;
    res->y = y;
    return res;
}

bool EjPointBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    //      EjTextBlock *cur_textBlock = &other;
    if(this->x != other.x || this->y != other.y)
        res = false;
    return res;
}

EjBlock *EjPropKeyBlock::makeCopy()
{
    EjPropKeyBlock *res = new EjPropKeyBlock();
    copyData(res);
    res->type = type;
    res->key = key;
    return res;
}

bool EjPropKeyBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
//    KeyBlock *cur_keyBlock = &other;
    if(this->key != ((EjPropKeyBlock*)(&other))->key)
        res = false;
    return res;
}

QDataStream &EjPropKeyBlock::write(QDataStream &os) const {
    EjBlock::write(os);
    os << num << key;
    return os;
}

QDataStream &EjPropKeyBlock::read(QDataStream &is) {
    EjBlock::read(is);
    is >> num >> key;
    return is;
}




//void ComplexBlock::setBlocks(QList<EjBlock *> *source)
//{
//    lBlocks = source;
//}

//void ComplexBlock::setProp(int num, QVariant value)
//{

//}

//EjBlock *ComplexBlock::makeCopy()
//{
//    ComplexBlock *res = new ComplexBlock();
//    copyData(res);
//    return res;
//}

//bool ComplexBlock::compare(const EjBlock &other) const
//{
//    return EjBlock::compare(other);
//}

//QDataStream &ComplexBlock::write(QDataStream &os) const
//{
//    EjBlock::write(os);
//    return os;
//}

//QDataStream &ComplexBlock::read(QDataStream &is)
//{
//    EjBlock::read(is);
//    return is;
//}

//void EjGroupBlock::calcParams(QList<EjBlock *> *lBlocks, bool force)
//{
//    QList<EjBlock*>::iterator iter;
//    EjBlock *cur_block;
//    if(force || m_index == -1 || lBlocks->at(m_index) != this)
//    {
//        m_index = lBlocks->indexOf(this);
//        if(force)
//            m_counts = 0;
//        if(m_counts == 0)
//        {
//            beforeCalc(lBlocks);
//            iter = lBlocks->begin() + m_index + 1;
//            cur_block = (*iter);
//            while(iter != lBlocks->end() && cur_block->type != END_GROUP) {
//                m_counts++;
//                cur_block->parent = this;
//                if(cur_block->type >= GROUP_BLOCK) {
//                    ((EjGroupBlock*)cur_block)->calcParams(lBlocks, force);
//                    iter += ((EjGroupBlock*)cur_block)->m_counts;
//                    m_counts += ((EjGroupBlock*)cur_block)->m_counts;
//                }
//                childCalc(cur_block);
//                iter++;
//                cur_block = (*iter);
//            }
//            if(iter != lBlocks->end() && (*iter)->type == END_GROUP) {
//                (*iter)->parent = this;
//                m_counts++;
//            }
//            afterCalc();
//        }
//    }
//}

EjBlock *EjGroupBlock::findProp(QList<EjBlock*> *lBlocks, int type, int num, bool check)
{
    EjBlock *block = nullptr;
    int index = findPropIndex(lBlocks, type, num, m_index + m_counts, check);
    if(index > -1)
        block = lBlocks->at(index);
    return  block;
}

int EjGroupBlock::findPropIndex(QList<EjBlock *> *lBlocks, int type, int num, int endIndex, bool check)
{
    return findPropIndex(lBlocks, type, num, m_index+1, endIndex, check);
}

int EjGroupBlock::findPropIndex(QList<EjBlock *> *lBlocks, int type, int num, int startIndex, int endIndex, bool check)
{
    int index;
    int res = -1;
    EjBlock *curBlock;
    EjPropBase *curPropBlock;
    EjPropDoc *curPropDoc;
    for(index = startIndex; index < endIndex; index++)
    {
        if(index > lBlocks->count() - 1)
            break;
        curBlock = lBlocks->at(index);
        if(curBlock->type == static_cast<quint8>(type))
        {
            if(curBlock->type == PROP_BIG_TEXT)
            {
                EjPropBigTextBlock *curBigText = dynamic_cast<EjPropBigTextBlock*>(curBlock);
                if(curBigText && curBigText->num == static_cast<quint8>(num))
                    res = index;
                break;

            }
            else if(curBlock->type == ACCESS_RULES)
            {
                EjAccessBlock *curAccesBlock = dynamic_cast<EjAccessBlock*>(curBlock);
                if(curAccesBlock)
                {
                    res = index;
                    break;
                }
            }
            else
            {
                curPropBlock = dynamic_cast<EjPropBase*>(curBlock);
                //        qDebug() << __FILE__ << __LINE__ << curBlock->type << type
                //                 << int(curBlock->type == static_cast<quint8>(type))
                //                 << curBlock->num << num
                //                 << int(curBlock->num == static_cast<quint8>(num));
                if(curPropBlock)
                {
                    if(curPropBlock->num == static_cast<quint8>(num))
                    {
                        if(!check) {
                            res = index;
                            break;
                        }
                        else {
                            if(res < 0)
                            {
                                res = index;
                            }
                            else
                            {
                                remBlock(lBlocks,curPropBlock);
                            }
                        }
                    }
                }
                else
                {
                    curPropDoc = dynamic_cast<EjPropDoc*>(curBlock);
                    if(curPropDoc->num == static_cast<quint8>(num))
                    {
                        if(!check) {
                            res = index;
                            break;
                        }
                        else {
                            if(res < 0)
                            {
                                res = index;
                            }
                            else
                            {
                                remBlock(lBlocks,curPropDoc);
                            }
                        }
                    }
                }
            }

        }
        if(curBlock->type > GROUP_BLOCK)
            index += (dynamic_cast<EjGroupBlock*>(curBlock))->m_counts;
    }
    return res;
}

void EjGroupBlock::addProp(QList<EjBlock*> *lBlocks, EjBlock *block)
{
    lBlocks->insert(m_index + m_counts,block);
    block->parent = this;
    m_counts++;
}

void EjGroupBlock::remBlock(QList<EjBlock *> *lBlocks, EjBlock *block)
{
    if(!block)
        return;
    int index = lBlocks->indexOf(this);
    if(index < 0)
        return;
    m_index = index;
    calcLenght(m_index,lBlocks);
    index = lBlocks->indexOf(block);
    if(index < m_index + 1 && index > m_index + m_counts - 1)
        return;
    if(block->type >= GROUP_BLOCK)
    {
        EjGroupBlock *groupBlock = dynamic_cast<EjGroupBlock*>(block);
        if(groupBlock)
        {
            groupBlock->remFromBlocks(lBlocks);
            delete groupBlock;
            calcLenght(m_index,lBlocks);
        }
    }
    else if(block->type == END_GROUP)
    {
        EjGroupBlock *groupBlock = dynamic_cast<EjGroupBlock*>(block->parent);
        if(groupBlock && groupBlock != this)
        {
            groupBlock->remFromBlocks(lBlocks);
            delete groupBlock;
            calcLenght(m_index,lBlocks);
        }

    }
    else
    {
        delete lBlocks->takeAt(index);
        m_counts--;
    }
}

void EjGroupBlock::createDefault(QList<EjBlock *> *lBlocks, int index)
{
    lBlocks->insert(index,this);
    EjBlock *cur_block;
    m_index = index = lBlocks->indexOf(this);
    index++;
    cur_block = new EjBlock(END_GROUP);
    lBlocks->insert(index,cur_block);
    m_counts = 1;
}

void EjGroupBlock::createDefaultWithNum(QList<EjBlock *> *lBlocks, int index, int num)
{
    m_index = index;
    lBlocks->insert(index,this);
    index++;
    lBlocks->insert(index,new EjPropIntBlock(0));
    ((EjPropIntBlock*)lBlocks->at(index))->value = num;
    index++;
    lBlocks->insert(index,new EjBlock(END_GROUP,this));
    m_counts = 2;
}

void EjGroupBlock::remFromBlocks(QList<EjBlock *> *lBlocks)
{
    QList<EjBlock*>::iterator iter;
    EjBlock *curBlock;
    EjGroupBlock *groupBlock;
    if(lBlocks->at(m_index) != this)
    {
        int index = lBlocks->indexOf(this);
        if(index < 0)
            return;
        m_index = index;
    }
    while (lBlocks->at(m_index + 1)->type != END_GROUP)
    {
        curBlock = lBlocks->at(m_index + 1);
        if(curBlock != this && curBlock->type >= GROUP_BLOCK) {
            groupBlock = dynamic_cast<EjGroupBlock*>(curBlock);
            if(groupBlock)
                groupBlock->remFromBlocks(lBlocks);
            delete groupBlock;
        }
        else delete lBlocks->takeAt(m_index + 1);
    }
    delete lBlocks->takeAt(m_index + 1);
    lBlocks->takeAt(m_index);
}


void EjGroupBlock::calcBlock(int &index, EjCalcParams *calcParams)
{
    QList<EjBlock*>::iterator iter;
    EjBlock *cur_block;
    QList<EjBlock*> *lBlocks = calcParams->control->doc->lBlocks;

    if(calcParams->force || m_index == -1 || lBlocks->at(m_index) != this)
    {
        m_index = index;
        if(calcParams->force)
            m_counts = 0;
        if(m_counts == 0)
        {
            beforeCalc(calcParams);
            this->width = 0;
            iter = lBlocks->begin() + m_index + 1;
            cur_block = (*iter);
            while(iter != lBlocks->end() && cur_block->type != END_GROUP) {
                m_counts++;
                cur_block->parent = this;
                if(cur_block->type >= GROUP_BLOCK) {
                    int index2 = iter - lBlocks->begin();
                    ((EjGroupBlock*)cur_block)->calcBlock(index2, calcParams);
                    iter += ((EjGroupBlock*)cur_block)->m_counts;
                    m_counts += ((EjGroupBlock*)cur_block)->m_counts;
                    this->width += cur_block->width;
                }
                else if(!cur_block->isProperty())
                {
                    int index2 = iter - lBlocks->begin();
                    cur_block->calcBlock(index2, calcParams);
                    this->width += cur_block->width;
                }
                childCalc(cur_block, calcParams);
                iter++;
                cur_block = (*iter);
            }
            if(iter != lBlocks->end() && (*iter)->type == END_GROUP) {
                (*iter)->parent = this;
                m_counts++;
            }
            afterCalc(calcParams);
        }
    }
    index += m_counts;
}

void EjGroupBlock::calcLenght(int &index, QList<EjBlock *> *lBlocks)
{
    QList<EjBlock*>::iterator iter;
    EjBlock *cur_block;
    if(index < 0 || index > lBlocks->count() - 1 || lBlocks->at(index) != this)
    {
        index = lBlocks->indexOf(this);
    }
    m_index = index;
    m_counts = 0;
    iter = lBlocks->begin() + m_index + 1;
    cur_block = (*iter);
    while(iter != lBlocks->end() && cur_block->type != END_GROUP) {
        m_counts++;
        cur_block->parent = this;
        if(cur_block->type >= GROUP_BLOCK) {
            int index2 = iter - lBlocks->begin();
            ((EjGroupBlock*)cur_block)->calcLenght(index2, lBlocks);
            iter += ((EjGroupBlock*)cur_block)->m_counts;
            m_counts += ((EjGroupBlock*)cur_block)->m_counts;
        }
        iter++;
        if(iter != lBlocks->end())
            cur_block = (*iter);
    }
    if(iter != lBlocks->end() && (*iter)->type == END_GROUP) {
        (*iter)->parent = this;
        m_counts++;
    }

}

void EjGroupBlock::clear(QList<EjBlock *> *lBlocks)
{
    m_index = lBlocks->indexOf(this);
    if(m_index == -1)
        return;
    int i = m_index + 1;
    int count_groups = 0;
    EjBlock *curBlock = lBlocks->at(i);
    while(curBlock->type != END_GROUP)
    {
        if(curBlock->type > GROUP_BLOCK)
        {
            count_groups = 1;
            delete lBlocks->takeAt(i);
            m_counts--;
            while(count_groups > 0 && i < lBlocks->count())
            {
                curBlock = lBlocks->at(i);
                if(curBlock->type == END_GROUP)
                    count_groups--;
                else if(curBlock->type > GROUP_BLOCK)
                    count_groups++;
                delete lBlocks->takeAt(i);
                m_counts--;
            }
        }
        else // if(!curBlock->isProperty())
        {
            delete lBlocks->takeAt(i);
            m_counts--;
        }
       // else
         //   i++;
        curBlock = lBlocks->at(i);
    }

}

bool EjMapLabelBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    //      EjTextBlock *cur_textBlock = &other;
    if(this->x != other.x || this->y != other.y)
        res = false;
    return res;
}

void EjMapLabelBlock::setCoords(qint32 x_souce, qint32 y_source)
{
    x = x_souce; y = y_source;
}





EjBlock *EjPropPntBlock::makeCopy()
{
    EjPropPntBlock *res = new EjPropPntBlock();
    copyData(res);
    res->type = type;
    res->num = num;
    res->x_value = x_value;
    res->y_value = y_value;
    return res;
}

bool EjPropPntBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropPntBlock const *cur_propPntBlock;
          cur_propPntBlock = (EjPropPntBlock *)&other;
    if(this->x_value != cur_propPntBlock->x_value || this->y_value != cur_propPntBlock->y_value || this->num != cur_propPntBlock->num)
        res = false;
    return res;
}

QDataStream &EjPropPntBlock::write(QDataStream &os) const {
    quint32 k1 = x_value;
	IntVarLen2 t1(k1);
    quint32 k2 = y_value;
	IntVarLen2 t2(k2);
    EjBlock::write(os);
    os << num << t1 << t2;
    return os;
}

QDataStream &EjPropPntBlock::read(QDataStream &is) {
    quint32 k1;
	IntVarLen2 t1(k1);
    quint32 k2;
	IntVarLen2 t2(k2);
    EjBlock::read(is);
    is >> num >> t1 >> t2;
    x_value = k1; y_value = k2;
    return is;
}

EjBlock *EjPropIntBlock::makeCopy()
{
    EjPropIntBlock *res = new EjPropIntBlock();
    copyData(res);
    res->num = num;
    res->value = value;
    return res;
}

bool EjPropIntBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropIntBlock const *cur_propIntBlock;
          cur_propIntBlock = (EjPropIntBlock *)&other;
    if(this->value != cur_propIntBlock->value || this->num != cur_propIntBlock->num)
        res = false;
    return res;
}

QDataStream &EjPropIntBlock::write(QDataStream &os) const
{
    quint32 k = value;
	IntVarLen2 t1(k);

    EjBlock::write(os);
    os << num << t1;
    return os;
}

QDataStream &EjPropIntBlock::read(QDataStream &is)
{
    quint32 k = value;
	IntVarLen2 t1(k);
    EjBlock::read(is);
    is >> num >> t1;
    value = k;
    return is;
}

EjBlock *EjPropTextBlock::makeCopy()
{
    EjPropTextBlock *res = new EjPropTextBlock();
    copyData(res);
    res->num = num;
    res->text = text;
    return res;
}

bool EjPropTextBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropTextBlock const *cur_propTxtBlock;
          cur_propTxtBlock = (EjPropTextBlock *)&other;
    if(this->num != cur_propTxtBlock->num || this->text != cur_propTxtBlock->text)
        res = false;
    return res;
}

QDataStream &EjPropTextBlock::write(QDataStream &os) const {
    EjBlock::write(os);
    os << num;
    writeSmallString(os,text); // os << text;
    return os;
}



QDataStream &EjPropTextBlock::read(QDataStream &is) {
    EjBlock::read(is);
    is >> num;
    readSmallString(is,text); //        is >> text;
    return is;
}


ItemBlock *VectorText::newItem(qreal viewScale, QQuickItem *parent)
{
    return NULL;
}

QQuickItem *VectorText::newViewItem(int vid, QQuickItem *parent)
{
    return NULL;
}

void TableFragment::setStartRow(quint16 row)
{
    quint32 tmp = row;
    tmp  = tmp << 16;
    tmp += (startBlock & 0xffff);
    startBlock = tmp;
}

void TableFragment::setStartColum(quint16 colum)
{
    quint32 tmp = (startBlock & 0xffff0000);
    tmp += colum;
    startBlock = tmp;

}

void TableFragment::setEndRow(quint16 row)
{
    quint32 tmp = row;
    tmp = tmp << 16;
    tmp += (endBlock & 0xffff);
    endBlock = tmp;
}

void TableFragment::setEndColum(quint16 colum)
{
    quint32 tmp = (endBlock & 0xffff0000);
    tmp += colum;
    endBlock = tmp;
}

TableFragment *TableFragment::makeCopy()
{
    TableFragment * res = new TableFragment();
    res->type = type;
    res->startBlock = startBlock;
    res->endBlock = endBlock;
    return res;
}

bool TableFragment::compare(const EjBlock &other) const
{
    if(!EjFragmentBlock::compare(other)) {
        return false;
    }
    bool res = true;
    TableFragment *curFragment = (TableFragment*)(&other);
    if(startBlock != curFragment->startBlock || endBlock != curFragment->endBlock)
        res = false;
    return res;
}

QDataStream &TableFragment::write(QDataStream &os) const
{
//    os << type << startBlock << endBlock;
//    return os;
    EjBlock::write(os);
    quint32 k1 = startRow();
	IntVarLen2 startR(k1);
    quint32 k2 = startColum();
	IntVarLen2 startC(k2);
    quint32 k3 = endRow();
	IntVarLen2 endR(k3);
    quint32 k4 = endColum();
	IntVarLen2 endC(k4);

    os << startR << startC << endR << endC << vid;
    return os;
}

QDataStream &TableFragment::read(QDataStream &is)
{
//    is >> type >> startBlock >> endBlock;
//     return is;
    EjBlock::read(is);
    quint32 k1;
	IntVarLen2 startR(k1);
    quint32 k2;
	IntVarLen2 startC(k2);
    quint32 k3;
	IntVarLen2 endR(k3);
    quint32 k4;
	IntVarLen2 endC(k4);

    is >> startR >> startC >> endR >> endC >> vid;
    setStartRow(k1);
    setStartColum(k2);
    setEndRow(k3);
    setEndColum(k4);
//    readSmallString(is,text); //        is >> text;
    return is;
}



EjBlock *EjFragmentBlock::makeCopy()
{
    EjFragmentBlock *res = new EjFragmentBlock();
    copyData(res);
    res->type = type;
    res->vid = vid;
    res->countBlocks = countBlocks;
    res->iValue = iValue;
    res->sValue = sValue;
    return res;
}

bool EjFragmentBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    EjFragmentBlock *curFragment = (EjFragmentBlock*)(&other);
    if(vid != curFragment->vid)
        res = false;
    return res;
}

QDataStream &EjFragmentBlock::write(QDataStream &os) const
{
    EjBlock::write(os);
    os << vid;

    return os;
}

QDataStream &EjFragmentBlock::read(QDataStream &is)
{
    EjBlock::read(is);
        is >> vid;

    return is;
}


EjBlock *EndFragmentBlock::makeCopy()
{
    EndFragmentBlock *res = new EndFragmentBlock();
    copyData(res);
    res->type = type;
    res->vid = vid;

    return res;
}

bool EndFragmentBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    EndFragmentBlock *curFragment = (EndFragmentBlock*)(&other);
    if(vid != curFragment->vid)
        res = false;
    return res;
}

QDataStream &EndFragmentBlock::write(QDataStream &os) const
{
    EjBlock::write(os);

    os << vid;
    return os;
}

QDataStream &EndFragmentBlock::read(QDataStream &is)
{
    EjBlock::read(is);

    is >> vid;

    return is;
}


//EjBlock *ImageBlock_old::makeCopy()
//{
//    ImageBlock_old *res = new ImageBlock_old();
//    copyData(res);
//    res->type = type;
//    res->name = name;
//    res->small_image = small_image;
//    return res;
//}

//bool ImageBlock_old::compare(const EjBlock &other) const
//{
//    if(!EjBlock::compare(other)) {
//        return false;
//    }
//    bool res = true;
//    const ImageBlock_old *otherImage = static_cast<const ImageBlock_old*>(&other);
//    if(this->vid != otherImage->vid || this->name != otherImage->name)
//        res = false;
//    if(vid == 1 && (this->m_width_image != otherImage->m_width_image
//                    || this->m_height_image != otherImage->m_height_image
//                    || this->m_is_interactive != otherImage->m_is_interactive
//                    || this->m_show_border != otherImage->m_show_border))
//        res = false;
//    return res;
//}

//QDataStream &ImageBlock_old::write(QDataStream &os) const {
//    EjBlock::write(os);
//    quint8 size = name.size(); os << vid; os << size;
//    os.writeRawData(name.data(),size);
//    switch (vid) {
//    case 1:
//        os << m_width_image << m_height_image << m_is_interactive << m_show_border;
//        break;
//    default:
//        break;
//    }
//    //        QBuffer buffer;
//    //        writeSmallString(os,name);
//    //        buffer.open(QIODevice::WriteOnly);
//    //        small_image.save(&buffer,"JPG");
//    //        quint16 size = buffer.size(); os << size;
//    //        os.writeRawData(buffer.buffer().data(),size);
//    return os;
//}

//QDataStream &ImageBlock_old::read(QDataStream &is) {
//    EjBlock::read(is);
//    //        readSmallString(is,name);
//    is >>vid;
//    quint8 size; is >> size;
//    QByteArray ba;
//    ba.resize(size);
//    is.readRawData(ba.data(),size);
//    name = ba;
//    switch (vid) {
//    case 1:
//        is >> m_width_image >> m_height_image >> m_is_interactive >> m_show_border;
//        break;
//    default:
//        break;
//    }
//    //        QBuffer buffer(&ba);
//    //        small_image.load(&buffer,"JPG");
//    //        height = small_image.height();
//    //        width = small_image.width();
//    //        is >> small_image;
//    return is;
//}


EjTextBlock::~EjTextBlock()
{
    text = "";
}

EjBlock *EjTextBlock::makeCopy()
{
    EjTextBlock *res = new EjTextBlock();
    copyData(res);
    res->type = type;
    res->text = text;
    return res;
}

bool EjTextBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    //      EjTextBlock *cur_textBlock = &other;
    if(this->text != static_cast<const EjTextBlock*>(&other)->text)
        //    if(this->text != cur_textBlock->text)
        res = false;
    return res;
}

QQuickItem *EjTextBlock::getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem)
{
#ifdef USE_QML
    Q_UNUSED(index)
    ItemBlock *curItem = NULL;
    bool isGlassy = false;
    //                    if(m_statusMode == SELECTED)
    //                    if(curTextStyle->m_brushColor.rgba() != 0 || (m_statusMode == SELECTED && i >= m_startSelectBlock && i <= m_endSelectBlock))
//    if(this->string)
    {

        EjTextBlock *cur_txtBlock = this;
        int x_select = 0;
        int x_end_select = this->width * calcParams->viewScale + 1;

        //                        curItem->setY(cur_block->string->y * m_scaleSize + m_contentY);
//        curItem->setY(this->y * calcParams->viewScale + calcParams->contentY);
        //                        curItem->setWidth(cur_block->width * m_scaleSize);
        //                        curItem->setX(cur_block->x * m_scaleSize + m_contentX);
        if(index == calcParams->control->m_startSelectBlock && (this->type == TEXT || this->type == BASECELL) && calcParams->control->m_startSelectPos > 0)
        {
			x_select = calcParams->textStyle->m_fontMetrics.horizontalAdvance(cur_txtBlock->text.left(calcParams->control->m_startSelectPos));
            x_select *= 100 * 0.347 * calcParams->viewScale;
            //                            curItem->setWidth(cur_block->width * m_scaleSize - x_select);
        }
        if(index == calcParams->control->m_endSelectBlock && (this->type == TEXT || this->type == BASECELL) )
        {
            //                            QFontMetrics fm(drawFont);
			x_end_select = calcParams->textStyle->m_fontMetrics.horizontalAdvance(cur_txtBlock->text.left(calcParams->control->m_endSelectPos));
            x_end_select *= 100 * 0.347 * calcParams->viewScale;
        }


        if((calcParams->control->m_statusMode == EDIT_TEXT || calcParams->control->m_statusMode == EDIT_CELL)
                && cur_txtBlock->parent != NULL && cur_txtBlock->parent->type > GROUP_BLOCK && ((EjGroupBlock*)cur_txtBlock->parent)->isGlassy())
        {
            isGlassy = true;
        }

        if(index >= calcParams->control->m_startSelectBlock && index <= calcParams->control->m_endSelectBlock || isGlassy) {
            curItem = new ItemSelectedBlock(parentItem);
            curItem->setHeight((interval_top + ascent + descent + interval_bottom) * calcParams->viewScale);
            curItem->setX(this->x * calcParams->viewScale + x_select + calcParams->contentX);
            curItem->setY(this->y * calcParams->viewScale + calcParams->contentY);
            curItem->setWidth(x_end_select - x_select);
            EjTableBlock *table =  calcParams->control->isTable(index);

            if(!table || table->prevCell(index) < calcParams->control->m_startSelectBlock)
                ((ItemBlock*)curItem)->m_backGround = QColor("#bbdcec");
            if(isGlassy)
                ((ItemBlock*)curItem)->m_backGround = QColor("#ccdc9c");
        }

        else
        {
//            curItem->setHeight(this->height * calcParams->viewScale);
//            curItem->setHeight(this->string->ascent * calcParams->viewScale + 4);
//            ((ItemBlock*)curItem)->m_backGround = calcParams->textStyle->m_brushColor;
        }
        if(curItem)
        {
            ((ItemSelectedBlock*)curItem)->pBlock = this;
            curItem = new ItemSelectedBlock(curItem);
            curItem->setX(-x_select);
//            curItem->setY((this->string->ascent - this->ascent) * calcParams->viewScale);
        }
        else
        {
            curItem = new ItemSelectedBlock(parentItem);
            curItem->setY(this->y * calcParams->viewScale + calcParams->contentY);
//            curItem->setY(this->y * calcParams->viewScale + calcParams->contentY);
            curItem->setX(this->x * calcParams->viewScale + calcParams->contentX);
            ((ItemSelectedBlock*)curItem)->pBlock = this;
        }

//        curItem->setWidth(x_end_select - x_select);
        curItem->setWidth(this->width * calcParams->viewScale);
        curItem->setHeight((interval_top + ascent + descent) * calcParams->viewScale);
//        curItem->setHeight((this->height) * calcParams->viewScale);
        ((ItemBlock*)curItem)->m_backGround = calcParams->textStyle->m_brushColor;

        ((ItemSelectedBlock*)curItem)->height_strike = (interval_top + ascent - calcParams->textStyle->m_fontMetrics.height() * 20 * 0.347) * calcParams->viewScale;
//        ((ItemSelectedBlock*)curItem)->height_under = this->string->ascent * calcParams->viewScale;
        ((ItemSelectedBlock*)curItem)->height_under = (interval_top + ascent + descent) * calcParams->viewScale;
        ((ItemSelectedBlock*)curItem)->m_strikeOut = calcParams->textStyle->m_font.strikeOut();
        ((ItemSelectedBlock*)curItem)->m_underLine = calcParams->textStyle->m_font.underline();
        ((ItemSelectedBlock*)curItem)->m_lineColor = calcParams->textStyle->m_fontColor;
        curItem->m_snapString = true;
        //                        qWarning() << "ItemSelectedBlock" << cur_block->height << cur_block->width;
        //                        ((ItemSelectedBlock*)curItem)->m_backGround = QColor("#bbdcec");
    }
    curItem = new ItemText(this->text, calcParams->textStyle->m_font, calcParams->textStyle->m_fontColor, this->width / (100 * 0.347),parentItem);
    //                    curItem = new ItemText(((EjTextBlock*)cur_block)->text, drawFont, Qt::black, cur_block->width / (100 * 0.347),this);
    //                    if(m_statusMode == SELECTED)
    //                    {
    //                       ((ItemText*)curItem)->m_backGround = QColor("#bbdcec");
    //                    }
    ((ItemText*)curItem)->pBlock = this;
    //                    curItem->setScale(35.3 * m_scaleSize);
    curItem->setX(this->x * calcParams->viewScale + calcParams->contentX);
    curItem->setY((y + interval_top) * calcParams->viewScale + calcParams->contentY);
    //                    curItem->setHeight(cur_block->height);
    //                    curItem->setWidth(cur_block->width);
    curItem->setHeight(0);
    curItem->setWidth(0);
    curItem->setScale(calcParams->viewScale*100*0.347);
    //                    curItem->setClip(true);
    //                    k = sizeof(*curItem);
    //                    k = sizeof(*testItem);
    return curItem;
#else
	return nullptr;
#endif
}

void EjTextBlock::calcBlock(int &index, EjCalcParams *calcParams)
{
//    EjTextBlock *cur_txtBlock = (EjTextBlock*)cur_Block;
    if(!calcParams || !calcParams->textStyle)
        return;
    this->ascent = calcParams->textStyle->m_fontMetrics.ascent() * 100 * 0.347;
    this->descent = calcParams->textStyle->m_fontMetrics.descent() * 100 * 0.347;

        {
            if(this->width == 0 || calcParams->force)
            {
				this->width = calcParams->textStyle->m_fontMetrics.horizontalAdvance(this->text) * 100 * 0.347; //0.236;
                if(calcParams->textStyle->m_font.italic())
                {
                    this->width += (calcParams->textStyle->m_fontMetrics.height() * 0.1 * 100 * 0.347);
                }
                this->flag_redraw = true;

            }
            //                    if(x > leftColontitul * k_scale / scaleSize && x + cur_txtBlock->width > (width - rightColontitul) * k_scale / scaleSize && back_type != TEXT)

        }


}


EjBlock *ContactBlock::makeCopy()
{
    ContactBlock *res = new ContactBlock();
    copyData(res);
    res->type = type;
    res->name = name;
    res->lData = lData;
    return res;
}


bool ContactBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    ContactBlock *otherBlock = (ContactBlock*)(&other);
    if(this->name != otherBlock->name)
        res = false;
    if(res)
    {
        //        if(lData != otherBlock->lData)
        //            res = false;
        foreach (ContactData *contact, this->lData) {
            foreach (ContactData *otherContact, otherBlock->lData)
            {
                if(!contact->compare(*otherContact))
                    return false;
            }
        }
    }
    return res;
}

void ContactBlock::calcBlock(int &index, EjCalcParams *calcParams)
{
    Q_UNUSED(index);
	if(this->width == 0 || calcParams->force) this->width = calcParams->textStyle->m_fontMetrics.horizontalAdvance(this->name);
}


QDataStream &EjFragment::write(QDataStream &os) const
{
    os << type << startBlock << endBlock;

    return os;

}

QDataStream &EjFragment::read(QDataStream &is)
{
    is >> type >> startBlock >> endBlock;

    return is;
}



EjBlock *EjSizeProp::makeCopy()
{
    EjSizeProp *res = new EjSizeProp();
    copyData(res);
    res->num = num;
    res->min = min;
    res->max = max;
    res->current = current;
    res->isMax = isMax;
    return res;
}

bool EjSizeProp::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other))
        return false;
    const EjSizeProp *cur = static_cast<const EjSizeProp*>(&other);
    if(!cur || this->num != cur->num ||  this->min != cur->min
            || this->max != cur->max )
    {
        return false;
    }
    return true;
}

QDataStream &EjSizeProp::write(QDataStream &os) const
{
    EjBlock::write(os);
    quint8 count_prop = 3;
    os << count_prop << num << min << max;
    return os;

}

QDataStream &EjSizeProp::read(QDataStream &is)
{
    quint8 count_prop;

    EjBlock::read(is);
    is >> count_prop >> num >> min >> max;
    return is;
}




EjBlock *EjNumStyleBlock::makeCopy()
{
    EjNumStyleBlock *res = new EjNumStyleBlock();
    copyData(res);
    res->type = type;
    res->x = x;
    res->y = y;
    res->num = num;
    res->style = style;
    return res;
}

bool EjNumStyleBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    EjNumStyleBlock *cur_numStyleBlock = (EjNumStyleBlock *)&other;
//    if(this->style != cur_numStyleBlock->style || this->num != cur_numStyleBlock->num)
    if(this->num != cur_numStyleBlock->num)
        res = false;
    return res;
}

QDataStream &EjNumStyleBlock::write(QDataStream &os) const {
    EjBlock::write(os);
    quint32 k = num;
	IntVarLen2 t(k);
    os << t;
    return os;
}

QDataStream &EjNumStyleBlock::read(QDataStream &is) {
    EjBlock::read(is);
    quint32 k;
	IntVarLen2 t(k);
    is >> t;
    num = (quint16)k;
    return is;
}

QQuickItem *EjNumStyleBlock::getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem)
{
    Q_UNUSED(index);
    Q_UNUSED(parentItem);
//    EjTextStyle *textStyle;
    if(style && style->m_vid == TEXT_STYLE)
    {
//        textStyle = calcParams->control->doc->getTextStyle(index);
//        if(textStyle)
            calcParams->textStyle = (EjTextStyle*)style;
//        calcParams->textStyle = calcParams->control->doc->getTextStyle(calcParams->control->doc->lBlocks.indexOf(this));
    }
    return nullptr;
}

void EjNumStyleBlock::calcBlock(int &index, EjCalcParams *calcParams)
{
    Q_UNUSED(index)
//    EjNumStyleBlock *curNumStyle = (EjNumStyleBlock*)cur_Block;

    if(!this->style) {
        this->style = calcParams->control->doc->getUndefinedStyle(this->num);
        if(!this->style)
            return;
    }
    if(this->style->m_vid == TEXT_STYLE)
    {
        calcParams->textStyle = (EjTextStyle*)(this->style);
        calcParams->textStyle->m_fontMetrics = QFontMetrics(calcParams->textStyle->m_font);
        calcParams->interval = calcParams->textStyle->m_fontMetrics.height() * 100 * 0.347 * 0.5;

    }
    if(this->style->m_vid == PARAGRAPH_STYLE)
    {
        calcParams->paragraphStyle = (EjParagraphStyle*)(this->style);        
    }
}


EjBlock *EjPropInt8Block::makeCopy()
{
    EjPropInt8Block *res = new EjPropInt8Block();
    copyData(res);
    res->num = num;
    res->value = value;
    return res;
}

bool EjPropInt8Block::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropInt8Block const *cur_propIntBlock;
          cur_propIntBlock = (EjPropInt8Block *)&other;
    if(this->value != cur_propIntBlock->value || this->num != cur_propIntBlock->num)
        res = false;
    return res;
}

QDataStream &EjPropInt8Block::write(QDataStream &os) const
{
    EjBlock::write(os);
    os << num << value;
    return os;
}

QDataStream &EjPropInt8Block::read(QDataStream &is)
{
    EjBlock::read(is);
    is >> num >> value;
    return is;
}

EjBlock *EjPropInt64Block::makeCopy()
{
    EjPropInt64Block *res = new EjPropInt64Block();
    copyData(res);
    res->num = num;
    res->value = value;
    return res;
}

bool EjPropInt64Block::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropInt64Block const *cur_propIntBlock;
          cur_propIntBlock = (EjPropInt64Block *)&other;
    if(this->value != cur_propIntBlock->value || this->num != cur_propIntBlock->num)
        res = false;
    return res;
}

QDataStream &EjPropInt64Block::write(QDataStream &os) const
{
    EjBlock::write(os);
    os << num << value;
    return os;
}

QDataStream &EjPropInt64Block::read(QDataStream &is)
{
    EjBlock::read(is);
    is >> num >> value;
    return is;
}

EjBlock *EjPropByteArrayBlock::makeCopy()
{
    EjPropByteArrayBlock *res = new EjPropByteArrayBlock();
    copyData(res);
    res->num = num;
    res->data = data;
    return res;
}

bool EjPropByteArrayBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropByteArrayBlock const *cur_propBABlock;
          cur_propBABlock = (EjPropByteArrayBlock *)&other;
    if(this->num != cur_propBABlock->num || this->data != cur_propBABlock->data)
        res = false;
    return res;
}

QDataStream &EjPropByteArrayBlock::write(QDataStream &os) const
{
    EjBlock::write(os);
    os << num;
    quint8 size = (quint8)data.size();
    os << size;
    os.device()->write(data.data(),size);
    return os;
}

QDataStream &EjPropByteArrayBlock::read(QDataStream &is)
{
    quint8 size;
    EjBlock::read(is);
    is >> num;
    is >> size;
    data = is.device()->read(size);
    return is;
}





EjCalcParams::EjCalcParams()
{
    textStyle = NULL; /*brushStyle = NULL;*/ lineStyle = NULL; paragraphStyle = NULL;
    control = NULL;
    viewScale = 0;
    index_string = 0;
}

EjBlock *EjSpaceBlock::makeCopy()
{
    EjSpaceBlock *res = new EjSpaceBlock();
    copyData(res);
    return res;
}

QQuickItem *EjSpaceBlock::getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem)
{
#ifdef USE_QML
    Q_UNUSED(index)
//    if(!this->string)
//        return NULL;
    bool isGlassy = false;
    if ((calcParams->control->m_statusMode == EDIT_TEXT
         || calcParams->control->m_statusMode == EDIT_CELL)
            && this->parent != NULL && this->parent->type > GROUP_BLOCK && ((EjGroupBlock*)this->parent)->isGlassy())
    {
        isGlassy = true;
    }

    ItemSelectedBlock *curItem = NULL;
    if(calcParams->textStyle->m_brushColor.rgba() != 0 || isGlassy ||
            (index >= calcParams->control->m_startSelectBlock &&
             index <= calcParams->control->m_endSelectBlock) || calcParams->textStyle->m_font.underline() ||
            calcParams->textStyle->m_font.strikeOut() )
    {
        if(index >= calcParams->control->m_startSelectBlock && index <= calcParams->control->m_endSelectBlock || isGlassy) {
            curItem = new ItemSelectedBlock(parentItem);
            curItem->setHeight((interval_top + ascent + descent + interval_bottom) * calcParams->viewScale);
            curItem->setWidth(this->width * calcParams->viewScale);
            curItem->setX(this->x * calcParams->viewScale + calcParams->contentX);
            curItem->setY(this->y * calcParams->viewScale + calcParams->contentY);
            EjTableBlock *table =  calcParams->control->isTable(index);

            if(!table || table->prevCell(index) < calcParams->control->m_startSelectBlock)
                ((ItemBlock*)curItem)->m_backGround = QColor("#bbdcec");
            if(isGlassy)
                ((ItemBlock*)curItem)->m_backGround = QColor("#ccdc9c");
        }
//        else {
//            curItem->setHeight(calcParams->textStyle->m_fontMetrics.height() * 100 * 0.347 * calcParams->viewScale);
//            ((ItemBlock*)curItem)->m_backGround = calcParams->textStyle->m_brushColor;
//            curItem->setHeight(this->string->ascent * calcParams->viewScale + 4);
//        }
        if(curItem)
        {
            curItem = new ItemSelectedBlock(curItem);
        }
        else
        {
            curItem = new ItemSelectedBlock(parentItem);
            curItem->setY(this->y * calcParams->viewScale + calcParams->contentY);
            curItem->setX(this->x * calcParams->viewScale + calcParams->contentX);
        }
        curItem->setWidth(this->width * calcParams->viewScale);
        curItem->setHeight((interval_top + ascent + descent) * calcParams->viewScale);

//        curItem->setHeight(this->string->ascent * calcParams->viewScale + 4);
        ((ItemBlock*)curItem)->m_backGround = calcParams->textStyle->m_brushColor;
        ((ItemSelectedBlock*)curItem)->height_strike = (interval_top + ascent - calcParams->textStyle->m_fontMetrics.height() * 20 * 0.347) * calcParams->viewScale;
        ((ItemSelectedBlock*)curItem)->height_under = (interval_top + ascent + descent) * calcParams->viewScale;
//        if(isGlassy)
//            curItem->setY(this->string->y * calcParams->viewScale + calcParams->contentY - 4);
//        else
        ((ItemSelectedBlock*)curItem)->pBlock = this;
//        qWarning() << "ItemSelectedBlock" << this->ascent + descent << this->width;
        ((ItemSelectedBlock*)curItem)->m_strikeOut = calcParams->textStyle->m_font.strikeOut();
        ((ItemSelectedBlock*)curItem)->m_underLine = calcParams->textStyle->m_font.underline();
        ((ItemSelectedBlock*)curItem)->m_lineColor = calcParams->textStyle->m_fontColor;
        curItem->m_snapString = true;
        //                        ((ItemSelectedBlock*)curItem)->m_backGround = QColor("#bbdcec");
    }
    return curItem;
}

void EjSpaceBlock::calcBlock(int &index, EjCalcParams *calcParams)
{
    if(!calcParams || !calcParams->textStyle)
        return;
    this->ascent = calcParams->textStyle->m_fontMetrics.ascent() * 100 * 0.347;
    this->descent = calcParams->textStyle->m_fontMetrics.descent() * 100 * 0.347;
//    this->height += calcParams->interval;
    this->width = calcParams->textStyle->m_fontMetrics.descent() * 100 * 0.347;
#else
	return nullptr;
#endif
}



EjBlock *EjBlock::rootBlock()
{
    EjBlock *res = this;
    while(res->parent)
    {
        if(res->parent->isGlassy())
        {
            EjBlock *next = res->parent;
            while (next && next->isGlassy())
            {
                next = next->parent;
            }
            if(next && next->parent)
                res = next;
            else
                break;
        }
        else
            res = res->parent;

    }
    return res;
}

bool EjBlock::isProperty()
{
    bool res = type >= NUM_STYLE && type <= PROP_BIG_TEXT;
    return res;
}


EjBlock *EjPropDoc::makeCopy() {
    EjPropDoc *res = new EjPropDoc();
    copyData(res);
    res->m_vid = m_vid;
    res->m_counts = m_counts;
    res->num = num;
    return res;
}

bool EjPropDoc::compare(const EjBlock &other) const{
    return EjBlock::compare(other) && ((EjPropDoc*)&other)->m_vid == m_vid; // && ((EjPropDoc*)&other)->num == num;
}

QDataStream &EjPropDoc::read(QDataStream &is){
    EjBlock::read(is);
    return is;
}

QDataStream &EjPropDoc::write(QDataStream &os) const{
    EjBlock::write(os);
    os << m_vid;
    return os;
}

EjBlock *EjPropColorBlock::makeCopy()
{
    EjPropColorBlock *res = new EjPropColorBlock();
    copyData(res);
    res->num = num;
    res->color = color;
    return res;
}

bool EjPropColorBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropColorBlock const *cur_propColorBlock;
          cur_propColorBlock = (EjPropColorBlock *)&other;
    if(this->color != cur_propColorBlock->color || this->num != cur_propColorBlock->num)
        res = false;
    return res;
}

QDataStream &EjPropColorBlock::write(QDataStream &os) const
{
    quint32 k = color.rgba();

    EjBlock::write(os);
    os << num << k;
    return os;
}

QDataStream &EjPropColorBlock::read(QDataStream &is)
{
    quint32 k;
    EjBlock::read(is);
    is >> num >> k;
    color = QColor::fromRgba(k);
    return is;
}

EjBlock *EjPropBigTextBlock::makeCopy()
{
    EjPropBigTextBlock *res = new EjPropBigTextBlock();
    EjGroupBlock::copyData((EjGroupBlock*)res);
    res->num = num;
    return (EjGroupBlock*)res;
}

bool EjPropBigTextBlock::compare(const EjBlock &other) const
{
    if(!EjGroupBlock::compare(other)) {
        return false;
    }
    bool res = true;
    EjPropBigTextBlock const *cur_propBigTextBlock;
    cur_propBigTextBlock = (EjPropBigTextBlock *)(EjPropBase*)(&other);
    if(this->num != cur_propBigTextBlock->num)
        res = false;
    return res;
}

QDataStream &EjPropBigTextBlock::write(QDataStream &os) const
{
    EjGroupBlock::write(os);
    os << num;
    return os;
}

QDataStream &EjPropBigTextBlock::read(QDataStream &is)
{
    EjGroupBlock::read(is);
    is >> num;
    return is;
}

void EjPropBigTextBlock::setText(QString source, QList<EjBlock *> *lBlocks)
{
    if(m_index < 0)
        return;
    clear(lBlocks);
    int i  = source.indexOf(' ');
    QString left, right;
    while(i > 0)
    {
        left = source.left(i);
        source = source.right(source.size() - i - 1);
        i = source.indexOf(' ');
        addProp(lBlocks,new EjTextBlock(left));
        addProp(lBlocks, new EjSpaceBlock());
//        inputText(left);
//        inputSpace();
    }
    if(source == " ")
    {
        addProp(lBlocks, new EjSpaceBlock());
//        inputSpace();
    }
    else
    {
        addProp(lBlocks,new EjTextBlock(source));
//        inputText(txt);
    }
}

QString EjPropBigTextBlock::text(QList<EjBlock *> *lBlocks)
{
    int index = m_index + 1;
    QString res;
    while(index < m_index + m_counts)
    {
        if(lBlocks->at(index)->type == TEXT)
        {
            res += ((EjTextBlock*)lBlocks->at(index))->text;
        }
        else if(lBlocks->at(index)->type == SPACE)
        {
            res += ' ';
        }
        index++;
    }
    return res;
}

EjBlock *EjAccessBlock::makeCopy()
{
    EjAccessBlock *res = new EjAccessBlock();
    copyData(res);
    res->value = value;
    return res;
}

bool EjAccessBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
    //      EjTextBlock *cur_textBlock = &other;
    if(this->value != static_cast<const EjAccessBlock*>(&other)->value)
        res = false;
    return res;
}

void EjAccessBlock::setIsEditAsParent(bool source)
{
    if(source)
        BIT_SET(value,0);
    else
        BIT_CLEAR(value,0);
}

void EjAccessBlock::setIsSecAsParent(bool source)
{
    if(source)
        BIT_SET(value,5);
    else
        BIT_CLEAR(value,5);
}

void EjAccessBlock::setEditLevel(int source)
{
    for(int i = 0; i < 2; i++)
    {
        if(BIT_CHECK(source, i))
            BIT_SET(value,1 + i);
        else
            BIT_CLEAR(value,1 + i);
    }
}

void EjAccessBlock::setSecLevel(int source)
{
//    value &= ((source & 0x03) << 6);
    for(int i = 0; i < 2; i++)
    {
        if(BIT_CHECK(source, i))
            BIT_SET(value,6 + i);
        else
            BIT_CLEAR(value,6 + i);
    }
}

void initCommonResources()
{
    Q_INIT_RESOURCE(ejcommon);
}

EjBlock *EjPropAccessBlock::makeCopy()
{
    EjPropAccessBlock *res = new EjPropAccessBlock();
    copyData(res);
    res->num = num;
    res->value = value;
    return res;
}

bool EjPropAccessBlock::compare(const EjBlock &other) const
{
    if(!EjBlock::compare(other)) {
        return false;
    }
    bool res = true;
          EjPropAccessBlock const *cur_propAccessBlock;
          cur_propAccessBlock = (EjPropAccessBlock *)&other;
    if(this->num != cur_propAccessBlock->num || this->value != cur_propAccessBlock->value)
        res = false;
    return res;
}

void EjPropAccessBlock::setIsEditAsParent(bool source)
{
    if(source)
        BIT_SET(value,0);
    else
        BIT_CLEAR(value,0);
}

void EjPropAccessBlock::setIsSecAsParent(bool source)
{
    if(source)
        BIT_SET(value,5);
    else
        BIT_CLEAR(value,5);
}

void EjPropAccessBlock::setEditLevel(int source)
{
    for(int i = 0; i < 2; i++)
    {
        if(BIT_CHECK(source, i))
            BIT_SET(value,1 + i);
        else
            BIT_CLEAR(value,1 + i);
    }
}

void EjPropAccessBlock::setSecLevel(int source)
{
    for(int i = 0; i < 2; i++)
    {
        if(BIT_CHECK(source, i))
            BIT_SET(value,6 + i);
        else
            BIT_CLEAR(value,6 + i);
    }
}
