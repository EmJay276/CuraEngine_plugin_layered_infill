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
        const int64_t tile_size,
        const bool absolute_tiles,
        const TileType tile_type,
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

        constexpr int64_t line_width = 200;
        auto width_offset = tile_size;
        auto height_offset = tile_size;
        auto alternating_row_offset = [width_offset, tile_type](const auto row_idx)
        {
            return tile_type == TileType::HEXAGON ? static_cast<int64_t>(row_idx % 2 * width_offset / 2) : 0;
        };
        // ------------------------------------------------------------
        // Current z height
        // ------------------------------------------------------------
        spdlog::info("Received z: {}", static_cast<int64_t>(z));
        spdlog::info("Received Center Distance x: {}", center_x);
        spdlog::info("Received Center Distance y: {}", center_y);

        std::vector<std::vector<Tile>> grid;

        //folder where all the files for each layer are
        auto folder_path = tiles_path;
        folder_path += "/";
        folder_path += pattern;

        //path used later in the plugin for the current layer file
        auto content_path = tiles_path;

        if (std::filesystem::is_directory(folder_path)) { //if a directory with the same name as the pattern exists
            auto layer_name = "/" + std::to_string(z) + "_";
            layer_name += pattern;
            layer_name += ".wkt";

            content_path = folder_path;
            content_path += layer_name;

            if(std::filesystem::exists(content_path)){ //if a file for the current layer exists
                spdlog::info("File used for current layer: {}", layer_name);
            } else { //no file exists, next higher file will be used error message
                std::filesystem::directory_iterator iter{folder_path};
                std::filesystem::path cur_path;
                std::string layer_prefix = "";
                std::vector<int> all_file_pre;

                for (auto const &dir_entry: iter) {
                    cur_path= iter->path();
                    auto filename = cur_path.filename().string();
                    auto prefix = std::stoi(filename.substr(0, filename.find("_")));

                    all_file_pre.push_back(prefix);
                }

                std::sort(all_file_pre.begin(),all_file_pre.end());
                for(auto i=all_file_pre.begin(); i<all_file_pre.end(); i++){
                    if(*i > z){
                        layer_prefix = std::to_string(*i);
                        break;
                    }
	            }
	            if (layer_prefix == ""){
                    layer_prefix = std::to_string(all_file_pre.back());
                }

                layer_name = "/" + layer_prefix + "_";
                layer_name += pattern;
                layer_name += ".wkt";

                content_path = folder_path;
                content_path += layer_name;

                spdlog::info("No file for z height {} exists, file used for current layer: {}", z, layer_name);

            }

        } else { // no directory with .wkt files exists for current infill
            spdlog::info("No .wkt folder found for current infill. Resuming with regular infill generation.");
            content_path.append(fmt::format("{}.wkt", pattern));
        }

        size_t row_count{ 0 };
        //auto start_y = bounding_box.at(0).Y; //absolute_tiles ? (bounding_box.at(0).Y / height_offset) * height_offset : bounding_box.at(0).Y - height_offset;
        //auto start_x = bounding_box.at(0).X; //absolute_tiles ? (bounding_box.at(0).X / width_offset) * width_offset : bounding_box.at(0).X - width_offset;
        spdlog::info("Bounding Box at 0 y: {}", bounding_box.at(0).Y);
        spdlog::info("Bounding Box at 0 x: {}", bounding_box.at(0).X);
        spdlog::info("Bounding Box at 1 y: {}", bounding_box.at(1).Y);
        spdlog::info("Bounding Box at 1 x: {}", bounding_box.at(1).X);

        /*
        for (auto y = start_y; y < bounding_box.at(1).Y + height_offset; y += height_offset)
        {
            std::vector<Tile> row;
            for (auto x = start_x + alternating_row_offset(row_count); x < bounding_box.at(1).X + width_offset; x += width_offset)
            {
                row.push_back({ .x = x, .y = y, .filepath = content_path, .magnitude = tile_size, .tile_type = tile_type });
                spdlog::info("x: {}", x);
                spdlog::info("y: {}", y);
            }
            grid.push_back(row);
            row_count++;
        }
        */

        std::vector<Tile> row;
        row.push_back({ .x = center_x, .y = center_y, .filepath = content_path, .magnitude = tile_size, .tile_type = tile_type });
        grid.push_back(row);
        // Cut the grid with the outer contour using Clipper
        auto [lines, polys] = gridToPolygon(grid);
        return { geometry::clip(lines, false, outer_contours), geometry::clip(polys, true, outer_contours) };
    }
};

} // namespace infill
#endif // INFILL_INFILL_GENERATOR_H