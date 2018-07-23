// MyThread.cpp file for MyThread Class , (Implements QThread)
// Purpose: Run the Texture Functionality behind the scenes so that the GUI
// operates without interference
// Developer: Michael Royal - mgro224@g.uky.edu
// October 12, 2015 - Spring Semester 2016
// Last Updated 10/24/2016 by: Michael Royal

// Copyright 2015 (Brent Seales: Volume Cartography Research)
// University of Kentucky VisCenter

#include <cmath>

#include "MyThread.hpp"

#include "vc/core/io/OBJWriter.hpp"
#include "vc/core/io/PLYReader.hpp"
#include "vc/core/neighborhood/LineGenerator.hpp"
#include "vc/core/util/MeshMath.hpp"
#include "vc/meshing/ACVD.hpp"
#include "vc/meshing/ITK2VTK.hpp"
#include "vc/meshing/OrderedPointSetMesher.hpp"
#include "vc/texturing/AngleBasedFlattening.hpp"
#include "vc/texturing/CompositeTexture.hpp"
#include "vc/texturing/IntegralTexture.hpp"
#include "vc/texturing/IntersectionTexture.hpp"
#include "vc/texturing/PPMGenerator.hpp"

namespace vc = volcart;
namespace vct = volcart::texturing;

MyThread::MyThread(GlobalValues* globals)
{
    _globals = globals;
    _globals->setThreadStatus(ThreadStatus::Active);  // Status Running/Active
    this->start();
}

void MyThread::run()
{
    try {
        ///// Load and resample the segmentation /////
        if (!_globals->getActiveSegmentation()->hasPointSet()) {
            std::cerr << "VC::message: Empty pointset" << std::endl;
            _globals->setThreadStatus(ThreadStatus::CloudError);
            return;
        }

        // Load the cloud
        auto cloud = _globals->getActiveSegmentation()->getPointSet();

        // Only do it if we have more than one iteration of segmentation
        if (cloud.height() <= 1) {
            std::cerr << "VC::message: Cloud height <= 1. Nothing to mesh."
                      << std::endl;
            _globals->setThreadStatus(ThreadStatus::CloudError);
            return;
        }

        // Mesh the point cloud
        volcart::meshing::OrderedPointSetMesher mesher;
        mesher.setPointSet(cloud);

        // declare pointer to new Mesh object
        auto mesh = mesher.compute();

        // Calculate sampling density
        auto voxelsize = _globals->volPkg()->volume()->voxelSize();
        auto sa = vc::meshmath::SurfaceArea(mesh) * std::pow(voxelsize, 2) *
                  (0.001 * 0.001);
        double densityFactor = 50;
        auto numVerts = static_cast<uint16_t>(std::round(densityFactor * sa));
        numVerts = (numVerts < CLEANER_MIN_REQ_POINTS) ? CLEANER_MIN_REQ_POINTS
                                                       : numVerts;

        // Convert to polydata
        auto vtkMesh = vtkSmartPointer<vtkPolyData>::New();
        volcart::meshing::ITK2VTK(mesh, vtkMesh);

        // Decimate using ACVD
        std::cout << "Resampling mesh..." << std::endl;
        auto acvdMesh = vtkSmartPointer<vtkPolyData>::New();
        volcart::meshing::ACVD(vtkMesh, acvdMesh, numVerts);

        // Merge Duplicates
        // Note: This merging has to be the last in the process chain for some
        // really weird reason. - SP
        auto Cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
        Cleaner->SetInputData(acvdMesh);
        Cleaner->Update();

        auto itkACVD = volcart::ITKMesh::New();
        volcart::meshing::VTK2ITK(Cleaner->GetOutput(), itkACVD);

        // ABF flattening
        std::cout << "Computing parameterization..." << std::endl;
        volcart::texturing::AngleBasedFlattening abf(itkACVD);
        // abf.setABFMaxIterations(5);
        abf.compute();

        // Get uv map
        volcart::UVMap uvMap = abf.getUVMap();
        auto width = static_cast<size_t>(std::ceil(uvMap.ratio().width));
        auto height = static_cast<size_t>(
            std::ceil(static_cast<double>(width) / uvMap.ratio().aspect));

        volcart::texturing::PPMGenerator ppmGen(height, width);
        ppmGen.setMesh(itkACVD);
        ppmGen.setUVMap(uvMap);
        auto ppm = ppmGen.compute();

        // Rendering parameters
        double radius = _globals->getRadius();
        auto method = _globals->getTextureMethod();
        auto direction =
            static_cast<vc::Direction>(_globals->getSampleDirection());
        auto volume = _globals->volPkg()->volume();

        auto generator = vc::LineGenerator::New();
        generator->setSamplingRadius(radius);
        generator->setSamplingDirection(direction);

        // Generate texture image
        vc::Texture texture;
        if (method == GlobalValues::Method::Intersection) {
            vct::IntersectionTexture textureGen;
            textureGen.setVolume(volume);
            textureGen.setPerPixelMap(ppm);
            texture = textureGen.compute();
        }

        else if (method == GlobalValues::Method::Integral) {
            vc::texturing::IntegralTexture textureGen;
            textureGen.setPerPixelMap(ppm);
            textureGen.setVolume(volume);
            textureGen.setGenerator(generator);
            texture = textureGen.compute();
        }

        else if (method == GlobalValues::Method::Composite) {
            vct::CompositeTexture textureGen;
            textureGen.setPerPixelMap(ppm);
            textureGen.setVolume(volume);
            textureGen.setFilter(_globals->getFilter());
            textureGen.setGenerator(generator);
            texture = textureGen.compute();
        }

        Rendering rendering;
        rendering.setTexture(texture);
        rendering.setMesh(itkACVD);

        _globals->setRendering(rendering);
        _globals->setThreadStatus(ThreadStatus::Successful);

    } catch (...) {
        _globals->setThreadStatus(ThreadStatus::Failed);
    };
}
