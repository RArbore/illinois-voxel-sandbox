#include <iostream>
#include <fstream>

#include <nlohmann_json.hpp>
using json = nlohmann::json;

#include <graphics/GraphicsContext.h>

int main(int argc, char *argv[]) {
    ASSERT(argv[1], "Must provide a scene to render.");
    std::string scene_path(argv[1]);

    std::ifstream scene_description_file(scene_path);
    json scene_description = json::parse(scene_description_file);

    std::cout << scene_description.dump() << std::endl;
}