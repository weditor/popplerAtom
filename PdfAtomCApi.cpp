//
// Created by weditor on 18-8-4.
//


#include <cstring>
#include "poppler_atom_types.h"
#include "PdfAtomInterface.h"
#include "PdfAtomCApi.h"

void c_initGlobalParams() {
    initGlobalParams();
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
        delete(pdfItem[i].text);
        deleteItems(pdfItem[i].children, pdfItem[i].children_len);
    }
    delete [] pdfItem;
}

static CPdfLine* copylines(const std::vector<PdfLine> &pdfLines) {
    auto cPdfLine = new CPdfLine[pdfLines.size()];
    for (size_t i = 0; i < pdfLines.size(); ++i) {
        cPdfLine[i].type = pdfLines[i].type;
        cPdfLine[i].x0 = pdfLines[i].x0;
        cPdfLine[i].y0 = pdfLines[i].y0;
        cPdfLine[i].x1 = pdfLines[i].x1;
        cPdfLine[i].y1 = pdfLines[i].y1;
    }
    return cPdfLine;
}

CPageInfos* renderHtml(void *parser, unsigned int pageNum, float scale) {
    PageInfos pageInfo;
    ((PdfAtomInterface*)parser)->renderHtml(pageNum, pageInfo, scale);
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

    return cPageInfos;
}

void deletePageInfos(CPageInfos *cPageInfos) {
    for (size_t i = 0; i < cPageInfos->font_len; ++i) {
        delete(cPageInfos->fonts[i].name);
        delete(cPageInfos->fonts[i].color);
    }
    delete [] cPageInfos->fonts;
    delete [] cPageInfos->images;

    deleteItems(cPageInfos->items, cPageInfos->item_len);

    delete [] cPageInfos->lines;
    delete [] cPageInfos->graphs;

    delete cPageInfos;
}



