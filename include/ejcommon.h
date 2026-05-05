/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include "common_global.h"
//#include "ejtextcontrol.h"
//#include "ejstyles.h"
// #include "popupmenumodel.h"
#include <QString>
#include <QDataStream>
#include <QColor>
#include <QImage>
#include <QDateTime>
#include <QVariant>
#include <QUuid>
#include <QFont>


COMMONSHARED_EXPORT int writeSmallString(QDataStream &stream,const QString &source);
COMMONSHARED_EXPORT void readSmallString(QDataStream &stream, QString &source);
COMMONSHARED_EXPORT int writeBA(QDataStream &stream, const QByteArray &source);
COMMONSHARED_EXPORT void readBA(QDataStream &stream, QByteArray &source);
COMMONSHARED_EXPORT double getDValue(QString s_value, bool *bOk);
COMMONSHARED_EXPORT QString getDText(double value, int accuracy);
COMMONSHARED_EXPORT void initCommonResources();



enum e_typeBlocks
{
    BASE = 0,
    ENTER,         // 1
    SPACE,         // 2
    TEXT,          // 3
    CONTACT,       // 4
    COLUM_D,       // 5
    ROW_D,         // 6
    BASECELL,      // 7
    PROP_DOC_D,    // 8
    ERROR_BLOCK,   // 9
    FRAGMENT,      // 10
    END_FRAGMENT,  // 11
    TABLEFRAGMENT, // 12
    END_GROUP,     // 13
    NUM_STYLE,     // 14

    PROP_KEY,      // 15
    PROP_INT,      // 16
    PROP_DOUBLE,   // 17
    PROP_TXT,      // 18
    PROP_PNT,      // 19
    PROP_INT8,     // 20
    PROP_INT64,    // 21
    PROP_BA,       // 22
    PROP_COLOR,    // 23
    PROP_ACCESS,   // 24
//    PROP_COLUM,    // 25
//    PROP_ROW,      // 26

    ACCESS_RULES = 49,

    GROUP_BLOCK = 50,   // ext blocks
    PROP_DOC,      // 51
    PROP_BIG_TEXT, // 52


    GROUP_PARAM,   // 53
    GROUP_COMPLEX, // 54
    VECTOR_POINT,  // 55
    VECTOR_TEXT,   // 56
    VECTOR_IMAGE,  // 57
    MAP_LABEL,     // 58
    FIGURE,        // 59

    //    STYLE = 50,

    EXT_MAP = 101, //
    EXT_DIAGRAM,   // 102
    EXT_IMAGE,     // 103
    EXT_TABLE,     // 104
    EXT_HADDEN_BAK,    // 105
    EXT_LABEL,  // 106
    EXT_LARGETEXT_BAK, // 107
    EXT_TASKS_GROUP,   // 108
};

enum e_viewMode
{
    RICH_TEXT = 0,
    FIXPAGE
};

enum e_statusMode
{
    READ_ONLY = 0,
    EDIT_TEXT,
    EDIT_CELL,
//    SELECTED,
//    NOEDIT_SELECTED
};

enum e_selectMode
{
    NO_SELECTED = 0,
    SELECTED,
};

enum e_securityMode
{
    SECURITY_FULL_EDIT = 0,
    SECURITY_EDIT = 32,
    SECURITY_INPUT = 64,
    SECURITY_READ_AND_SELECT = 128,
    SECURITY_READ = 255,
};

class ItemBlock;
class QQuickItem;
class EjString;
class EjTableBlock;
class EjCellBlock;
class EjDocument;
class LargeTextBlock;
class LabelBlock;

///**
// * @brief The IntVarLen class writes integer values as a variable-length
// * sequences.
// */
//class IntVarLen
//{
//public:
//    IntVarLen(quint32 &v) :
//        value(v)
//    {}

//    friend QDataStream& operator>>(QDataStream &is, IntVarLen &v)
//    {
//        v.value = 0;
//        quint8 c;
//        do {
//            is >> c;
//            v.value = (v.value << 7) | (c & 0x7f);
//        }while(c & 0x80);

//        return is;
//    }

//    friend QDataStream& operator<<(QDataStream &os, IntVarLen &v)
//    {
//        quint32 value = v.value;

//        quint64 buf = value & 0x7f;
//        value = value >> 7;

//        while(value) {
//            buf = (buf << 8) | quint8((value & 0x7f) | 0x80);
//            value = value >> 7;
//        };

//        while(buf & 0x80) {
//            os << quint8(buf & 0xff);
//            buf = buf >> 8;
//        };
//        os << quint8(buf & 0xff);

//        return os;
//    }
//    int getValue() { return value; }

//private:
//    quint32 &value;
//};

struct EjCalcParams;

