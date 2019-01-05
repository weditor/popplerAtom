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
#include "poppler/cpp/poppler-page-renderer.h"
#include "poppler/cpp/poppler-page.h"
#include "poppler/cpp/poppler-document.h"


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
    m_poppler_document = poppler::document::load_from_file(
        m_pdfName->toStr(), 
        (m_ownerPW?m_ownerPW->toStr():""), 
        (m_userPW?m_userPW->toStr():"")
    );
}

PdfAtomInterface::~PdfAtomInterface() {
    delete(m_poppler_document);
    delete(m_atomOutputDev);
    delete(m_doc);

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


#include <iostream>
void PdfAtomInterface::cropImage(char **data, unsigned long *size, 
        unsigned long* out_w, unsigned long* out_h, unsigned int pageNum, 
        int x, int y, int w, int h, float scale) {

    if (pageNum <= 0 || pageNum >= m_poppler_document->pages()) {
        return;
    }
    poppler::page* page = m_poppler_document->create_page(pageNum-1);
    if (!page) {
        return;
    }

    poppler::page_renderer pr;
    pr.set_render_hint(poppler::page_renderer::antialiasing, true);
    pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);

    poppler::image img = pr.render_page(page, 72.0*scale, 72.0*scale, 
                                        x, y, w, h);
    if (!img.is_valid()) {
        delete page;
        return;
    }
    *size = img.width() * img.height() * 4;
    *out_w = img.width();
    *out_h = img.height();
    
    std::cout << img.format() << std::endl;
    *data = (char *)malloc(sizeof(char) * (*size));
    memcpy(*data, img.const_data(), *size);

    delete page;
}

