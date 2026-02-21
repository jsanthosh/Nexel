#ifndef NV_PATH_H
#define NV_PATH_H

#include "nativeviz/nv_types.h"
#include <vector>

namespace nv {

// Bezier path: sequence of commands (moveTo, lineTo, cubicTo, close)
class NVPath {
public:
    NVPath();

    // Path construction
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void cubicTo(float cx1, float cy1, float cx2, float cy2, float x, float y);
    void quadTo(float cx, float cy, float x, float y);
    void arcTo(float cx, float cy, float radius, float startAngle, float endAngle);
    void close();
    void clear();

    // Flatten Bezier curves into polyline (adaptive subdivision)
    std::vector<NVPoint> flatten(float tolerance = 0.5f) const;

    // Generate triangle strip for stroking with given width
    struct StrokeVertex {
        NVPoint position;
        NVPoint normal;
        float side;        // -1 or +1
        float distance;   // cumulative distance along path
    };

    std::vector<StrokeVertex> strokeGeometry(float width, NVLineJoin join, NVLineCap cap) const;

    // Triangulate path fill (ear clipping for simple polygons)
    std::vector<NVPoint> fillTriangles() const;

    bool isEmpty() const { return m_commands.empty(); }
    NVRect bounds() const;

private:
    enum class Command { MoveTo, LineTo, CubicTo, QuadTo, Close };

    struct PathElement {
        Command cmd;
        NVPoint points[3]; // up to 3 control points
    };

    std::vector<PathElement> m_commands;

    // Adaptive Bezier subdivision
    void flattenCubic(NVPoint p0, NVPoint p1, NVPoint p2, NVPoint p3,
                      float tolerance, std::vector<NVPoint>& out) const;
    void flattenQuad(NVPoint p0, NVPoint p1, NVPoint p2,
                     float tolerance, std::vector<NVPoint>& out) const;
};

} // namespace nv

#endif // NV_PATH_H