class COMMONSHARED_EXPORT EjBlock
{
public:
    EjBlock(EjBlock *parent_ = nullptr){
        type = width = x = y = ascent = descent = interval_top = interval_bottom = 0; flag_redraw = true; parent = parent_; }
    EjBlock(int f_type, EjBlock *parent_ = nullptr):EjBlock(parent_) {  type = static_cast<quint8>(f_type); }
    EjBlock(const EjBlock&) = default;
    EjBlock(EjBlock&&) = default;
    EjBlock& operator=(const EjBlock&) = default;
    EjBlock& operator=(EjBlock&&) = default;
    virtual ~EjBlock() {}
    quint8 type; // 0-BASE 1-ENTER 2-SPACE 3-TEXT 4-CONTACT 5 - IMAGE ...
//    qint8 style;
//    qint16 page;
    qint32 width;
//    qint32 height;
    qint32 interval_top;
    qint32 ascent;
    quint16 descent;
    quint16 interval_bottom;
    qint32 x;
    qint32 y;
    bool flag_redraw;
	EjBlock *parent;
//    EjString *string;

    int height() { return ascent + descent; }
    int fullHeight() { return interval_top + ascent + descent + interval_bottom; }
    virtual  QQuickItem *getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem) { Q_UNUSED(index); Q_UNUSED(calcParams); Q_UNUSED(parentItem); return NULL; }
    EjBlock *rootBlock();


    virtual EjBlock* makeCopy()
    {
        EjBlock *res = new EjBlock();
        copyData(res);
        return res;
    }

    void copyData(EjBlock *result)
    {
//        result->height = height;
        //    result->page = page;
        //    result->style = style;
        result->ascent = ascent;
        result->descent = descent;
        result->interval_top = interval_top;
        result->interval_bottom = interval_bottom;
        result->type = type;
        result->width = width;
        result->x = x;
        result->y = y;
    }


    bool operator ==(const EjBlock &other) const
    {
        return compare(other);
    }

    friend QDataStream& operator <<(QDataStream &os, const EjBlock &value)
    {
        return value.write(os);
    }

    friend QDataStream& operator >>(QDataStream &is, EjBlock &value)
    {
        return value.read(is);
    }

    virtual bool compare(const EjBlock &other) const {
        bool res = (this->type == other.type) ? true : false;
        return res;
    }

    virtual QDataStream& write(QDataStream &os) const {
//        os << type << style << page;
        os << type;
        return os;
    }

    virtual QDataStream& read(QDataStream &is) {
//        is >> style >> page;
        return is;
    }

//    Q_INVOKABLE virtual QQuickItem* getActiveViewItem(int vid, QQuickItem *parent) = 0;
    virtual ItemBlock* newItem(qreal viewScale, QQuickItem *parent) { Q_UNUSED(viewScale); Q_UNUSED(parent); return NULL; }
    virtual QQuickItem* newViewItem(int vid, QQuickItem *parent) { Q_UNUSED(vid); Q_UNUSED(parent); return NULL; }
    virtual void calcBlock(int &index, EjCalcParams *calcParams) { Q_UNUSED(index); Q_UNUSED(calcParams); }
    virtual bool isSelected(int &index, int &startSelect, int &endSelect) { return index >= startSelect && index <= endSelect; }
    virtual bool isGlassy() { return false; }
    bool isProperty();

};


class EjString
{
public:
    void calcCursor();
    EjString() { startBlock = endBlock = 0; height = width = x = y = ascent = 0; }
//    virtual ~EjString(){}
    qint32 startBlock;
    qint32 endBlock;
    qint32 width;
    qint32 height;
    qint16 x;
    qint32 y;
    qint32 ascent;
    qint32 interval;

};


struct Param
{
 //   quint8 type;
    Param() {iValue = 0;}
    bool operator ==(const Param &other) const
    {
        return iValue == other.iValue && sValue == other.sValue;
    }

    quint32 iValue;
    QString sValue;
};

class MAlign
{
public:
    MAlign() { align = 0; }
    quint8 align; // 0- noAlign(auto) 1-alignLeft 2-alignRight 3-alignHCenter
                  // 4-alignTop 8-alignBottom 12-alignVCenter
                  // 16-(90) 32-(-90) 48-(180)
                  // 64-absolute 128-baseline
    bool isLeft() { return (align & 1) == 1; }
    bool isRigth() { return (align & 2) == 2; }
    bool isHCenter() { return (align & 3) == 3; }
    bool isTop() { return (align & 4) == 4; }
    bool isBottom() { return (align & 8) == 8; }
    bool isVCenter() { return (align & 12) == 12; }
};

class EjFragment
{
public:
    enum Type {
        Clear,
        Bullets,
        Numbering,
        Bold,
        Italic,
        Underline,
        DBold,
        DItalic,
        DUnderline,
        AlignHAuto,
        AlignLeft,
        AlignRight,
        AlignHCenter,
        AlignVAuto,
        AlignTop,
        AlignBottom,
        AlignVCenter,
        Angle,
        Absolute,
        BaseLine,
        DAbsolute,
        DBaseLine,
        FontSize,
        FontFamily,
        PenColor,
        BrushColor,
        FontColor,
        ParagraphColor,
        Select
    };

