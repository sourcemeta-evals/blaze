#include <sourcemeta/blaze/output_simple.h>
#include <sourcemeta/blaze/output_standard.h>

#include <cassert>    // assert
#include <cstdint>    // std::int64_t
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

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format,
              const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::core::JSON {
  auto result{standard(evaluator, schema, instance, format)};

  if (result.defines("annotations")) {
    for (auto &unit : result.at("annotations").as_array()) {
      const auto instance_location{sourcemeta::core::to_pointer(
          unit.at("instanceLocation").to_string())};
      const auto position{positions.get(instance_location)};
      if (position.has_value()) {
        auto instance_position{sourcemeta::core::JSON::make_array()};
        const std::int64_t line_start{
            static_cast<std::int64_t>(std::get<0>(position.value()))};
        const std::int64_t column_start{
            static_cast<std::int64_t>(std::get<1>(position.value()))};
        const std::int64_t line_end{
            static_cast<std::int64_t>(std::get<2>(position.value()))};
        const std::int64_t column_end{
            static_cast<std::int64_t>(std::get<3>(position.value()))};
        instance_position.push_back(sourcemeta::core::JSON(line_start));
        instance_position.push_back(sourcemeta::core::JSON(column_start));
        instance_position.push_back(sourcemeta::core::JSON(line_end));
        instance_position.push_back(sourcemeta::core::JSON(column_end));
        unit.assign("instancePosition", std::move(instance_position));
      }
    }
  }

  if (result.defines("errors")) {
    for (auto &unit : result.at("errors").as_array()) {
      const auto instance_location{sourcemeta::core::to_pointer(
          unit.at("instanceLocation").to_string())};
      const auto position{positions.get(instance_location)};
      if (position.has_value()) {
        auto instance_position{sourcemeta::core::JSON::make_array()};
        const std::int64_t line_start{
            static_cast<std::int64_t>(std::get<0>(position.value()))};
        const std::int64_t column_start{
            static_cast<std::int64_t>(std::get<1>(position.value()))};
        const std::int64_t line_end{
            static_cast<std::int64_t>(std::get<2>(position.value()))};
        const std::int64_t column_end{
            static_cast<std::int64_t>(std::get<3>(position.value()))};
        instance_position.push_back(sourcemeta::core::JSON(line_start));
        instance_position.push_back(sourcemeta::core::JSON(column_start));
        instance_position.push_back(sourcemeta::core::JSON(line_end));
        instance_position.push_back(sourcemeta::core::JSON(column_end));
        unit.assign("instancePosition", std::move(instance_position));
      }
    }
  }

  return result;
}

} // namespace sourcemeta::blaze
