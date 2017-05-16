#include <gflags/gflags.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "ncode_common/src/common.h"
#include "ncode_common/src/substitute.h"
#include "ncode_common/src/strutil.h"
#include "ncode_common/src/file.h"
#include "ncode_common/src/logging.h"
#include "metrics_parser.h"

DEFINE_string(input, "", "The metrics file.");
DEFINE_string(metric, "", "The id of the metric");
DEFINE_string(fields, "", "The fields to match");
DEFINE_bool(summary, false,
            "If specified will print summary for a specific metric or all "
            "metrics in the file(s) and exit.");

using namespace nc;
using namespace nc::metrics::parser;

static std::string DataSummaryToString(
    const std::vector<std::pair<uint64_t, double>>& data) {
  std::vector<double> all_data;
  all_data.reserve(data.size());

  uint64_t start_timestamp = std::numeric_limits<uint64_t>::max();
  uint64_t end_timestamp = 0;

  for (const auto& timestamp_and_data : data) {
    uint64_t timestamp = timestamp_and_data.first;
    double data = timestamp_and_data.second;

    start_timestamp = std::min(start_timestamp, timestamp);
    end_timestamp = std::max(end_timestamp, timestamp);
    all_data.emplace_back(data);
  }

  std::vector<double> percentiles = Percentiles(&all_data, 10);
  return Substitute("$0 values, min/max timestamps: $1/$2, percentiles: [$3]",
                    data.size(), start_timestamp, end_timestamp,
                    Join(percentiles, ","));
}

static void ProcessSingleFile(const std::string& input_file) {
  if (FLAGS_summary) {
    MetricsParser parser(FLAGS_input);
    LOG(INFO) << "Reading manifest from " << FLAGS_input;
    Manifest manifest = parser.ParseManifest();

    if (FLAGS_metric.empty()) {
      std::cout << manifest.FullToString();
      return;
    }

    std::cout << manifest.ToString(FLAGS_metric);
    return;
  }

  std::map<std::pair<std::string, std::string>,
           std::vector<std::pair<uint64_t, double>>> data =
      SimpleParseNumericData(input_file, FLAGS_metric, FLAGS_fields, 0,
                             std::numeric_limits<uint64_t>::max(), 0);
  for (const auto& ids_and_data : data) {
    const std::pair<std::string, std::string>& id_and_fields =
        ids_and_data.first;
    const std::vector<std::pair<uint64_t, double>>& data = ids_and_data.second;

    std::cout << id_and_fields.first << ":" << id_and_fields.second << " --> "
              << DataSummaryToString(data) << std::endl;
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  CHECK(!FLAGS_input.empty()) << "need --input";

  std::vector<std::string> inputs = Glob(FLAGS_input);
  for (const auto& input : inputs) {
    ProcessSingleFile(input);
  }
}
