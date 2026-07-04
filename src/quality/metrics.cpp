// CyberMeshGenerator — mesh quality metrics and reporting.
#include "cmg/quality/metrics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "cmg/core/circumcenter.hpp"
#include "cmg/predicates/robust.hpp"

namespace cmg::quality {
namespace {

constexpr double kPi = 3.14159265358979323846;

struct Vec3 {
    double x = 0, y = 0, z = 0;
};

inline Vec3 sub(const Point3& a, const Point3& b) {
    return {double(a.x) - b.x, double(a.y) - b.y, double(a.z) - b.z};
}
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline double norm(const Vec3& a) { return std::sqrt(dot(a, a)); }

/// Signed-volume magnitude of tet a,b,c,d: |det[b-a,c-a,d-a]| / 6.
double tet_volume(const Point3& a, const Point3& b, const Point3& c,
                  const Point3& d) {
    const Vec3 ab = sub(b, a), ac = sub(c, a), ad = sub(d, a);
    return std::fabs(dot(ab, cross(ac, ad))) / 6.0;
}

/// Shortest of the 6 edge lengths of tet P[0..3].
double shortest_edge(const std::array<Point3, 4>& P) {
    double m = std::numeric_limits<double>::infinity();
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
            m = std::min(m, norm(sub(P[j], P[i])));
    return m;
}

/// The 6 interior dihedral angles (degrees) of tet P, matching the convention in
/// src/optimize/smooth.cpp (~70.53° for a regular tet). Appends to `out`.
void tet_dihedrals(const std::array<Point3, 4>& P, std::array<double, 6>& out) {
    std::array<Vec3, 4> nrm;
    for (int e = 0; e < 4; ++e) {
        const int a = (e + 1) % 4, b = (e + 2) % 4, c = (e + 3) % 4;
        Vec3 nv = cross(sub(P[b], P[a]), sub(P[c], P[a]));
        if (dot(nv, sub(P[e], P[a])) > 0.0) { nv.x = -nv.x; nv.y = -nv.y; nv.z = -nv.z; }
        nrm[e] = nv;
    }
    int idx = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
            int k = -1, l = -1;
            for (int mm = 0; mm < 4; ++mm)
                if (mm != i && mm != j) (k < 0 ? k : l) = mm;
            const double na = norm(nrm[k]), nb = norm(nrm[l]);
            double ang;
            if (na == 0.0 || nb == 0.0) {
                ang = 0.0; // degenerate face; keep finite
            } else {
                double cd = dot(nrm[k], nrm[l]) / (na * nb);
                cd = std::clamp(cd, -1.0, 1.0);
                ang = (kPi - std::acos(cd)) * 180.0 / kPi;
            }
            out[idx++] = ang;
        }
}

} // namespace

QualityReport report(const Mesh& mesh, int worst_count) {
    QualityReport r;
    r.num_tets = mesh.tetrahedra.size();

    const std::size_t n = mesh.tetrahedra.size();

    // Per-tet radius-edge ratios (for worst_tets ordering); infinite/degenerate
    // tets get a large finite sentinel so they can still rank as "worst".
    std::vector<double> ratio(n, 0.0);
    std::vector<char> ratio_valid(n, 0);

    double min_re = 0, max_re = 0, sum_re = 0;
    std::size_t re_count = 0;

    double min_dih = 0, max_dih = 0, sum_dih = 0;
    std::size_t dih_count = 0;

    double min_vol = 0, max_vol = 0, total_vol = 0;

    for (std::size_t t = 0; t < n; ++t) {
        const auto& tet = mesh.tetrahedra[t];
        const std::array<Point3, 4> P = {mesh.points[tet[0]], mesh.points[tet[1]],
                                         mesh.points[tet[2]], mesh.points[tet[3]]};

        // Volume.
        const double vol = tet_volume(P[0], P[1], P[2], P[3]);
        total_vol += vol;
        if (t == 0) {
            min_vol = max_vol = vol;
        } else {
            min_vol = std::min(min_vol, vol);
            max_vol = std::max(max_vol, vol);
        }

        // Radius-edge ratio.
        Point3 cc;
        double radius = 0;
        const double se = shortest_edge(P);
        bool ok = geom::circumcenter(P[0], P[1], P[2], P[3], cc, radius);
        if (ok && se > 0.0 && std::isfinite(radius)) {
            const double re = radius / se;
            ratio[t] = re;
            ratio_valid[t] = 1;
            if (re_count == 0) {
                min_re = max_re = re;
            } else {
                min_re = std::min(min_re, re);
                max_re = std::max(max_re, re);
            }
            sum_re += re;
            ++re_count;
        } else {
            // Degenerate: excluded from min/mean stats but flagged so it can still
            // be listed among the worst tets with a large finite ratio.
            ratio[t] = std::numeric_limits<double>::max();
            ratio_valid[t] = 0;
        }

        // Dihedral angles (all 6 counted, histogram + stats).
        std::array<double, 6> ang;
        tet_dihedrals(P, ang);
        for (double a : ang) {
            if (dih_count == 0) {
                min_dih = max_dih = a;
            } else {
                min_dih = std::min(min_dih, a);
                max_dih = std::max(max_dih, a);
            }
            sum_dih += a;
            ++dih_count;
            int b = static_cast<int>(std::floor(a / 10.0));
            b = std::clamp(b, 0, kHistogramBuckets - 1);
            ++r.dihedral_histogram[static_cast<std::size_t>(b)];
        }
    }

    r.min_radius_edge = min_re;
    r.max_radius_edge = max_re;
    r.mean_radius_edge = re_count ? sum_re / static_cast<double>(re_count) : 0.0;

    r.min_dihedral = min_dih;
    r.max_dihedral = max_dih;
    r.mean_dihedral = dih_count ? sum_dih / static_cast<double>(dih_count) : 0.0;

    r.min_volume = min_vol;
    r.max_volume = max_vol;
    r.total_volume = total_vol;

    // Worst tets: descending radius-edge ratio. Include degenerate tets only when
    // their sentinel ratio is finite (it is: numeric_limits::max()).
    std::vector<int> order;
    order.reserve(n);
    for (std::size_t t = 0; t < n; ++t)
        if (ratio_valid[t] || std::isfinite(ratio[t])) order.push_back(static_cast<int>(t));
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return ratio[a] > ratio[b]; });
    if (worst_count >= 0 && order.size() > static_cast<std::size_t>(worst_count))
        order.resize(static_cast<std::size_t>(worst_count));
    r.worst_tets = std::move(order);

    return r;
}

} // namespace cmg::quality
