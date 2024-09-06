// Copyright (c) 2023 UltiMaker
// curaengine_plugin_generate_infill is released under the terms of the AGPLv3 or higher

#ifndef INFILL_INFILL_GENERATOR_H
#define INFILL_INFILL_GENERATOR_H

#include "infill/geometry.h"
#include "infill/point_container.h"
#include "infill/tile.h"
#include <spdlog/spdlog.h>

#include <polyclipping/clipper.hpp>
#include <range/v3/algorithm/minmax.hpp>

#include <filesystem>
#include <iostream>
#include <numbers>
#include <numeric>
#include <string>

namespace infill
{

class InfillGenerator
{
public:
    std::filesystem::path tiles_path;

    static std::tuple<std::vector<geometry::polyline<>>, std::vector<geometry::polygon_outer<>>> gridToPolygon(const auto& grid)
    {
        std::tuple<std::vector<geometry::polyline<>>, std::vector<geometry::polygon_outer<>>> shape;
        for (const auto& row : grid)
        {
            for (const auto& tile : row)
            {
                auto [lines, polys] = tile.render(false);
                std::get<0>(shape).insert(std::get<0>(shape).end(), lines.begin(), lines.end());
                std::get<1>(shape).insert(std::get<1>(shape).end(), polys.begin(), polys.end());
            }
        }
        return shape;
    }

    std::tuple<ClipperLib::Paths, ClipperLib::Paths> generate(
        const std::vector<geometry::polygon_outer<>>& outer_contours,
        std::string_view pattern,
        const int64_t infill_scale,
        const int64_t center_x,
        const int64_t center_y,
        const int64_t z)
    {
        auto bounding_boxes = outer_contours
                            | ranges::views::transform(
                                  [](const auto& contour)
                                  {
                                      return geometry::computeBoundingBox(contour);
                                  })
                            | ranges::views::join | ranges::to_vector;
        auto bounding_box = geometry::computeBoundingBox(bounding_boxes);

        // ------------------------------------------------------------
        // Current z height
        // ------------------------------------------------------------
        spdlog::info("Received z: {}", static_cast<int64_t>(z));

        std::vector<std::vector<Tile>> grid;

        //path used later in the plugin for the current layer file
        auto content_path = tiles_path;

        if (std::filesystem::is_directory(tiles_path)) { //if the given directory exists
            if (!std::filesystem::is_empty(tiles_path)) { //if the given directory is empty
                auto layer_name = "/" + std::to_string(z) + "_";
                layer_name += pattern;
                layer_name += ".wkt";

                content_path += layer_name; //given directory + /'z_height'_'layered_infill'

                if(std::filesystem::exists(content_path)){ //if a file for the current layer exists
                    spdlog::info("File used for current layer: {}", layer_name);
                } else { //no file exists, next higher file will be used
                    std::filesystem::directory_iterator iter{tiles_path};
                    std::filesystem::path cur_path;
                    std::string layer_prefix = "";
                    std::vector<int> all_file_pre;

                    for (auto const &dir_entry: iter) { //add all z height prefixes to array
                        cur_path= iter->path();
                        auto filename = cur_path.filename().string();
                        auto prefix = std::stoi(filename.substr(0, filename.find("_")));

                        all_file_pre.push_back(prefix);
                    }

                    std::sort(all_file_pre.begin(),all_file_pre.end()); //sort all z height prefixes
                    for(auto i=all_file_pre.begin(); i<all_file_pre.end(); i++){
                        if(*i > z){ //find next higher z height
                            layer_prefix = std::to_string(*i);
                            break;
                        }
                    }
                    if (layer_prefix == ""){ // if no higher z height was found, use highest z height
                        layer_prefix = std::to_string(all_file_pre.back());
                    }

                    layer_name = "/" + layer_prefix + "_";
                    layer_name += pattern;
                    layer_name += ".wkt";

                    content_path = tiles_path;
                    content_path += layer_name; //given directory + /'z_height'_'layered_infill'

                    spdlog::info("No file for z height {} found, file used for current layer: {}", z, layer_name);
                    }
                } else {
                    spdlog::error("No files in given directory");
                }
        } else {
            spdlog::error("The given directory does not exist. Slicing failed");
        }

        size_t row_count{ 0 };

        std::vector<Tile> row;
        row.push_back({ .x = center_x, .y = center_y, .filepath = content_path, .magnitude = infill_scale });
        grid.push_back(row);
        // Cut the grid with the outer contour using Clipper
        auto [lines, polys] = gridToPolygon(grid);
        return { geometry::clip(lines, false, outer_contours), geometry::clip(polys, true, outer_contours) };
    }
};

} // namespace infill
#endif // INFILL_INFILL_GENERATOR_H