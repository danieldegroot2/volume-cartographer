#include <iostream>

#include "vc/core/types/UVMap.hpp"

int main()
{

    volcart::UVMap uvMap;

    // Initialize iterators
    std::vector<cv::Vec2d> storage;
    for (double u = 0; u <= 1; u += 0.25) {
        for (double v = 0; v <= 1; v += 0.25) {
            cv::Vec2d uv(u, v);
            storage.push_back(uv);
        }
    }

    // Insert mappings relative to the top-left (default origin)
    int pointID = 0;
    for (auto it = storage.begin(); it != storage.end(); ++it) {
        std::cout << "Point: " << pointID << " | " << *it << std::endl;
        uvMap.set(pointID, *it);
        ++pointID;
    }

    std::cout << std::endl;

    // Retrieve mappings relative to the bottom-left
    uvMap.setOrigin(volcart::UVMap::Origin::BottomLeft);
    pointID = 0;
    for (auto it = storage.begin(); it != storage.end(); ++it) {
        std::cout << "Point: " << pointID << " | " << uvMap.get(pointID)
                  << std::endl;
        ++pointID;
    }

    std::cout << std::endl;

    if (uvMap.get(uvMap.size()) == volcart::NULL_MAPPING)
        std::cout << "UV Mapping not found for p_id: " << uvMap.size()
                  << std::endl;

    return EXIT_SUCCESS;
}
