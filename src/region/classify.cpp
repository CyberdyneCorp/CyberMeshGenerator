// CyberMeshGenerator — region attributes and hole classification.
#include "cmg/region/classify.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <numeric>
#include <set>
#include <vector>

#include "cmg/predicates/robust.hpp"

namespace cmg::region {
namespace {

// Union-find over tetrahedron indices.
struct DSU {
    std::vector<int> parent;
    explicit DSU(int n) : parent(n) { std::iota(parent.begin(), parent.end(), 0); }
    int find(int x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    }
    void unite(int a, int b) { parent[find(a)] = find(b); }
};

std::array<int, 3> sorted3(int a, int b, int c) {
    std::array<int, 3> k{a, b, c};
    std::sort(k.begin(), k.end());
    return k;
}

// Index of the tetrahedron containing `p` (tets stored orient3d(v0..v3) < 0), or -1.
int locate(const Mesh& m, const Point3& p) {
    for (int i = 0; i < static_cast<int>(m.tetrahedra.size()); ++i) {
        const auto& t = m.tetrahedra[i];
        const Point3 &v0 = m.points[t[0]], &v1 = m.points[t[1]],
                     &v2 = m.points[t[2]], &v3 = m.points[t[3]];
        if (robust::orient3d(p, v1, v2, v3) <= 0 &&
            robust::orient3d(v0, p, v2, v3) <= 0 &&
            robust::orient3d(v0, v1, p, v3) <= 0 &&
            robust::orient3d(v0, v1, v2, p) <= 0)
            return i;
    }
    return -1;
}

void rebuild_boundary(Mesh& m) {
    m.faces.clear();
    m.face_markers.clear();
    std::map<std::array<int, 3>, std::pair<int, std::array<int, 3>>> fc;
    auto add = [&](int a, int b, int c) {
        auto& e = fc[sorted3(a, b, c)];
        e.first++;
        e.second = {a, b, c};
    };
    for (const auto& t : m.tetrahedra) {
        add(t[1], t[2], t[3]);
        add(t[0], t[2], t[3]);
        add(t[0], t[1], t[3]);
        add(t[0], t[1], t[2]);
    }
    for (const auto& [key, val] : fc)
        if (val.first == 1) {
            m.faces.push_back(val.second);
            m.face_markers.push_back(1);
        }
}

} // namespace

void apply(Mesh& mesh, const PLC& plc, bool label_regions) {
    if (plc.regions.empty() && plc.holes.empty() && !label_regions) return;
    const int nt = static_cast<int>(mesh.tetrahedra.size());
    if (nt == 0) return;

    // Connected components of the interior mesh via shared-face adjacency.
    DSU dsu(nt);
    std::map<std::array<int, 3>, int> face_owner;
    for (int i = 0; i < nt; ++i) {
        const auto& t = mesh.tetrahedra[i];
        const int f[4][3] = {{t[1], t[2], t[3]}, {t[0], t[2], t[3]},
                             {t[0], t[1], t[3]}, {t[0], t[1], t[2]}};
        for (const auto& tri : f) {
            auto key = sorted3(tri[0], tri[1], tri[2]);
            auto it = face_owner.find(key);
            if (it == face_owner.end()) face_owner[key] = i;
            else dsu.unite(i, it->second);
        }
    }

    // Hole-seeded components are removed.
    std::set<int> removed_roots;
    for (const Point3& h : plc.holes) {
        int t = locate(mesh, h);
        if (t >= 0) removed_roots.insert(dsu.find(t));
    }

    // Region-seeded components take their attribute.
    std::map<int, int> root_attr;
    for (const Region& r : plc.regions) {
        int t = locate(mesh, r.seed);
        if (t >= 0) root_attr[dsu.find(t)] = static_cast<int>(r.attribute);
    }

    // Auto-label surviving, unseeded components with distinct nonzero attributes,
    // in a deterministic order (by smallest member index).
    if (label_regions) {
        std::map<int, int> root_min;
        for (int i = 0; i < nt; ++i) {
            int rt = dsu.find(i);
            auto it = root_min.find(rt);
            if (it == root_min.end() || i < it->second) root_min[rt] = i;
        }
        std::vector<std::pair<int, int>> order(root_min.begin(), root_min.end());
        std::sort(order.begin(), order.end(),
                  [](auto& a, auto& b) { return a.second < b.second; });
        int next = 1;
        for (auto& [rt, mn] : order) {
            (void)mn;
            if (removed_roots.count(rt) || root_attr.count(rt)) continue;
            root_attr[rt] = next++;
        }
    }

    // Rebuild the tetrahedron list, dropping removed components and writing markers.
    std::vector<Tetrahedron> kept;
    std::vector<int> markers;
    for (int i = 0; i < nt; ++i) {
        int rt = dsu.find(i);
        if (removed_roots.count(rt)) continue;
        kept.push_back(mesh.tetrahedra[i]);
        auto it = root_attr.find(rt);
        markers.push_back(it == root_attr.end() ? 0 : it->second);
    }
    mesh.tetrahedra = std::move(kept);
    mesh.tet_markers = std::move(markers);
    rebuild_boundary(mesh);
}

} // namespace cmg::region
