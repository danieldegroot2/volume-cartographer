//
// Created by Seth Parker on 3/14/16.
//

#include <iostream>

#include <boost/program_options.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vtkPLYReader.h>

#include "vc/core/io/OBJWriter.hpp"
#include "vc/core/neighborhood/LineGenerator.hpp"
#include "vc/core/types/VolumePkg.hpp"
#include "vc/core/util/DateTime.hpp"
#include "vc/experimental/texturing/ClothModelingUVMapping.hpp"
#include "vc/meshing/ITK2VTK.hpp"
#include "vc/texturing/CompositeTexture.hpp"
#include "vc/texturing/PPMGenerator.hpp"

using namespace volcart;
namespace po = boost::program_options;
namespace vce = volcart::experimental;

void getPins(
    std::string path,
    ITKMesh::Pointer mesh,
    vce::texturing::ClothModelingUVMapping::VertIDList& pinList);

int main(int argc, char* argv[])
{

    // Set up options
    // clang-format off
    po::options_description required("Required arguments");
    required.add_options()
        ("help,h", "Show this message")
        ("volpkg,v", po::value<std::string>()->required(), "VolumePkg path")
        ("input-mesh,i", po::value<std::string>()->required(), "Input mesh path [PLY]")
        ("generate-texture,t",
            po::value<bool>()->default_value(false),
            "Generate a textured mesh from the resulting UV map");

    // Unfurl options
    po::options_description unfurlOptions("Unfurl options");
    unfurlOptions.add_options()
        ("unfurl-iterations", po::value<uint16_t>()->required(),
         "Number of iterations to run the unfurl step")
        ("unfurl-a", po::value<double>()->default_value(10),
         "Acceleration rate of unpinned points (m/s^2) during the unfurl step")
        ("unfurl-pins", po::value<std::string>(), "PLY containing pins used during unfurl step");

    // Collision options
    po::options_description collisionOptions("Collision options");
    collisionOptions.add_options()
        ("collision-iterations", po::value<uint16_t>()->required(),
         "Number of iterations to run the collision step")
        ("collision-a", po::value<double>()->default_value(-10),
         "Acceleration rate of unpinned points (m/s^2) during the collision step");

    // Expansion options
    po::options_description expandOptions("Expansion/Relaxation options");
    expandOptions.add_options()
        ("expand-iterations", po::value<uint16_t>()->required(),
         "Number of iterations to run the expansion step")
        ("expand-a", po::value<double>()->default_value(10),
         "Acceleration rate of unpinned points (m/s^2) during the expansion step")
        ("expand-pins", po::value<std::string>(), "PLY containing pins used during expansion step");

    // clang-format on
    po::options_description all("Usage");
    all.add(required)
        .add(unfurlOptions)
        .add(collisionOptions)
        .add(expandOptions);

    // Parse and handle options
    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, all), opts);

    // Display help
    if (argc == 1 || opts.count("help")) {
        std::cout << all << std::endl;
        std::exit(1);
    }

    // Warn of missing options
    try {
        po::notify(opts);
    } catch (po::error& e) {
        std::cerr << "[error]: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // assign the parsed options
    double unfurlA, collideA, expandA;
    std::string uPins_path, ePins_path;

    VolumePkg vpkg(opts["volpkg"].as<std::string>());
    std::string input_path = opts["input-mesh"].as<std::string>();
    bool genTexture = opts["generate-texture"].as<bool>();

    uint16_t unfurlIt = opts["unfurl-iterations"].as<uint16_t>();
    if (opts.count("unfurl-a"))
        unfurlA = opts["unfurl-a"].as<double>();
    if (opts.count("unfurl-pins"))
        uPins_path = opts["unfurl-pins"].as<std::string>();

    uint16_t collisionIt = opts["collision-iterations"].as<uint16_t>();
    if (opts.count("collision-a"))
        collideA = opts["collision-a"].as<double>();

    uint16_t expansionIt = opts["expand-iterations"].as<uint16_t>();
    if (opts.count("expand-a"))
        expandA = opts["expand-a"].as<double>();
    if (opts.count("expand-pins"))
        ePins_path = opts["expand-pins"].as<std::string>();

    // Get Mesh
    vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();
    reader->SetFileName(input_path.c_str());
    reader->Update();
    ITKMesh::Pointer mesh = ITKMesh::New();
    volcart::meshing::VTK2ITK(reader->GetOutput(), mesh);

    // Get pinned points for unfurling step
    vce::texturing::ClothModelingUVMapping::VertIDList unfurl;
    getPins(uPins_path, mesh, unfurl);

    // Get pinned points for expansion step
    vce::texturing::ClothModelingUVMapping::VertIDList expand;
    getPins(ePins_path, mesh, expand);

    // Run the simulation
    vce::texturing::ClothModelingUVMapping clothUV(
        mesh, unfurlIt, collisionIt, expansionIt, unfurl, expand);
    clothUV.setAcceleration(
        vce::texturing::ClothModelingUVMapping::Stage::Unfurl, 10);
    clothUV.setAcceleration(
        vce::texturing::ClothModelingUVMapping::Stage::Collision, -10);
    clothUV.setAcceleration(
        vce::texturing::ClothModelingUVMapping::Stage::Expansion, 10);
    clothUV.compute();

    // Write the scaled mesh
    ITKMesh::Pointer output = clothUV.getMesh();
    std::string path = volcart::DateTime() + "_uvMap.obj";
    volcart::io::OBJWriter writer(path, output);
    writer.write();

    if (!genTexture) {
        return EXIT_SUCCESS;
    }

    // Convert soft body to itk mesh
    volcart::UVMap uvMap = clothUV.getUVMap();

    auto width = static_cast<size_t>(std::ceil(uvMap.ratio().width));
    auto height = static_cast<size_t>(std::ceil(uvMap.ratio().height));

    volcart::texturing::PPMGenerator ppmGen(height, width);
    ppmGen.setUVMap(uvMap);
    ppmGen.setMesh(mesh);
    ppmGen.compute();

    auto line = volcart::LineGenerator::New();
    line->setSamplingRadius(7);

    volcart::texturing::CompositeTexture result;
    result.setPerPixelMap(ppmGen.getPPM());
    result.setVolume(vpkg.volume());
    result.setGenerator(line);
    result.compute();

    volcart::io::OBJWriter objwriter(
        "textured.obj", mesh, uvMap, result.getTexture().image(0));
    objwriter.write();

    if (result.getTexture().mask().data) {
        cv::imwrite("PerPixelMask.png", result.getTexture().mask());
    }

    if (result.getTexture().ppm().initialized()) {
        PerPixelMap::WritePPM("PerPixelMapping", result.getTexture().ppm());
    }

    return EXIT_SUCCESS;
}

/////////// Get pinned points from file //////////
void getPins(
    std::string path,
    ITKMesh::Pointer mesh,
    vce::texturing::ClothModelingUVMapping::VertIDList& pinList)
{

    // Clear the pin list
    pinList.clear();

    // Load the pin list mesh from file
    vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();
    reader->SetFileName(path.c_str());
    reader->Update();
    ITKMesh::Pointer pins = ITKMesh::New();
    volcart::meshing::VTK2ITK(reader->GetOutput(), pins);

    // Setup points locator
    typename ITKPointsLocator::Pointer pointsLocator = ITKPointsLocator::New();
    pointsLocator->SetPoints(mesh->GetPoints());
    pointsLocator->Initialize();

    // Iterate over all of the pins and find them in the mesh, add their IDs to
    // the pinList
    for (ITKPointIterator pin = pins->GetPoints()->Begin();
         pin != pins->GetPoints()->End(); ++pin) {
        uint64_t pinID =
            pointsLocator->FindClosestPoint(pins->GetPoint(pin->Index()));
        pinList.push_back(pinID);
    }
}
