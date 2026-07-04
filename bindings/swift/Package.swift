// swift-tools-version:5.9
// CyberMesh — Swift bindings for CyberMeshGenerator (iOS / macOS).
//
// An idiomatic Swift value-type API over the stable C ABI shim (bindings/c). On
// Apple platforms the underlying core can use the Metal backend when available,
// falling back to the CPU path otherwise. All meshing logic lives in the C++
// core; this package is a thin translation layer.
import PackageDescription

let package = Package(
    name: "CyberMesh",
    platforms: [.iOS(.v15), .macOS(.v12)],
    products: [
        .library(name: "CyberMesh", targets: ["CyberMesh"])
    ],
    targets: [
        // C ABI headers exposed to Swift. In a real distribution this wraps the
        // prebuilt libcmg_c as a binaryTarget / XCFramework.
        .target(
            name: "CCyberMesh",
            path: "Sources/CCyberMesh"
        ),
        .target(
            name: "CyberMesh",
            dependencies: ["CCyberMesh"],
            path: "Sources/CyberMesh"
        ),
        .testTarget(
            name: "CyberMeshTests",
            dependencies: ["CyberMesh"],
            path: "Tests/CyberMeshTests"
        ),
    ]
)
