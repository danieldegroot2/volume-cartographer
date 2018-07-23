#pragma once

#include <boost/program_options.hpp>

#include "vc/core/types/ITKMesh.hpp"
#include "vc/core/types/Texture.hpp"
#include "vc/core/types/UVMap.hpp"

/** Get the flattening/UV options */
boost::program_options::options_description GetUVOpts();

/** Get the generic texture filtering options */
boost::program_options::options_description GetFilteringOpts();

/** Get the Composite Texture options */
boost::program_options::options_description GetCompositeOpts();

/** Get the Integral Texture options */
boost::program_options::options_description GetIntegralOpts();

/** Available neighborhood generators */
enum class Shape { Line = 0, Cuboid };

/** Available texturing algorithms */
enum class Method { Composite = 0, Intersection, Integral };

/** Perform flattening and UV ops */
volcart::UVMap FlattenMesh(
    const volcart::ITKMesh::Pointer& mesh, bool resampled);

/** Perform texturing ops */
volcart::Texture TextureMesh(
    const volcart::ITKMesh::Pointer& mesh, const volcart::UVMap& uvMap);