	EjFragment() { startBlock = endBlock = 0;  }
	virtual ~EjFragment() {}
    qint32 startBlock;
    qint32 endBlock;
    quint8 type; // 0-normal text 1-bullets 2-numbering ......
    Param param;
//    MAlign align;
//    QColor color;
//    QFont font;
//    QMap<quint8,Param> mParams;
//    type = styleNum = levelNum = 0;
//    quint8 levelNum;
//    qint8 styleNum;
//    QFont font;
//    QColor HighlightingColor;
//    QColor backgroundColor;
	bool operator ==(const EjFragment &other) const
    {
        return startBlock == other.startBlock && endBlock == other.endBlock && type == other.type; // && mParams == other.mParams;
    }

//    friend QDataStream &operator<<(QDataStream &dataStream, const EjFragment &src);

//    friend QDataStream &operator>>(QDataStream &dataStream, EjFragment &src);
	friend QDataStream& operator <<(QDataStream &os, const EjFragment &value)
    {
        return value.write(os);
    }

	friend QDataStream& operator >>(QDataStream &is, EjFragment &value)
    {
        return value.read(is);
    }


    virtual QDataStream& write(QDataStream &os) const;

    virtual QDataStream& read(QDataStream &is);

};





class COMMONSHARED_EXPORT EjGroupBlock : public EjBlock
{
public:
	EjGroupBlock():EjBlock() { type = GROUP_BLOCK; m_index = -1; m_counts = 0; }
//    virtual ~EjGroupBlock() override {}
//    QList<EjBlock*> getElements();
//    void calcParams(QList<EjBlock*> *lBlocks, bool force = false);
    EjBlock* findProp(QList<EjBlock *> *lBlocks, int type, int num, bool check = false);
    int findPropIndex(QList<EjBlock *> *lBlocks, int type, int num, int endIndex, bool check = false);
    int findPropIndex(QList<EjBlock *> *lBlocks, int type, int num, int startIndex, int endIndex, bool check = false);
    void addProp(QList<EjBlock*> *lBlocks, EjBlock *block);
    void remBlock(QList<EjBlock*> *lBlocks, EjBlock *block);
    virtual void createDefault(QList<EjBlock*> *lBlocks, int index);
    virtual void createDefaultWithNum(QList<EjBlock*> *lBlocks, int index, int num);
    virtual void remFromBlocks(QList<EjBlock*> *lBlocks);
    virtual void beforeCalc(EjCalcParams *calcParams) { Q_UNUSED(calcParams) }
    virtual void childCalc(EjBlock *child, EjCalcParams *calcParams) { Q_UNUSED(child) Q_UNUSED(calcParams) }
    virtual void afterCalc(EjCalcParams *calcParams) { Q_UNUSED(calcParams) }
//    bool isGlassy() override { return false; }
    virtual QString getTextData(QList<EjBlock *> *lBlocks) { Q_UNUSED(lBlocks) return QString(); }
    void calcBlock(int &index, EjCalcParams *calcParams) override;
    void calcLenght(int &index, QList<EjBlock *> *lBlocks);
    void clear(QList<EjBlock*> *lBlocks);

    virtual EjBlock* makeCopy() override {
			EjGroupBlock *res = new EjGroupBlock();
            copyData(res);
            return res;
    }
    bool compare(const EjBlock &other) const override{
        return EjBlock::compare(other);
    }
    QDataStream& write(QDataStream &os) const override{
        EjBlock::write(os);
        return os;
    }

    QDataStream& read(QDataStream &is) override{
        EjBlock::read(is);
        return is;
    }
//    int num;
    int m_index;
    int m_counts;
};


class EjDocument;
// INTERFACESHARED_EXPORT
class COMMONSHARED_EXPORT EjPropDoc : public EjGroupBlock
{

public:
	EjPropDoc():EjGroupBlock() { type = PROP_DOC; m_vid = 0; num = 0; m_doc = nullptr; }
	virtual ~EjPropDoc() override{}

//    EjBlock* findProp(QList<EjBlock *> *lBlocks, quint8 type, qint16 num);
//    void addProp(QList<EjBlock*> *lBlocks, EjBlock *block);
//    virtual void createDefault(QList<EjBlock*> *lBlocks, int index);
//    virtual void remFromBlocks(QList<EjBlock*> *lBlocks);
//    virtual void beforeCalc() { }
//    virtual void childCalc(EjBlock *child) { Q_UNUSED(child) }
//    virtual void afterCalc() { }
//    virtual void calcBlock(int &index, EjDocument *document);
//    void clear(EjDocument *doc);

    EjBlock* makeCopy() override;
    bool compare(const EjBlock &other) const override;
    QDataStream& write(QDataStream &os) const override;
    QDataStream& read(QDataStream &is) override;
//    int num;
//    int m_index;
//    int m_counts;
    quint8 m_vid;
    int num;

//protected:
	EjDocument *m_doc;
//    QList<EjBlock*> *m_lBlocks;

};

class VectorText : public EjGroupBlock
{
	VectorText():EjGroupBlock() {type = VECTOR_TEXT;}
    ItemBlock* newItem(qreal viewScale, QQuickItem *parent) override;
    QQuickItem* newViewItem(int vid, QQuickItem *parent) override;

};


////class GroupComplexBlock;
//class ComplexBlock : public EjBlock
//{
//public:
//    ComplexBlock():EjBlock() {  mProps = NULL; }
//    virtual ~ComplexBlock() {}
////    void setBlocks(QList<EjBlock*> *source);
//    void setProp(int num, QVariant value);
//    void setData(EjGroupBlock *block);
////    EjGroupBlock* getData() { return mData; }

