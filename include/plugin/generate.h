// BSD 4-Clause License
//
// Copyright (c) 2024 Michael Jaeger, Marie Schmid
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// 3. All advertising materials mentioning features or use of this software must
//   display the following acknowledgement:
//     This product includes software developed by UltiMaker.
//
// 4. Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef PLUGIN_GENERATE_H
#define PLUGIN_GENERATE_H

#include "infill/infill_generator.h"
#include "plugin/broadcast.h"
#include "plugin/metadata.h"
#include "plugin/settings.h"

#include <boost/asio/awaitable.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#if __has_include(<coroutine>)

#include <coroutine>

#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#define USE_EXPERIMENTAL_COROUTINE
#endif

#include <filesystem>
#include <memory>

namespace plugin::infill_generate
{

template<class T, class Rsp, class Req>
struct Generate
{
    using service_t = std::shared_ptr<T>;
    service_t generate_service{ std::make_shared<T>() };
    Broadcast::shared_settings_t settings{ std::make_shared<Broadcast::settings_t>() };
    std::shared_ptr<Metadata> metadata{ std::make_shared<Metadata>() };
    std::filesystem::path tiles_path;
    infill::InfillGenerator generator;

    boost::asio::awaitable<void> run()
    {
        while (true)
        {
            grpc::ServerContext server_context;
            grpc::Status status = grpc::Status::OK;
            cura::plugins::slots::infill::v0::generate::CallRequest request;
            grpc::ServerAsyncResponseWriter<Rsp> writer{ &server_context };
            co_await agrpc::request(&T::RequestCall, *generate_service, server_context, request, writer, boost::asio::use_awaitable);
            const auto pattern_setting = Settings::getPattern(request.pattern(), metadata->plugin_name, metadata->plugin_version);
            const auto infill_scale_setting = Settings::retrieveSettings("infill_scale", request, metadata);
            const auto infill_directory_setting = Settings::retrieveSettings("infill_directory", request, metadata);
            const auto center_x_setting = Settings::retrieveSettings("center_x", request, metadata);
            const auto center_y_setting = Settings::retrieveSettings("center_y", request, metadata);
            const auto z_setting = Settings::retrieveZ(request);
            const auto [machine_width, machine_depth] = Settings::machineSize(request);

            if (! pattern_setting.has_value() || ! infill_scale_setting.has_value() || ! infill_directory_setting.has_value() || ! center_x_setting.has_value() || ! center_y_setting.has_value() || ! machine_width.has_value() || ! machine_depth.has_value() || ! z_setting.has_value())
            {
                spdlog::error(
                    "pattern: {}, infill scale: {}, infill directory: {}, center distance x: {}, center distance y: {}, machine width: {}, machine depth: {}, z: {}",
                    pattern_setting.has_value(),
                    infill_scale_setting.has_value(),
                    infill_directory_setting.has_value(),
                    center_x_setting.has_value(),
                    center_y_setting.has_value(),
                    machine_width.has_value(),
                    machine_depth.has_value(),
                    z_setting.has_value());
                spdlog::error(request.DebugString());
                status = grpc::Status(
                    grpc::StatusCode::INTERNAL,
                    fmt::format(
                        "Plugin could not retrieve settings! pattern: {}, infill scale: {}, infill directory: {}, center distance x: {}, center distance y: {}, machine width: {}, machine depth: {}, z: {}",
                        pattern_setting.has_value(),
                        infill_scale_setting.has_value(),
                        infill_directory_setting.has_value(),
                        center_x_setting.has_value(),
                        center_y_setting.has_value(),
                        machine_width.has_value(),
                        machine_depth.has_value(),
                        z_setting.has_value()));
            }

            if (! status.ok())
            {
                co_await agrpc::finish_with_error(writer, status, boost::asio::use_awaitable);
                continue;
            }

            const int64_t infill_scale = std::stold(infill_scale_setting.value());
                generator.tiles_path = infill_directory_setting.value();
            const int64_t center_x = (long long) (1000.0 * (std::stold(machine_width.value()) / 2.0 + std::stold(center_x_setting.value())));
            const int64_t center_y = (long long) (1000.0 * (std::stold(machine_depth.value()) / 2.0 - std::stold(center_y_setting.value())));
            const int64_t z = std::stoll(z_setting.value());
            auto client_metadata = getUuid(server_context);

            auto outlines = std::vector<infill::geometry::polygon_outer<>>{};
            for (const auto& msg_outline : request.infill_areas().polygons())
            {
                auto outline = infill::geometry::polygon_outer<>{};
                for (const auto& point : msg_outline.outline().path())
                {
                    outline.push_back({ point.x(), point.y() });
                }
                outlines.push_back(outline);
                for (const auto& hole : msg_outline.holes())
                {
                    auto hole_outline = infill::geometry::polygon_outer<>{};
                    for (const auto& point : hole.path())
                    {
                        hole_outline.push_back({ point.x(), point.y() });
                    }
                    outlines.push_back(hole_outline);
                }
            }

            Rsp response;

            ClipperLib::Paths lines;
            ClipperLib::Paths polys;
            try
            {
                auto [lines_, polys_] = generator.generate(outlines, pattern_setting.value(), infill_scale, center_x, center_y, z);
                lines = std::move(lines_);
                polys = std::move(polys_);
            }
            catch (const std::exception& e)
            {
                spdlog::error("Error: {}", e.what());
                status = grpc::Status(grpc::StatusCode::INTERNAL, static_cast<std::string>(e.what()));
            }
            if (! status.ok())
            {
                co_await agrpc::finish_with_error(writer, status, boost::asio::use_awaitable);
                continue;
            }

            // convert poly_lines to protobuf response
            auto* poly_lines_msg = response.mutable_poly_lines();
            for (auto& poly_line : lines)
            {
                auto* path_msg = poly_lines_msg->add_paths();
                for (auto& point : poly_line)
                {
                    auto* point_msg = path_msg->add_path();
                    point_msg->set_x(point.X);
                    point_msg->set_y(point.Y);
                }
            }

            auto* polygons_msg = response.mutable_polygons();

            for (const auto& pp : polys)
            {
                auto* path_msg = polygons_msg->add_polygons()->mutable_outline();
                for (const auto& point : pp)
                {
                    auto* point_msg = path_msg->add_path();
                    point_msg->set_x(point.X);
                    point_msg->set_y(point.Y);
                }
            }

            co_await agrpc::finish(writer, response, status, boost::asio::use_awaitable);
        }
    }
};

} // namespace plugin::infill_generate

#endif // PLUGIN_GENERATE_H