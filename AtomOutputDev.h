//
// Created by weditor on 18-7-31.
//

#ifndef POPPLER_ATOMOUTPUTDEV_H
#define POPPLER_ATOMOUTPUTDEV_H


#include <poppler/OutputDev.h>
#include "HtmlOutputDev.h"

class Page;
class HtmlFontAccu;


class AtomString {
public:

    // Constructor.
    AtomString(GfxState *state, double fontSize, HtmlFontAccu* fonts);

    // Destructor.
    ~AtomString();

    AtomString(const AtomString &) = delete;
    AtomString& operator=(const AtomString &) = delete;

    // Add a character to the string.
    void addChar(GfxState *state, double x, double y,
                 double dx, double dy,
                 Unicode u);
    HtmlLink* getLink() { return link; }
    const HtmlFont &getFont() const { return *fonts->Get(fontpos); }
    void endString(); // postprocessing

private:
    // aender die text variable
    HtmlLink *link;
    double xMin, xMax;		// bounding box x coordinates
    double yMin, yMax;		// bounding box y coordinates
    int col;			// starting column
    Unicode *text;		// the text
    double *xRight;		// right-hand x coord of each char
    AtomString *yxNext;		// next string in y-major order
    AtomString *xyNext;		// next string in x-major order
    int fontpos;
    GooString* htext;
    int len;			// length of text and xRight
    int size;			// size of text and xRight arrays
    UnicodeTextDirection dir;	// direction (left to right/right to left)
    HtmlFontAccu *fonts;

    friend class AtomPage;

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
                 Unicode *u, int uLen); //Guchar c);

    void updateFont(GfxState *state);
//
    // End the current string, sorting it into the list of strings.
    void endString();

    // Coalesce strings that look like parts of the same line.
    void coalesce();

    void dump(int pageNum);

    // Clear the page.
    void clear();

    void conv();
private:
    double m_fontSize;		// current font size
    AtomString *m_curStr;		// currently active string

    AtomString *m_yxStrings;	// strings in y-major order
    AtomString *m_yxTail;	// tail cursor for m_yxStrings list

    HtmlFontAccu *m_fonts;
    int m_pageWidth;
    int m_pageHeight;

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
    virtual ~AtomOutputDev();

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

private:
    void drawJpegImage(GfxState *state, Stream *str);
    void drawPngImage(GfxState *state, Stream *str, int width, int height, GfxImageColorMap *colorMap,
            GBool isMask=gFalse);

    // is ok? if AtomOutputDev Construct failed, it's false.
    GBool m_ok;

    // current page number
    int m_pageNum;

    // save pages
    AtomPage *m_pages;
};


#endif //POPPLER_ATOMOUTPUTDEV_H
