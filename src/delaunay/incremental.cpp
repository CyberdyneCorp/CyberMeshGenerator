// CyberMeshGenerator — incremental Bowyer-Watson Delaunay tetrahedralization.
//
// Ports TetGen's default operation. State lives in a per-call Triangulation (no
// globals). Every orientation / in-sphere / power decision uses the exact
// predicates, so degenerate (coplanar / cospherical) input is handled robustly.
// Tets are stored internally in POSITIVE orientation (orient3d(v0,v1,v2,v3) > 0)
// so that insphere(...) > 0 means "inside the circumsphere"; output tets are
// re-normalised to the library's orient3d < 0 convention.
#include "cmg/delaunay/incremental.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <unordered_set>
#include <vector>

#include "cmg/backend/dispatch.hpp"
#include "cmg/predicates/robust.hpp"

namespace cmg::dt {
namespace {

using predicates::REAL;

struct Tet {
    std::array<int, 4> v;
    std::array<int, 4> nbr; // neighbor across face opposite v[k]; -1 = none
    bool dead = false;
};

// ---- BRIO-Hilbert spatial sort ------------------------------------------

// Skilling's transpose form of the 3-D Hilbert curve (public algorithm).
void axes_to_transpose(std::uint32_t* X, int bits) {
    std::uint32_t M = 1u << (bits - 1), P, Q, t;
    for (Q = M; Q > 1; Q >>= 1) {
        P = Q - 1;
        for (int i = 0; i < 3; i++) {
            if (X[i] & Q) {
                X[0] ^= P;
            } else {
                t = (X[0] ^ X[i]) & P;
                X[0] ^= t;
                X[i] ^= t;
            }
        }
    }
    for (int i = 1; i < 3; i++) X[i] ^= X[i - 1];
    t = 0;
    for (Q = M; Q > 1; Q >>= 1)
        if (X[2] & Q) t ^= Q - 1;
    for (int i = 0; i < 3; i++) X[i] ^= t;
}

std::uint64_t hilbert_key(std::uint32_t x, std::uint32_t y, std::uint32_t z,
                          int bits) {
    std::uint32_t X[3] = {x, y, z};
    axes_to_transpose(X, bits);
    std::uint64_t key = 0;
    for (int b = bits - 1; b >= 0; --b)
        for (int i = 0; i < 3; i++)
            key = (key << 1) | ((X[i] >> b) & 1u);
    return key;
}

// Deterministic small PRNG (splitmix64) for BRIO round assignment.
std::uint64_t splitmix(std::uint64_t& s) {
    std::uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

// Produce the insertion order over the given point indices, BRIO-Hilbert sorted
// (or input order when disabled). `idx` holds the indices to order.
std::vector<int> insertion_order(std::span<const Point3> pts,
                                 std::vector<int> idx, const MeshOptions& opts) {
    if (!opts.spatial_sort || idx.size() < 3) return idx;

    constexpr int bits = 20;
    Real lo[3] = {pts[idx[0]].x, pts[idx[0]].y, pts[idx[0]].z};
    Real hi[3] = {lo[0], lo[1], lo[2]};
    for (int i : idx)
        for (int a = 0; a < 3; a++) {
            lo[a] = std::min(lo[a], pts[i][a]);
            hi[a] = std::max(hi[a], pts[i][a]);
        }
    const std::uint32_t scale = (1u << bits) - 1;
    auto grid = [&](Real val, int a) -> std::uint32_t {
        Real range = hi[a] - lo[a];
        if (range <= 0) return 0;
        Real f = (val - lo[a]) / range;
        return static_cast<std::uint32_t>(f * static_cast<Real>(scale));
    };

    // BRIO round per point (geometric: promote with prob 1/2), then Hilbert key.
    std::uint64_t seed = opts.sort_seed ? opts.sort_seed : 1;
    std::vector<int> round(idx.size());
    std::vector<std::uint64_t> key(idx.size());
    for (std::size_t k = 0; k < idx.size(); ++k) {
        int i = idx[k];
        int r = 0;
        while ((splitmix(seed) & 1u) && r < 31) ++r;
        round[k] = r;
        key[k] = hilbert_key(grid(pts[i].x, 0), grid(pts[i].y, 1),
                             grid(pts[i].z, 2), bits);
    }
    std::vector<int> perm(idx.size());
    for (std::size_t k = 0; k < perm.size(); ++k) perm[k] = static_cast<int>(k);
    std::sort(perm.begin(), perm.end(), [&](int a, int b) {
        if (round[a] != round[b]) return round[a] < round[b];
        return key[a] < key[b];
    });
    std::vector<int> ordered(idx.size());
    for (std::size_t k = 0; k < perm.size(); ++k) ordered[k] = idx[perm[k]];
    return ordered;
}

// ---- The triangulation --------------------------------------------------

class Triangulation {
public:
    Triangulation(std::span<const Point3> input, const MeshOptions& opts)
        : opts_(opts), n_real_(static_cast<int>(input.size())),
          weighted_(opts.weighted) {
        pts_.assign(input.begin(), input.end());
        build_heights();
        add_super_tet();
    }

    double orient(int a, int b, int c, int d) const {
        return robust::orient3d(pts_[a], pts_[b], pts_[c], pts_[d]);
    }

    // > 0 iff p is inside the (power) circumsphere of positively-oriented tet t.
    double conflict(const Tet& t, int p) const {
        if (!weighted_)
            return robust::insphere(pts_[t.v[0]], pts_[t.v[1]], pts_[t.v[2]],
                                    pts_[t.v[3]], pts_[p]);
        return power(t, p);
    }

    void insert(int p) {
        int t0 = locate(p);
        if (weighted_ && conflict(tets_[t0], p) <= 0) return; // redundant point

        // 1. Flood the Delaunay cavity: connected tets in conflict with p.
        ++stamp_now_;
        std::vector<int> cavity{t0};
        stamp_[t0] = stamp_now_;
        for (std::size_t h = 0; h < cavity.size(); ++h) {
            int t = cavity[h];
            for (int k = 0; k < 4; k++) {
                int m = tets_[t].nbr[k];
                if (m < 0 || stamp_[m] == stamp_now_ || tets_[m].dead) continue;
                if (conflict(tets_[m], p) > 0) {
                    stamp_[m] = stamp_now_;
                    cavity.push_back(m);
                }
            }
        }

        // 2. Boundary faces = cavity faces whose neighbor is outside the cavity.
        struct BFace { std::array<int, 3> vtx; int ext; int extslot; };
        std::vector<BFace> faces;
        for (int t : cavity) {
            for (int k = 0; k < 4; k++) {
                int m = tets_[t].nbr[k];
                if (m >= 0 && stamp_[m] == stamp_now_) continue; // interior
                faces.push_back({face_of(tets_[t], k), m, slot_back(m, t)});
            }
        }

        // 3. Delete cavity tets, then join p to each boundary face.
        for (int t : cavity) tets_[t].dead = true;
        std::map<std::pair<int, int>, std::pair<int, int>> edge_map;
        int last = -1;
        for (const BFace& bf : faces) {
            last = build_tet(bf.vtx, p, bf.ext, bf.extslot, edge_map);
        }
        if (last >= 0) hint_ = last;
    }

    expected<Mesh, MeshError> finalize() {
        Mesh mesh;
        mesh.index_base = opts_.index_base;
        mesh.points.assign(pts_.begin(), pts_.begin() + n_real_);
        mesh.point_markers.assign(n_real_, 0);

        std::vector<int> remap(tets_.size(), -1);
        for (std::size_t i = 0; i < tets_.size(); ++i) {
            if (!is_output(static_cast<int>(i))) continue;
            remap[i] = static_cast<int>(mesh.tetrahedra.size());
            const Tet& t = tets_[i];
            // Emit hull faces (neighbor is not an output tet) before re-orienting.
            for (int k = 0; k < 4; k++) {
                int m = t.nbr[k];
                if (m < 0 || !is_output(m)) {
                    mesh.faces.push_back(face_of(t, k));
                    mesh.face_markers.push_back(1);
                }
            }
            mesh.tetrahedra.push_back(oriented_out(t));
            mesh.tet_markers.push_back(0);
        }

        if (mesh.tetrahedra.empty()) {
            return unexpected(MeshError{
                MeshErrorCode::InvalidInput,
                "no tetrahedron could be formed (are all points coplanar?)"});
        }
        if (opts_.emit_neighbors) fill_neighbors(mesh, remap);
        return mesh;
    }

private:
    void build_heights() {
        height_.resize(pts_.size());
        for (std::size_t i = 0; i < pts_.size(); ++i) {
            const Point3& p = pts_[i];
            double h = double(p.x) * p.x + double(p.y) * p.y + double(p.z) * p.z;
            if (weighted_ && i < opts_.weights.size()) h -= opts_.weights[i];
            height_[i] = h;
        }
    }

    double power(const Tet& t, int p) const {
        REAL a[3], b[3], c[3], d[3], e[3];
        fill(a, t.v[0]); fill(b, t.v[1]); fill(c, t.v[2]); fill(d, t.v[3]);
        fill(e, p);
        return predicates::orient4d(
            a, b, c, d, e, static_cast<REAL>(height_[t.v[0]]),
            static_cast<REAL>(height_[t.v[1]]), static_cast<REAL>(height_[t.v[2]]),
            static_cast<REAL>(height_[t.v[3]]), static_cast<REAL>(height_[p]));
    }

    void fill(REAL out[3], int i) const {
        out[0] = static_cast<REAL>(pts_[i].x);
        out[1] = static_cast<REAL>(pts_[i].y);
        out[2] = static_cast<REAL>(pts_[i].z);
    }

    void add_super_tet() {
        Real lo[3] = {pts_[0].x, pts_[0].y, pts_[0].z};
        Real hi[3] = {lo[0], lo[1], lo[2]};
        for (int i = 0; i < n_real_; i++)
            for (int a = 0; a < 3; a++) {
                lo[a] = std::min(lo[a], pts_[i][a]);
                hi[a] = std::max(hi[a], pts_[i][a]);
            }
        Real cx = (lo[0] + hi[0]) / 2, cy = (lo[1] + hi[1]) / 2,
             cz = (lo[2] + hi[2]) / 2;
        Real ext = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2],
                             static_cast<Real>(1)});
        Real big = 1000 * ext;
        const int dir[4][3] = {{1, 1, 1}, {1, -1, -1}, {-1, 1, -1}, {-1, -1, 1}};
        for (auto& d : dir)
            pts_.push_back({cx + big * d[0], cy + big * d[1], cz + big * d[2]});
        height_.resize(pts_.size());
        for (std::size_t i = n_real_; i < pts_.size(); ++i) {
            const Point3& p = pts_[i];
            height_[i] = double(p.x) * p.x + double(p.y) * p.y + double(p.z) * p.z;
        }

