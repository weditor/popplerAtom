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

static CPdfShape* copylines(const std::vector<PdfShape> &pdfShapes) {
    if (pdfShapes.empty()) {
        return nullptr;
    }

    auto *cPdfShape = new CPdfShape[pdfShapes.size()];
    for (unsigned long i = 0; i < pdfShapes.size(); ++i) {
        cPdfShape[i].type = pdfShapes[i].type;
        cPdfShape[i].path_len = pdfShapes[i].pathes.size();
        cPdfShape[i].pathes = new CPdfPath[pdfShapes[i].pathes.size()];

        for (unsigned long j = 0; j < pdfShapes[i].pathes.size(); ++j) {

            CPdfPath &cPdfPath = cPdfShape[i].pathes[j];
            const PdfPath &pdfPath = pdfShapes[i].pathes[j];

            cPdfPath.line_len = pdfShapes[i].pathes[j].lines.size();
            cPdfPath.lines = new CPdfLine[pdfPath.lines.size()];
            for (unsigned long k = 0; k < pdfPath.lines.size(); ++k) {
                cPdfPath.lines[k].type = pdfPath.lines[k].type;
                cPdfPath.lines[k].x0 = pdfPath.lines[k].x0;
                cPdfPath.lines[k].y0 = pdfPath.lines[k].y0;
                cPdfPath.lines[k].x1 = pdfPath.lines[k].x1;
                cPdfPath.lines[k].y1 = pdfPath.lines[k].y1;
                cPdfPath.lines[k].cx = pdfPath.lines[k].cx;
                cPdfPath.lines[k].cy = pdfPath.lines[k].cy;
            }
        }
    }
    return cPdfShape;
}

static void deleteLines(CPdfShape* pdfShape, unsigned long size) {

    for (unsigned long i = 0; i < size; ++i) {
        for (unsigned long j = 0; j < pdfShape[i].path_len; ++j) {
            delete [] pdfShape[i].pathes[j].lines;
        }
        delete [] pdfShape[i].pathes;
    }
    delete [] pdfShape;
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
    cPageInfos->lines = copylines(pageInfo.m_lines);

    cPageInfos->graph_len = pageInfo.m_graphs.size();
    cPageInfos->graphs = copylines(pageInfo.m_graphs);

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

    deleteItems(cPageInfos->items, cPageInfos->item_len);
    deleteLines(cPageInfos->lines, cPageInfos->line_len);
    deleteLines(cPageInfos->graphs, cPageInfos->graph_len);

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



