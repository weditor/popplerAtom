//
// Created by weditor on 18-7-31.
//

#include <poppler/GlobalParams.h>
#include <poppler/PDFDocFactory.h>
#include <poppler/StructElement.h>
#include "StructTreeRoot.h"
#include "PdfAtomInterface.h"
#include "goo/GooString.h"
#include "AtomOutputDev.h"
#include "CairoOutputDev.h"

// todo : delete these global vars
GBool fontFullName = gFalse;
GBool xml = gFalse;


void initGlobalParams(const char* popplerData){
    if(!globalParams) {
        if (popplerData){
            globalParams = new GlobalParams(popplerData);
        }
        else {
            globalParams = new GlobalParams("/usr/share/poppler");
        }
    }
}

void destroyGlobalParams() {
    if (globalParams) {
        delete globalParams;
        globalParams = nullptr;
    }
}

PdfAtomInterface::PdfAtomInterface(const char *pdfName, const char* ownerPW, const char * userPW) {
    m_pdfName = new GooString(pdfName);

    // todo: 应该在打开pdf之后立即删除用户名密码??
    if (ownerPW && ownerPW[0]) {
        m_ownerPW = new GooString(ownerPW);
    } else {
        m_ownerPW = nullptr;
    }
    if (userPW && userPW[0]) {
        m_userPW = new GooString(userPW);
    } else {
        m_userPW = nullptr;
    }

    m_doc = PDFDocFactory().createPDFDoc(*m_pdfName, m_ownerPW, m_userPW);
    m_atomOutputDev = new AtomOutputDev();
}

PdfAtomInterface::~PdfAtomInterface() {
    delete(m_doc);
    delete(m_atomOutputDev);
    if (m_ownerPW){
        delete(m_ownerPW);
    }
    if (m_userPW){
        delete(m_userPW);
    }
    delete(m_pdfName);
}

GBool PdfAtomInterface::isOk() {
    return m_doc->isOk() && m_atomOutputDev->isOk();
}

int PdfAtomInterface::getNumPages() {
    return m_doc->getNumPages();
}

void PdfAtomInterface::getDocInfo() {
//    Object info = m_doc->getDocInfo();
//    if (info.isDict()) {
//        docTitle = getInfoString(info.getDict(), "Title");
//        author = getInfoString(info.getDict(), "Author");
//        keywords = getInfoString(info.getDict(), "Keywords");
//        subject = getInfoString(info.getDict(), "Subject");
//        date = getInfoDate(info.getDict(), "ModDate");
//        if( !date )
//            date = getInfoDate(info.getDict(), "CreationDate");
//    }
}

void PdfAtomInterface::renderHtml(unsigned int pageNum, PageInfos &pageInfos, float scale) {
    m_doc->displayPage(m_atomOutputDev, pageNum, DFLT_SOLUTION*scale, DFLT_SOLUTION*scale, 0,
                       gTrue, gFalse, gFalse);
    m_atomOutputDev->getInfo(pageNum, pageInfos);
}

std::vector<PdfStructInfo> PdfAtomInterface::getStructure() {
    std::vector<PdfStructInfo> structVec;
    auto catalog = this->m_doc->getCatalog();
    if (!catalog || !catalog->isOk()) {
        return structVec;
    }
    StructTreeRoot* structTree = catalog->getStructTreeRoot();
    if (!structTree) {
        return structVec;
    }
    for (unsigned i = 0; i < structTree->getNumChildren(); i++) {
        getStructureInner(structTree->getChild(i), structVec);
    }
    return structVec;
}

void PdfAtomInterface::getStructureInner(const StructElement *element, std::vector<PdfStructInfo> &infoVec) {
    // todo: attrubutes, Object
    if (element->isObjectRef()) {
        // printf("Object %i %i\n", element->getObjectRef().num, element->getObjectRef().gen);
        return ;
    }

    if (!element->isOk()) {
        return ;
    }
    PdfStructInfo info;
    if (element->isContent()){
        info.type = "text";
        info.mcid = element->getMCID();
        Ref ref;
        if (element->getPageRef(ref)) {
            info.page = this->m_doc->findPage(ref.num, ref.gen);
        }
    }
    else {
        info.type = element->getTypeName();
//        info.mcid = element->getMCID();
        for (unsigned i = 0; i < element->getNumChildren(); i++) {
            getStructureInner(element->getChild(i), info.children);
        }
    }
    infoVec.push_back(info);
}

