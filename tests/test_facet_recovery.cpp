// CyberMeshGenerator — facet recovery (conforming Delaunay, increment 1) tests.
//
// Covers the four objective scenarios of the add-facet-recovery change:
//   (a) a facet that is not a Delaunay face is recovered as mesh faces, volume
//       conserved through the `preserve_facets` pipeline;
//   (b) an internal facet separates a domain into two distinctly-marked regions,
//       with no tetrahedron straddling it;
//   (c) a convex PLC whose facets are already Delaunay faces adds zero Steiner
//       points and yields the same mesh as without recovery;
//   (d) an exhausted Steiner budget reports incompleteness without looping.
#include "cmg/cmg.hpp"
#include "cmg/recover/facets.hpp"
#include "cmg/recover/segments.hpp"
#include "harness.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <vector>

using namespace cmg;

namespace {

std::array<int, 3> sorted3(int a, int b, int c) {
    std::array<int, 3> k{a, b, c};
    std::sort(k.begin(), k.end());
    return k;
}

std::set<std::array<int, 3>> mesh_faces(const Mesh& m) {
    std::set<std::array<int, 3>> f;
    for (const auto& t : m.tetrahedra) {
        f.insert(sorted3(t[1], t[2], t[3]));
        f.insert(sorted3(t[0], t[2], t[3]));
        f.insert(sorted3(t[0], t[1], t[3]));
        f.insert(sorted3(t[0], t[1], t[2]));
    }
    return f;
}

double tet_volume(const Point3& a, const Point3& b, const Point3& c,
                  const Point3& d) {
    double m[3][3] = {{b.x - a.x, c.x - a.x, d.x - a.x},
                      {b.y - a.y, c.y - a.y, d.y - a.y},
                      {b.z - a.z, c.z - a.z, d.z - a.z}};
    double det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                 m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                 m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    return std::fabs(det) / 6.0;
}

double mesh_volume(const Mesh& m) {
    double v = 0;
    for (const auto& t : m.tetrahedra)
        v += tet_volume(m.points[t[0]], m.points[t[1]], m.points[t[2]],
                        m.points[t[3]]);
    return v;
}

// Design §5(a): thin prism over the (non-rectangular) convex quad A,B,C,D at
// z in {0,1}. The quad's fan diagonal disagrees with the Delaunay diagonal, so
// the cap subfaces are NOT faces of the plain Delaunay tetrahedralization and
// must be recovered by Steiner insertion.
//   bottom A0=0 B0=1 C0=2 D0=3   top A1=4 B1=5 C1=6 D1=7
PLC prism_plc() {
    PLC p;
    p.points = {{0, 0, 0}, {6, 0, 0}, {5, 4, 0}, {1.2f, 4, 0},
                {0, 0, 1}, {6, 0, 1}, {5, 4, 1}, {1.2f, 4, 1}};
    auto quad = [&](int a, int b, int c, int d) {
        Facet f;
        f.polygons.push_back({{a, b, c, d}});
        p.facets.push_back(f);
    };
    quad(0, 1, 2, 3); // bottom
    quad(4, 5, 6, 7); // top
    quad(0, 1, 5, 4); // sides
    quad(1, 2, 6, 5);
    quad(2, 3, 7, 6);
    quad(3, 0, 4, 7);
    return p;
}

// A triangular bipyramid split by the internal triangular facet {0,1,2}: two
// tetrahedra glued on that shared face, apex 3 above and apex 4 below. Every
// facet is a triangle in general position (no cospherical degeneracy), so the
// shared facet is already a Delaunay face and recovery adds no Steiner points —
// the facet's role here is to SEPARATE the two region cells.
PLC bipyramid_plc() {
    PLC p;
    p.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0},
                {0.3f, 0.3f, 1}, {0.3f, 0.3f, -1}};
    auto tri = [&](int a, int b, int c) {
        Facet f;
        f.polygons.push_back({{a, b, c}});
        p.facets.push_back(f);
    };
    tri(0, 1, 3); tri(1, 2, 3); tri(2, 0, 3); // upper cell
    tri(0, 1, 4); tri(1, 2, 4); tri(2, 0, 4); // lower cell
    tri(0, 1, 2);                             // internal separating facet
    return p;
}