        Tet s;
        s.v = {n_real_, n_real_ + 1, n_real_ + 2, n_real_ + 3};
        if (orient(s.v[0], s.v[1], s.v[2], s.v[3]) < 0) std::swap(s.v[2], s.v[3]);
        s.nbr = {-1, -1, -1, -1};
        tets_.push_back(s);
        stamp_.push_back(0);
        hint_ = 0;
    }

    // The face opposite v[k], as a vertex triple (order irrelevant downstream).
    static std::array<int, 3> face_of(const Tet& t, int k) {
        std::array<int, 3> f{};
        int j = 0;
        for (int i = 0; i < 4; i++)
            if (i != k) f[j++] = t.v[i];
        return f;
    }

    int slot_back(int m, int t) const {
        if (m < 0) return -1;
        for (int k = 0; k < 4; k++)
            if (tets_[m].nbr[k] == t) return k;
        return -1;
    }

    int build_tet(std::array<int, 3> face, int p, int ext, int extslot,
                  std::map<std::pair<int, int>, std::pair<int, int>>& edge_map) {
        Tet nt;
        nt.v = {face[0], face[1], face[2], p};
        if (orient(nt.v[0], nt.v[1], nt.v[2], nt.v[3]) < 0)
            std::swap(nt.v[0], nt.v[1]); // make positively oriented
        nt.nbr = {-1, -1, -1, ext};
        int idx = static_cast<int>(tets_.size());
        tets_.push_back(nt);
        stamp_.push_back(0);
        if (ext >= 0 && extslot >= 0) tets_[ext].nbr[extslot] = idx;

        // Side faces (slots 0,1,2) each carry an edge of the boundary triangle.
        for (int s = 0; s < 3; s++) {
            int u = -1, w = -1;
            for (int i = 0; i < 3; i++)
                if (i != s) (u < 0 ? u : w) = tets_[idx].v[i];
            auto e = std::minmax(u, w);
            auto it = edge_map.find({e.first, e.second});
            if (it == edge_map.end()) {
                edge_map[{e.first, e.second}] = {idx, s};
            } else {
                int j = it->second.first, sj = it->second.second;
                tets_[idx].nbr[s] = j;
                tets_[j].nbr[sj] = idx;
            }
        }
        return idx;
    }

