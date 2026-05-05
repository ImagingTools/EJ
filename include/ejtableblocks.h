/********************************************************************************
**
**  Copyright (C) 2014 Victor Shcherbina
**  This file is part of the EasyJotter
**
********************************************************************************/

#ifndef TABLEBLOCKS_H
#define TABLEBLOCKS_H
// #include <QQuickItem>
#include "ej_interfaces.h"
#include "ejtextcontrol.h"

//class TableBlocks
//{
//public:
//    TableBlocks();
//};

class COMMONSHARED_EXPORT ColumDelegate : public QObject
{
    Q_OBJECT
public:
    ColumDelegate(QObject *parent = nullptr);
	virtual QQuickItem *onClick(int statusMode, EjTextControl *control, EjCellBlock* cell, QQuickItem *parent);
	virtual void prepare(EjCellBlock* cell);
	virtual bool isStandartEdit(EjCellBlock *cell);
    virtual JotInterface::ResultMenuActivate menuActivate(QString command, QString data, PopupMenuModel *popupModel, e_statusMode statusMode, int row, int column); // 0 - not active, 1 - item active, 2 -  menu active
    virtual QQuickItem* getActivePropItem(int vid, QQuickItem *parent, QString command, QString data, int row, int column);

signals:
    void onFinishEdit();
};
/*
class COMMONSHARED_EXPORT AdditionProps : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int security READ security WRITE setSecurity NOTIFY securityChanged)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(QString refGuid READ refGuid WRITE setRefGuid NOTIFY refGuidChanged)
    Q_PROPERTY(bool isMultiSelect READ isMultiSelect WRITE setIsMultiSelect NOTIFY isMultiSelectChanged)

public:
QString name() const;

int security() const;

int type() const;

int width() const
{
    return m_width;
}

public slots:
void setName(QString name);

void setSecurity(int security);

void setType(int type)
{
    if (m_type == type)
        return;

    m_type = type;
    emit typeChanged(m_type);
}

void setWidth(int width)
{
    if (m_width == width)
        return;

    m_width = width;
    emit widthChanged(m_width);
}

signals:
void nameChanged(QString name);

void securityChanged(int security);

void typeChanged(int type);

void widthChanged(int width);

private:
QString m_name;
int m_security;
int m_type;
int m_width;
};
*/

class COMMONSHARED_EXPORT EjTableBlock : public EjGroupBlock
{
public:
    enum NumProp {
        TBL_KEY = 0,
        TBL_COLUMS,
        TBL_ROWS,
        TBL_VID,
        TBL_STYLE,
        TBL_NAME,
        TBL_COLUM_NAME,
        TBL_COLUM_MAX_WIDTH,
        TBL_COLUM_MIN_WIDTH,
        TBL_ADDITIONAL,
        TBL_PROP_COLUM,
        TBL_PROP_ROW,
        TBL_ACCESS = 100,
        TBL_COLUM_ACCESS,
        TBL_ROW_ACCESS,
        TBL_PROP_ACCESS,
        TBL_ADD_ROW_ACCESS,
        TBL_ADD_COLUM_ACCESS,
    };

	EjTableBlock();
	EjTableBlock(quint32 key_);
	EjTableBlock(int rows, int colums, EjDocument *doc, int index);
	virtual ~EjTableBlock() override;
    quint8 style; // 0,1 - small, price, qty, full 2 - checking 3 - numbering     // bits
    quint8 vid;  // 0 - shoplist, 1 - cleantable ...
    quint8 accuracy;
    int key;
//    quint16 num;
    int start_column;
    int start_row;
//    qint32 startBlock;
//    qint32 endBlock;
//    qint32 countBlocks;
    qint32 deltaProps;

    QColor borderColor;
    quint8 spacing;

    int m_rowStartSelect, m_columStartSelect, m_rowEndSelect, m_columEndSelect;
    int m_startSelectBlock, m_endSelectBlock;
	EjDocument *m_doc;


    qint32 startCell() { return m_index + deltaProps; }
    qint32 endBlock() { return m_index + m_counts; }