// Design §5(A): the PREVIOUSLY-FAILING case. A bipyramid over the prism's
// non-rectangular convex quad wall {0,1,2,3} at z=0, apex 4 above and apex 5
// below. The quad's fan diagonal disagrees with the Delaunay diagonal, so the
// wall subfaces {0,1,2},{0,2,3} are NOT plain Delaunay faces — recovering the
// internal wall needs a Steiner point. Under the old ray-cast carve every ray had
// positive z, so from a lower centroid the wall + upper boundary made an even
// crossing count and the whole lower half was dropped (~55% volume lost, two
// regions collapsing to one). The seed/flood carve keeps both cells.
PLC square_bipyramid_plc() {
    PLC p;
    p.points = {{0, 0, 0}, {6, 0, 0}, {5, 4, 0}, {1.2f, 4, 0}, // wall quad, z=0
                {3, 2, 3},                                     // upper apex
                {3, 2, -3}};                                   // lower apex
    auto tri = [&](int a, int b, int c) {
        Facet f;
        f.polygons.push_back({{a, b, c}});
        p.facets.push_back(f);
    };
    Facet wall;
    wall.polygons.push_back({{0, 1, 2, 3}}); // internal separating wall
    p.facets.push_back(wall);
    tri(0, 1, 4); tri(1, 2, 4); tri(2, 3, 4); tri(3, 0, 4); // upper cell
    tri(0, 1, 5); tri(1, 2, 5); tri(2, 3, 5); tri(3, 0, 5); // lower cell
    return p;
}

// A genuinely NON-CONVEX domain: an L-shaped cross-section (a 2x2 square minus a
// 1x1 corner, area 3) extruded to z in [0, 4] (volume 12). Its facets are not all
// Delaunay faces of the vertices, so `preserve_facets` must recover them, and the
// carve must still return the exact non-convex volume (not a convex-hull over-fill).
PLC l_prism_plc() {
    PLC p;
    const Real H = 4;
    const double xy[6][2] = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}};
    for (int z = 0; z < 2; ++z)
        for (const auto& c : xy)
            p.points.push_back({static_cast<Real>(c[0]), static_cast<Real>(c[1]),
                                static_cast<Real>(z) * H});
    Facet bot; bot.polygons.push_back({{0, 1, 2, 3, 4, 5}}); p.facets.push_back(bot);
    Facet top; top.polygons.push_back({{6, 7, 8, 9, 10, 11}}); p.facets.push_back(top);
    for (int i = 0; i < 6; ++i) {          // six side quads
        int j = (i + 1) % 6;
        Facet f; f.polygons.push_back({{i, j, j + 6, i + 6}}); p.facets.push_back(f);
    }
    return p;
}

// Single reference tetrahedron as a 4-facet PLC (convex, already Delaunay).
PLC tet_plc() {
    PLC p;
    p.points = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const int tri[4][3] = {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}};
    for (auto& t : tri) {
        Facet f;
        f.polygons.push_back({{t[0], t[1], t[2]}});
        p.facets.push_back(f);
    }
    return p;
}

// Regular octahedron as an 8-triangle PLC (convex, already Delaunay).
PLC octahedron_plc() {
    PLC p;
    p.points = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
                {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    const int t[8][3] = {{0, 2, 4}, {2, 1, 4}, {1, 3, 4}, {3, 0, 4},
                         {2, 0, 5}, {1, 2, 5}, {3, 1, 5}, {0, 3, 5}};
    for (auto& tri : t) {
        Facet f;
        f.polygons.push_back({{tri[0], tri[1], tri[2]}});
        p.facets.push_back(f);
    }
    return p;
}

// Unit cube as 6 quad facets. All eight vertices are cospherical, so conforming
// facet recovery cannot terminate on it — used only to exercise the Steiner
// budget guard.
PLC cube_plc() {
    PLC p;
    p.points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    const int q[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                         {2, 3, 7, 6}, {1, 2, 6, 5}, {0, 4, 7, 3}};
    for (auto& f : q) {
        Facet facet;
        facet.polygons.push_back({{f[0], f[1], f[2], f[3]}});
        p.facets.push_back(facet);
    }
    return p;
}

} // namespace

