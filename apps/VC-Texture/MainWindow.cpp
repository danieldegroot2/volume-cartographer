//----------------------------------------------------------------------------------------------------------------------------------------
// MainWindow.cpp file for MainWindow Class , (Implements QMainWindow)
// Purpose: Create a Main Window for the GUI
// Developer: Michael Royal - mgro224@g.uky.edu
// October 12, 2015 - Spring Semester 2016
// Last Updated 09/26/2016 by: Michael Royal

// Copyright 2015 (Brent Seales: Volume Cartography Research)
// University of Kentucky VisCenter
//----------------------------------------------------------------------------------------------------------------------------------------

#include <regex>

#include <boost/algorithm/string/case_conv.hpp>
#include <opencv2/imgcodecs.hpp>

#include "MainWindow.hpp"
#include "vc/core/io/OBJWriter.hpp"

// Volpkg version required by this app
static constexpr int VOLPKG_SUPPORTED_VERSION = 5;

namespace fs = boost::filesystem;

MainWindow::MainWindow(GlobalValues* globals)
{
    _globals = globals;  // Enables access to Global Values Object

    setWindowTitle("VC Texture");  // Set Window Title

    // NOTE: Minimum Height and Width -------------------------
    // will be different on other display screens,
    // if Resolution is too small may cause distortion
    // of Buttons Visually when Program first Initiates
    //----------------------------------------------------------

    // MIN DIMENSIONS
    window()->setMinimumHeight(_globals->getHeight() / 2);
    window()->setMinimumWidth(_globals->getWidth() / 2);
    // MAX DIMENSIONS
    window()->setMaximumHeight(_globals->getHeight());
    window()->setMaximumWidth(_globals->getWidth());
    //---------------------------------------------------------

    // Create new TextureViewer Object (Left Side of GUI Display)
    TextureViewer* texture_Image = new TextureViewer(globals);
    // Create new SegmentationsViewer Object (Right Side of GUI Display)
    SegmentationsViewer* segmentations =
        new SegmentationsViewer(globals, texture_Image);
    _segmentations_Viewer = segmentations;

    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->addLayout(
        texture_Image->getLayout());  // THIS LAYOUT HOLDS THE WIDGETS FOR THE
                                      // OBJECT "TextureViewer" which Enables
                                      // the user to view images, zoom in, zoom
                                      // out, and reset the image.
    mainLayout->addLayout(
        segmentations->getLayout());  // THIS LAYOUT HOLDS THE WIDGETS FOR THE
                                      // OBJECT "SegmentationsViewer" which
                                      // Enables the user to load segmentations,
                                      // and generate new texture images.

    QWidget* w = new QWidget();  // Creates the Primary Widget to display GUI
                                 // Functionality
    w->setLayout(
        mainLayout);  // w(the main window) gets assigned the mainLayout

    // Display Window
    //------------------------------
    setCentralWidget(
        w);  // w is a wrapper widget for all of the widgets in the main window.

    create_Actions();  // Creates the Actions for the Menu Bar & Sub-Menus
    create_Menus();    // Creates the Menus and adds them to the Menu Bar
}