    enum VidProp {
        SHOP_LIST = 0,
        CLEAN_TABLE,
        LINKED_TABLE
    };

    struct ColumProp
    {
        ColumProp() {delegate = NULL;}
        ColumDelegate *delegate;
		EjSizeProp sizeProp;
    };

//    QList<TableFragment*> lFragments;
    QList<ColumProp*> lColums;
	QList<EjSizeProp*> lRows;
//    int nRows() const { return m_nRows; }
//    int nColums() const { return m_nColums; }
    int nRows() const { return lRows.count(); }
    int nColums() const { return lColums.count(); }
    int calc();
	EjBlock* makeCopy() override;
//    bool compare(const EjBlock &other) const override;
//    QDataStream& write(QDataStream &os) const override;
//    QDataStream& read(QDataStream &is) override;
	void calcBlock(int &index, EjCalcParams *calcParams) override;
    bool isSelected(int &index, int &startSelect, int &endSelect) override;


	void cellParams(EjBlock *block, int &row, int &colum, QList<EjBlock *> *l_blocks = nullptr);
	void addString(EjTextControl *control, EjBlock *curBlock, bool force = true);
	void addColum(EjTextControl *control, EjBlock *curBlock);
	void delString(QList<EjBlock *> *l_blocks, int &active_block);
	void delColum(QList<EjBlock*> *l_blocks, int &active_block);
	void moveString(EjTextControl *control, EjBlock *curBlock, bool isUp);
	void moveColum(EjTextControl *control, EjBlock *curBlock, bool isLeft);

	int cellIndex(int row, int colum, QList<EjBlock *> *l_blocks);
    int cellIndex(int row, int colum);
    int prevCell(int index);
    int currCellIndex(int index);
	EjCellBlock *currentCell(int index);
	EjCellBlock* getCell(int row, int colum);
	EjCellBlock *prevCell(EjBlock *block); // slow function!
    int nextCell(int index);
	EjCellBlock *nextCell(EjBlock *block); // slow function!
	bool chekMinMax(int &startRow, int &startColum, int &endRow, int &endColum, EjCellBlock *cell); // return true if startRow or startColum is changed
	void setCellStyles(int startRow,int startColum,int endRow,int endColum,EjCellStyle *style);
	void setCellStyle(EjCellBlock *cell,EjCellStyle *style);
	void clearCellStyle(EjCellBlock *cell);
	void setParagraphStyle(int startRow,int startColum,int endRow,int endColum,EjParagraphStyle *style);
	void setParagraphStyle(EjCellBlock *cell,EjParagraphStyle *style);
	EjCellStyle *getCellStyle(int block);
    void setColumDelegate(int colum, ColumDelegate *delegate);
    void removeColumDelegate(int colum);
	QQuickItem *onCellClicked(int statusMode, EjTextControl *control, EjCellBlock *cell, QQuickItem *parent);
    QString tableName();
    void setTableName(QString name);
    QString tableAdditional();
    void setTableAdditional(QString additional);
    QString columName(int colum);
    void setColumName(QString name, int colum);
    int lastIndexProp();
	void getAccessColum(EjPropAccessBlock *source, int colum);
	void setAccessColum(EjPropAccessBlock *source,int colum);
//    int columMaxWidth(int colum); // slow function!
    void setColumMaxWidth(quint16 width, int colum);
    void setColumMinWidth(quint16 width, int colum);
    bool containsMerginCells(int &startSelect, int &endSelect);
    bool containsMerginCells(int startRow,int startColum,int endRow,int endColum);
    void selectParams(int &startRow, int &startColum, int &endRow, int &endColum, int &startSelect, int &endSelect);


protected:
//    QList<EjBlock*> *m_lBlocks;

	EjCellStyle *fromStyles(EjCellStyle *newStyle);
	virtual void childCalc(EjBlock *child, EjCalcParams *calcParams) override;
	EjPropByteArrayBlock *getNameColumProp(int colum);
    int getColumPropIndex(int colum);
	EjPropBase *getPropFromMulty(int startIndex, int type, int vid);

//    quint32 m_nColums;
//    quint32 m_nRows;


};



