// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <iostream>
#include <random>
#include <boost/program_options.hpp>
#include <math.h>
#include <cmath>
#include "utils.h"

namespace po = boost::program_options;
class ZipfDistribution {
 public:
  ZipfDistribution(int num_points, int num_labels)
      : uniform_zero_to_one(std::uniform_real_distribution<>(0.0, 1.0)),
        num_points(num_points), num_labels(num_labels) {
  }

  std::unordered_map<int, int> createDistributionMap() {
    std::unordered_map<int, int> map;
    int primary_label_freq = ceil(num_points * distribution_factor);
    for (int i{1}; i < num_labels + 1; i++) {
      map[i] = ceil(primary_label_freq / i);
    }
    return map;
  }

  int writeDistribution(std::ofstream& outfile) {
    auto distribution_map = createDistributionMap();
    auto primary_label_frequency = num_points * distribution_factor;
    for (int i{0}; i < num_points; i++) {
      bool label_written = false;
      for (auto it = distribution_map.cbegin(), next_it = it;
           it != distribution_map.cend(); it = next_it) {
        next_it++;
        auto label_selection_probability = std::bernoulli_distribution(
            distribution_factor / (double) it->first);
        if (label_selection_probability(rand_engine)) {
          if (label_written) {
            outfile << ',';
          }
          outfile << it->first;
          label_written = true;
          // remove label from map if we have used all labels
          distribution_map[it->first] -= 1;
          if (distribution_map[it->first] == 0) {
            distribution_map.erase(it);
          }
        }
      }
      if (!label_written) {
        outfile << 0;
      }
      if (i < num_points - 1) {
        outfile << '\n';
      }
    }
    return 0;
  }

  int writeDistribution(std::string filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
      std::cerr << "Error: could not open output file " << filename << '\n';
      return -1;
    }
    writeDistribution(outfile);
    outfile.close();
  }

 private:
  int                                          num_labels;
  const int                                    num_points;
  const double                                 distribution_factor = 0.7;
  std::knuth_b                                 rand_engine;
  const std::uniform_real_distribution<double> uniform_zero_to_one;
};

int main(int argc, char** argv) {
  std::string output_file, distribution_type;
  _u64        num_labels, num_points;

  try {
    po::options_description desc{"Arguments"};

    desc.add_options()("help,h", "Print information on arguments");
    desc.add_options()("output_file,O",
                       po::value<std::string>(&output_file)->required(),
                       "Filename for saving the label file");
    desc.add_options()("num_points,N",
                       po::value<uint64_t>(&num_points)->required(),
                       "Number of points in dataset");
    desc.add_options()("num_labels,L",
                       po::value<uint64_t>(&num_labels)->required(),
                       "Number of unique labels, up to 5000");
    desc.add_options()(
        "distribution_type,DT",
        po::value<std::string>(&distribution_type)->default_value("random"),
        "Distribution function for labels <random/zipf> defaults to random");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
      std::cout << desc;
      return 0;
    }
    po::notify(vm);
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return -1;
  }

  if (num_labels > 5000) {
    std::cerr << "Error: num_labels must be 5000 or less" << '\n';
    return -1;
  }

  if (num_points <= 0) {
    std::cerr << "Error: num_points must be greater than 0" << '\n';
    return -1;
  }

  std::cout << "Generating synthetic labels for " << num_points
            << " points with " << num_labels << " unique labels" << '\n';

  try {
    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
      std::cerr << "Error: could not open output file " << output_file << '\n';
      return -1;
    }

    if (distribution_type == "zipf") {
      ZipfDistribution zipf(num_points, num_labels);
      zipf.writeDistribution(outfile);
    } else if (distribution_type == "random") {
      for (int i = 0; i < num_points; i++) {
        bool label_written = false;
        for (int j = 1; j <= num_labels; j++) {
          // 50% chance to assign each label
          if (rand() > (RAND_MAX / 2)) {
            if (label_written) {
              outfile << ',';
            }
            outfile << j;
            label_written = true;
          }
        }
        if (!label_written) {
          outfile << 0;
        }
        if (i < num_points - 1) {
          outfile << '\n';
        }
      }
    }
    if (outfile.is_open()) {
      outfile.close();
    }

    std::cout << "Labels written to " << output_file << '\n';

  } catch (const std::exception& ex) {
    std::cerr << "Label generation failed: " << ex.what() << '\n';
    return -1;
  }

  return 0;
}