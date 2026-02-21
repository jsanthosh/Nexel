#include "NVPath.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace nv {

NVPath::NVPath() = default;

void NVPath::moveTo(float x, float y) {
    PathElement el;
    el.cmd = Command::MoveTo;
    el.points[0] = {x, y};
    m_commands.push_back(el);
}

void NVPath::lineTo(float x, float y) {
    PathElement el;
    el.cmd = Command::LineTo;
    el.points[0] = {x, y};
    m_commands.push_back(el);
}

void NVPath::cubicTo(float cx1, float cy1, float cx2, float cy2, float x, float y) {
    PathElement el;
    el.cmd = Command::CubicTo;
    el.points[0] = {cx1, cy1};
    el.points[1] = {cx2, cy2};
    el.points[2] = {x, y};
    m_commands.push_back(el);
}

void NVPath::quadTo(float cx, float cy, float x, float y) {
    PathElement el;
    el.cmd = Command::QuadTo;
    el.points[0] = {cx, cy};
    el.points[1] = {x, y};
    m_commands.push_back(el);
}

void NVPath::arcTo(float cx, float cy, float radius, float startAngle, float endAngle) {
    // Approximate arc with cubic Bezier segments (90 degree max per segment)
    float sweep = endAngle - startAngle;
    int segments = std::max(1, static_cast<int>(std::ceil(std::abs(sweep) / (M_PI * 0.5f))));
    float segAngle = sweep / segments;

    for (int i = 0; i < segments; i++) {
        float a1 = startAngle + segAngle * i;
        float a2 = a1 + segAngle;
        float alpha = 4.0f * std::tan(segAngle * 0.25f) / 3.0f;

        float cos1 = std::cos(a1), sin1 = std::sin(a1);
        float cos2 = std::cos(a2), sin2 = std::sin(a2);

        float x1 = cx + radius * cos1;
        float y1 = cy + radius * sin1;
        float x2 = cx + radius * cos2;
        float y2 = cy + radius * sin2;

        float cx1 = x1 - alpha * radius * sin1;
        float cy1 = y1 + alpha * radius * cos1;
        float cx2 = x2 + alpha * radius * sin2;
        float cy2 = y2 - alpha * radius * cos2;

        if (i == 0 && m_commands.empty()) {
            moveTo(x1, y1);
        }
        cubicTo(cx1, cy1, cx2, cy2, x2, y2);
    }
}

void NVPath::close() {
    PathElement el;
    el.cmd = Command::Close;
    m_commands.push_back(el);
}

void NVPath::clear() {
    m_commands.clear();
}

std::vector<NVPoint> NVPath::flatten(float tolerance) const {
    std::vector<NVPoint> result;
    NVPoint current = {0, 0};

    for (const auto& el : m_commands) {
        switch (el.cmd) {
            case Command::MoveTo:
                result.push_back(el.points[0]);
                current = el.points[0];
                break;
            case Command::LineTo:
                result.push_back(el.points[0]);
                current = el.points[0];
                break;
            case Command::CubicTo:
                flattenCubic(current, el.points[0], el.points[1], el.points[2], tolerance, result);
                current = el.points[2];
                break;
            case Command::QuadTo:
                flattenQuad(current, el.points[0], el.points[1], tolerance, result);
                current = el.points[1];
                break;
            case Command::Close:
                if (!result.empty()) {
                    result.push_back(result.front()); // close the loop
                }
                break;
        }
    }
    return result;
}

