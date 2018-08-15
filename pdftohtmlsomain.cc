//========================================================================
//
// pdftohtml.cc
//
//
// Copyright 1999-2000 G. Ovtcharov
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2007-2008, 2010, 2012, 2015-2018 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2010 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2010 Mike Slegeir <tehpola@yahoo.com>
// Copyright (C) 2010, 2013 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2010 OSSD CDAC Mumbai by Leena Chourey (leenac@cdacmumbai.in) and Onkar Potdar (onkar@cdacmumbai.in)
// Copyright (C) 2011 Steven Murdoch <Steven.Murdoch@cl.cam.ac.uk>
// Copyright (C) 2012 Igor Slepchin <igor.redhat@gmail.com>
// Copyright (C) 2012 Ihar Filipau <thephilips@gmail.com>
// Copyright (C) 2012 Luis Parravicini <lparravi@gmail.com>
// Copyright (C) 2014 Pino Toscano <pino@kde.org>
// Copyright (C) 2015 William Bader <williambader@hotmail.com>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>. Work sponsored by the LiMux project of the city of Munich
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================
#include <iostream>
#include <vector>
#include "PdfAtomInterface.h"
#include "PdfAtomCApi.h"

using namespace std;

//extern void printItems(vector<PdfItem> pdfItems, unsigned int level);
//extern void printItems(CPdfItem *pdfItem, size_t size, unsigned int level);

static void printItems(vector<PdfItem> pdfItems, unsigned int level = 0) {
    for (auto &pdfItem : pdfItems) {
        for (unsigned int j = 0; j < level; ++j) {
            std::cout << "    ";
        }
        cout << (pdfItem.type == TEXT ? "text: " : "cell: ") << pdfItem.text << endl;
        printItems(pdfItem.children, level + 1);
    }
}


static void printItems(CPdfItem *pdfItem, size_t size, unsigned int level = 0) {
    for (size_t i = 0; i < size; ++i) {

        for (unsigned int j = 0; j < level; ++j) {
            std::cout << "    ";
        }
        cout << (pdfItem[i].type == 0 ? "text: " : "cell: ") << pdfItem[i].text
        <<"("<<pdfItem[i].left<<","<<pdfItem[i].top<<","<<pdfItem[i].right<<","<<pdfItem[i].bottom<<")"<< endl;
        printItems(pdfItem[i].children, pdfItem[i].children_len, level + 1);
    }
}


static void printImages(CPdfImage *pdfImage, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        cout << i << ">> " << pdfImage[i].left << "," << pdfImage[i].top << "," << pdfImage[i].right << ","
             << pdfImage[i].bottom << endl;
    }
}

static void printStructure(CPdfStructInfo *pdfStructInfo, unsigned long size, unsigned int level = 0) {
    for (size_t i = 0; i < size; ++i) {

        for (unsigned int j = 0; j < level; ++j) {
            std::cout << "    ";
        }
        cout << pdfStructInfo[i].mcid << " p" << pdfStructInfo[i].page << " " << pdfStructInfo[i].type << endl;
        printStructure(pdfStructInfo[i].children, pdfStructInfo[i].children_len, level + 1);
    }
}

static void printLines(CPdfPath *pdfPath, unsigned long size) {
    for (size_t i = 0; i < size; ++i) {
        cout << "path:" << i << "*********************************************" << endl;

        CPdfLine *pdfLine = pdfPath[i].lines;
        for (size_t j = 0; j < pdfPath[i].line_len; ++j) {
//            cout << "\tline:" << j << "*********************************************" << endl;
            cout << "\t" << j << " (" << pdfPath[i].type << ")>> " << pdfLine[j].x0
                 << "," << pdfLine[j].y0 << "," << pdfLine[j].x1 << "," << pdfLine[j].y1 << endl;
        }
    }
}

static void printFonts(CPdfFont *pdfFont, unsigned long size) {
    for (size_t i = 0; i < size; ++i) {
        cout << pdfFont[i].id << ">>" << pdfFont[i].name << "," << pdfFont[i].size << "," << pdfFont[i].color << ","
             << pdfFont[i].weight
             << "," << pdfFont[i].is_bold << "," << pdfFont[i].is_italic << "," << pdfFont[i].line_height << ","
             << pdfFont[i].type << "," << pdfFont[i].render << endl;
    }
}


int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "usage: " << argv[0] << " pdf_path " << endl;
        return 1;
    }
    char *pdf_path = argv[1];
    cout << "pdf path is: " << pdf_path << endl;

    initGlobalParams();

//  auto pdf = PdfAtomInterface(pdf_path);
//  std::cout<<"number pages:" <<pdf.getNumPages()<<std::endl;
//  PageInfos pageInfos;
//  pdf.renderHtml(5, pageInfos);
//  printItems(pageInfos.m_items);

    auto parser = createAtomParser(pdf_path);
    cout << "pageNumber:" << getNumPages(parser) << endl;
    for (int i = 0; i < getNumPages(parser); ++i) {
        if (i != 5) {
            continue;
        }
        auto pageInfos = renderHtml(parser, i);
        printItems(pageInfos->items, pageInfos->item_len);
        cout << "images: **************************************************" << endl;
        printImages(pageInfos->images, pageInfos->image_len);
        cout << "vec graphs: **************************************************" << endl;
        printLines(pageInfos->graphs, pageInfos->graph_len);
        cout << "lines: **************************************************" << endl;
        printLines(pageInfos->lines, pageInfos->line_len);
        cout << "fonts: **************************************************" << endl;
        printFonts(pageInfos->fonts, pageInfos->font_len);
        deletePageInfos(pageInfos);
    }

//  cout<<"structure: *****************************************************"<<endl;
//  unsigned long size;
//  auto structInfo = getStructure(parser, &size);
//  printStructure(structInfo, size);

    destroyAtomparser(parser);

    destroyGlobalParams();
    return 0;
}

