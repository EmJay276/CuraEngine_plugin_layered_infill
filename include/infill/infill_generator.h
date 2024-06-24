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
        const TileType tile_type)
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

        std::vector<std::vector<Tile>> grid;
        auto content_path = tiles_path;
        content_path.append(fmt::format("{}.wkt", pattern));

        //folder where all the files for each layer are
        auto folder_path = tiles_path;
        folder_path += "/";
        folder_path += pattern;

        if (std::filesystem::is_directory(folder_path)) { //if a directory with the same name as the pattern exists

            auto prefix_size = 5; //number of digits before the file name
            std::string prefix; // string for the prefix of the currently used layer file
            std::string next_prefix; // string for the prefix of the next layer file

            // path to copy the next layer
            auto nextlayer_path = folder_path;
            nextlayer_path += "/";

            // path to copy the used layer
            auto usedlayer_path = folder_path;
            usedlayer_path += "/";

            std::filesystem::directory_iterator iter{folder_path}; //to iterate over every file in the directory

            int prev_num = -1; //previous number found in layer files, starts at -1 to see if 0 is missing
            int curr_num; //current number found in layer files
            int missing_num = -1; //the number that is missing in the layer files folder, -1 to detect if it was changed

            for (auto const &dir_entry: iter) { //for every file in the layer folder

                std::string string_value = iter->path().filename().string(); // get filename from path and convert to string
                string_value = string_value.substr(0, prefix_size); // get only the numbers at the beginning of the filename

                curr_num = std::stoi(string_value); //convert String to Int
                if (curr_num != (prev_num + 1)) { // missing file found
                    missing_num = (curr_num - 1);
                    break;
                }
                prev_num = curr_num;
            }

            next_prefix = std::to_string((missing_num + 1));

            if (missing_num == -1) { // if no missing number was found the last number is missing
                missing_num = curr_num + 1;
                next_prefix = "0";
            }

            prefix = std::to_string(missing_num);

            //add zeros at the beginning of the string until the length matches prefix_size
            while (prefix.length() != prefix_size) {
                prefix.insert(0, "0");
            }

            spdlog::info(prefix);

            //add zeros at the beginning of the string until the length matches prefix_size
            while (next_prefix.length() != prefix_size) {
                next_prefix.insert(0, "0");
            }

            //construct the new path for the used layer
            usedlayer_path += prefix;
            usedlayer_path += pattern;
            usedlayer_path += ".wkt";

            //construct the existing path for the next layer
            nextlayer_path += next_prefix;
            nextlayer_path += pattern;
            nextlayer_path += ".wkt";

            std::filesystem::rename(content_path, usedlayer_path); //move and rename used layer
            std::filesystem::rename(nextlayer_path, content_path); //move and rename next layer
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