#include <sourcemeta/blaze/output_simple.h>
#include <sourcemeta/blaze/output_standard.h>

#include <sourcemeta/core/jsonpointer.h>

#include <cassert>    // assert
#include <cstdint>    // std::uint64_t
#include <functional> // std::ref
#include <optional>   // std::optional
#include <string>     // std::string
#include <tuple>      // std::tuple

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

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format,
              const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::core::JSON {
  auto result{standard(evaluator, schema, instance, format)};

  if (format == StandardOutput::Flag) {
    return result;
  }

  const auto augment_unit = [&positions](sourcemeta::core::JSON &unit) {
    if (!unit.defines("instanceLocation")) {
      return;
    }

    const auto &instance_location_json{unit.at("instanceLocation")};
    if (!instance_location_json.is_string()) {
      return;
    }

    const auto instance_location_str{instance_location_json.to_string()};
    const auto pointer{
        instance_location_str.empty()
            ? sourcemeta::core::Pointer{}
            : sourcemeta::core::to_pointer(instance_location_str)};
    const auto position{positions.get(pointer)};

    if (position.has_value()) {
      auto position_array{sourcemeta::core::JSON::make_array()};
      position_array.push_back(sourcemeta::core::JSON(
          static_cast<std::int64_t>(std::get<0>(position.value()))));
      position_array.push_back(sourcemeta::core::JSON(
          static_cast<std::int64_t>(std::get<1>(position.value()))));
      position_array.push_back(sourcemeta::core::JSON(
          static_cast<std::int64_t>(std::get<2>(position.value()))));
      position_array.push_back(sourcemeta::core::JSON(
          static_cast<std::int64_t>(std::get<3>(position.value()))));
      unit.assign("instancePosition", std::move(position_array));
    }
  };

  if (result.defines("errors") && result.at("errors").is_array()) {
    for (auto &error : result.at("errors").as_array()) {
      augment_unit(error);
    }
  }

  if (result.defines("annotations") && result.at("annotations").is_array()) {
    for (auto &annotation : result.at("annotations").as_array()) {
      augment_unit(annotation);
    }
  }

  return result;
}

} // namespace sourcemeta::blaze
