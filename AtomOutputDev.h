//
// Created by weditor on 18-7-31.
//

#ifndef POPPLER_ATOMOUTPUTDEV_H
#define POPPLER_ATOMOUTPUTDEV_H

#include <vector>
#include <poppler/OutputDev.h>
#include "poppler_atom_types.h"


class Page;
class AtomFontManager;
class GfxPath;
class GooList;
class GooString;
class AtomPath;
class GfxFont;
class AtomXPath;


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

    double xMin, xMax;		// image x coordinates
    double yMin, yMax;		// image y coordinates
};


class AtomFont{
private:
    unsigned int size;
    bool italic;
    bool bold;
    bool rotOrSkewed;
    std::string fontName;
    std::string color;
    short weight;
    unsigned int type;
    unsigned int render;
    double rotSkewMat[4]; // only four values needed for rotation and skew

public:

    AtomFont(GfxFont *font,int _size, std::string color, int render);
    AtomFont(const AtomFont& x);
    AtomFont& operator=(const AtomFont& x);
    const std::string& getColor() const {return color;}
    ~AtomFont();
    GBool isItalic() const {return italic;}
    GBool isBold() const {return bold;}
    GBool isRotOrSkewed() const { return rotOrSkewed; }
    unsigned int getSize() const {return size;}
    unsigned int getType() const {return type;}
    short getWeight() const {return weight;}
    unsigned int getRender() const {return render;}
    void setRotMat(const double * const mat) { rotOrSkewed = gTrue; memcpy(rotSkewMat, mat, sizeof(rotSkewMat)); }
    const double *getRotMat() const { return rotSkewMat; }
    const std::string& getFontName() const;
    GBool isEqual(const AtomFont& x) const;

    static const std::string defaultFont;
};


class AtomFontManager{
private:
    std::vector<AtomFont> m_fonts;

public:
    AtomFontManager();
    ~AtomFontManager();
    AtomFontManager(const AtomFontManager &) = delete;
    AtomFontManager& operator=(const AtomFontManager &) = delete;
    unsigned long AddFont(const AtomFont& font);
    AtomFont *Get(int i){
        return &(m_fonts[i]);
    }
    unsigned long size() const {return m_fonts.size();}

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
    void addLine(PdfPath shape);
    void setPageBoarder(double width, double height);
private:
    double m_fontSize;		// current font size

    AtomFontManager *m_fonts;
    GooList *m_lineList;

    PageInfos m_pageInfos;
    AtomBox m_lastBox;
    AtomBox m_pageBox;
    int m_lastBoxId;

    friend class AtomOutputDev;
};

class AtomOutputDev: public OutputDev {
public:
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

    // Does this device use tilingPatternFill()?  If this returns false,
    // tiling pattern fills will be reduced to a series of other drawing
    // operations.
    virtual GBool useTilingPatternFill() { return gTrue; }
    
    virtual GBool tilingPatternFill(GfxState * /*state*/, Gfx * /*gfx*/, Catalog * /*cat*/, Object * /*str*/,
				  double * /*pmat*/, int /*paintType*/, int /*tilingType*/, Dict * /*resDict*/,
				  double * /*mat*/, double * /*bbox*/,
				  int /*x0*/, int /*y0*/, int /*x1*/, int /*y1*/,
				  double /*xStep*/, double /*yStep*/)
    { return gTrue; }

    // Start a page.
    void startPage(int pageNum, GfxState *state, XRef *xref) override;

    // End a page.
    void endPage() override;

    void updateCTM(GfxState *state, double m11, double m12,
                                  double m21, double m22,
                                  double m31, double m32) override;
    void updateFlatness(GfxState *state) override;

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
    AtomPath *convertPath(GfxState *state, GfxPath *path, GBool dropEmptySubPaths);
    bool inMarkedContent() const { return m_mcidStack.size() > 0; }
    void setFlatness(double flatness) ;
    AtomXPath* getSplashXPath(AtomPath *path);
    // is ok? if AtomOutputDev Construct failed, it's false.
    GBool m_ok;

    // current page number
    int m_pageNum;

    // save pages
    AtomPage *m_pages;
    std::vector<int> m_mcidStack;

    double m_matrix[6];
    double m_flatness;
};



#endif //POPPLER_ATOMOUTPUTDEV_H