class COMMONSHARED_EXPORT EjCellBlock : public EjTextBlock
{
public:
	EjCellBlock();
//    BaseCellBlock(int _vid) { BaseCellBlock::BaseCellBlock(); vid = _vid; }
	virtual ~EjCellBlock() override;
//    EjTableBlock *parent;
//    QString formula;
//    MAlign m_align;
//    quint8 font_size;
//    quint8 font_family;
    quint8 vid; // 0 - auto, 1 - text, 2 - number, 3 - formula, 4 - check, 4 - refbook
//    quint16 txtWidth;
//    quint16 txt_y;
//    qint16 txt_x;
    quint16 txtHeight;
    double value;
	EjCellStyle *cellStyle;

    bool visible;
//    bool isMerging;
    quint8 mergeRows;
    quint8 mergeColums;
//    QList<EjCellBlock*> *lMergeCells;

    enum Vid {
        CELL_AUTO = 0,
        CELL_TEXT,
        CELL_NUMBER,
        CELL_FORMULA,
        CELL_CHECK,
        CELL_REFBOOK,
        CELL_DATETIME,
//        ENDTABLE
    };

    enum NumProp {
        CELL_ADDITIONAL = 0,
        CELL_MERGE_COLUMS,
        CELL_MERGE_ROWS,
        CELL_NAME,
        CELL_VID,
        CELL_GUID_REF,
        CELL_GUID_DATA,
        CELL_MULTI_SELECT,
        CELL_HIDDEN_TEXT,
        CELL_ACCESS = 100,
    };

//    int calc();

	EjBlock* makeCopy() override {
		EjCellBlock *res = new EjCellBlock();
        copyData(res);
//        res->formula = formula;
//        res->m_align = m_align;
//        res->text = text;
        res->value = value;
        res->vid = vid;
        return res;
    }
	void copyCell(EjCellBlock *res)
    {
//        res->formula = formula;
//        res->text = text;
        res->value = value; res->vid = vid;
        //res->cellStyle = cellStyle;//
    }
    void setValue(double value);
    void setFormula(QString formula);
    QString formula();
    void setAdditional(QString additional);
    QString additional();
    void setHiddenText(QString hiddenText);
    QString hiddentext();
    void setTimeFormat(QString timeFormat);
    QString timeFormat();
    void setName(QString name);
    QString getName();
    void setVid(quint8 vidSource);
    void setGUIDRefBook(qint64 guidRef);
    qint64 getGUIDRefBook();
    void setIsMultiSelect(bool isMultiSelect);
    bool getIsMultiSelect();
    void setGUIDRefData(QList<qint64> lGUIDData);
    QList<qint64> getGUIDRefData();
	void getCurrentCellStyle(EjCellStyle *style);
	void setAccessBlock(EjPropAccessBlock* source);
	void getAccessBlock(EjPropAccessBlock* source);

	EjTableBlock *getTable();

	void addProp(EjPropBase *prop);
	EjPropBase *findProp(int num);
    void removeProp(int num, bool isAll = false);
	void clearAll(EjDocument *doc);
	void clearData(EjDocument *doc, bool isAll = false);

//    bool compare(const EjBlock &other) const override;
//    QDataStream& write(QDataStream &os) const override;

//    QDataStream& read(QDataStream &is) override;
	virtual  QQuickItem *getItem(int &index, EjCalcParams *calcParams, QQuickItem *parentItem) override;
    bool isSelected(int &index, int &startSelect, int &endSelect) override;
    void setText(const QString &source, EjTextControl *control = nullptr);
    QString getText();
//    void setParagraph(EjParagraphStyle *paragraph);
	void setTextStyle(EjTextStyle *style, EjTextControl *control);
//    void setCellStyle(EjCellStyle *style);
    void merge(int rows, int colums);
    void unMerge();
};


#endif // TABLEBLOCKS_H