// (a) core recovery: a non-Delaunay facet subface becomes a mesh face.
CMG_TEST("recover_facets makes a non-Delaunay subface a mesh face") {
    PLC plc = prism_plc();
    auto subs = recover::plc_subfaces(plc);
    CMG_CHECK(subs.size() == 12); // 6 quads x 2 fan triangles

    // PRE: the cap subface {0,1,2} is NOT a face of the plain DT (it uses the
    // opposite diagonal on the bottom quad).
    auto m0 = delaunay(plc.points, {});
    CMG_CHECK(bool(m0));
    CMG_CHECK(!mesh_faces(*m0).count(sorted3(0, 1, 2)));

    // Recover, protecting the facet-boundary segments.
    auto segs = recover::facet_segments(plc);
    auto r = recover::recover_facets(plc.points, subs, 1000, segs);
    CMG_CHECK(r.complete);
    CMG_CHECK(r.steiner_added >= 1);
    CMG_CHECK(r.points.size() == plc.points.size() + r.steiner_added);

    // POST: every leaf subface is a face of the augmented DT.
    auto m1 = delaunay(r.points, {});
    CMG_CHECK(bool(m1));
    auto faces = mesh_faces(*m1);
    for (const auto& s : r.subfaces)
        CMG_CHECK(faces.count(sorted3(s[0], s[1], s[2])));
}

// (a) pipeline: preserve_facets recovers the facets and conserves volume.
CMG_TEST("preserve_facets pipeline recovers facets with conserved volume") {
    PLC plc = prism_plc();
    auto r0 = tetrahedralize(plc, MeshOptions{.plc = true});
    CMG_CHECK(bool(r0));

    MeshOptions o;
    o.plc = true;
    o.preserve_facets = true;
    auto r = tetrahedralize(plc, o);
    CMG_CHECK(bool(r));

    // Volume is conserved (convex domain => exact) and equals the carve-only run.
    CMG_CHECK(std::fabs(mesh_volume(*r) - 19.6) < 1e-6);
    CMG_CHECK(std::fabs(mesh_volume(*r) - mesh_volume(*r0)) < 1e-6);

    // Every recovered leaf subface is a face of the output tetrahedralization.
    // The pipeline is deterministic, so a standalone recovery reproduces the same
    // augmented points and leaf subfaces the pipeline used internally.
    auto segs = recover::facet_segments(plc);
    auto fr = recover::recover_facets(plc.points, recover::plc_subfaces(plc),
                                      100000, segs);
    CMG_CHECK(fr.complete);
    CMG_CHECK(r->points.size() == fr.points.size());
    auto faces = mesh_faces(*r);
    for (const auto& s : fr.subfaces)
        CMG_CHECK(faces.count(sorted3(s[0], s[1], s[2])));
}

// (a') a genuinely non-convex domain: facets recovered AND the exact non-convex
// volume conserved (a convex-hull over-fill would read ~14, not 12).
CMG_TEST("preserve_facets meshes a non-convex domain with exact volume") {
    PLC plc = l_prism_plc();
    MeshOptions o;
    o.plc = true;
    o.preserve_facets = true;
    auto r = tetrahedralize(plc, o);
    CMG_CHECK(bool(r));
    CMG_CHECK(std::fabs(mesh_volume(*r) - 12.0) < 1e-6); // exact L volume, not hull

    // Reproduce the pipeline's recovery (segments then facets, mirroring the pipeline)
    // and confirm every recovered leaf subface is an exact face of the output mesh.
    auto segs = recover::facet_segments(plc);
    auto srec = recover::recover_segments(plc.points, segs, 100000);
    auto fr = recover::recover_facets(srec.points, recover::plc_subfaces(plc), 100000,
                                      segs);
    CMG_CHECK(fr.complete);
    CMG_CHECK(r->points.size() == fr.points.size());
    auto faces = mesh_faces(*r);
    for (const auto& s : fr.subfaces)
        CMG_CHECK(faces.count(sorted3(s[0], s[1], s[2])));
}