//    EjBlock* makeCopy();
//    bool compare(const EjBlock &other) const;
//    QDataStream& write(QDataStream &os) const;

//    QDataStream& read(QDataStream &is);

////    void createBody();
////    virtual QVariant getProp(int num) = 0;
////    virtual QString nameProp(int num) = 0;

////    virtual EjBlock* makeCopy() = 0;
////    virtual bool compare(const EjBlock &other) const = 0;
////    virtual QDataStream& write(QDataStream &os) const = 0;
////    virtual QDataStream& read(QDataStream &is) = 0;
//protected:
//    int m_index;
//    QMultiMap<int,EjBlock*> *mProps;
////    EjGroupBlock *mData;
//};

//class GroupComplexBlock : public ComplexBlock
//{
//public:
//    GroupComplexBlock():ComplexBlock() { lBlocks = NULL; mProps = NULL; mBody = NULL; type = GROUP_COMPLEX;  }
//    virtual ~GroupComplexBlock() {}
//    QList<ComplexBlock*>* getElements() { return &lElements; }
//    ComplexBlock* addElement(int type) {};
//    virtual QVariant getProp(int ) { return QVariant(); }
//    virtual QString nameProp(int ) { return QString(); }

//    EjBlock* makeCopy() {
//            GroupComplexBlock *res = new GroupComplexBlock();
//            copyData(res);
//            return res;
//    }
//    bool compare(const EjBlock &other) const {
//        return EjBlock::compare(other);
//    }
//    QDataStream& write(QDataStream &os) const {
//        EjBlock::write(os);
//        return os;
//    }

//    QDataStream& read(QDataStream &is) {
//        EjBlock::read(is);
//        return is;
//    }

//protected:
//    QList<ComplexBlock*> lElements;
//};

//#define TXT_STYLE_NARMAL_CLEAR 0
//#define TXT_STYLE_NARMAL_BOLD 1
//#define TXT_STYLE_NARMAL_ITALIC 2
//#define TXT_STYLE_NARMAL_UNDERLINE 3


class EjPointBlock : public EjBlock
{
public:
	EjPointBlock():EjBlock(VECTOR_POINT) { }
//    virtual ~EjPointBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const {
        EjBlock::write(os);
        os << x << y;
        return os;
    }

    QDataStream& read(QDataStream &is) {
        EjBlock::read(is);
        is >> x >> y;
        return is;
    }
};



class EjPropBase : public EjBlock
{
public:
    EjPropBase(int type):EjBlock(type) { num = 0; }
    virtual ~EjPropBase() override {}
    quint8 num;
    EjBlock* makeCopy() override = 0;
    bool compare(const EjBlock &other) const override = 0;
    QDataStream& write(QDataStream &os) const override = 0;
    QDataStream& read(QDataStream &is) override = 0;
};

class COMMONSHARED_EXPORT EjPropKeyBlock : public EjPropBase
{
public:
	EjPropKeyBlock():EjPropBase(PROP_KEY) { key = QUuid::createUuid(); }
//    EjPropKeyBlock(QUuid _key):EjBlock(PROP_KEY) { key = _key; }
	EjPropKeyBlock(quint8 num_):EjPropBase(PROP_KEY) { num = num_; key = QUuid::createUuid(); }
    QUuid key;
//    virtual ~EjPropKeyBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;
    QDataStream& read(QDataStream &is);
};

class COMMONSHARED_EXPORT EjPropPntBlock : public EjPropBase
{
public:
	EjPropPntBlock():EjPropBase(PROP_PNT) { }
	EjPropPntBlock(quint8 num_):EjPropBase(PROP_PNT) { num = num_; }
//    virtual ~EjPropPntBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;

    QDataStream& read(QDataStream &is);
    qint32 x_value;
    qint32 y_value;

};

class COMMONSHARED_EXPORT EjPropColorBlock : public EjPropBase
{
public:
	EjPropColorBlock():EjPropBase(PROP_COLOR) { }
	EjPropColorBlock(quint8 num_):EjPropBase(PROP_COLOR) { num = num_; }
//    virtual ~EjPropColorBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;

    QDataStream& read(QDataStream &is);
    QColor color;
};

class COMMONSHARED_EXPORT EjPropIntBlock : public EjPropBase
{
public:
	EjPropIntBlock():EjPropBase(PROP_INT) { }
	EjPropIntBlock(quint8 num_):EjPropBase(PROP_INT) { num = num_; }
//    virtual ~EjPropIntBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;

    QDataStream& read(QDataStream &is);
    qint32 value;
};

//class COMMONSHARED_EXPORT PropColumBlock : public EjPropIntBlock
//{
//public:
//    PropColumBlock():EjPropIntBlock() { type = PROP_COLUM; }
//    PropColumBlock(quint8 num_):EjPropIntBlock(num_) { type = PROP_COLUM; }
//};

//class COMMONSHARED_EXPORT PropRowBlock : public EjPropIntBlock
//{
//public:
//    PropRowBlock():EjPropIntBlock() { type = PROP_ROW; }
//    PropRowBlock(quint8 num_):EjPropIntBlock(num_) { type = PROP_ROW; }
//};

