#pragma once

#include <opencv2/core.hpp>

#include "vc/core/types/Texture.hpp"
#include "vc/core/types/UVMap.hpp"
#include "vc/core/types/VolumePkg.hpp"
#include "vc/core/vc_defines.hpp"
#include "texturing/TexturingUtils.hpp"

namespace volcart
{
namespace texturing
{

class CompositeTextureV2
{
public:
    CompositeTextureV2(
        ITKMesh::Pointer inputMesh,
        VolumePkg& volpkg,
        UVMap uvMap,
        double radius,
        size_t width,
        size_t height,
        CompositeOption method = CompositeOption::NonMaximumSuppression,
        DirectionOption direction = DirectionOption::Bidirectional);

    const volcart::Texture& texture() const { return texture_; }
    volcart::Texture& texture() { return texture_; }

private:
    int process_();

    // Variables
    ITKMesh::Pointer input_;
    VolumePkg& volpkg_;
    size_t width_;
    size_t height_;
    double radius_;
    CompositeOption method_;
    DirectionOption direction_;

    UVMap uvMap_;
    Texture texture_;
};
}
}
