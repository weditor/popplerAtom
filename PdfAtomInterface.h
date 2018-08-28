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
class StructElement;
class AtomOutputDev;
class CairoOutputDev;


#define DFLT_SOLUTION 72


extern void initGlobalParams(const char* popplerData = nullptr);
extern void destroyGlobalParams();


class PdfAtomInterface {
public:
    explicit PdfAtomInterface(const char *pdfName, const char* ownerPW=nullptr, const char * userPW=nullptr);
    ~PdfAtomInterface();

    GBool isOk();
    int getNumPages();
    void getDocInfo();
    void renderHtml(unsigned int pageNum, PageInfos &pageInfos, float scale=1.0);
    std::vector<PdfStructInfo> getStructure();
    void cropImage(char **data, unsigned long* size,
            unsigned int pageNum, unsigned int x=0, unsigned int y=0, unsigned int w=0, unsigned int h=0, float scale=1.0);
private:
    void getStructureInner(const StructElement *element, std::vector<PdfStructInfo> &infoVec);
    GlobalParams *globalParams;
    GooString *m_pdfName;
    GooString *m_ownerPW;
    GooString *m_userPW;
    PDFDoc *m_doc;
    AtomOutputDev *m_atomOutputDev;
    CairoOutputDev *m_cairoOutputDev;
};


#endif //POPPLER_PDFATOMINTERFACE_H
