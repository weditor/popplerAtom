//
// Created by weditor on 18-7-31.
//

#ifndef POPPLER_PDFATOMINTERFACE_H
#define POPPLER_PDFATOMINTERFACE_H

#include "poppler_atom_types.h"
// #include "gtypes.h"
#include <string>

class GlobalParams;
class GooString;
class PDFDoc;
class StructElement;
class AtomOutputDev;

namespace poppler
{
class document;
}

#define DFLT_SOLUTION 72

extern void initGlobalParams(const char *popplerData = nullptr);
extern void destroyGlobalParams();

class PdfAtomInterface
{
public:
    explicit PdfAtomInterface(const char *pdfName, const char *ownerPW = nullptr, const char *userPW = nullptr);
    ~PdfAtomInterface();

    /**! \brief is the pdf ok.
     * \return is the pdf file ok.
     */
    bool isOk();

    /**
     * \return total page number of current pdf
     */
    int getNumPages();
    void getDocInfo();

    /**! \brief get all items in one page.
     * 
     * \param pageNum whitch page to read.
     * \param pageInfos variable to store all items.
     * \param scale resize the output coordinates.
     * \return 
     */
    void renderHtml(unsigned int pageNum, PageInfos &pageInfos, float scale = 1.0);

    /**! \brief get pdf tag info
     * \return 
     */
    std::vector<PdfStructInfo> getStructure();

    /**! \brief crop rectangle area as png from pdf page,
     * \param data data pointer to store image content
     * \param size store the size of image returned.
     * \param out_w width of output image
     * \param out_h height of output image
     * \param pageNum whitch page to crop.
     * \param x x point of the crop rect in pdf page.
     * \param y y point of the crop rect in pdf page.
     * \param w width of the crop rect in pdf page.
     * \param h height of the crop rect in pdf page.
     * \param scale scale the output image.
     */
    void cropImage(char **data, unsigned long *size,
                   unsigned long *out_w, unsigned long *out_h, unsigned int pageNum,
                   int x = -1, int y = -1, int w = -1, int h = -1, float scale = 1.0);

private:
    void getStructureInner(const StructElement *element, std::vector<PdfStructInfo> &infoVec);
    GlobalParams *globalParams;
    GooString *m_pdfName;
    GooString *m_ownerPW;
    GooString *m_userPW;
    PDFDoc *m_doc;
    AtomOutputDev *m_atomOutputDev;
    poppler::document *m_poppler_document;
};

#endif //POPPLER_PDFATOMINTERFACE_H
