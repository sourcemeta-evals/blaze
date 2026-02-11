#include <sourcemeta/blaze/output_simple.h>
#include <sourcemeta/blaze/output_standard.h>

#include <cassert>    // assert
#include <cstdint>    // std::uint64_t
#include <functional> // std::ref
#include <tuple>      // std::get

namespace sourcemeta::blaze {

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format) -> sourcemeta::core::JSON {
  // We avoid a callback for this specific case for performance reasons
  if (format == StandardOutput::Flag) {
    auto result{sourcemeta::core::JSON::make_object()};
    const auto valid{evaluator.validate(schema, instance)};
    result.assign("valid", sourcemeta::core::JSON{valid});
    return result;
  } else {
    assert(format == StandardOutput::Basic);
    SimpleOutput output{instance};
    const auto valid{evaluator.validate(schema, instance, std::ref(output))};

    if (valid) {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto annotations{sourcemeta::core::JSON::make_array()};
      for (const auto &annotation : output.annotations()) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(annotation.first.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{annotation.first.schema_location});
        unit.assign(
            "instanceLocation",
            sourcemeta::core::to_json(annotation.first.instance_location));
        unit.assign("annotation", sourcemeta::core::to_json(annotation.second));
        annotations.push_back(std::move(unit));
      }

      if (!annotations.empty()) {
        result.assign("annotations", std::move(annotations));
      }

      return result;
    } else {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto errors{sourcemeta::core::JSON::make_array()};
      for (const auto &entry : output) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(entry.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{entry.schema_location});
        unit.assign("instanceLocation",
                    sourcemeta::core::to_json(entry.instance_location));
        unit.assign("error", sourcemeta::core::JSON{entry.message});
        errors.push_back(std::move(unit));
      }

      assert(!errors.empty());
      result.assign("errors", std::move(errors));
      return result;
    }
  }
}

static auto
set_instance_position(sourcemeta::core::JSON &unit,
                      const sourcemeta::core::WeakPointer &instance_location,
                      const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  const auto position{
      positions.get(sourcemeta::core::to_pointer(instance_location))};
  if (position.has_value()) {
    auto array{sourcemeta::core::JSON::make_array()};
    array.push_back(sourcemeta::core::JSON{
        static_cast<std::int64_t>(std::get<0>(position.value()))});
    array.push_back(sourcemeta::core::JSON{
        static_cast<std::int64_t>(std::get<1>(position.value()))});
    array.push_back(sourcemeta::core::JSON{
        static_cast<std::int64_t>(std::get<2>(position.value()))});
    array.push_back(sourcemeta::core::JSON{
        static_cast<std::int64_t>(std::get<3>(position.value()))});
    unit.assign("instancePosition", std::move(array));
  }
}

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format,
              const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::core::JSON {
  if (format == StandardOutput::Flag) {
    auto result{sourcemeta::core::JSON::make_object()};
    const auto valid{evaluator.validate(schema, instance)};
    result.assign("valid", sourcemeta::core::JSON{valid});
    return result;
  } else {
    assert(format == StandardOutput::Basic);
    SimpleOutput output{instance};
    const auto valid{evaluator.validate(schema, instance, std::ref(output))};

    if (valid) {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto annotations{sourcemeta::core::JSON::make_array()};
      for (const auto &annotation : output.annotations()) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(annotation.first.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{annotation.first.schema_location});
        unit.assign(
            "instanceLocation",
            sourcemeta::core::to_json(annotation.first.instance_location));
        unit.assign("annotation", sourcemeta::core::to_json(annotation.second));
        set_instance_position(unit, annotation.first.instance_location,
                              positions);
        annotations.push_back(std::move(unit));
      }

      if (!annotations.empty()) {
        result.assign("annotations", std::move(annotations));
      }

      return result;
    } else {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign("valid", sourcemeta::core::JSON{valid});
      auto errors{sourcemeta::core::JSON::make_array()};
      for (const auto &entry : output) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign("keywordLocation",
                    sourcemeta::core::to_json(entry.evaluate_path));
        unit.assign("absoluteKeywordLocation",
                    sourcemeta::core::JSON{entry.schema_location});
        unit.assign("instanceLocation",
                    sourcemeta::core::to_json(entry.instance_location));
        unit.assign("error", sourcemeta::core::JSON{entry.message});
        set_instance_position(unit, entry.instance_location, positions);
        errors.push_back(std::move(unit));
      }

      assert(!errors.empty());
      result.assign("errors", std::move(errors));
      return result;
    }
  }
}

} // namespace sourcemeta::blaze