class COMMONSHARED_EXPORT EjPropInt8Block : public EjPropBase
{
public:
	EjPropInt8Block():EjPropBase(PROP_INT8) {  }
	EjPropInt8Block(quint8 num_):EjPropBase(PROP_INT8) { num = num_; }
//    virtual ~EjPropInt8Block() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;

    QDataStream& read(QDataStream &is);
    qint8 value;
};

class COMMONSHARED_EXPORT EjPropInt64Block : public EjPropBase
{
public:
	EjPropInt64Block():EjPropBase(PROP_INT64) {  }
	EjPropInt64Block(quint8 num_):EjPropBase(PROP_INT64) { num = num_; }
//    virtual ~EjPropInt64Block() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;

    QDataStream& read(QDataStream &is);
    qint64 value;
};


class COMMONSHARED_EXPORT EjPropTextBlock : public EjPropBase
{
public:
	EjPropTextBlock():EjPropBase(PROP_TXT) { }
	EjPropTextBlock(quint8 num_):EjPropBase(PROP_TXT) { num = num_; }
//    virtual ~EjPropTextBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;

    QDataStream& read(QDataStream &is);
    QString text;
};

class COMMONSHARED_EXPORT EjPropByteArrayBlock : public EjPropBase
{
public:
    EjPropByteArrayBlock():EjPropBase(PROP_BA) {  }
    EjPropByteArrayBlock(quint8 num_):EjPropBase(PROP_BA) { num = num_; }
//    virtual ~EjPropByteArrayBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;

    QDataStream& read(QDataStream &is);
    QByteArray data;
};


class COMMONSHARED_EXPORT EjPropAccessBlock : public EjPropBase
{
public:
    enum e_editLevel{
        ONLY_READ,
        READ_AND_SELECT,
        READ_AND_WRITE,
        FULL_ACCESS
    };

    EjPropAccessBlock():EjPropBase(PROP_ACCESS) {  }
    EjPropAccessBlock(quint8 num_):EjPropBase(PROP_ACCESS) { num = num_; }
//    virtual ~EjPropAccessBlock() override{}

    EjBlock* makeCopy() override;
    bool compare(const EjBlock &other) const override;
    QDataStream& write(QDataStream &os) const override {
        EjBlock::write(os);
        os << num << value;
        return os;
    }

    QDataStream& read(QDataStream &is) override {
        EjBlock::read(is);
        is >> num >> value;
        return is;
    }
    bool isEditAsParent() { return value & 0x01; }
    bool isSecAsParent() { return (value >> 5) & 0x01; }
    int editLevel() { return (value >> 1) & 0x03; }
    int secLevel() { return (value >> 6) & 0x03; }
    void setIsEditAsParent(bool source);
    void setIsSecAsParent(bool source);
    void setEditLevel(int source);
    void setSecLevel(int source);
//    QQuickItem *getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem) override;
//    void calcBlock(int &index, EjCalcParams *calcParams) override;

//private:
    quint8 value;
};

class EjAccessBlock : public EjBlock
{
public:
    enum e_editLevel{
        ONLY_READ,
        READ_AND_SELECT,
        READ_AND_WRITE,
        FULL_ACCESS
    };

	EjAccessBlock():EjBlock() { type = ACCESS_RULES; }
//    virtual ~EjAccessBlock() override{}

    EjBlock* makeCopy() override;
    bool compare(const EjBlock &other) const override;
    QDataStream& write(QDataStream &os) const override {
        EjBlock::write(os);
        os << value;
        return os;
    }

    QDataStream& read(QDataStream &is) override {
        EjBlock::read(is);
        is >> value;
        return is;
    }
    bool isEditAsParent() { return value & 0x01; }
    bool isSecAsParent() { return (value >> 5) & 0x01; }
    int editLevel() { return (value >> 1) & 0x03; }
    int secLevel() { return (value >> 6) & 0x03; }
    void setIsEditAsParent(bool source);
    void setIsSecAsParent(bool source);
    void setEditLevel(int source);
    void setSecLevel(int source);
//    QQuickItem *getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem) override;
//    void calcBlock(int &index, EjCalcParams *calcParams) override;

//private:
    quint8 value;
};

class COMMONSHARED_EXPORT EjPropBigTextBlock : public EjGroupBlock
{
public:
	EjPropBigTextBlock(): EjGroupBlock() { type = PROP_BIG_TEXT; num = 0; }
	EjPropBigTextBlock(quint8 num_):EjGroupBlock() { type = PROP_BIG_TEXT; num = num_; }
//    ~EjPropBigTextBlock() {}
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;
    QDataStream& read(QDataStream &is);
    void setText(QString source, QList<EjBlock*> *lBlocks);
    QString text(QList<EjBlock*> *lBlocks);
    quint8 num;
};

class EjBaseStyle;

class COMMONSHARED_EXPORT EjNumStyleBlock : public EjBlock
{
public:
	EjNumStyleBlock():EjBlock() { type = NUM_STYLE; num = 0; style = NULL; }
//    EjTextBlock(QString txt) { EjBlock(); text = txt; }
	~EjNumStyleBlock() override {}
    EjBaseStyle *style;

    EjBlock* makeCopy() override;
    bool compare(const EjBlock &other) const override;
    QDataStream& write(QDataStream &os) const override;

