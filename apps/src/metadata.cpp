// VC Metadata Viewer/Editor
#include <iostream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "core/types/VolumePkg.h"
#include "core/types/VolumePkgVersion.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char* argv[])
{
    // clang-format off
    po::options_description options{"options arguments"};
    options.add_options()
        ("help,h", "Show this message")
        ("print,p", "Print current metadata")
        ("test,t", "Test metadata changes but do not write to file")
        ("write,w", "Write metadata changes to file")
        ("volpkg,v", po::value<fs::path>()->required(), "Path to volumepkg")
        ("configs", po::value<std::vector<std::string>>(),
            "New metadata key/value pairs");
    po::positional_options_description positional;
    positional.add("configs", -1);
    // clang-format on

    po::command_line_parser parser{argc, argv};
    parser.options(options).positional(positional).allow_unregistered();
    auto parsed = parser.run();
    po::variables_map opts;
    po::store(parsed, opts);

    // Print help
    if (argc == 1 || opts.count("help")) {
        std::cout << "Usage: " << argv[0]
                  << " [options] key=value [key=value ...]" << std::endl;
        std::cout << options << std::endl;
        std::exit(1);
    }

    // Verify only one mode specified
    if (opts.count("print") + opts.count("test") + opts.count("write") > 1) {
        std::cerr
            << "Multiple modes specified. Only pick one of [print/test/write]"
            << std::endl;
        std::exit(1);
    }

    auto configs = opts["configs"].as<std::vector<std::string>>();
    if (configs.empty()) {
        std::cout << "No metadata changes to make, exiting" << std::endl;
        std::exit(0);
    }

    // Build volumepkg
    VolumePkg volpkg{opts["volpkg"].as<fs::path>()};

    // Print metadata
    if (opts.count("print")) {
        std::cout << "INITIAL METADATA: " << std::endl;
        volpkg.printJSON();
        std::cout << std::endl;
        return EXIT_SUCCESS;
    }

    // Test or write metadata
    if (opts.count("test") || opts.count("write")) {
        std::map<std::string, std::string> parsedMetadata;

        // Parse the metadata key and its value
        for (auto&& config : configs) {
            auto delimiter = config.find("=");
            if (delimiter == std::string::npos) {
                std::cerr << "\"" << config
                          << "\" does not match the format key=value."
                          << std::endl;
                continue;
            }
            auto key = config.substr(0, delimiter);
            auto value = config.substr(delimiter + 1, config.npos);

            parsedMetadata[key] = value;
        }

        std::cout << std::endl;
        if (parsedMetadata.empty()) {
            std::cout << "No recognized key=value pairs given. Metadata will "
                         "not be changed."
                      << std::endl;
            return EXIT_SUCCESS;
        }

        // Change the version number first since that affects everything else
        auto versionFind = parsedMetadata.find("version");
        if (versionFind != std::end(parsedMetadata)) {
            std::cerr
                << "ERROR: Version upgrading is not available at this time."
                << std::endl;
            /* We only have one volpkg version, so this is disabled for right
            now - SP, 4/30/2015
            int currentVersion = volpkg.getVersion();
            int newVersion = std::stoi(versionFind->second);
            if (currentVersion == newVersion) {
                std::cout << "Volpkg v." << currentVersion << " == v." <<
            versionFind->second << std::endl;
                std::cout << "Volpkg will not be converted." << std::endl;
                parsedMetadata.erase(versionFind);
            } else {
                std::cout << "Attempting to convert volpkg from v." <<
            currentVersion << " -> v." <<
                versionFind->second << std::endl;
                if (volpkg.setMetadata(versionFind->first,
            versionFind->second) == EXIT_SUCCESS) {
                    // To-Do: Upgrade the json object to match the new dict
                    std::cout << "Version set successfully." << std::endl;
                    parsedMetadata.erase(versionFind);
                }
                else {
                    std::cerr << "ERROR: Volpkg could not be converted." <<
            std::endl;
                    return EXIT_FAILURE;
                }
            } */
            parsedMetadata.erase(versionFind->first);
            std::cout << std::endl;
        }

        // Find metadata type mapping for given version.
        auto types_it = volcart::VersionLibrary.find(volpkg.getVersion());
        if (types_it == std::end(volcart::VersionLibrary)) {
            std::cerr << "Could not find type mapping for version "
                      << volpkg.getVersion() << std::endl;
            std::exit(1);
        }
        auto typeMap = types_it->second;

        // Parse metadata arguments.
        for (auto&& pair : parsedMetadata) {
            std::cout << "Attempting to set key \"" << pair.first
                      << "\" to value \"" << pair.second << "\"" << std::endl;
            switch (typeMap[pair.first]) {
                case volcart::Type::STRING:
                    volpkg.setMetadata(pair.first, pair.second);
                    break;
                case volcart::Type::INT:
                    volpkg.setMetadata(pair.first, std::stoi(pair.second));
                    break;
                case volcart::Type::DOUBLE:
                    volpkg.setMetadata(pair.first, std::stod(pair.second));
                    break;
            }
            std::cout << std::endl;
        }

        // Only print, don't save.
        if (opts.count("test")) {
            std::cout << "FINAL METADATA: " << std::endl;
            volpkg.printJSON();
            std::cout << std::endl;
            return EXIT_SUCCESS;
        }

        // Actually save.
        if (opts.count("write")) {
            volpkg.readOnly(false);
            // save the new json file to test.json
            std::cout << "Writing metadata to file..." << std::endl;
            volpkg.saveMetadata();
            std::cout << "Metadata written successfully." << std::endl
                      << std::endl;
            return EXIT_SUCCESS;
        }
    }
}
