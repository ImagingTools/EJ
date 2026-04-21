
#include "ejconvertorexcel.h"
#include "ejtableblocks.h"
#include "ejutils.h"

#include <QtCore/QFile>
#include <QtCore/QDir>


const QString EjConvertorExcel::format = "xlsx";


bool EjConvertorExcel::unzip() const {
    return zipper.unzip(temp_folder + subfolder,
                        ":/formats/" + format + ".zip");
}

bool EjConvertorExcel::zip(QString const& file_name) const {
    return zipper.toZip(temp_folder + subfolder, file_name);
}

bool EjConvertorExcel::readDoc(EjDocument *doc){
    bool result = true;
	QList<EjBlock*> *list = doc->lBlocks;
	EjUtils utils;
    int maxIndexCol = utils.findMaxIndexColumnInDocumentByBlocks(*list);
	sheet_document->setMaxIndexColForTable(maxIndexCol);

	EjAttrProp* docAttributes = doc->attributes();
	EjDocLayout* docLayout = docAttributes->getDocLayout();

	int docHeight = docLayout->docHeight();
	int docWidth = docLayout->docWidth();
	EjDocLayout::Orientation docOrientation = docLayout->docOrientation();
	sheet_document->setDocWidth(docWidth/100);
	sheet_document->setDocHeight(docHeight/100);
	sheet_document->setOrientation(docOrientation);

	EjDocMargings* docMargins = docAttributes->getDocMargings();
	int bottomMargin = docMargins->bottom();
	int topMargin = docMargins->top();
	int leftMargin = docMargins->left();
	int rightMargin = docMargins->right();
	sheet_document->setPageTopMargin(topMargin/100*0.0394);
	sheet_document->setPageBottomMargin(bottomMargin/100*0.0394);
	sheet_document->setPageLeftMargin(leftMargin/100*0.0394);
	sheet_document->setPageRightMargin(rightMargin/100*0.0394);

    for(int i = 0; i < list->size(); i++){
		EjBlock* block = list->at(i);
		EjExcelDocumentWriter *editor =editors.value(block->type, nullptr);
        if(editor != nullptr){
			editor->edit(sheet_document, sharedStrings_document, styles_document, list, i);
        }
    }
	utils.checkUnaddedText(sheet_document, sharedStrings_document, styles_document);
    return result;
}

bool EjConvertorExcel::write() {
    QFile file_worksheets(temp_folder + subfolder + "/xl/worksheets/sheet1.xml");
    bool result = file_worksheets.open(QIODevice::WriteOnly);
    if (result) {
		result = file_worksheets.write(sheet_document->getDocumentData()) != -1;
        file_worksheets.close();
    }
    QFile file_sheet(temp_folder + subfolder + "/xl/sharedStrings.xml");
    if ((result &= file_sheet.open(QIODevice::WriteOnly))) {
		result = file_sheet.write(sharedStrings_document->getDocumentData()) != -1;
        file_sheet.close();
    }
    QFile file_styles(temp_folder + subfolder + "/xl/styles.xml");
    if ((result &= file_styles.open(QIODevice::WriteOnly))) {
		result = file_styles.write(styles_document->getDocumentData()) != -1;
        file_styles.close();
    }
    return result;
}

void EjConvertorExcel::clear() {
    QDir(temp_folder + subfolder).removeRecursively();
}

EjConvertorExcel::EjConvertorExcel():
	temp_folder(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + '/' + QUuid::createUuid().toString(QUuid::WithoutBraces) + '/'), subfolder(format + "_test/" ){
	sheet_document = new EjSheetDocument();
	sharedStrings_document = new EjSharedStringsDocument();
	styles_document = new EjStylesDocument();
    editors.insert(e_typeBlocks::TEXT, new TextWriterExcel());
    editors.insert(e_typeBlocks::SPACE, new SpaceWriterExcel());
    editors.insert(e_typeBlocks::ENTER, new EnterWriterExcel());
    editors.insert(e_typeBlocks::EXT_TABLE, new TableWriterExcel());
    editors.insert(e_typeBlocks::BASECELL, new CellWriterExcel());
    editors.insert(e_typeBlocks::NUM_STYLE, new StyleWriterExcel());
    editors.insert(e_typeBlocks::END_GROUP, new EndTableWriterExcel());
}

bool EjConvertorExcel::convert(EjDocument *doc, QString const& file_name) {
    bool result = unzip();
    result &= readDoc(doc);
    result &= write();
    result &= zip(file_name);
    clear();
    return result;
}

EjConvertorExcel::~EjConvertorExcel() {
	for (QMap<quint8, EjExcelDocumentWriter*>::iterator i = editors.begin(); i != editors.end(); ++i) {
        delete *i;
    }
	delete sheet_document;
	delete sharedStrings_document;
	delete styles_document;
}

EjSheetDocument* EjConvertorExcel::getSheetDocument(){
	return sheet_document;
}
