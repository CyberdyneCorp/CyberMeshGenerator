// CyberMesh — idiomatic Swift API over the CyberMeshGenerator C ABI.
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
}

/// A recoverable meshing failure surfaced from the C++ core.
public struct MeshError: Error { public let code: Int32; public let message: String }

public enum CyberMesh {
    /// Library version string, e.g. "0.1.0".
    public static var version: String { String(cString: cmg_version()) }

    /// Delaunay tetrahedralization of a point set. Metal-accelerated on device
    /// when available; CPU otherwise.
    public static func delaunay(_ points: [Point3]) throws -> Mesh {
        var flat = [Double]()
        flat.reserveCapacity(points.count * 3)
        for p in points { flat.append(p.x); flat.append(p.y); flat.append(p.z) }

        let opts = cmg_options_create()
        defer { cmg_options_destroy(opts) }
        var out: OpaquePointer? = nil
        var err = [CChar](repeating: 0, count: 256)

        let st = flat.withUnsafeBufferPointer { buf in
            cmg_delaunay(buf.baseAddress, points.count, opts, &out, &err, 256)
        }
        guard st == CMG_OK, let mesh = out else {
            throw MeshError(code: Int32(st.rawValue), message: String(cString: err))
        }
        defer { cmg_mesh_destroy(mesh) }

        let npts = cmg_mesh_num_points(mesh)
        let ntet = cmg_mesh_num_tets(mesh)
        let pp = cmg_mesh_points(mesh)!
        let tp = cmg_mesh_tets(mesh)!
        var pts = [Point3](); pts.reserveCapacity(npts)
        for i in 0..<npts { pts.append(Point3(pp[i*3], pp[i*3+1], pp[i*3+2])) }
        var tets = [[Int32]](); tets.reserveCapacity(ntet)
        for i in 0..<ntet { tets.append([tp[i*4], tp[i*4+1], tp[i*4+2], tp[i*4+3]]) }
        return Mesh(points: pts, tetrahedra: tets)
    }
}
