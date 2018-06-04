// invertcloud.cpp
// Seth Parker, Aug 2015

#include <fstream>
#include <iostream>

#include <boost/filesystem/path.hpp>
#include <opencv2/core.hpp>

#include "vc/core/io/PointSetIO.hpp"
#include "vc/core/types/VolumePkg.hpp"

namespace fs = boost::filesystem;
namespace vc = volcart;

int main(int argc, char** argv)
{
    if (argc < 5) {
        std::cout << "Usage: vc_invertCloud [volpkg] [volume-id] [input].vcps "
                     "[output].vcps"
                  << std::endl;
        std::exit(-1);
    }

    vc::VolumePkg vpkg{argv[1]};
    auto volumeID = argv[2];
    fs::path inputPath = argv[3];
    fs::path outputPath = argv[4];

    if (vpkg.version() != 5) {
        std::cerr << "ERROR: Volume package is version " << vpkg.version()
                  << " but this program requires a version >= 5" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << inputPath << std::endl;

    // Load the cloud
    auto input = vc::PointSetIO<cv::Vec3d>::ReadOrderedPointSet(inputPath);

    // Flip the rows
    vc::OrderedPointSet<cv::Vec3d> output(input.width());
    for (size_t r = 0; r < input.height(); r++) {
        size_t index = input.height() - 1 - r;
        output.pushRow(input.getRow(index));
    }

    // Flip the z-indices of the pts
    auto maxZ = vpkg.volume(volumeID)->numSlices() - 1;
    for (auto& pt : output) {
        if (pt[2] != -1) {
            pt[2] = maxZ - pt[2];
        }
    }

    vc::PointSetIO<cv::Vec3d>::WriteOrderedPointSet(outputPath, output);

    return 0;
}  // end main
