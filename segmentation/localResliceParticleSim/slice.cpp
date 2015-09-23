#include <cmath>
#include "slice.h"
#include "NormalizedIntensityMap.h"

// color defines
// mostly the same as what's in structureTensorParticleSim.cpp
// both should be moved somewhere more global
#define WHITE 255
#define BLACK 0

#define BGR_BLUE cv::Scalar(255, 0, 0)
#define BGR_GREEN cv::Scalar(0, 255, 0)
#define BGR_RED cv::Scalar(0, 0, 255)

#define BGR_CYAN cv::Scalar(255, 255, 0)
#define BGR_YELLOW cv::Scalar(0, 255, 255)
#define BGR_MAGENTA cv::Scalar(255, 0, 255)

#define BGR_WHITE cv::Scalar(255, 255, 255)

#define DEBUG_ARROW_SCALAR 20

using namespace volcart::segmentation;

// basic constructor
Slice::Slice(cv::Mat slice, cv::Vec3f origin, cv::Vec3f center, cv::Vec3f x_direction, cv::Vec3f y_direction)
    : _slice(slice), _origin(origin), _center(center), _xvec(x_direction), _yvec(y_direction) {
}

cv::Vec3f Slice::findNextPosition() {
    constexpr auto lookaheadDepth = 5;
    auto center = cv::Point(_slice.cols / 2, _slice.rows / 2);

    const auto map = NormalizedIntensityMap(_slice.row(center.y + lookaheadDepth));
    auto maxima = map.findNMaxima(4);
    for (auto p : maxima) {
        std::cout << "idx = " << p.first << ", intensity = " << p.second << std::endl;
    }

    // Sort maxima by whichever is closest to current index of center (using standard euclidean 1D distance)
    using Pair = std::pair<uint32_t, double>;
    std::sort(maxima.begin(), maxima.end(), [center](Pair lhs, Pair rhs) {
        auto x = center.x;
        auto ldist2 = int32_t(std::sqrt(double((lhs.first - x)) * (lhs.first - x)));
        auto rdist2 = int32_t(std::sqrt(double((rhs.first - x)) * (rhs.first - x)));
        return ldist2 < rdist2;
    });

    // Find next point in slice space
    auto nextPoint = cv::Point(maxima.at(0).first, center.y + lookaheadDepth);
    std::cout << _center << " -> " << sliceCoordToVoxelCoord(nextPoint) << std::endl;
    return sliceCoordToVoxelCoord(nextPoint);
}

void Slice::drawSliceAndCenter() {
    const auto map = NormalizedIntensityMap(_slice.row(_slice.rows / 2 + 5));
    map.draw(400, 400);
    auto debug = _slice.clone();
    debug /= 255.0;
    debug.convertTo(debug, CV_8UC3);
    cvtColor(debug, debug, CV_GRAY2BGR);
    cv::Point imcenter(debug.cols / 2, debug.rows / 2);

    // Draw circle at pixel representing center
    circle(debug, imcenter, 0, BGR_MAGENTA, -1);
    namedWindow("DEBUG SLICE", cv::WINDOW_NORMAL);
    imshow("DEBUG SLICE", debug);
}

void Slice::debugDraw(int debugDrawOptions) {
    cv::Mat debug = _slice.clone();
    debug *= 1. / 255;
    debug.convertTo(debug, CV_8UC3);
    cvtColor(debug, debug, CV_GRAY2BGR);


    // project xyz coordinate reference onto viewing plane with the formula
    //
    // [_xvec] [x]
    // [_yvec] [y]
    //                [z]
    //
    // which becomes componentwise pairs (x_1, x_2) (y_1, y_2) (z_1, z_2) when we only care about i, j, and k
    if (debugDrawOptions & DEBUG_DRAW_XYZ) {
        cv::Point x_arrow_offset(DEBUG_ARROW_SCALAR * _xvec(VC_INDEX_X),
                                 DEBUG_ARROW_SCALAR * _yvec(VC_INDEX_X));
        cv::Point y_arrow_offset(DEBUG_ARROW_SCALAR * _xvec(VC_INDEX_Y),
                                 DEBUG_ARROW_SCALAR * _yvec(VC_INDEX_Y));
        cv::Point z_arrow_offset(DEBUG_ARROW_SCALAR * _xvec(VC_INDEX_Z),
                                 DEBUG_ARROW_SCALAR * _yvec(VC_INDEX_Z));

        cv::Point coordinate_origin(DEBUG_ARROW_SCALAR, DEBUG_ARROW_SCALAR);
        rectangle(debug, cv::Point(0, 0), 2 * cv::Point(DEBUG_ARROW_SCALAR, DEBUG_ARROW_SCALAR), BGR_WHITE);
        arrowedLine(debug, coordinate_origin, coordinate_origin + x_arrow_offset, BGR_RED);
        arrowedLine(debug, coordinate_origin, coordinate_origin + y_arrow_offset, BGR_GREEN);
        arrowedLine(debug, coordinate_origin, coordinate_origin + z_arrow_offset, BGR_BLUE);
    }

    if (debugDrawOptions & DEBUG_DRAW_CORNER_COORDINATES) {
        std::stringstream trc;
        cv::Vec3f top_right_corner = _origin + debug.cols * _xvec;
        trc << "(" << (int) top_right_corner(VC_INDEX_X)
        << "," << (int) top_right_corner(VC_INDEX_Y)
        << "," << (int) top_right_corner(VC_INDEX_Z) << ")";

        std::stringstream blc;
        cv::Vec3f bottom_left_corner = _origin + debug.rows * _yvec;
        blc << "(" << (int) bottom_left_corner(VC_INDEX_X)
        << "," << (int) bottom_left_corner(VC_INDEX_Y)
        << "," << (int) bottom_left_corner(VC_INDEX_Z) << ")";

        putText(debug, trc.str(), cv::Point(debug.cols - 125, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, BGR_WHITE);
        putText(debug, blc.str(), cv::Point(5, debug.rows - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, BGR_WHITE);
    }

    if (debugDrawOptions & DEBUG_DRAW_CENTER) {
        cv::Point imcenter(debug.cols / 2, debug.rows / 2);
        arrowedLine(debug, imcenter, imcenter + cv::Point(debug.cols / 2 - 1, 0), BGR_YELLOW);
        circle(debug, imcenter, 2, BGR_MAGENTA, -1);
    }

    namedWindow("DEBUG DRAW", cv::WINDOW_AUTOSIZE);
    imshow("DEBUG DRAW", debug);
}

cv::Mat Slice::mat() {
    return _slice.clone();
}

cv::Vec3f Slice::sliceCoordToVoxelCoord(cv::Point point) {
    return _origin + (point.x * _xvec + point.y * _yvec);
}