    QDataStream& read(QDataStream &is) override;
    QQuickItem *getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem) override;
    void calcBlock(int &index, EjCalcParams *calcParams) override;

    quint16 num;
};



class PointStyle;
class COMMONSHARED_EXPORT EjMapLabelBlock : public EjGroupBlock
{
public:
	EjMapLabelBlock():EjGroupBlock() { type = MAP_LABEL; m_style = NULL; }
//    virtual ~EjMapLabelBlock() {}
//    void calcParams(QList<EjBlock*> *lBlocks, bool force=false);
    bool compare(const EjBlock &other) const override;
//    void createDefault(QList<EjBlock*> *lBlocks, int index);
    void setCoords(qint32 x_souce, qint32 y_source);

    QDataStream& write(QDataStream &os) const override{
        EjBlock::write(os);
        os << x << y;
        return os;
    }

    QDataStream& read(QDataStream &is) override{
        EjBlock::read(is);
        is >> x >> y;
        return is;
    }
protected:
    PointStyle *m_style;
//    EjPointBlock *m_point;
};

class EjLineStyle;

class PolylineBlock : public EjGroupBlock
{
public:
	PolylineBlock():EjGroupBlock() { m_style = NULL; index = 0; }
    ~PolylineBlock() override {}
    void setPoints(QList<EjBlock*> lPoints);
	QList<EjPointBlock*> *getPoints();
protected:
    EjLineStyle *m_style;
    int index;
};

class EjFragmentBlock : public EjBlock
{
public:
	EjFragmentBlock() { EjBlock(); type = FRAGMENT; vid = EjFragment::Clear; countBlocks = iValue = index = startBlock = endBlock = 0; sValue = ""; }
//    virtual ~EjFragmentBlock() {}
	quint8 vid; // EjFragment::Type
    quint16 countBlocks;
    quint16 index;
    quint32 iValue;
    QString sValue;

    qint32 startBlock;
    qint32 endBlock;

    virtual EjBlock* makeCopy() override;
    virtual bool compare(const EjBlock &other) const override;
    virtual QDataStream& write(QDataStream &os) const override;

    virtual QDataStream& read(QDataStream &is) override;
};

class EndFragmentBlock : public EjBlock
{
public:
	EndFragmentBlock() { EjBlock(); type = END_FRAGMENT; vid = EjFragment::Clear;  }
//    virtual ~EndFragmentBlock() {}
	quint8 vid; // EjFragment::Type

    virtual EjBlock* makeCopy();
    virtual bool compare(const EjBlock &other) const;
    virtual QDataStream& write(QDataStream &os) const;

    virtual QDataStream& read(QDataStream &is);
};

class TableFragment : public EjFragmentBlock
{
public:
    TableFragment() { EjFragmentBlock(); type = TABLEFRAGMENT; }
    quint16 startRow() const { return quint16(startBlock >> 16);  }
    quint16 startColum() const { return quint16(startBlock & 0xff); }
    quint16 endRow() const { return quint16(endBlock >> 16); }
    quint16 endColum() const { return quint16(endBlock & 0xff); }

    void setStartRow(quint16 row);
    void setStartColum(quint16 colum);
    void setEndRow(quint16 row);
    void setEndColum(quint16 colum);
    TableFragment *makeCopy() override;
    virtual bool compare(const EjBlock &other) const override;

    virtual QDataStream& write(QDataStream &os) const override;

    virtual QDataStream& read(QDataStream &is) override;
};



class EjSpaceBlock : public EjBlock
{
public:
	EjSpaceBlock():EjBlock() {type = SPACE;}
	virtual ~EjSpaceBlock() override {}

    EjBlock* makeCopy() override;
//    bool compare(const EjBlock &other) const override;
    QDataStream& write(QDataStream &os) const override {
        EjBlock::write(os);
        return os;
    }

    QDataStream& read(QDataStream &is) override {
        EjBlock::read(is);
        return is;
    }

    QQuickItem *getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem) override;
    void calcBlock(int &index, EjCalcParams *calcParams) override;
};

class COMMONSHARED_EXPORT EjTextBlock : public EjBlock
{
public:
	EjTextBlock():EjBlock() {type = TEXT;}
	EjTextBlock(QString txt) : EjTextBlock() { text = txt; }
	virtual ~EjTextBlock() override;
    QString text;

    EjBlock* makeCopy() override;
    bool compare(const EjBlock &other) const override;
    QDataStream& write(QDataStream &os) const override {
        EjBlock::write(os);
        writeSmallString(os,text); // os << text;
        return os;
    }

    QDataStream& read(QDataStream &is) override {
        EjBlock::read(is);
        readSmallString(is,text); //        is >> text;
        return is;
    }

    QQuickItem *getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem) override;
    void calcBlock(int &index, EjCalcParams *calcParams) override;

};

struct EjSizeProp : public EjBlock {
    EjSizeProp() { type = 9; min = 0; max = 0xffff; }
    quint16 num;
    quint16 min;
    quint16 max;
    quint16 current;
    bool isMax;
    EjBlock* makeCopy();
    bool compare(const EjBlock &other) const;
    QDataStream& write(QDataStream &os) const;
    QDataStream& read(QDataStream &is);

};