    int locate(int p) {
        int t = tets_[hint_].dead ? any_live() : hint_;
        for (int steps = 0; steps < 2 * static_cast<int>(tets_.size()) + 16;
             ++steps) {
            const auto v = tets_[t].v;
            double o0 = orient(p, v[1], v[2], v[3]);
            if (o0 < 0 && tets_[t].nbr[0] >= 0) { t = tets_[t].nbr[0]; continue; }
            double o1 = orient(v[0], p, v[2], v[3]);
            if (o1 < 0 && tets_[t].nbr[1] >= 0) { t = tets_[t].nbr[1]; continue; }
            double o2 = orient(v[0], v[1], p, v[3]);
            if (o2 < 0 && tets_[t].nbr[2] >= 0) { t = tets_[t].nbr[2]; continue; }
            double o3 = orient(v[0], v[1], v[2], p);
            if (o3 < 0 && tets_[t].nbr[3] >= 0) { t = tets_[t].nbr[3]; continue; }
            return t;
        }
        return brute_locate(p);
    }

    int any_live() const {
        for (std::size_t i = 0; i < tets_.size(); ++i)
            if (!tets_[i].dead) return static_cast<int>(i);
        return 0;
    }

    int brute_locate(int p) const {
        for (std::size_t i = 0; i < tets_.size(); ++i) {
            if (tets_[i].dead) continue;
            const auto v = tets_[i].v;
            if (orient(p, v[1], v[2], v[3]) >= 0 &&
                orient(v[0], p, v[2], v[3]) >= 0 &&
                orient(v[0], v[1], p, v[3]) >= 0 &&
                orient(v[0], v[1], v[2], p) >= 0)
                return static_cast<int>(i);
        }
        return any_live();
    }

