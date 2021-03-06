#include "rapidcheck/Check.h"

#include "detail/DefaultTestListener.h"
#include "detail/Testing.h"

namespace rc {
namespace detail {

TestResult
checkProperty(const Property &property,
              const TestMetadata &metadata,
              const FuzzData::Ptr &data,
              const TestParams &params,
              TestListener &listener,
              const std::unordered_map<std::string, Reproduce> &reproduceMap) {
  if (reproduceMap.empty()) {
    return testProperty(property, metadata, data, params, listener);
  }

  // TODO: I do not understand the next block. When is 'reproduceProperty' being
  // called?
  const auto it = reproduceMap.find(metadata.id);
  if (metadata.id.empty() || (it == end(reproduceMap))) {
    SuccessResult success;
    success.numSuccess = 0;
    return success;
  } else {
    auto reproduce = it->second;
    if (params.disableShrinking) {
      reproduce.shrinkPath.clear();
    }
    throw std::logic_error("reproduceProperty about to be called but I have not implemented it.");
    return reproduceProperty(property, reproduce);
  }
}

TestResult checkProperty(const Property &property,
                         const TestMetadata &metadata,
                         const FuzzData::Ptr &data,
                         const TestParams &params,
                         TestListener &listener) {
  return checkProperty(
      property, metadata, data, params, listener, configuration().reproduce);
}

TestResult checkProperty(const Property &property,
                         const TestMetadata &metadata,
                         const FuzzData::Ptr &data,
                         const TestParams &params) {
  return checkProperty(property, metadata, data, params, globalTestListener());
}

TestResult checkProperty(const Property &property,
                         const TestMetadata &metadata,
                         const FuzzData::Ptr &data) {
  return checkProperty(property, metadata, data, configuration().testParams);
}

TestResult checkProperty(const Property &property,
                         const FuzzData::Ptr &data) {
  return checkProperty(property, TestMetadata(), data);
}

} // namespace detail
} // namespace rc
