//
// Created by weditor on 18-8-4.
//


#include <iostream>
#include <cstring>
#include "poppler_atom_types.h"
#include "PdfAtomInterface.h"
#include "PdfAtomCApi.h"

void c_initGlobalParams(const char* popplerData) {
    initGlobalParams(popplerData);
}

void c_destroyGlobalParams(){
    destroyGlobalParams();
}

void * createAtomParser(const char *fileName, const char * ownerPW, const char * userPW){
    return new PdfAtomInterface(fileName, ownerPW, userPW);
}

void destroyAtomparser(void *parser) {
    delete((PdfAtomInterface*)parser);
}

bool isParserOk(void *parser) {
    return ((PdfAtomInterface*)parser)->isOk();
}

int getNumPages(void *parser) {
    return ((PdfAtomInterface*)parser)->getNumPages();
}

static char* string2char(const std::string &str) {
    char *dst = new char [str.size()+1];
    memcpy(dst, str.c_str(), str.size());
    dst[str.size()] = 0;
    return dst;
}


static CPdfItem* copyItems(const std::vector<PdfItem> &pdfItems) {
    if (pdfItems.empty()) {
        return nullptr;
    }

    auto* items = new CPdfItem[pdfItems.size()];
    for (size_t i = 0; i < pdfItems.size(); ++i) {
        items[i].id = pdfItems[i].id;
        items[i].mcid = pdfItems[i].mcid;
        items[i].type = pdfItems[i].type;
        items[i].text = string2char(pdfItems[i].text);

        items[i].left = pdfItems[i].left;
        items[i].top = pdfItems[i].top;
        items[i].right = pdfItems[i].right;
        items[i].bottom = pdfItems[i].bottom;

        items[i].font = pdfItems[i].font;
        items[i].style = pdfItems[i].style;

        items[i].children_len = pdfItems[i].children.size();
        items[i].children = copyItems(pdfItems[i].children);
    }
    return items;
}

static void deleteItems(CPdfItem* pdfItem, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        delete [] pdfItem[i].text;
        deleteItems(pdfItem[i].children, pdfItem[i].children_len);
    }
    delete [] pdfItem;
}

static CPdfPath* copyGraphs(const std::vector<PdfPath> &pdfPathes) {
    if (pdfPathes.empty()) {
        return nullptr;
    }

    auto *cPdfPath = new CPdfPath[pdfPathes.size()];
    for (unsigned long i = 0; i < pdfPathes.size(); ++i) {

//        CPdfLine &cPdfLine = pdfPathes[i].lines;
//        const PdfPath &pdfPath = pdfPathes[i].pathes[i];

        cPdfPath[i].line_len = pdfPathes[i].lines.size();
        cPdfPath[i].lines = new CPdfLine[pdfPathes[i].lines.size()];
        cPdfPath[i].type = pdfPathes[i].m_type;
        for (unsigned long j = 0; j < pdfPathes[i].lines.size(); ++j) {
            cPdfPath[i].lines[j].type = pdfPathes[i].lines[j].type;
            cPdfPath[i].lines[j].x0 = pdfPathes[i].lines[j].x0;
            cPdfPath[i].lines[j].y0 = pdfPathes[i].lines[j].y0;
            cPdfPath[i].lines[j].x1 = pdfPathes[i].lines[j].x1;
            cPdfPath[i].lines[j].y1 = pdfPathes[i].lines[j].y1;
//            cPdfPath[i].lines[j].cx = pdfPathes[i].lines[j].cx;
//            cPdfPath[i].lines[j].cy = pdfPathes[i].lines[j].cy;
        }
    }
    return cPdfPath;
}

static void deleteGraphs(CPdfPath *pdfPath, unsigned long size) {

    for (unsigned long i = 0; i < size; ++i) {
        delete [] pdfPath[i].lines;
    }
    delete [] pdfPath;
}