// (b) an internal facet separates the domain into two distinctly-marked regions.
// This triangular bipyramid's wall {0,1,2} is already a Delaunay face (zero Steiner),
// so it exercises separation without a wall Steiner. The square-bipyramid test below
// covers the harder case where recovering the wall needs a Steiner point; with the
// seed/flood carve both now work (general internal-facet separation, once recovery
// completes, is no longer a non-goal — only non-terminating recovery on flat
// cospherical walls remains deferred).
CMG_TEST("internal facet separates two regions with no straddling tet") {
    PLC plc = bipyramid_plc();
    plc.regions.push_back({{0.3f, 0.3f, 0.4f}, 1.0, -1.0});  // upper cell
    plc.regions.push_back({{0.3f, 0.3f, -0.4f}, 2.0, -1.0}); // lower cell

    // Without facet recovery the two cells share the internal facet and merge
    // into one component, so a single region marker wins.
    auto merged = tetrahedralize(plc, MeshOptions{.plc = true});
    CMG_CHECK(bool(merged));
    std::set<int> merged_markers(merged->tet_markers.begin(),
                                 merged->tet_markers.end());
    CMG_CHECK(merged_markers.size() == 1);

    // With preserve_facets the internal facet is a constraint face: the cells
    // become distinct components and receive distinct markers.
    MeshOptions o;
    o.plc = true;
    o.preserve_facets = true;
    auto r = tetrahedralize(plc, o);
    CMG_CHECK(bool(r));
    CMG_CHECK(r->tetrahedra.size() == 2);

    std::set<int> markers(r->tet_markers.begin(), r->tet_markers.end());
    CMG_CHECK(markers.count(1) && markers.count(2)); // both regions present
    CMG_CHECK(markers.size() == 2);

    // The internal facet {0,1,2} is present as a mesh face and no tet straddles
    // it: every tetrahedron lies entirely on one side of the z=0 plane.
    CMG_CHECK(mesh_faces(*r).count(sorted3(0, 1, 2)));
    for (const auto& t : r->tetrahedra) {
        int pos = 0, neg = 0;
        for (int k = 0; k < 4; ++k) {
            double z = r->points[t[k]].z;
            if (z > 1e-9) ++pos;
            else if (z < -1e-9) ++neg;
        }
        CMG_CHECK(!(pos > 0 && neg > 0)); // no vertex straddling z=0
    }
}

// (b') the PREVIOUSLY-FAILING case: a square bipyramid whose internal wall needs a
// Steiner point to recover. Under the old ray-cast carve the entire lower half was
// dropped (~55% volume lost, both regions collapsing to one marker). The seed/flood
// carve must now keep both cells: volume conserved, two distinct markers, no
// straddling tet.
CMG_TEST("internal facet needing a wall Steiner keeps both cells (seed carve)") {
    PLC plc = square_bipyramid_plc();

    // Recovery of the internal wall must COMPLETE (adds a finite wall Steiner) — the
    // pipeline gates the flood carve on this. Confirm it empirically here.
    auto segs = recover::facet_segments(plc);
    auto srec = recover::recover_segments(plc.points, segs, 100000);
    auto fr = recover::recover_facets(srec.points, recover::plc_subfaces(plc), 100000,
                                      segs);
    CMG_CHECK(fr.complete);
    CMG_CHECK(fr.steiner_added >= 1); // the wall is not a plain Delaunay face

    plc.regions.push_back({{3.0f, 2.0f, 1.0f}, 1.0, -1.0});  // upper cell
    plc.regions.push_back({{3.0f, 2.0f, -1.0f}, 2.0, -1.0}); // lower cell

    MeshOptions o;
    o.plc = true;
    o.preserve_facets = true;
    auto r = tetrahedralize(plc, o);
    CMG_CHECK(bool(r));

    // Two pyramids over the shoelace-area-19.6 quad, each height 3:
    // 2 * (1/3) * 19.6 * 3 = 39.2. The old carve returned ~19.6 (lower half gone).
    CMG_CHECK(std::fabs(mesh_volume(*r) - 39.2) < 1e-4);

    // Both region markers present and distinct.
    std::set<int> markers(r->tet_markers.begin(), r->tet_markers.end());
    CMG_CHECK(markers.count(1) && markers.count(2));
    CMG_CHECK(markers.size() == 2);

    // Every recovered leaf subface (the wall is split at its Steiner point) is a
    // face of the output mesh, and no tet straddles the z=0 wall plane.
    CMG_CHECK(r->points.size() == fr.points.size());
    auto faces = mesh_faces(*r);
    for (const auto& s : fr.subfaces)
        CMG_CHECK(faces.count(sorted3(s[0], s[1], s[2])));
    for (const auto& t : r->tetrahedra) {
        int pos = 0, neg = 0;
        for (int k = 0; k < 4; ++k) {
            double z = r->points[t[k]].z;
            if (z > 1e-9) ++pos;
            else if (z < -1e-9) ++neg;
        }
        CMG_CHECK(!(pos > 0 && neg > 0)); // no vertex straddling z=0
    }
}

