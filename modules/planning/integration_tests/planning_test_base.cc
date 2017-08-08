/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "modules/planning/integration_tests/planning_test_base.h"

#include "modules/common/log.h"
#include "modules/common/vehicle_state/vehicle_state.h"

namespace apollo {
namespace planning {

using common::adapter::AdapterManager;

DEFINE_string(test_routing_response_file,
              "modules/planning/testdata/garage_routing.pb.txt",
              "The routing file used in test");
DEFINE_string(test_localization_file,
              "modules/planning/testdata/garage_localization.pb.txt",
              "The localization test file");
DEFINE_string(test_chassis_file,
              "modules/planning/testdata/garage_chassis.pb.txt",
              "The chassis test file");
DEFINE_string(test_prediction_file, "", "The prediction module test file");

void PlanningTestBase::SetUpTestCase() {
  FLAGS_qp_spline_path_config_file =
      "modules/planning/testdata/conf/qp_spline_path_config.pb.txt";
  FLAGS_qp_spline_st_speed_config_file =
      "modules/planning/testdata/conf/qp_spline_st_speed_config_file.pb.txt";
  FLAGS_st_boundary_config_file =
      "modules/planning/testdata/conf/st_boundary_config_file.pb.txt";
  FLAGS_planning_config_file =
      "modules/planning/testdata/conf/planning_config.pb.txt";
  FLAGS_adapter_config_path = "modules/planning/testdata/conf/adapter.conf";
  FLAGS_map_filename = "modules/planning/testdata/base_map.txt";
  FLAGS_reference_line_smoother_config_file =
      "modules/planning/testdata/conf/reference_line_smoother_config.pb.txt";
  FLAGS_dp_poly_path_config_file =
      "modules/planning/testdata/conf/dp_poly_path_config.pb.txt";
  FLAGS_dp_st_speed_config_file =
      "modules/planning/testdata/conf/dp_st_speed_config.pb.txt";
  FLAGS_test_localization_file =
      "modules/planning/testdata/garage_localization.pb.txt";
  FLAGS_test_chassis_file = "modules/planning/testdata/garage_chassis.pb.txt",
  FLAGS_v = 4;
  FLAGS_alsologtostderr = true;
}

bool PlanningTestBase::SetUpAdapters() {
  if (!AdapterManager::Initialized()) {
    AdapterManager::Init(FLAGS_adapter_config_path);
  }
  if (!AdapterManager::GetRoutingResponse()) {
    AERROR << "routing is not registered in adapter manager, check adapter "
              "config file."
           << FLAGS_adapter_config_path;
    return false;
  }
  if (!AdapterManager::FeedRoutingResponseFile(
          FLAGS_test_routing_response_file)) {
    AERROR << "failed to routing file: " << FLAGS_test_routing_response_file;
    return false;
  }
  AINFO << "Using Routing " << FLAGS_test_routing_response_file;
  if (!AdapterManager::FeedLocalizationFile(FLAGS_test_localization_file)) {
    AERROR << "Failed to load localization file: "
           << FLAGS_test_localization_file;
    return false;
  }
  AINFO << "Using Localization file: " << FLAGS_test_localization_file;
  if (!AdapterManager::FeedChassisFile(FLAGS_test_chassis_file)) {
    AERROR << "Failed to load chassis file: " << FLAGS_test_chassis_file;
    return false;
  }
  AINFO << "Using Chassis file: " << FLAGS_test_chassis_file;
  if (!FLAGS_test_prediction_file.empty() &&
      !AdapterManager::FeedPredictionFile(FLAGS_test_prediction_file)) {
    AERROR << "Failed to load prediction file: " << FLAGS_test_prediction_file;
    return false;
  }
  AINFO << "Using Prediction file: " << FLAGS_test_chassis_file;
  return true;
}

void PlanningTestBase::SetUp() {
  adc_trajectory_ = nullptr;
  CHECK(SetUpAdapters()) << "Failed to setup adapters";
  planning_.Init();
}

void PlanningTestBase::RunPlanning() {
  planning_.RunOnce();
  adc_trajectory_ = AdapterManager::GetPlanning()->GetLatestPublished();
}

void PlanningTestBase::export_sl_points(
    const std::vector<std::vector<common::SLPoint>>& points,
    const std::string& filename) {
  AINFO << "Write sl_points to file " << filename;
  std::ofstream ofs(filename);
  ofs << "level, s, l" << std::endl;
  int level = 0;
  for (const auto& level_points : points) {
    for (const auto& point : level_points) {
      ofs << level << ", " << point.s() << ", " << point.l() << std::endl;
    }
    ++level;
  }
  ofs.close();
}

void PlanningTestBase::export_path_data(const PathData& path_data,
                                        const std::string& filename) {
  AINFO << "Write path_data to file " << filename;
  std::ofstream ofs(filename);
  ofs << "s, l, dl, ddl, x, y, z" << std::endl;
  const auto& frenet_path = path_data.frenet_frame_path();
  const auto& discrete_path = path_data.discretized_path();
  if (frenet_path.NumOfPoints() != discrete_path.NumOfPoints()) {
    AERROR << "frenet_path and discrete path have different number of points";
    return;
  }
  for (uint32_t i = 0; i < frenet_path.NumOfPoints(); ++i) {
    const auto& frenet_point = frenet_path.PointAt(i);
    const auto& discrete_point = discrete_path.PathPointAt(i);
    ofs << frenet_point.s() << ", " << frenet_point.l() << ", "
        << frenet_point.dl() << ", " << frenet_point.ddl() << discrete_point.x()
        << ", " << discrete_point.y() << ", " << discrete_point.z()
        << std::endl;
  }
  ofs.close();
}

}  // namespace planning
}  // namespace apollo
