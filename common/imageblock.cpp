#include "imageblock.h"
#include "ejstoragehelper.h"

#include <QUrl>


ImageBlock::ImageBlock():GroupBlock()
{
    type = EXT_IMAGE; m_vid = 0; m_isInteractive = true; m_showBorders = true;
    m_widthImage = m_heightImage = 0;
    key = 0;
}

ImageBlock::ImageBlock(QImage &image, EjDocument *doc, int index) : ImageBlock()
{
    width = 4000;
    ascent = 3000;
    EjStorageHelper::addImage(image,m_Name);
    createDefault(doc->lBlocks,index);
}

ImageBlock::ImageBlock(quint32 userKey): ImageBlock()
{
    Q_UNUSED(userKey)
}

void ImageBlock::createDefault(QList<Block *> *lBlocks, int index)
{
    m_index = index;
    lBlocks->insert(index,this);
    index++;
//    lBlocks->insert(index,new EjPropIntBlock(IMG_KEY));
//    ((EjPropIntBlock*)lBlocks->at(index))->value = key;
//    index++;
    qDebug() << "image1";
    EjPropByteArrayBlock *propData = new EjPropByteArrayBlock(IMG_NAME);
    qDebug() << "image2";
    propData->data = m_Name;
    lBlocks->insert(index,propData);
    index++;
    qDebug() << "image3";
//    lBlocks->insert(index,new EjPropPntBlock(IMG_SIZE));
//    index++;
//    lBlocks->insert(index,new EjPropInt8Block(IMG_VID));
//    index++;
//    lBlocks->insert(index,new EjPropInt8Block(IMG_SHOW_BORDER));
//    index++;
//    lBlocks->insert(index,new EjPropInt8Block(IMG_INTERACTIVE));
//    index++;
    lBlocks->insert(index,new Block(END_GROUP,this));
    qDebug() << "image4";
    m_counts = 2;
}

//void ImageBlock::beforeCalc(QList<Block *> *lBlocks)
//{
////    m_lBlocks = lBlocks;
//}

void ImageBlock::childCalc(Block *child, CalcParams *calcParams)
{
    Q_UNUSED(calcParams)
    EjPropInt8Block *curInt8;
    EjPropIntBlock *curInt;
    switch (child->type) {
    case PROP_INT:
        curInt = (EjPropIntBlock *)child;
        if(curInt->num == IMG_KEY)
            key = ((EjPropIntBlock*)child)->value;
        break;
    case PROP_INT8:
        curInt8 = (EjPropInt8Block *)child;
        if(curInt8->num == IMG_INTERACTIVE)
            m_isInteractive = curInt8->value;
        else if(curInt8->num == IMG_VID)
            m_vid = curInt8->value;
        else if(curInt8->num == IMG_SHOW_BORDER)
            m_showBorders = curInt8->value;
        break;
    case PROP_PNT:
        m_widthImage = dynamic_cast<EjPropPntBlock*>(child)->x_value;
        m_heightImage = dynamic_cast<EjPropPntBlock*>(child)->y_value;
        break;
    case PROP_BA:
        m_Name = ((EjPropByteArrayBlock *)child)->data;
        break;
    default:
        break;
    }
}

void ImageBlock::afterCalc(CalcParams *calcParams)
{
    if(m_smallImage.isNull())
    {
        EjStorageHelper::loadSmallImage(&m_smallImage,m_Name,m_isInteractive);
//                cur_imgBlock->width = cur_imgBlock->small_image.width() * 100 * 0.347;
//                cur_imgBlock->height = cur_imgBlock->small_image.height() * 100 * 0.347;
        flag_redraw = true;
    }
    if(m_vid == 0)
    {
        ascent = m_smallImage.height() * 100 * 0.347;
        width = m_smallImage.width() * 100 * 0.347;
    }
    else
    {
        ascent = m_heightImage * 100 * 0.347;
        width = m_widthImage * 100 * 0.347;
    }
//    this->ascent = calcParams->textStyle->m_fontMetrics.ascent() * 100 * 0.347;
    if(m_showBorders)
    {
//        ascent = height - 5 * 100 * 0.347;
//        height += calcParams->interval;
    }
//    else
    //        ascent = height;
}

