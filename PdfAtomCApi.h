//
// Created by weditor on 18-8-4.
//

#ifndef POPPLER_PDFATOMCAPI_H
#define POPPLER_PDFATOMCAPI_H

#ifdef __cplusplus
extern "C" {
#endif

struct CAtomBox {
    double x1;
    double y1;
    double x2;
    double y2;
};

struct CPdfFont {
    int id;
    char * name;
    float size;
    short weight;
    char * color;
    bool is_bold;
    bool is_italic;
    int line_height;
    int type;
    int render;
};

struct CPdfImage {
    int left;
    int top;
    int right;
    int bottom;
};

struct CPdfItem {
    unsigned int id;
    int mcid;
    int type;
    char* text;

    int left;
    int top;
    int right;
    int bottom;

    int font;
    int style;
    CPdfItem* children;
    unsigned long children_len;
};

struct CPdfLine {
    int x0;
    int y0;
    int x1;
    int y1;
    int cx;
    int cy;
    int type;
};

struct CPdfPath {
    CPdfLine* lines;
    unsigned long line_len;
};

struct CPdfShape {
    int type;
    CPdfPath* pathes;
    unsigned long path_len;
};

struct CPageInfos {
    int page_num;
    int width;
    int height;
    CPdfFont* fonts;
    unsigned long font_len;
    CPdfImage* images;
    unsigned long image_len;
    CPdfItem* items;
    unsigned long item_len;
    CPdfShape* lines;
    unsigned long line_len;
    CPdfShape* graphs;
    unsigned long graph_len;
};


struct CStructAttr {
    char *key;
    char *value;
};

struct CPdfStructInfo {
    char* type;
    CStructAttr* attribute;
    unsigned long attribute_len;
    int mcid;
    int page;
    CPdfStructInfo* children;
    unsigned long children_len;
};

extern void c_initGlobalParams(const char* popplerData = nullptr);
extern void c_destroyGlobalParams();

void * createAtomParser(const char *fileName, const char * ownerPW=nullptr, const char * userPW=nullptr);
void destroyAtomparser(void *parser);

bool isParserOk(void *parser);
int getNumPages(void *parser);

CPageInfos* renderHtml(void *parser, unsigned int pageNum, float scale=1.0);
void deletePageInfos(CPageInfos *cPageInfos);

CPdfStructInfo* getStructure(void *parser, unsigned long *size);
void deleteStructure(CPdfStructInfo *cStructInfo, unsigned long size);

#ifdef __cplusplus
}
#endif

#endif //POPPLER_PDFATOMCAPI_H
