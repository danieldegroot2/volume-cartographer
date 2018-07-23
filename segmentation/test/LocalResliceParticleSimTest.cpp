#define BOOST_TEST_MODULE LocalResliceSegmentation

#include <cmath>

#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include "vc/core/types/VolumePkg.hpp"
#include "vc/segmentation/LocalResliceParticleSim.hpp"

using namespace volcart::segmentation;
namespace tt = boost::test_tools;

struct PointXYZ {
    double x, y, z;

    explicit PointXYZ(const cv::Vec3d& p) : x(p[0]), y(p[1]), z(p[2]) {}
};

std::ostream& operator<<(std::ostream& s, PointXYZ p);

inline double NormL2(const PointXYZ p1, const PointXYZ p2)
{
    return std::sqrt(
        (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) +
        (p1.z - p2.z) * (p1.z - p2.z));
}

// Main fixture containing the LocalResliceSegmentation object
struct LocalResliceSegmentationFix {
    LocalResliceSegmentationFix() = default;

    volcart::VolumePkg pkg_{"Testing.volpkg"};
    LocalResliceSegmentation segmenter_;
};

// Test for default segmentation
BOOST_FIXTURE_TEST_CASE(DefaultSegmentationTest, LocalResliceSegmentationFix)
{
    // Get the cloud to compare against
    auto groundTruthSeg = pkg_.segmentation("local-reslice-particle-sim");
    const auto groundTruthCloud = groundTruthSeg->getPointSet();

    // Get the starting cloud to segment
    auto pathSeed = pkg_.segmentation("starting-path")->getPointSet().getRow(0);

    // Run segmentation
    // XXX These params are manually input now, later they will be dynamically
    // read from the parameters.json file in each segmentation directory
    int endIndex = 182;
    int numIters = 15;
    int stepNumLayers = 1;
    double alpha = 1.0 / 3.0;
    double k1 = 0.5;
    double k2 = 0.5;
    double beta = 1.0 / 3.0;
    double delta = 1.0 / 3.0;
    int peakDistanceWeight = 50;
    bool shouldIncludeMiddle = false;
    bool dumpVis = false;
    bool visualize = false;

    segmenter_.setChain(pathSeed);
    segmenter_.setVolume(pkg_.volume());
    segmenter_.setTargetZIndex(endIndex);
    segmenter_.setStepSize(stepNumLayers);
    segmenter_.setOptimizationIterations(numIters);
    segmenter_.setAlpha(alpha);
    segmenter_.setK1(k1);
    segmenter_.setK2(k2);
    segmenter_.setBeta(beta);
    segmenter_.setDelta(delta);
    segmenter_.setMaterialThickness(pkg_.materialThickness());
    segmenter_.setDistanceWeightFactor(peakDistanceWeight);
    segmenter_.setConsiderPrevious(shouldIncludeMiddle);
    segmenter_.setVisualize(visualize);
    segmenter_.setDumpVis(dumpVis);
    auto resultCloud = segmenter_.compute();

    // Save the results
    auto testCloudSeg = pkg_.newSegmentation("lrps-test-results");
    testCloudSeg->setPointSet(resultCloud);

    // First compare cloud sizes
    BOOST_REQUIRE_EQUAL(groundTruthCloud.size(), resultCloud.size());
    BOOST_REQUIRE_EQUAL(groundTruthCloud.width(), resultCloud.width());
    BOOST_REQUIRE_EQUAL(groundTruthCloud.height(), resultCloud.height());

    // Compare clouds, make sure each point is within a certain tolerance.
    // Currently set in this file, may be set outside later on
    constexpr double voxelDiffTol = 10;  // %
    size_t diffCount = 0;
    for (size_t i = 0; i < groundTruthCloud.size(); ++i) {
        PointXYZ trueV(groundTruthCloud[i]);
        PointXYZ testV(resultCloud[i]);
        auto xdiff = std::abs(trueV.x - testV.x);
        auto ydiff = std::abs(trueV.y - testV.y);
        auto zdiff = std::abs(trueV.z - testV.z);
        auto normDiff = NormL2(trueV, testV);

        if (xdiff > voxelDiffTol || ydiff > voxelDiffTol ||
            zdiff > voxelDiffTol || normDiff > voxelDiffTol) {
            diffCount++;
        }

        BOOST_WARN_SMALL(normDiff, voxelDiffTol);
        BOOST_WARN_SMALL(xdiff, voxelDiffTol);
        BOOST_WARN_SMALL(ydiff, voxelDiffTol);
        BOOST_WARN_SMALL(zdiff, voxelDiffTol);
    }

    // Check that the clouds never vary in point differences by 10%
    auto maxAllowedDiffCount =
        size_t(std::round(0.1 * groundTruthCloud.size()));
    std::cout << "# different points: " << diffCount
              << " (max allowed: " << maxAllowedDiffCount << ")" << std::endl;
    BOOST_CHECK(diffCount < maxAllowedDiffCount);
}

std::ostream& operator<<(std::ostream& s, PointXYZ p)
{
    return s << "[" << p.x << ", " << p.y << ", " << p.z << "]";
}
