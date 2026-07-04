// CyberMeshGenerator — error and result model.
//
// Recoverable meshing failures are returned as `cmg::expected<…, MeshError>`;
// programmer errors and violated invariants throw `cmg::error`. The library never
// calls exit()/abort()/longjmp out of a meshing call (unlike TetGen's
// terminatetetgen()). When CMG_WITH_NUMPP is on, cmg::error derives from
// numpp::error so a single catch site spans both layers.
#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include "cmg/backend/config.hpp"

#if CMG_WITH_NUMPP
#include "numpp/core/error.hpp" // provides numpp::error
#endif

namespace cmg {

/// Base class for unrecoverable CyberMeshGenerator errors (invariant violations,
/// misuse). Recoverable conditions use the MeshError/ParseError result types
/// below instead of throwing.
#if CMG_WITH_NUMPP
class error : public numpp::error {
public:
    explicit error(const std::string& what) : numpp::error(what) {}
};
#else
class error : public std::runtime_error {
public:
    explicit error(const std::string& what) : std::runtime_error(what) {}
};
#endif

/// Categories of recoverable meshing failure, mirroring the conditions TetGen
/// reports through its status codes.
enum class MeshErrorCode {
    InvalidInput,        ///< malformed PLC / point set
    SelfIntersection,    ///< intersecting facets without -d handling enabled
    UnsupportedOption,   ///< incompatible or not-yet-implemented option combo
    QualityNotMet,       ///< Steiner budget exhausted before bounds satisfied
    NotImplemented,      ///< capability not yet ported (foundation stubs)
    Internal,            ///< an internal invariant failed recoverably
};

/// A recoverable meshing failure returned from tetrahedralize()/delaunay().
struct MeshError {
    MeshErrorCode code = MeshErrorCode::Internal;
    std::string message;

    MeshError() = default;
    MeshError(MeshErrorCode c, std::string msg)
        : code(c), message(std::move(msg)) {}
};

/// A failure parsing a TetGen-compatible switch string.
struct ParseError {
    std::string message;
    std::size_t position = 0; ///< index into the switch string, when known

    ParseError() = default;
    explicit ParseError(std::string msg, std::size_t pos = 0)
        : message(std::move(msg)), position(pos) {}
};

} // namespace cmg