//class TableBlock_old : public EjBlock
//{
//public:
//    TableBlock_old() {type = 6; style = 0; vid = 0; accuracy = 3; evenRowsColor = QColor("#4fc2c0c0"); deltaProps = 0;
//                  columColor = borderColor = lineColor = QColor("#d2d0d0"); startBlock = endBlock = 0; }
//    virtual ~TableBlock_old();
//    quint8 style; // 0,1 - small, price, qty, full 2 - checking 3 - numbering     // bits
//    quint8 vid;  // 0 - shoplist, 1 - cleantable ...
//    quint8 accuracy;
//    quint16 num;
//    int start_colum;
//    int start_row;
//    qint32 startBlock;
//    qint32 endBlock;
//    qint32 countBlocks;
//    qint32 deltaProps;
//    QColor foneColor;
//    QColor evenColumsColor;
//    QColor evenRowsColor;
//    QColor lineColor;
//    QColor columColor;
//    QColor borderColor;
//    quint8 spacing;

//    qint32 startCell() { return startBlock + deltaProps + 1; }

//    enum VidProp {
//        SHOPLIST = 0,
//        CLEANTABLE
//    };

//    QList<TableFragment*> lFragments;
//    QList<EjSizeProp*> lColums;
//    QList<EjSizeProp*> lRows;
////    int nRows() const { return m_nRows; }
////    int nColums() const { return m_nColums; }
//    int nRows() const { return lRows.count(); }
//    int nColums() const { return lColums.count(); }
//    int calc();
//    EjBlock* makeCopy();
//    bool compare(const EjBlock &other) const;
//    QDataStream& write(QDataStream &os) const;
//    QDataStream& read(QDataStream &is);

//protected:
////    quint32 m_nColums;
////    quint32 m_nRows;


//};



//class BaseCellBlock_old : public EjTextBlock
//{
//public:
//    BaseCellBlock_old() { type = 7; value = 0; parent = 0; vid = 0;}
////    BaseCellBlock(int _vid) { BaseCellBlock::BaseCellBlock(); vid = _vid; }
//    virtual ~BaseCellBlock_old(){}
//    TableBlock_old *parent;
//    QString formula;
////    MAlign m_align;
////    quint8 font_size;
////    quint8 font_family;
//    quint8 vid; //0 - text, 1 - number, 2 - formula, 3 - check, 4 - end table
//    quint16 txtWidth;
//    quint16 txt_y;
//    qint16 txt_x;
//    quint16 txtHeight;
//    int calc();
//    double value;

//    enum Vid {
//        TEXT = 0,
//        NUMBER,
//        FORMULA,
//        CHECK,
//        ENDTABLE
//    };

//    EjBlock* makeCopy() {
//        BaseCellBlock_old *res = new BaseCellBlock_old();
//        copyData(res);
//        res->formula = formula;
////        res->m_align = m_align;
//        res->text = text;
//        res->value = value;
//        res->vid = vid;
//        return res;
//    }
//    void copyCell(BaseCellBlock_old *res)
//    {
//        res->formula = formula; res->text = text; res->value = value; res->vid = vid;
//    }

//    bool compare(const EjBlock &other) const;
//    QDataStream& write(QDataStream &os) const;

//    QDataStream& read(QDataStream &is);
//};

//class CheckBlock : public BaseCellBlock
//{
//public:
//    CheckBlock():BaseCellBlock() { type = 8; state = 0; parent = 0; }
//    quint8 state;
////    EjTableBlock *parent;
//    EjBlock* makeCopy() {
//        CheckBlock *res = new CheckBlock();
//        copyData(res);
//        return res;
//    }
//    bool compare(const EjBlock &other) const {
//        if(!EjBlock::compare(other) || this->state != static_cast<const CheckBlock*>(&other)->state) {
//            return false;
//        }
//        return true;
//    }
//    QDataStream& write(QDataStream &os) const {
//        EjBlock::write(os);
//        os << quint8(0) << state;
//        return os;
//    }

//    QDataStream& read(QDataStream &is) {
//        EjBlock::read(is);
//        quint8 ver;
//        is >> ver >> state;
//        return is;
//    }
//};

class ContactData
{

public:
    ContactData() {vid=0;}
    QString name;
    quint8 vid; //1...10 mobile phone
                 //11...20 homo phone
                //21...30 work phone
                 //101...110 emale

    bool compare(const ContactData &other) const {if(other.name==name && other.vid==vid) return true;
                                           else return false; }

    QDataStream& write(QDataStream &os) const {
        os << vid;
        writeSmallString(os,name); // os << text;
        return os;
    }

    QDataStream& read(QDataStream &is) {
        is >> vid;
        readSmallString(is,name); //        is >> text;
        return is;
    }

    bool operator==(const ContactData& right) {
        return compare(right);
    }

//    bool operator==(const const ContactData& left,const const ContactData& right) {
//        return left.name == right.name && left.vid == right.vid;
//    }
};

//Q_DECLARE_METATYPE(ContactData)
//Q_DECLARE_METATYPE(QList<ContactData>)

class COMMONSHARED_EXPORT ContactBlock : public EjBlock
{
//    Q_OBJECT
public:
    ContactBlock() {type = 4;}
    ~ContactBlock() override { qDeleteAll(lData);  lData.clear();}
    QString name;
    QList<ContactData*>lData;

