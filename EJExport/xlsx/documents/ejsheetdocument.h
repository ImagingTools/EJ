#ifndef WORKSHEETS_DOCUMENT_H
#define WORKSHEETS_DOCUMENT_H

#include <QXmlStreamWriter>
#include <QFile>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>


const int INDEX_STRING = 3;
const int INDEX_COLUMN = 2;

class EjSheetDocument {

	QByteArray streamData;
	QXmlStreamWriter* documentWriter;
	QJsonArray sheetDataModel;
	QJsonArray mergeCellsModel;
	// QJsonArray colsModel;
	QJsonArray rowsModel;
	QMap<int, float> colsModel;
	int index_string = INDEX_STRING;
	int index_column = INDEX_COLUMN;
	int count_cells = 0;
	QString alphabet = "0ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	bool mergeCellsExist = false;
	bool colsExist = false;
	int countColsCurrentTable = - 1;
	int countStrCurrentTable = - 1;
	bool activeTable = false;
	int maxIndexColForTable = 1;
	int pageOrientation = 0;
	double paperWidth = 210.00;
	double paperHeight = 297.00;
	double pageTopMargin = 0.750;
	double pageBottomMargin = 0.750;
	double pageLeftMargin = 0.750;
	double pageRightMargin = 0.750;

	/*Ситуация аналогичная тексту, будем хранить id этих стилей,
      пока не встретим текстовый блоки, как встретим добавим текст сразу со стилями*/
	int fillId, borderId;
	float heightCell;

public:
	EjSheetDocument();
	~EjSheetDocument();
	bool addTextNumber(int number, int numberStyle);
	bool addTextNumberIntoTable(int number, int numberStyle);
	bool newString();
	bool newColumn();
	void setDefaultIndexColumn();
	void validate();
	QString getAlphabet();
	QByteArray getDocumentData();
	void addMergeCells(int rows, int cols);
	void addWidthCell(float width);
	void addHeightCell(float height);
	void setCountColsCurrentTable(int value);
	void setCountStrCurrentTable(int value);
	void setActiveTable(bool value);
	bool getActiveTable() const;
	int getIndexString() const;
	int getIndexColumn() const;
	void setMaxIndexColForTable(int value);

	int getFillId() const;
	void setFillId(int value);
	int getBorderId() const;
	void setBorderId(int value);
	double getDocWidth() const;
	void setDocWidth(double width);
	double getDocHeight() const;
	void setDocHeight(double height);
	int getOrientation() const;
	void setOrientation(int orientation);
	double getPageTopMargin() const;
	void setPageTopMargin(double topMargin);
	double getPageBottomMargin() const;
	void setPageBottomMargin(double bottomMargin);
	double getPageLeftMargin() const;
	void setPageLeftMargin(double leftMargin);
	double getPageRightMargin() const;
	void setPageRightMargin(double rightMargin);
};

#endif // WORKSHEETS_DOCUMENT_H
