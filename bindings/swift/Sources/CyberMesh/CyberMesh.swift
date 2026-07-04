// CyberMesh — idiomatic Swift API over the CyberMeshGenerator C ABI.
//
// NOTE: this environment has no Swift toolchain, so this wrapper is verified only
// at the C-ABI layer (which is tested); the Swift source itself is not compiled
// here. It mirrors the tested Python binding one-to-one over the same C ABI.
import CCyberMesh
import Foundation

/// A point in 3-D space.
public struct Point3: Equatable {
    public var x, y, z: Double
    public init(_ x: Double, _ y: Double, _ z: Double) { (self.x, self.y, self.z) = (x, y, z) }
}

/// An owning tetrahedral mesh returned by the generator.
public struct Mesh {
    public let points: [Point3]
    public let tetrahedra: [[Int32]]   // 4 vertex indices each
    public let faces: [[Int32]]        // 3 vertex indices each
}

/// Typed meshing options (a subset mirroring the C++ MeshOptions).
public struct MeshOptions {
    public var plc = false
    public var maxVolume: Double? = nil
    public var quality: (radiusEdge: Double, minDihedral: Double)? = nil
    public var preserveEdges = false
    public init() {}

    /// Build a C options handle (caller destroys it).
    func makeHandle() -> OpaquePointer? {
        let o = cmg_options_create()
        cmg_options_set_plc(o, plc ? 1 : 0)
        if let v = maxVolume { cmg_options_set_max_volume(o, v) }
        if let q = quality { cmg_options_set_quality(o, q.radiusEdge, q.minDihedral) }
        cmg_options_set_preserve_edges(o, preserveEdges ? 1 : 0)
        return o
    }
}

/// A recoverable meshing failure surfaced from the C++ core.
public struct MeshError: Error { public let code: Int32; public let message: String }

/// A Piecewise-Linear Complex (input domain). Owns a C handle.
public final class PLC {
    let handle: OpaquePointer?
    private let owns: Bool

    public init() { handle = cmg_plc_create(); owns = true }
    init(adopting h: OpaquePointer?) { handle = h; owns = true }
    deinit { if owns, let h = handle { cmg_plc_destroy(h) } }

    public func addPoints(_ pts: [Point3]) {
        var flat = [Double](); flat.reserveCapacity(pts.count * 3)
        for p in pts { flat.append(p.x); flat.append(p.y); flat.append(p.z) }
        flat.withUnsafeBufferPointer { cmg_plc_set_points(handle, $0.baseAddress, pts.count) }
    }

    public func addFacet(_ indices: [Int32], marker: Int32 = 0) {
        indices.withUnsafeBufferPointer {
            cmg_plc_add_facet(handle, $0.baseAddress, indices.count, marker)
        }
    }

    public var numPoints: Int { cmg_plc_num_points(handle) }

    /// The PLC vertices (e.g. read back from a file).
    public var points: [Point3] {
        let n = cmg_plc_num_points(handle)
        guard n > 0, let p = cmg_plc_points(handle) else { return [] }
        return (0..<n).map { Point3(p[$0 * 3], p[$0 * 3 + 1], p[$0 * 3 + 2]) }
    }

    /// The PLC facets fan-triangulated to vertex-index triples.
    public var triangles: [[Int32]] {
        let m = cmg_plc_num_triangles(handle)
        guard m > 0, let t = cmg_plc_triangles(handle) else { return [] }
        return (0..<m).map { [t[$0 * 3], t[$0 * 3 + 1], t[$0 * 3 + 2]] }
    }
}

public enum CyberMesh {
    /// Library version string, e.g. "0.1.0".
    public static var version: String { String(cString: cmg_version()) }

