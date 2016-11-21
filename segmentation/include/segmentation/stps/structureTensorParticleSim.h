#pragma once

#include <iostream>

#include <opencv2/opencv.hpp>

#include "core/types/VolumePkg.h"

#include "segmentation/stps/chain.h"
#include "segmentation/stps/particle.h"

namespace volcart
{
namespace segmentation
{
volcart::OrderedPointSet<volcart::Point3d> structureTensorParticleSim(
    std::vector<volcart::Point3d> segPath,
    const VolumePkg& volpkg,
    double gravity_scale = 0.5,
    int threshold = 1,
    int endOffset = -1);
}  // namespace segmentation
}  // namespace volcart