// (b'') a hole seed inside a recovered domain still removes that component. Same
// square bipyramid: the flood carve keeps both cells (the wall blocks it), then
// region::apply removes the lower (hole-seeded) component, leaving the upper only.
CMG_TEST("hole inside a recovered domain removes its component") {
    PLC plc = square_bipyramid_plc();
    plc.regions.push_back({{3.0f, 2.0f, 1.0f}, 1.0, -1.0}); // upper cell kept
    plc.holes.push_back({3.0f, 2.0f, -1.0f});               // lower cell is a hole

    MeshOptions o;
    o.plc = true;
    o.preserve_facets = true;
    auto r = tetrahedralize(plc, o);
    CMG_CHECK(bool(r));

    // Only the upper pyramid remains: (1/3) * 19.6 * 3 = 19.6.
    CMG_CHECK(std::fabs(mesh_volume(*r) - 19.6) < 1e-4);

    // No surviving tet is below the wall plane (the hole cell is gone).
    for (const auto& t : r->tetrahedra) {
        int neg = 0;
        for (int k = 0; k < 4; ++k)
            if (r->points[t[k]].z < -1e-9) ++neg;
        CMG_CHECK(neg == 0);
    }
}

// (c) a convex PLC already meshed by Delaunay faces adds zero Steiner points and
// yields the identical mesh with and without preserve_facets.
CMG_TEST("convex PLC preserve_facets adds zero Steiner and is unchanged") {
    for (const PLC& plc : {tet_plc(), octahedron_plc()}) {
        // Core recovery adds nothing.
        auto segs = recover::facet_segments(plc);
        auto fr = recover::recover_facets(plc.points, recover::plc_subfaces(plc),
                                          1000, segs);
        CMG_CHECK(fr.complete);
        CMG_CHECK(fr.steiner_added == 0);

        // Pipeline output is identical with and without preserve_facets.
        auto base = tetrahedralize(plc, MeshOptions{.plc = true});
        MeshOptions o;
        o.plc = true;
        o.preserve_facets = true;
        auto pf = tetrahedralize(plc, o);
        CMG_CHECK(bool(base) && bool(pf));
        CMG_CHECK(base->points.size() == pf->points.size());
        CMG_CHECK(base->tetrahedra.size() == pf->tetrahedra.size());
        CMG_CHECK(base->tetrahedra == pf->tetrahedra);
    }
}

// (d) an exhausted Steiner budget reports incompleteness without looping. The
// cube's cospherical vertices make conforming recovery non-terminating, so a
// small budget must stop cleanly.
CMG_TEST("facet recovery respects the Steiner budget without looping") {
    PLC plc = cube_plc();
    auto subs = recover::plc_subfaces(plc);
    auto segs = recover::facet_segments(plc);
    const std::size_t budget = 20;
    auto r = recover::recover_facets(plc.points, subs, budget, segs);
    CMG_CHECK(!r.complete);
    CMG_CHECK(r.steiner_added <= budget);
    CMG_CHECK(r.points.size() == plc.points.size() + r.steiner_added);
}
