//
// Created by weditor on 18-8-26.
// these code are copied from class SplashPath and SplashXPath.
// because some of members are not public.
//

#ifndef POPPLER_ATOMPATH_H
#define POPPLER_ATOMPATH_H

#include <poppler/OutputDev.h>
#include "poppler_atom_types.h"


struct AtomXPathPoint {
    double x, y;
};

struct AtomXPathAdjust {
    int firstPt, lastPt;		// range of points
    bool  vert;			// vertical or horizontal hint
    // hint boundaries
    double x0a, x0b,		
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
    unsigned int flags;
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
    int close(bool  force = false);

    // Add (<dx>, <dy>) to every point on this path.
    void offset(double dx, double dy);

    // Get the points on the path.
    int getLength() { return length; }
    void getPoint(int i, double *x, double *y, unsigned char *f)
    { *x = pts[i].x; *y = pts[i].y; *f = flags[i]; }

    // Reserve space for at least n points
    void reserve(int n);

//protected:

    AtomPath(AtomPath *path);
    void grow(int nPts);
    bool  noCurrentPoint() { return curSubpath == length; }
    bool  onePointSubpath() { return curSubpath == length - 1; }
    bool  openSubpath() { return curSubpath < length - 1; }

    AtomPathPoint *pts;		// array of points
    unsigned char *flags;		// array of flags
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
              double flatness, bool  closeSubpaths,
              bool  adjustLines = false, int linePosI = 0);

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
                  double flatness);
    void addSegment(double x0, double y0,
                    double x1, double y1);

    SplashXPathSeg *segs;
    int length, size;		// length and size of segs array
};

#endif //POPPLER_ATOMPATH_H
