//
// Created by weditor on 18-7-31.
//

#ifndef POPPLER_PDFATOMINTERFACE_H
#define POPPLER_PDFATOMINTERFACE_H

#include "poppler_atom_types.h"
#include "gtypes.h"
#include <string>

class GlobalParams;
class GooString;
class PDFDoc;

#define DFLT_SOLUTION 72


extern void initGlobalParams();
extern void destroyGlobalParams();


class PdfAtomInterface {
public:
    explicit PdfAtomInterface(const char *pdfName, const char* ownerPW=nullptr, const char * userPW=nullptr);
    ~PdfAtomInterface();

    // todo: GlobalParams移动到外外面, 是否保持静默. 不打印错误.
    void errQuiet(GBool flag);
    GBool isOk();
    int getNumPages();
    void getDocInfo();
    void renderHtml(unsigned int pageNum, PageInfos &pageInfos, float scale=1.0);
private:
    GlobalParams *globalParams;
    GooString *m_pdfName;
    GooString *m_ownerPW;
    GooString *m_userPW;
    PDFDoc *m_doc;
};


#endif //POPPLER_PDFATOMINTERFACE_H