void ImageBlock::setName(QList<Block*> *lBlocks, QByteArray source)
{
    EjPropByteArrayBlock *propBABlock =  dynamic_cast<EjPropByteArrayBlock*>(findProp(lBlocks,PROP_BA,IMG_NAME));
    if(propBABlock)
    {
        m_Name = source;
        propBABlock->data = source;
    }
}

//QQuickItem *ImageBlock::getItem(int &index, CalcParams *calcParams, QQuickItem *parentItem)
//{
//    Q_UNUSED(index);
//    ItemImage *curItem = new ItemImage(this->m_smallImage,parentItem);
//    curItem->pBlock = this;
//    curItem->m_viewScale = calcParams->viewScale;
//    curItem->setHeight(this->ascent * calcParams->viewScale);
//    curItem->setWidth(this->width * calcParams->viewScale);
//    curItem->setX(this->x * calcParams->viewScale + calcParams->contentX);
//    curItem->setY(this->y * calcParams->viewScale + calcParams->contentY);
//    return curItem;
//}

void ImageBlock::setVid(int vid, QList<Block *> *lBlocks)
{
    EjPropInt8Block *curBlock = dynamic_cast<EjPropInt8Block*>(findProp(lBlocks,PROP_INT8,ImageBlock::IMG_VID));
    if(!curBlock)
    {
        curBlock = new EjPropInt8Block(ImageBlock::IMG_VID);
        addProp(lBlocks,curBlock);
    }
    m_vid = curBlock->value = static_cast<qint8>(vid);

}

void ImageBlock::setSize(int width, int height, QList<Block *> *lBlocks)
{
    EjPropPntBlock *curBlock = dynamic_cast<EjPropPntBlock*>(findProp(lBlocks,PROP_PNT,ImageBlock::IMG_SIZE));
    if(!curBlock)
    {
        curBlock = new EjPropPntBlock(ImageBlock::IMG_SIZE);
        addProp(lBlocks,curBlock);
    }
    m_widthImage = curBlock->x_value = width;
    if(m_widthImage < 10) {
        m_widthImage = curBlock->x_value = 10;
    }
    if(m_widthImage > 400) {
        m_widthImage = curBlock->x_value = 400;
    }
    m_heightImage = curBlock->y_value = height;
    if(m_heightImage < 10) {
        m_heightImage = curBlock->y_value = 10;
    }
    if(m_heightImage > 400) {
        m_heightImage = curBlock->y_value = 400;
    }

}

void ImageBlock::setIsInteractive(bool isInteractive, QList<Block *> *lBlocks)
{
    EjPropInt8Block *curBlock = dynamic_cast<EjPropInt8Block*>(findProp(lBlocks,PROP_INT8,ImageBlock::IMG_INTERACTIVE));
    if(!curBlock)
    {
        curBlock = new EjPropInt8Block(ImageBlock::IMG_INTERACTIVE);
        addProp(lBlocks,curBlock);
    }
    m_isInteractive = curBlock->value = isInteractive;

}

void ImageBlock::setIsShowBorders(bool isShowBorders, QList<Block *> *lBlocks)
{
    EjPropInt8Block *curBlock = dynamic_cast<EjPropInt8Block*>(findProp(lBlocks,PROP_INT8,ImageBlock::IMG_SHOW_BORDER));
    if(!curBlock)
    {
        curBlock = new EjPropInt8Block(ImageBlock::IMG_SHOW_BORDER);
        addProp(lBlocks,curBlock);
    }
    m_showBorders = curBlock->value = isShowBorders;

}

Block *ImageBlock::makeCopy() {
    ImageBlock *res = new ImageBlock();
    copyData(res);
    res->m_heightImage = m_heightImage;
    res->m_widthImage = m_widthImage;
    res->m_smallImage = m_smallImage;
    res->m_vid = m_vid;
    res->m_isInteractive = m_isInteractive;
    res->m_Name = m_Name;
    return res;
}