    EjBlock* makeCopy() override;
    bool compare(const EjBlock &other) const override;
    QDataStream& write(QDataStream &os) const override {
        EjBlock::write(os);
        writeSmallString(os,name);
        quint8 size = lData.size(); os << size;
        foreach (ContactData *contact, lData) {
            contact->write(os);
        }
        return os;
    }

    QDataStream& read(QDataStream &is) override{
        quint8 size;
        ContactData* contact;
        EjBlock::read(is);
        readSmallString(is,name);
        is >> size;
        for(int i = 0; i < size; i++) {
            contact = new ContactData();
            contact->read(is);
            lData.append(contact);
        }
        return is;
    }
    void calcBlock(int &index, EjCalcParams *calcParams) override;

};

//Q_DECLARE_METATYPE(ContactBlock)
//Q_DECLARE_METATYPE(QList<ContactBlock>)

//class ImageBlock_old : public EjBlock
//{
//public:
//    ImageBlock_old() { type = IMAGE_D; vid = 0; m_descent = 0; m_is_interactive = true; m_show_border = true; }
//    virtual ~ImageBlock_old(){}
////    ~ContactBlock() {lData.clear();}
////    QString name;
//    QByteArray name;
//    QImage small_image;
//    quint8 vid;
//    int m_descent;
//    quint16 m_width_image;
//    quint16 m_height_image;
//    bool m_is_interactive;
//    bool m_show_border;

//    EjBlock* makeCopy();
//    bool compare(const EjBlock &other) const;
//    QDataStream& write(QDataStream &os) const;

//    QDataStream& read(QDataStream &is);
//};

//<<<<<<< local
////class COMMONSHARED_EXPORT EjPropDoc : public QObject, public EjGroupBlock
//class INTERFACESHARED_EXPORT EjPropDoc : public QObject, public EjGroupBlock
//{
//    Q_OBJECT
//    Q_PROPERTY(QString numDoc READ numDoc NOTIFY numDocChanged)

//public:
//    enum NumProp {
//        DOC_KEY      = 0,
//        DOC_LINK,
//    };

//    struct Link
//    {
//        Link(QUuid linkKey) { doc = NULL; key = linkKey; }
//        ~Link();
//        QUuid key;
//        EjDocument *doc;
//    };

//    EjPropDoc(QUuid docKey = QUuid());
//    virtual ~EjPropDoc();

//    Q_INVOKABLE QVariant getLinks();

////    void createDefault(QList<EjBlock*> *lBlocks, int index) override;
//    virtual void beforeCalc(EjCalcParams *calcParams) override;
//    virtual void afterCalc(EjCalcParams *calcParams) override;
//    virtual void childCalc(EjBlock *child, EjCalcParams *calcParams) override;
//    void addLink(QUuid linkKey);
//    void removeLink(QUuid linkKey);
//    Q_INVOKABLE void removeLink(int index);
//    Q_INVOKABLE void copyLinkToClipboard();
//    Q_INVOKABLE void addLinkFromClipboard();
//    QString numDoc() const;

//    QUuid m_key;
//    QList<Link*> m_lLinks;

//signals:
//    void numDocChanged();

//protected:
//    QList<EjBlock*> *m_lBlocks;

//};

////class INTERFACESHARED_EXPORT EjDocument : public QObject
//class COMMONSHARED_EXPORT EjDocument : public QObject
//{
//    Q_OBJECT
//  public:
//    EjDocument(QUuid key = QUuid(), QObject *parent=0);
//    ~EjDocument();
//    void clear();

//    QList<EjBlock*> *lBlocks;
//    QList<EjString*> *lStrings;
//    QList<EjPage*> *lPages;
////    QList<EjFragmentBlock*> lFragments;
//    QList<EjTableBlock*> *lTables;
//    QList<LargeTextBlock*>  *lLabels_bak;
//    QList<LabelBlock*>  *lLabels;
//    QList<EjBaseStyle*>  *lStyles;
//    QList<EjBaseStyle*>  *lNewStyles;
////    QString title;
////    QStringList lTags;
//    QDateTime dt_modify;
//    quint32 user_key;
//    Q_INVOKABLE QDateTime getTime() { return dt_modify; }
//    Q_INVOKABLE quint32 getUserKey() { return user_key; }
//    EjBaseStyle *getStyle(int num);
//    void copy(EjDocument *doc);
//    void move(EjDocument *doc);
//    EjBaseStyle *getUndefinedStyle(int num);
//    EjTextStyle *getTextStyle(int num);
//    EjTextStyle *createDefaultTextStyle();
//    EjParagraphStyle *createDefaultPrgStyle();
//    EjParagraphStyle *getParagraphStyle(int num);
//    EjParagraphStyle *fromParagraphStyles(EjParagraphStyle *inputStyle);
//    EjCellStyle *createDefaultCellStyle();
//    EjCellStyle *getCellStyle(int num);
//    Q_INVOKABLE EjPropDoc *getPropDoc();
//    int nextTableKey();
//    int nextLabelKey();

//};
//=======
//>>>>>>> other

//class COMMONSHARED_EXPORT Common
//{

//public:
//    Common();
//};

#endif // COMMON_H
