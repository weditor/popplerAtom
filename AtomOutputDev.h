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


GooString* textFilter(const Unicode* u, int uLen);


struct AtomXPathPoint {
    double x, y;
};

struct AtomXPathAdjust {
    int firstPt, lastPt;		// range of points
    GBool vert;			// vertical or horizontal hint
    double x0a, x0b,		// hint boundaries
            xma, xmb,
            x1a, x1b;
    double x0, x1, xm;	// adjusted coordinates
};

#define splashMaxCurveSplits (1 << 10)

struct SplashXPathSeg {
    double x0, y0;		// first endpoint
    double x1, y1;		// second endpoint
    double dxdy;		// slope: delta-x / delta-y
    double dydx;		// slope: delta-y / delta-x
    Guint flags;
};
#define splashXPathHoriz   0x01 // segment is vertical (y0 == y1)
//   (dxdy is undef)
#define splashXPathVert    0x02 // segment is horizontal (x0 == x1)
//   (dydx is undef)
#define splashXPathFlip	   0x04	// y0 > y1



//------------------------------------------------------------------------
// SplashPathPoint
//------------------------------------------------------------------------

struct AtomPathPoint {
    double x, y;
};

//------------------------------------------------------------------------
// AtomPath.flags
//------------------------------------------------------------------------

// first point on each subpath sets this flag
#define splashPathFirst         0x01

// last point on each subpath sets this flag
#define splashPathLast          0x02

// if the subpath is closed, its first and last points must be
// identical, and must set this flag
#define splashPathClosed        0x04

// curve control points set this flag
#define splashPathCurve         0x08


//------------------------------------------------------------------------

#define splashOk                 0	// no error

#define splashErrNoCurPt         1	// no current point

#define splashErrEmptyPath       2	// zero points in path

#define splashErrBogusPath       3	// only one point in subpath

#define splashErrNoSave	         4	// state stack is empty

#define splashErrOpenFile        5	// couldn't open file

#define splashErrNoGlyph         6	// couldn't get the requested glyph

#define splashErrModeMismatch    7	// invalid combination of color modes

#define splashErrSingularMatrix  8	// matrix is singular

#define splashErrBadArg          9      // bad argument

#define splashErrZeroImage     254      // image of 0x0

#define splashErrGeneric       255
//------------------------------------------------------------------------
// SplashPathHint
//------------------------------------------------------------------------

struct AtomPathHint {
    int ctrl0, ctrl1;
    int firstPt, lastPt;
};

class AtomPath {
public:

    // Create an empty path.
    AtomPath();

    // Copy a path.
    AtomPath *copy() { return new AtomPath(this); }

    ~AtomPath();

    AtomPath(const AtomPath&) = delete;
    AtomPath& operator=(const AtomPath&) = delete;

    // Append <path> to <this>.
    void append(AtomPath *path);

    // Start a new subpath.
    int moveTo(double x, double y);

    // Add a line segment to the last subpath.
    int lineTo(double x, double y);

    // Add a third-order (cubic) Bezier curve segment to the last
    // subpath.
    int curveTo(double x1, double y1,
                        double x2, double y2,
                        double x3, double y3);

    // Close the last subpath, adding a line segment if necessary.  If
    // <force> is true, this adds a line segment even if the current
    // point is equal to the first point in the subpath.
    int close(GBool force = gFalse);

    // Add a stroke adjustment hint.  The controlling segments are
    // <ctrl0> and <ctrl1> (where segments are identified by their first
    // point), and the points to be adjusted are <firstPt> .. <lastPt>.
    void addStrokeAdjustHint(int ctrl0, int ctrl1, int firstPt, int lastPt);

    // Add (<dx>, <dy>) to every point on this path.
    void offset(double dx, double dy);

    // Get the points on the path.
    int getLength() { return length; }
    void getPoint(int i, double *x, double *y, Guchar *f)
    { *x = pts[i].x; *y = pts[i].y; *f = flags[i]; }

    // Get the current point.
    GBool getCurPt(double *x, double *y);

    // Reserve space for at least n points
    void reserve(int n);

//protected:

    AtomPath(AtomPath *path);
    void grow(int nPts);
    GBool noCurrentPoint() { return curSubpath == length; }
    GBool onePointSubpath() { return curSubpath == length - 1; }
    GBool openSubpath() { return curSubpath < length - 1; }

    AtomPathPoint *pts;		// array of points
    Guchar *flags;		// array of flags
    int length, size;		// length/size of the pts and flags arrays
    int curSubpath;		// index of first point in last subpath

    AtomPathHint *hints;	// list of hints
    int hintsLength, hintsSize;
};


class AtomXPath {
public:

    // Expands (converts to segments) and flattens (converts curves to
    // lines) <path>.  Transforms all points from user space to device
    // space, via <matrix>.  If <closeSubpaths> is true, closes all open
    // subpaths.
    AtomXPath(AtomPath *path, double *matrix,
                double flatness, GBool closeSubpaths,
                GBool adjustLines = gFalse, int linePosI = 0);

    // Copy an expanded path.
    AtomXPath *copy() { return new AtomXPath(this); }

    ~AtomXPath();

    AtomXPath(const AtomXPath&) = delete;
    AtomXPath& operator=(const AtomXPath&) = delete;

    // Sort by upper coordinate (lower y), in y-major order.
    void sort();
    AtomXPath(AtomXPath *xPath);

    int getSegLength() {return length;}
    SplashXPathSeg* getSeg(int i) {return &segs[i];}

private:

    void transform(double *matrix, double xi, double yi,
                   double *xo, double *yo);
    void strokeAdjust(AtomXPathAdjust *adjust,
                      double *xp, double *yp);
    void grow(int nSegs);
    void addCurve(double x0, double y0,
                  double x1, double y1,
                  double x2, double y2,
                  double x3, double y3,
                  double flatness,
                  GBool first, GBool last, GBool end0, GBool end1);
    void addSegment(double x0, double y0,
                    double x1, double y1);

    SplashXPathSeg *segs;
    int length, size;		// length and size of segs array
};


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
    int size() const {return m_fonts.size();}

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
    void drawJpegImage(GfxState *state, Stream *str);
    void drawPngImage(GfxState *state, Stream *str, int width, int height, GfxImageColorMap *colorMap,
            GBool isMask=gFalse);
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