    private static func mesh(from handle: OpaquePointer) -> Mesh {
        defer { cmg_mesh_destroy(handle) }
        let np = cmg_mesh_num_points(handle), nt = cmg_mesh_num_tets(handle)
        let nf = cmg_mesh_num_faces(handle)
        let pp = cmg_mesh_points(handle)!, tp = cmg_mesh_tets(handle)
        var pts = [Point3](); pts.reserveCapacity(np)
        for i in 0..<np { pts.append(Point3(pp[i*3], pp[i*3+1], pp[i*3+2])) }
        var tets = [[Int32]]()
        if let tp { for i in 0..<nt { tets.append([tp[i*4], tp[i*4+1], tp[i*4+2], tp[i*4+3]]) } }
        var faces = [[Int32]]()
        if let fp = cmg_mesh_faces(handle) { for i in 0..<nf { faces.append([fp[i*3], fp[i*3+1], fp[i*3+2]]) } }
        return Mesh(points: pts, tetrahedra: tets, faces: faces)
    }

    /// Delaunay tetrahedralization of a point set.
    public static func delaunay(_ points: [Point3], _ options: MeshOptions = MeshOptions()) throws -> Mesh {
        var flat = [Double](); flat.reserveCapacity(points.count * 3)
        for p in points { flat.append(p.x); flat.append(p.y); flat.append(p.z) }
        let opts = options.makeHandle(); defer { cmg_options_destroy(opts) }
        var out: OpaquePointer? = nil; var err = [CChar](repeating: 0, count: 256)
        let st = flat.withUnsafeBufferPointer {
            cmg_delaunay($0.baseAddress, points.count, opts, &out, &err, 256)
        }
        guard st == CMG_OK, let m = out else {
            throw MeshError(code: Int32(st.rawValue), message: String(cString: err))
        }
        return mesh(from: m)
    }

    /// Tetrahedralize a PLC (boundary-conforming solid for a closed input).
    public static func tetrahedralize(_ plc: PLC, _ options: MeshOptions = MeshOptions()) throws -> Mesh {
        let opts = options.makeHandle(); defer { cmg_options_destroy(opts) }
        var out: OpaquePointer? = nil; var err = [CChar](repeating: 0, count: 256)
        let st = cmg_tetrahedralize(plc.handle, opts, &out, &err, 256)
        guard st == CMG_OK, let m = out else {
            throw MeshError(code: Int32(st.rawValue), message: String(cString: err))
        }
        return mesh(from: m)
    }

    /// Simplify a PLC surface by grid vertex clustering (`grid` cells along the
    /// longest axis; higher keeps more triangles). Returns a new PLC — handy to
    /// decimate a dense loaded surface before meshing.
    public static func simplify(_ plc: PLC, grid: Int32 = 34) throws -> PLC {
        var out: OpaquePointer? = nil; var err = [CChar](repeating: 0, count: 256)
        let st = cmg_plc_simplify(plc.handle, grid, &out, &err, 256)
        guard st == CMG_OK, let h = out else {
            throw MeshError(code: Int32(st.rawValue), message: String(cString: err))
        }
        return PLC(adopting: h)
    }

    /// Load a PLC surface from a file (.stl / .obj / .off / .ply / .poly / .smesh).
    public static func readPLC(_ path: String) throws -> PLC {
        var out: OpaquePointer? = nil; var err = [CChar](repeating: 0, count: 256)
        let st = path.withCString { cmg_read_plc($0, &out, &err, 256) }
        guard st == CMG_OK, let h = out else {
            throw MeshError(code: Int32(st.rawValue), message: String(cString: err))
        }
        return PLC(adopting: h)
    }

    /// Load a volumetric mesh from a file (.ele+.node / .vtk / .mesh).
    public static func readMesh(_ path: String) throws -> Mesh {
        var out: OpaquePointer? = nil; var err = [CChar](repeating: 0, count: 256)
        let st = path.withCString { cmg_read_mesh($0, &out, &err, 256) }
        guard st == CMG_OK, let m = out else {
            throw MeshError(code: Int32(st.rawValue), message: String(cString: err))
        }
        return mesh(from: m)
    }
}