    bool is_output(int i) const {
        if (i < 0 || tets_[i].dead) return false;
        for (int k = 0; k < 4; k++)
            if (tets_[i].v[k] >= n_real_) return false; // touches a super-vertex
        return true;
    }

    // Re-orient a positively-oriented internal tet to the library's orient<0 form.
    Tetrahedron oriented_out(const Tet& t) const {
        Tetrahedron out{t.v[0], t.v[1], t.v[2], t.v[3]};
        std::swap(out[2], out[3]); // orient(>0) -> orient(<0)
        return out;
    }

    void fill_neighbors(Mesh& mesh, const std::vector<int>& remap) const {
        for (std::size_t i = 0; i < tets_.size(); ++i) {
            if (remap[i] < 0) continue;
            const Tet& t = tets_[i];
            // Output tet had v2,v3 swapped, so faces (neighbor slots) 2 and 3 swap.
            std::array<int, 4> nb{};
            for (int k = 0; k < 4; k++) {
                int m = t.nbr[k];
                nb[k] = (m >= 0 && remap[m] >= 0) ? remap[m] : -1;
            }
            std::swap(nb[2], nb[3]);
            mesh.neighbors.push_back(nb);
        }
    }

    const MeshOptions& opts_;
    int n_real_;
    bool weighted_;
    std::vector<Point3> pts_;
    std::vector<double> height_;
    std::vector<Tet> tets_;
    std::vector<int> stamp_;
    int stamp_now_ = 0;
    int hint_ = 0;
};

// Deduplicate exact-coincident points; return the indices to actually insert.
std::vector<int> unique_indices(std::span<const Point3> pts) {
    std::map<std::array<Real, 3>, int> seen;
    std::vector<int> keep;
    for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
        std::array<Real, 3> key{pts[i].x, pts[i].y, pts[i].z};
        if (seen.emplace(key, i).second) keep.push_back(i);
    }
    return keep;
}

} // namespace

expected<Mesh, MeshError> incremental(std::span<const Point3> points,
                                      const MeshOptions& opts) {
    robust::ensure_initialized();
    if (opts.weighted && !opts.weights.empty() &&
        opts.weights.size() != points.size()) {
        return unexpected(MeshError{
            MeshErrorCode::InvalidInput,
            "weighted Delaunay requires one weight per point"});
    }

    Triangulation tri(points, opts);
    std::vector<int> order =
        insertion_order(points, unique_indices(points), opts);

    // Touch the dispatch shim so the accelerable-phase contract is exercised;
    // the CPU path runs today, device kernels are Phase 11.
    (void)backend::should_offload(backend::Op::PointLocation, order.size());

    for (int p : order) tri.insert(p);
    return tri.finalize();
}

} // namespace cmg::dt