static void renderPage(cairo_surface_t * surface, PDFDoc *doc, CairoOutputDev *cairoOut, int pg, int crop_x, int crop_y, float scale)
{
    cairo_t *cr;
    cairo_status_t status;

    cr = cairo_create(surface);

    cairoOut->setCairo(cr);
    cairoOut->setAntialias(CAIRO_ANTIALIAS_DEFAULT);

    cairo_save(cr);

    cairo_translate (cr, -crop_x, -crop_y);
    cairo_scale (cr, scale, scale);
    doc->displayPageSlice(cairoOut,
                          pg,
                          72.0, 72.0,
                          0, /* rotate */
                          gTrue, /* useMediaBox */
                          gFalse, /* Crop */
                          gFalse,
                          -1, -1, -1, -1);
    cairo_restore(cr);
    cairoOut->setCairo(nullptr);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_restore(cr);

    status = cairo_status(cr);
    if (status)
        fprintf(stderr, "cairo error: %s\n", cairo_status_to_string(status));
    cairo_destroy (cr);
}


#include "goo/ImgWriter.h"
#include "goo/JpegWriter.h"
#include "goo/PNGWriter.h"
static void writePageImage(GooString *filename, cairo_surface_t *surface, float resolution)
{
    ImgWriter *writer = nullptr;
    FILE *file;
    int height, width, stride;
    unsigned char *data;

    // use libpng
    writer = new PNGWriter(PNGWriter::RGBA);
//#ifdef ENABLE_LIBJPEG
//        if (gray)
//            writer = new JpegWriter(JpegWriter::GRAY);
//        else
//            writer = new JpegWriter(JpegWriter::RGB);
//
//        static_cast<JpegWriter*>(writer)->setProgressive(jpegProgressive);
//        if (jpegQuality >= 0)
//            static_cast<JpegWriter*>(writer)->setQuality(jpegQuality);
//#endif

    file = fopen(filename->getCString(), "wb");

    if (!file) {
        fprintf(stderr, "Error opening output file %s\n", filename->getCString());
        exit(2);
    }

    height = cairo_image_surface_get_height(surface);
    width = cairo_image_surface_get_width(surface);
    stride = cairo_image_surface_get_stride(surface);
    cairo_surface_flush(surface);
    data = cairo_image_surface_get_data(surface);

    if (!writer->init(file, width, height, resolution, resolution)) {
        fprintf(stderr, "Error writing %s\n", filename->getCString());
        exit(2);
    }
    unsigned char *row = (unsigned char *) gmallocn(width, 4);

    for (int y = 0; y < height; y++ ) {
        uint32_t *pixel = (uint32_t *) (data + y*stride);
        unsigned char *rowp = row;
        int bit = 7;
        for (int x = 0; x < width; x++, pixel++) {
            // unpremultiply into RGBA format
            uint8_t a;
            a = (*pixel & 0xff000000) >> 24;
            if (a == 0) {
                *rowp++ = 0;
                *rowp++ = 0;
                *rowp++ = 0;
            } else {
                *rowp++ = (((*pixel & 0xff0000) >> 16) * 255 + a / 2) / a;
                *rowp++ = (((*pixel & 0x00ff00) >>  8) * 255 + a / 2) / a;
                *rowp++ = (((*pixel & 0x0000ff) >>  0) * 255 + a / 2) / a;
            }
            *rowp++ = a;
        }
        writer->writeRow(&row);
    }
    gfree(row);
    writer->close();
    delete writer;
    fclose(file);
}


void PdfAtomInterface::cropImage(void **data, unsigned int *size,
        unsigned int pageNum, unsigned int x, unsigned int y, unsigned int w, unsigned int h, float scale) {
    auto *cairoOut = new CairoOutputDev();
    cairoOut->startDoc(m_doc);

    double pg_w = m_doc->getPageMediaWidth(pageNum)*scale;
    double pg_h = m_doc->getPageMediaHeight(pageNum)*scale;
    int crop_w = w;
    int crop_h = h;

    if (crop_w == 0)
        crop_w = (int)ceil(pg_w);

    if (crop_h == 0)
        crop_h = (int)ceil(pg_h);

    int output_w =  (x + crop_w > pg_w ? (int)ceil(pg_w - x) : w);
    int output_h = (y + crop_h > pg_h ? (int)ceil(pg_h - y) : h);
    cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, output_w, output_h);
    renderPage(surface, m_doc, cairoOut, pageNum, x, y, scale);

    cairo_status_t status;

    writePageImage(new GooString("test.png"), surface, DFLT_SOLUTION*scale);
    cairo_surface_finish(surface);
    status = cairo_surface_status(surface);
    if (status)
        fprintf(stderr, "cairo error: %s\n", cairo_status_to_string(status));
    cairo_surface_destroy(surface);
}

