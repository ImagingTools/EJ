#ifndef REPORTDOC_H
#define REPORTDOC_H
#include <QString>
#include "ejcommon.h"

#if defined(REPORT_LIBRARY)
#  define REPORTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define REPORTSHARED_EXPORT Q_DECL_IMPORT
#endif

class REPORTSHARED_EXPORT ReportDoc
{

public:
    ReportDoc();
    bool createReport(EjDocument *doc, QString vid);
    bool createReport2(EjDocument *doc, QString vid);
};

#endif // REPORTDOC_H