CPageInfos* renderHtml(void *parser, unsigned int pageNum, float scale) {
//    std::cout<<"parser:"<<parser<<std::endl;
//    std::cout<<"pageNum:"<<pageNum<<std::endl;
//    std::cout<<"scale:"<<scale<<std::endl;
    PageInfos pageInfo;
    ((PdfAtomInterface*)parser)->renderHtml(pageNum, pageInfo, scale);
//    std::cout<<"renderHtml success"<<std::endl;

    auto *cPageInfos = new CPageInfos();
    cPageInfos->page_num = pageInfo.m_page_num;
    cPageInfos->width = pageInfo.m_width;
    cPageInfos->height = pageInfo.m_height;

    cPageInfos->font_len = pageInfo.m_fonts.size();
    cPageInfos->fonts = new CPdfFont[pageInfo.m_fonts.size()];
    for (size_t i = 0; i < pageInfo.m_fonts.size(); ++i) {
        cPageInfos->fonts[i].id = pageInfo.m_fonts[i].id;
        cPageInfos->fonts[i].name = string2char(pageInfo.m_fonts[i].name);
        cPageInfos->fonts[i].size = pageInfo.m_fonts[i].size;
        cPageInfos->fonts[i].weight = pageInfo.m_fonts[i].weight;
        cPageInfos->fonts[i].color = string2char(pageInfo.m_fonts[i].color);
        cPageInfos->fonts[i].is_bold = pageInfo.m_fonts[i].is_bold;
        cPageInfos->fonts[i].is_italic = pageInfo.m_fonts[i].is_italic;
        cPageInfos->fonts[i].line_height = pageInfo.m_fonts[i].line_height;
        cPageInfos->fonts[i].type = pageInfo.m_fonts[i].type;
        cPageInfos->fonts[i].render = pageInfo.m_fonts[i].render;
    }

    cPageInfos->image_len = pageInfo.m_images.size();
    cPageInfos->images = new CPdfImage[pageInfo.m_images.size()];
    for (size_t i = 0; i < pageInfo.m_images.size(); ++i) {
        cPageInfos->images[i].left = pageInfo.m_images[i].left;
        cPageInfos->images[i].top = pageInfo.m_images[i].top;
        cPageInfos->images[i].right = pageInfo.m_images[i].right;
        cPageInfos->images[i].bottom = pageInfo.m_images[i].bottom;
    }
    cPageInfos->item_len = pageInfo.m_items.size();
    cPageInfos->items = copyItems(pageInfo.m_items);

    cPageInfos->line_len = pageInfo.m_lines.size();
    cPageInfos->lines = new CPdfLine[pageInfo.m_lines.size()];
    for (size_t i = 0; i < pageInfo.m_lines.size(); ++i) {
        cPageInfos->lines[i].x0 = pageInfo.m_lines[i].x0;
        cPageInfos->lines[i].y0 = pageInfo.m_lines[i].y0;
        cPageInfos->lines[i].x1 = pageInfo.m_lines[i].x1;
        cPageInfos->lines[i].y1 = pageInfo.m_lines[i].y1;
    }

    cPageInfos->graph_len = pageInfo.m_graphs.size();
    cPageInfos->graphs = copyGraphs(pageInfo.m_graphs);

//    delete(&pageInfo);
    return cPageInfos;
}

void deletePageInfos(CPageInfos *cPageInfos) {
    for (size_t i = 0; i < cPageInfos->font_len; ++i) {
        delete [] cPageInfos->fonts[i].name;
        delete [] cPageInfos->fonts[i].color;
    }
    delete [] cPageInfos->fonts;
    delete [] cPageInfos->images;

    delete [] cPageInfos->lines;
    deleteItems(cPageInfos->items, cPageInfos->item_len);
    deleteGraphs(cPageInfos->graphs, cPageInfos->graph_len);

    delete cPageInfos;
}


static CPdfStructInfo * copyStruct(std::vector<PdfStructInfo> structInfo) {
    if (structInfo.empty()) {
        return nullptr;
    }
    auto * cStructInfo = new CPdfStructInfo[structInfo.size()];
    for (size_t i = 0; i < structInfo.size(); ++i) {
        cStructInfo[i].type = string2char(structInfo[i].type);
        cStructInfo[i].mcid = structInfo[i].mcid;
        cStructInfo[i].page = structInfo[i].page;
        cStructInfo[i].children_len = structInfo[i].children.size();
        cStructInfo[i].children = copyStruct(structInfo[i].children);
    }
    return cStructInfo;
}

CPdfStructInfo *getStructure(void *parser, unsigned long * size) {
    std::vector<PdfStructInfo> structInfo = ((PdfAtomInterface*)parser)->getStructure();
    if (structInfo.empty()){
        return nullptr;
    }
    *size = structInfo.size();
    return copyStruct(structInfo);
}

void deleteStructure(CPdfStructInfo *cStructInfo, unsigned long size) {
    for (size_t i = 0; i < size; ++i) {
        delete [] cStructInfo[i].type;
        deleteStructure(cStructInfo[i].children, cStructInfo[i].children_len);
    }
    delete [] cStructInfo;
}



