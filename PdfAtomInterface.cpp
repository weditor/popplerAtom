//
// Created by weditor on 18-7-31.
//

#include <poppler/GlobalParams.h>
#include <poppler/PDFDocFactory.h>
#include "PdfAtomInterface.h"
#include "goo/GooString.h"
#include "AtomOutputDev.h"

// todo : delete these global vars
GBool fontFullName = gFalse;
GBool xml = gFalse;

extern GlobalParams *globalParams;

void initGlobalParams(){
    if(!globalParams) {
        globalParams = new GlobalParams();
    }
}

void destroyGlobalParams() {
    if (globalParams) {
        delete globalParams;
        globalParams = nullptr;
    }
}

PdfAtomInterface::PdfAtomInterface(const char *pdfName, const char* ownerPW, const char * userPW) {
//    globalParams = new GlobalParams();
//    globalParams.
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
}

PdfAtomInterface::~PdfAtomInterface() {
    delete globalParams;
    delete(m_pdfName);
    if (m_ownerPW){
        delete(m_ownerPW);
    }
    if (m_userPW){
        delete(m_userPW);
    }
}

void PdfAtomInterface::errQuiet(GBool errQuiet) {
    globalParams->setErrQuiet(errQuiet);
}

GBool PdfAtomInterface::isOk() {
    return m_doc->isOk();
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

void PdfAtomInterface::renderHtml(unsigned int pageNum, float scale) {
    auto *atomOut = new AtomOutputDev();
    if (atomOut->isOk())
    {
//        m_doc->displayPages(atomOut, pageNum, pageNum, DFLT_SOLUTION * scale, DFLT_SOLUTION * scale, 0,
//                          gTrue, gFalse, gFalse);
        m_doc->displayPage(atomOut, pageNum, DFLT_SOLUTION*scale, DFLT_SOLUTION*scale, 0,
                           gTrue, gFalse, gFalse);
    }
}

