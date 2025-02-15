/*
Copyright 2022-2024 NVIDIA CORPORATION

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "nvblox/utils/rates.h"

#include <iostream>

namespace nvblox {
namespace timing {

void Ticker::tick(GetTimestampFunctor get_timestamp_ns_functor) {
  circular_buffer_.push(get_timestamp_ns_functor());
}

float Ticker::getMeanRateHz() const {
  if (circular_buffer_.empty()) {
    return 0.0f;
  }
  const int64_t time_span_ns =
      circular_buffer_.newest() - circular_buffer_.oldest();
  if (time_span_ns <= 0) {
    return 0.0f;
  }
  constexpr int64_t kSecondsToNanoSeconds = 1e9;
  return static_cast<float>(
      static_cast<double>(circular_buffer_.size() * kSecondsToNanoSeconds) /
      static_cast<double>(time_span_ns));
}

int Ticker::getNumSamples() const { return circular_buffer_.size(); }

int64_t GetChronoTimestampFunctor::operator()() const {
  const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto timestamp_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
  return timestamp_ns.count();
}

Rates& Rates::getInstance() {
  static Rates rates;
  return rates;
}

Ticker& Rates::getTicker(const std::string& tag) {
  std::lock_guard<std::mutex> lock(getInstance().mutex_);
  TickerMap& tickers = getInstance().tickers_;
  auto it = tickers.find(tag);
  if (it != tickers.end()) {
    // Ticker already exists so return what we've found.
    return it->second;
  } else {
    // This tag hasn't been ticked before. Let's create it, and return the new
    // ticker.
    auto insert_status = tickers.emplace(tag, Ticker());
    getInstance().max_tag_length_ =
        std::max(getInstance().max_tag_length_, tag.size());
    return insert_status.first->second;
  }
}

void Rates::tick(const std::string& tag) {
  Ticker& ticker = getInstance().getTicker(tag);
  std::lock_guard<std::mutex> lock(getInstance().mutex_);
  ticker.tick(getInstance().get_timestamp_ns_functor_);
}

void Rates::setGetTimestampFunctor(
    GetTimestampFunctor get_timestamp_ns_functor) {
  getInstance().get_timestamp_ns_functor_ = get_timestamp_ns_functor;
}

float Rates::getMeanRateHz(const std::string& tag) {
  if (!getInstance().exists(tag)) {
    return 0.0f;
  }
  const Ticker& ticker = getInstance().getTicker(tag);
  std::lock_guard<std::mutex> lock(getInstance().mutex_);
  return ticker.getMeanRateHz();
}

std::vector<std::string> Rates::getTags() {
  const Rates& rates = Rates::getInstance();
  std::vector<std::string> keys;
  keys.reserve(rates.tickers_.size());
  for (const auto& [tag, value] : rates.tickers_) {
    keys.push_back(tag);
  }
  return keys;
}

bool Rates::exists(const std::string& tag) {
  return !(getInstance().tickers_.find(tag) == getInstance().tickers_.end());
}

std::string Rates::rateToString(float rate_hz) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%0.1f", rate_hz);
  return buffer;
}

void Rates::Print(std::ostream& out) {
  out << "\nNVBlox Rates (in Hz)\n";
  out << "namespace/tag - NumSamples (Window Length) - Mean \n";
  out << "-----------\n";

  for (const auto& tag_ticker_pair : getInstance().tickers_) {
    const std::string& ticker_name = tag_ticker_pair.first;

    out.width(static_cast<std::streamsize>(getInstance().max_tag_length_));
    out.setf(std::ios::left, std::ios::adjustfield);
    out << ticker_name << "\t";
    out.width(7);

    const Ticker& ticker = tag_ticker_pair.second;
    const int num_samples = ticker.getNumSamples();
    out << num_samples << "\t";
    if (num_samples > 0) {
      const float mean_rate = ticker.getMeanRateHz();
      out << rateToString(mean_rate);
    }
    out << std::endl;
  }
  out << "-----------\n";
}

std::string Rates::Print() {
  std::stringstream ss;
  Print(ss);
  return ss.str();
}

}  // namespace timing
}  // namespace nvblox