// Gets the Folder Path of the Volume Package location, and initiates a Volume
// Package.
void MainWindow::getFilePath()
{
    // If processing...
    if (_globals->getStatus() == ThreadStatus::Active) {
        QMessageBox::information(
            this, tr("Error Message"), "Please Wait While Texture Generates.");
        return;
    }

    // If there's something to save...
    if (_globals->getStatus() == ThreadStatus::Successful) {

        // Ask User to Save unsaved Data
        QMessageBox msgBox;
        msgBox.setText(
            "A new texture image was generated, do you want to save it?");
        msgBox.setStandardButtons(
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        int option = msgBox.exec();

        switch (option) {

            // Save was clicked
            case QMessageBox::Save:
                saveTexture();
                break;

            // Discard was clicked
            case QMessageBox::Discard:
                _globals->setThreadStatus(ThreadStatus::Inactive);
                break;

            // Cancel was clicked
            case QMessageBox::Cancel:
                return;

            default:
                // should never be reached
                break;
        }
    }

    _globals->setThreadStatus(ThreadStatus::Inactive);
    clearGUI();
    QFileDialog* dialogBox = new QFileDialog();
    fs::path filename = dialogBox->getExistingDirectory().toStdString();

    // If the user selected a Folder Path
    if (!filename.empty()) {

        // Checks the Folder Path for .volpkg extension
        std::regex volpkg(".*volpkg$");
        if (std::regex_match(filename.extension().string(), volpkg)) {
            try {
                // Sets Folder Path in Globals
                _globals->setPath(filename.c_str());
                _globals->createVolumePackage();

                // Check for Volume Package Version Number
                auto version = _globals->getVolPkg()->version();
                if (version != VOLPKG_SUPPORTED_VERSION) {
                    std::cerr << "VC::Error: Volume package is version "
                              << version
                              << " but this program requires a version "
                              << VOLPKG_SUPPORTED_VERSION << "." << std::endl;

                    QMessageBox::warning(
                        this, tr("ERROR"),
                        "Volume package is version " +
                            QString::number(version) +
                            " but this program requires version " +
                            QString::number(VOLPKG_SUPPORTED_VERSION) + ".");

                    // Reset Values
                    _globals->clearVolumePackage();
                    _globals->setPath("");
                    return;
                }

                // Gets Segmentations and assigns them to "segmentations" in
                // Globals
                _globals->getMySegmentations();
                // Initialize Segmentations in segmentations_Viewer
                _segmentations_Viewer->setSegmentations();
                // Sets the name of the Volume Package to display on the GUI
                auto name = _globals->getVolPkg()->name();
                _segmentations_Viewer->setVol_Package_Name(name.c_str());
            } catch (...) {
                QMessageBox::warning(
                    this, tr("Error Message"), "Error Opening File.");
            };
        } else {
            QMessageBox::warning(this, tr("Error Message"), "Invalid File.");
        }
    }
}

// Overrides the Current Texture Image in the Segmentation's Folder with the
// newly Generated Texture Image.
void MainWindow::saveTexture()
{
    // If processing...
    if (_globals->getStatus() == ThreadStatus::Active) {
        QMessageBox::information(
            this, tr("Error Message"), "Please Wait While Texture Generates.");
        return;
    }

    // If A Volume Package is Loaded and there are Segmentations (continue)
    if (_globals->isVPKG_Intantiated() &&
        _globals->getVolPkg()->hasSegmentations()) {
        // Checks to see if there are images
        if (_globals->getRendering().getTexture().hasImages()) {
            try {
                auto path =
                    _globals->getActiveSegmentation()->path() / "textured.obj";
                volcart::io::OBJWriter writer;
                writer.setPath(path.string());
                writer.setMesh(_globals->getRendering().getMesh());
                writer.setUVMap(_globals->getRendering().getTexture().uvMap());
                writer.setTexture(
                    _globals->getRendering().getTexture().image(0));
                writer.write();
                _globals->setThreadStatus(ThreadStatus::Inactive);
                QMessageBox::information(
                    this, tr("Error Message"), "Saved Successfully.");

            } catch (...) {
                QMessageBox::warning(
                    _globals->getWindow(), "Error",
                    "Failed to Save Texture Image Properly!");
            }

        } else
            QMessageBox::information(
                this, tr("Error Message"),
                "Please Generate a New Texture Image.");

    } else
        QMessageBox::warning(
            this, tr("Error Message"), "There is no Texture Image to Save!");
}

// Exports the Image as .tif, .tiff, .png, .jpg, and .jpeg
void MainWindow::exportTexture()
{
    // If processing...
    if (_globals->getStatus() == ThreadStatus::Active) {
        QMessageBox::information(
            this, tr("Error Message"), "Please Wait While Texture Generates.");
        return;
    }

    // Return if no volume package is loaded or if volpkg doesn't have
    // segmentations
    if (!_globals->isVPKG_Intantiated() ||
        !_globals->getVolPkg()->hasSegmentations()) {
        QMessageBox::warning(
            this, "Error",
            "Volume package not loaded/no segmentations in volume.");
        std::cerr << "vc::export::error: no volpkg loaded" << std::endl;
        return;
    }

    cv::Mat output;

    // Export the generated texture first, otherwise the one already saved to
    // disk
    if (_globals->getRendering().getTexture().hasImages()) {
        output = _globals->getRendering().getTexture().image(0);
    } else {
        auto path = _globals->getActiveSegmentation()->path() / "textured.png";
        output = cv::imread(path.string(), -1);
    }

    // Return if no image to export
    if (!output.data) {
        QMessageBox::warning(
            this, "Error",
            "No image to export. Please load a different segmentation or "
            "generate a new texture.");
        std::cerr << "vc::export::error: no image data to export" << std::endl;
        return;
    }

    // Get the output path
    fs::path outputPath;
    outputPath = QFileDialog::getSaveFileName(
                     this, tr("Export Texture Image"), "",
                     tr("Images (*.png *jpg *jpeg *tif *tiff)"))
                     .toStdString();

    // If no path provided/dialog cancelled
    if (outputPath.empty()) {
        std::cerr << "vc::export::status: dialog cancelled." << std::endl;
        return;
    }

    ///// Deal with edge cases /////
    // Default to png if no extension provided
    if (outputPath.extension().empty())
        outputPath = outputPath.string() + ".png";

    // For convenience
    std::string extension(
        boost::to_upper_copy<std::string>(outputPath.extension().string()));

    // Check for approved format
    std::vector<std::string> approvedExtensions{".PNG", ".JPG", ".JPEG", ".TIF",
                                                ".TIFF"};
    auto it = std::find(
        approvedExtensions.begin(), approvedExtensions.end(), extension);
    if (it == approvedExtensions.end()) {
        QMessageBox::warning(
            this, "Error",
            "Unknown file format for export. Please use .png, .jpg, or .tif.");
        std::cerr << "vc::export::error: unknown output format: " << extension
                  << std::endl;
        return;
    }

    // Convert to 8U if JPG
    if (extension == ".JPG" || extension == ".JPEG") {
        output = output.clone();
        output.convertTo(output, CV_8U, 255.0 / 65535.0);
        std::cerr << "vc::export::status: downsampled to 8U" << std::endl;
    }

    // Write the image
    try {
        cv::imwrite(outputPath.string(), output);
    } catch (std::runtime_error& ex) {
        QMessageBox::warning(this, "Error", "Error writing file.");
        std::cerr << "vc::export::error: exception writing image: " << ex.what()
                  << std::endl;
        return;
    }

    std::cerr << "vc::export::status: export successful" << std::endl;
}

void MainWindow::create_Actions()
{
    actionGetFilePath = new QAction("Open Volume...", this);
    connect(actionGetFilePath, SIGNAL(triggered()), this, SLOT(getFilePath()));

    actionSave = new QAction("Save Texture", this);
    connect(actionSave, SIGNAL(triggered()), this, SLOT(saveTexture()));

    actionExport = new QAction("Export Texture", this);
    connect(actionExport, SIGNAL(triggered()), this, SLOT(exportTexture()));
}

void MainWindow::create_Menus()
{
    fileMenu = new QMenu(tr("&File"), this);
    _globals->setFileMenu(fileMenu);

    fileMenu->addAction(actionGetFilePath);
    fileMenu->addAction(actionSave);
    fileMenu->addAction(actionExport);

    menuBar()->addMenu(fileMenu);
}

// User cannot exit program while texture is still running.
void MainWindow::closeEvent(QCloseEvent* closing)
{
    if (_globals->getStatus() == ThreadStatus::Active) {
        QMessageBox::warning(
            this, "Error",
            "This application cannot be closed while a texture is being "
            "generated. Please wait until the texturing process is complete "
            "and try again.");
        closing->ignore();
        return;
    } else if (_globals->getStatus() == ThreadStatus::Successful) {

        // Ask User to Save unsaved Data
        QMessageBox msgBox;
        msgBox.setText(
            "A new texture image was generated, do you want to save it before "
            "quitting?");
        msgBox.setStandardButtons(
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        int option = msgBox.exec();

        switch (option) {
            // Save was clicked
            case QMessageBox::Save:
                saveTexture();
                break;

            // Discard was clicked
            case QMessageBox::Discard:
                break;

            // Cancel was clicked
            case QMessageBox::Cancel:
                closing->ignore();
                return;

            default:
                break;
        }
    }

    // Exit
    closing->accept();
}

void MainWindow::clearGUI()
{
    _globals->clearGUI();
    _segmentations_Viewer->clearGUI();
    update();
}
