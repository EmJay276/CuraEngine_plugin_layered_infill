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
        auto width_offset = tile_type == TileType::HEXAGON ? static_cast<int64_t>(std::sqrt(3) * tile_size + line_width) :tile_size;
        auto height_offset = tile_type == TileType::HEXAGON ? 3 * tile_size / 2 + line_width : tile_size;
        auto alternating_row_offset = [width_offset, tile_type](const auto row_idx)
        {
            return tile_type == TileType::HEXAGON ? static_cast<int64_t>(row_idx % 2 * width_offset / 2) : 0;
        };
        // ------------------------------------------------------------
        // Current z height
        // ------------------------------------------------------------
        spdlog::info("Received z: {}", static_cast<int64_t>(z));

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
                spdlog::info(layer_name);
            } else { //no file exists, slicing will fail with the error message
                spdlog::error("No file found for z height: {}", static_cast<int64_t>(z));
            }
        } else { // no directory with .wkt files exists for current infill
            spdlog::info("No .wkt folder found for current infill. Resuming with regular infill generation.");
            content_path.append(fmt::format("{}.wkt", pattern));
        }

        size_t row_count{ 0 };
        auto start_y = absolute_tiles ? (bounding_box.at(0).Y / height_offset) * height_offset : bounding_box.at(0).Y - height_offset;
        auto start_x = absolute_tiles ? (bounding_box.at(0).X / width_offset) * width_offset : bounding_box.at(0).X - width_offset;
        for (auto y = start_y; y < bounding_box.at(1).Y + height_offset; y += height_offset)
        {
            std::vector<Tile> row;
            for (auto x = start_x + alternating_row_offset(row_count); x < bounding_box.at(1).X + width_offset; x += width_offset)
            {
                row.push_back({ .x = x, .y = y, .filepath = content_path, .magnitude = tile_size, .tile_type = tile_type });
            }
            grid.push_back(row);
            row_count++;
        }

        // Cut the grid with the outer contour using Clipper
        auto [lines, polys] = gridToPolygon(grid);
        return { geometry::clip(lines, false, outer_contours), geometry::clip(polys, true, outer_contours) };
    }
};

} // namespace infill
#endif // INFILL_INFILL_GENERATOR_H