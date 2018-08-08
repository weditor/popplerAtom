//
// Created by weditor on 18-7-31.
//

#ifndef POPPLER_ATOMOUTPUTDEV_H
#define POPPLER_ATOMOUTPUTDEV_H

#include <vector>
#include <poppler/OutputDev.h>
#include "poppler_atom_types.h"


class Page;
class HtmlFontAccu;
class GfxPath;
class GooList;
class GooString;


GooString* textFilter(const Unicode* u, int uLen);


struct AtomPoint
{
    AtomPoint(double x, double y):x(x), y(y){}
    double x;
    double y;
};

class AtomLine
{
public:
    AtomLine(int type, AtomPoint p0, AtomPoint p1,  AtomPoint c=AtomPoint(-1, -1))
    :m_type(type), m_p0(p0), m_p1(p1), m_c(c) { }

    int m_type;
    AtomPoint m_p0;
    AtomPoint m_p1;
    AtomPoint m_c;
};

class AtomImage
{
public:
    explicit AtomImage(GfxState *state);
    ~AtomImage() = default;

//    AtomImage(const AtomImage &) = delete;
//    AtomImage& operator=(const AtomImage &) = delete;

    double xMin, xMax;		// image x coordinates
    double yMin, yMax;		// image y coordinates
};


class AtomPage {
public:

    // Constructor.
    AtomPage();

    // Destructor.
    ~AtomPage();

    AtomPage(const AtomPage &) = delete;
    AtomPage& operator=(const AtomPage &) = delete;

    // Begin a new string.
    void beginString(GfxState *state, const GooString *s);

    // Add a character to the current string.
    void addChar(GfxState *state, double x, double y,
                 double dx, double dy,
                 double ox, double oy,
                 Unicode *u, int uLen, int mcid); //Guchar c);

    void updateFont(GfxState *state);
//
    // End the current string, sorting it into the list of strings.
    void endString();

    // Coalesce strings that look like parts of the same line.
    void coalesce();

    void dump(unsigned int pageNum, PageInfos &pageInfos);

    // Clear the page.
    void clear();

    void conv();
    void addImage(AtomImage img);
    void addLine(PdfShape shape);
    void setPageBoarder(double width, double height);
private:
    double m_fontSize;		// current font size

    HtmlFontAccu *m_fonts;
    GooList *m_imgList;
    GooList *m_lineList;

    PageInfos m_pageInfos;
    AtomBox m_lastBox;
    AtomBox m_pageBox;
    int m_lastBoxId;

    friend class AtomOutputDev;
};

class AtomOutputDev: public OutputDev {
public:

    // Open a text output file.  If <fileName> is NULL, no file is written
    // (this is useful, e.g., for searching text).  If <useASCII7> is true,
    // text is converted to 7-bit ASCII; otherwise, text is converted to
    // 8-bit ISO Latin-1.  <useASCII7> should also be set for Japanese
    // (EUC-JP) text.  If <rawOrder> is true, the text is kept in content
    // stream order.
    AtomOutputDev();

    // Destructor.
    ~AtomOutputDev() override;

    // Check if file was successfully created.
    virtual GBool isOk() { return m_ok; }

    //---- get info about output device

    // Does this device use upside-down coordinates?
    // (Upside-down means (0,0) is the top left corner of the page.)
    GBool upsideDown() override { return gTrue; }

    // Does this device use drawChar() or drawString()?
    GBool useDrawChar() override { return gTrue; }

    // Does this device use beginType3Char/endType3Char?  Otherwise,
    // text in Type 3 fonts will be drawn with drawChar/drawString.
    GBool interpretType3Chars() override { return gFalse; }

    // Does this device need non-text content?
    GBool needNonText() override { return gTrue; }

    //----- initialization and control

    GBool checkPageSlice(Page *page, double hDPI, double vDPI,
                         int rotate, GBool useMediaBox, GBool crop,
                         int sliceX, int sliceY, int sliceW, int sliceH,
                         GBool printing,
                         GBool (* abortCheckCbk)(void *data) = NULL,
                         void * abortCheckCbkData = NULL,
                         GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
                         void *annotDisplayDecideCbkData = NULL) override
    {
        return gTrue;
    }


    // Start a page.
    void startPage(int pageNum, GfxState *state, XRef *xref) override;

    // End a page.
    void endPage() override;

    //----- update text state
    void updateFont(GfxState *state) override;

    //----- text drawing
    void beginString(GfxState *state, const GooString *s) override;
    void endString(GfxState *state) override;
    void drawChar(GfxState *state, double x, double y,
                  double dx, double dy,
                  double originX, double originY,
                  CharCode code, int nBytes, Unicode *u, int uLen) override;

    void drawImageMask(GfxState *state, Object *ref,
                       Stream *str,
                       int width, int height, GBool invert,
                       GBool interpolate, GBool inlineImg) override;
    void drawImage(GfxState *state, Object *ref, Stream *str,
                   int width, int height, GfxImageColorMap *colorMap,
                   GBool interpolate, int *maskColors, GBool inlineImg) override;
    void stroke(GfxState *state) override;
    void fill(GfxState *state) override;
    void eoFill(GfxState *state) override;
    void clip(GfxState *state) override;
    void eoClip(GfxState *state) override;

    void beginMarkedContent(const char * name, Dict * properties) override;
    void endMarkedContent(GfxState * state) override;

    void getInfo(unsigned int pageNum, PageInfos &pageInfos);
private:
    int getMcid() {return inMarkedContent()?m_mcidStack[m_mcidStack.size() - 1]:-1; }
    void convertPath(GfxState *state, GfxPath *path, GBool dropEmptySubpaths, int type);
    void drawJpegImage(GfxState *state, Stream *str);
    void drawPngImage(GfxState *state, Stream *str, int width, int height, GfxImageColorMap *colorMap,
            GBool isMask=gFalse);
    bool inMarkedContent() const { return m_mcidStack.size() > 0; }

    // is ok? if AtomOutputDev Construct failed, it's false.
    GBool m_ok;

    // current page number
    int m_pageNum;

    // save pages
    AtomPage *m_pages;
    std::vector<int> m_mcidStack;
};


#endif //POPPLER_ATOMOUTPUTDEV_H