void NVPath::flattenCubic(NVPoint p0, NVPoint p1, NVPoint p2, NVPoint p3,
                           float tolerance, std::vector<NVPoint>& out) const {
    // De Casteljau subdivision — check flatness
    float dx = p3.x - p0.x;
    float dy = p3.y - p0.y;
    float d2 = std::abs((p1.x - p3.x) * dy - (p1.y - p3.y) * dx);
    float d3 = std::abs((p2.x - p3.x) * dy - (p2.y - p3.y) * dx);

    float flatness = (d2 + d3) * (d2 + d3);
    float tolSq = tolerance * tolerance * (dx * dx + dy * dy);

    if (flatness <= tolSq) {
        out.push_back(p3);
        return;
    }

    // Subdivide at t=0.5
    NVPoint q0 = {(p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f};
    NVPoint q1 = {(p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f};
    NVPoint q2 = {(p2.x + p3.x) * 0.5f, (p2.y + p3.y) * 0.5f};
    NVPoint r0 = {(q0.x + q1.x) * 0.5f, (q0.y + q1.y) * 0.5f};
    NVPoint r1 = {(q1.x + q2.x) * 0.5f, (q1.y + q2.y) * 0.5f};
    NVPoint s  = {(r0.x + r1.x) * 0.5f, (r0.y + r1.y) * 0.5f};

    flattenCubic(p0, q0, r0, s, tolerance, out);
    flattenCubic(s, r1, q2, p3, tolerance, out);
}

void NVPath::flattenQuad(NVPoint p0, NVPoint p1, NVPoint p2,
                          float tolerance, std::vector<NVPoint>& out) const {
    // Elevate quadratic to cubic
    NVPoint c1 = {p0.x + 2.0f / 3.0f * (p1.x - p0.x), p0.y + 2.0f / 3.0f * (p1.y - p0.y)};
    NVPoint c2 = {p2.x + 2.0f / 3.0f * (p1.x - p2.x), p2.y + 2.0f / 3.0f * (p1.y - p2.y)};
    flattenCubic(p0, c1, c2, p2, tolerance, out);
}

std::vector<NVPath::StrokeVertex> NVPath::strokeGeometry(float width, NVLineJoin join, NVLineCap cap) const {
    auto points = flatten();
    if (points.size() < 2) return {};

    std::vector<StrokeVertex> vertices;
    vertices.reserve(points.size() * 2);

    float halfWidth = width * 0.5f;
    float cumDist = 0;

    for (size_t i = 0; i < points.size() - 1; i++) {
        NVPoint a = points[i];
        NVPoint b = points[i + 1];

        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) continue;

        float nx = -dy / len;
        float ny = dx / len;

        // Cap at start
        if (i == 0 && cap == NV_LINE_CAP_SQUARE) {
            a.x -= dx / len * halfWidth;
            a.y -= dy / len * halfWidth;
        }

        // Cap at end
        NVPoint bAdj = b;
        if (i == points.size() - 2 && cap == NV_LINE_CAP_SQUARE) {
            bAdj.x += dx / len * halfWidth;
            bAdj.y += dy / len * halfWidth;
        }

        StrokeVertex v0, v1, v2, v3;

        v0.position = a;
        v0.normal = {nx, ny};
        v0.side = -1.0f;
        v0.distance = cumDist;

        v1.position = a;
        v1.normal = {nx, ny};
        v1.side = 1.0f;
        v1.distance = cumDist;

        float segDist = cumDist + len;

        v2.position = bAdj;
        v2.normal = {nx, ny};
        v2.side = -1.0f;
        v2.distance = segDist;

        v3.position = bAdj;
        v3.normal = {nx, ny};
        v3.side = 1.0f;
        v3.distance = segDist;

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);

        cumDist = segDist;

        // Join to next segment (degenerate strip connection)
        if (i < points.size() - 2 && join != NV_LINE_JOIN_MITER) {
            vertices.push_back(v3); // repeat last vertex
            // Next segment's first vertex will be pushed in next iteration
        }
    }

    return vertices;
}

std::vector<NVPoint> NVPath::fillTriangles() const {
    // Simple ear-clipping triangulation for convex/simple polygons
    auto points = flatten();
    std::vector<NVPoint> triangles;
    if (points.size() < 3) return triangles;

    // Remove duplicate close point
    if (points.size() > 1 &&
        std::abs(points.front().x - points.back().x) < 0.01f &&
        std::abs(points.front().y - points.back().y) < 0.01f) {
        points.pop_back();
    }

    // Simple fan triangulation (works for convex polygons)
    for (size_t i = 1; i < points.size() - 1; i++) {
        triangles.push_back(points[0]);
        triangles.push_back(points[i]);
        triangles.push_back(points[i + 1]);
    }

    return triangles;
}

NVRect NVPath::bounds() const {
    auto points = flatten();
    if (points.empty()) return {0, 0, 0, 0};

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }

    return {minX, minY, maxX - minX, maxY - minY};
}

} // namespace nv
