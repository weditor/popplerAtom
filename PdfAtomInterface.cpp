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

// todo : delete these global vars
GBool fontFullName = gFalse;
GBool xml = gFalse;

extern GlobalParams *globalParams;

void initGlobalParams(){
    if(!globalParams) {
        globalParams = new GlobalParams("/usr/share/poppler");
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
}

PdfAtomInterface::~PdfAtomInterface() {
    delete(m_pdfName);
    if (m_ownerPW){
        delete(m_ownerPW);
    }
    if (m_userPW){
        delete(m_userPW);
    }
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

void PdfAtomInterface::renderHtml(unsigned int pageNum, PageInfos &pageInfos, float scale) {
    auto *atomOut = new AtomOutputDev();
    if (atomOut->isOk())
    {
        m_doc->displayPage(atomOut, pageNum, DFLT_SOLUTION*scale, DFLT_SOLUTION*scale, 0,
                           gTrue, gFalse, gFalse);
        atomOut->getInfo(pageNum, pageInfos);
    }
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

