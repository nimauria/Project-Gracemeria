#include "gracemeria/hint_provider.hpp"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "xenon/core/json.hpp"
#include "xenon/recomp/analysis_schema.hpp"
#include "xenon/recomp/analysis_schema_json.hpp"

#ifndef GRACEMERIA_REVISIONS_DIR
#error "GRACEMERIA_REVISIONS_DIR must be defined by the build (see CMakeLists.txt)"
#endif

namespace gracemeria {
namespace {
constexpr std::uint32_t kTitleId = 0x4E4D07D1u;
constexpr std::uint32_t kUsRetailMediaId = 0x531C30BEu;

std::string hex32(std::uint32_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << std::uppercase << value;
  return out.str();
}

std::string fallback_function_name(std::uint32_t address) {
  std::ostringstream out;
  out << "rex_sub_" << std::hex << std::uppercase << std::setw(8)
      << std::setfill('0') << address;
  return out.str();
}

bool append_legacy_function_hints(
    const std::filesystem::path& csv_path,
    xenon::recomp::analysis::AnalysisHintSetV2& hint_set,
    std::string& error) {
  std::ifstream file(csv_path, std::ios::binary);
  if (!file) {
    error = "Project Gracemeria: cannot open " + csv_path.string();
    return false;
  }

  std::string header;
  if (!std::getline(file, header)) {
    error = "Project Gracemeria: empty " + csv_path.string();
    return false;
  }
  if (!header.empty() && header.back() == '\r') header.pop_back();
  if (header != "address,name,legacy_fields,activation") {
    error = "Project Gracemeria: unexpected legacy-symbols.csv header in " +
            csv_path.string();
    return false;
  }

  std::unordered_set<std::uint32_t> known_addresses;
  known_addresses.reserve(hint_set.functions.size() + 10527);
  for (const auto& function : hint_set.functions) {
    known_addresses.insert(function.address);
  }

  hint_set.functions.reserve(hint_set.functions.size() + 10527);

  std::string line;
  std::size_t line_number = 1;
  while (std::getline(file, line)) {
    ++line_number;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;

    const auto first_comma = line.find(',');
    const auto second_comma =
        first_comma == std::string::npos ? std::string::npos
                                         : line.find(',', first_comma + 1);
    if (first_comma == std::string::npos ||
        second_comma == std::string::npos) {
      error = "Project Gracemeria: malformed " + csv_path.string() +
              " at line " + std::to_string(line_number);
      return false;
    }

    const std::string address_text = line.substr(0, first_comma);
    std::string name =
        line.substr(first_comma + 1, second_comma - first_comma - 1);

    std::size_t parsed = 0;
    unsigned long raw_address = 0;
    try {
      raw_address = std::stoul(address_text, &parsed, 0);
    } catch (const std::exception&) {
      error = "Project Gracemeria: invalid function address '" + address_text +
              "' in " + csv_path.string() + " at line " +
              std::to_string(line_number);
      return false;
    }

    if (parsed != address_text.size() ||
        raw_address > std::numeric_limits<std::uint32_t>::max()) {
      error = "Project Gracemeria: invalid function address '" + address_text +
              "' in " + csv_path.string() + " at line " +
              std::to_string(line_number);
      return false;
    }

    const auto address = static_cast<std::uint32_t>(raw_address);
    if ((address & 0x3u) != 0) {
      error = "Project Gracemeria: unaligned PPC function address " +
              hex32(address) + " in " + csv_path.string() + " at line " +
              std::to_string(line_number);
      return false;
    }

    // The original AC6_recomp table has 35 entries written as `0x... = {}`
    // with no explicit name. Preserve the start and generate the same
    // deterministic rex_sub-style name family used by the named entries.
    if (name.empty()) name = fallback_function_name(address);

    // Allow future hints.v2.json files to carry selected explicit function
    // hints without creating duplicate starts when this revision CSV is
    // layered on top.
    if (!known_addresses.insert(address).second) continue;

    xenon::recomp::analysis::FunctionHint hint{};
    hint.address = address;
    hint.name = std::move(name);
    hint_set.functions.push_back(std::move(hint));
  }

  if (!file.good() && !file.eof()) {
    error = "Project Gracemeria: failed reading " + csv_path.string();
    return false;
  }

  return true;
}

}  // namespace

RevisionMatch match_supported_revision(const xenon::xbox::XexEffectiveIdentity& identity) {
  if (identity.title_id != kTitleId) {
    return {false, {}, "title ID " + hex32(identity.title_id) + " is not Ace Combat 6"};
  }
  if (identity.media_id != kUsRetailMediaId) {
    return {false, {}, "Ace Combat 6 media ID " + hex32(identity.media_id) +
                       " is not the address-verified US retail revision (0x531C30BE)"};
  }
  // The supplied AC6_recomp source is explicitly US-only and its fixed
  // addresses are from the base executable. Xenon applies title updates to
  // produce an effective XEX; once a TU's version/hash is locally verified,
  // it must receive a separate revision profile rather than inheriting TU0's
  // base-executable addresses.
  if (identity.title_update_applied) {
    return {false, {}, "title-update effective image requires its own verified Gracemeria revision profile"};
  }
  // "usa-retail-tu0" is this revision's actual on-disk directory name under
  // revisions/ (its own revision.json's "id" field spells out the longer,
  // more descriptive "ac6-us-retail-531c30be-tu0" as internal metadata, but
  // that string is not itself a path anywhere) - this must stay the literal
  // directory name since provide() uses it directly to locate the revision's
  // analysis files.
  return {true, "usa-retail-tu0", "matched title/media identity"};
}

Ac6HintProvider::Ac6HintProvider(std::filesystem::path revisions_root)
    : revisions_root_(std::move(revisions_root)) {}

std::filesystem::path Ac6HintProvider::default_revisions_root() {
  return std::filesystem::path(GRACEMERIA_REVISIONS_DIR);
}

bool Ac6HintProvider::provide(const xenon::xbox::XexEffectiveIdentity& identity,
                              xenon::recomp::analysis::AnalysisHintSetV2& out_hint_set,
                              std::string& error) const {
  const auto match = match_supported_revision(identity);
  if (!match.matched) {
    error = "Project Gracemeria: unsupported executable revision: " + match.reason;
    return false;
  }

  const auto analysis_dir =
      revisions_root_ / match.revision_id / "analysis";
  const auto hints_path = analysis_dir / "hints.v2.json";
  const auto legacy_symbols_path = analysis_dir / "legacy-symbols.csv";

  std::ifstream file(hints_path, std::ios::binary);
  if (!file) {
    error = "Project Gracemeria: cannot open " + hints_path.string();
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  if (!file.good() && !file.eof()) {
    error = "Project Gracemeria: failed reading " + hints_path.string();
    return false;
  }

  xenon::core::JsonValue json;
  std::string json_error;
  if (!xenon::core::JsonValue::parse(buffer.str(), json, &json_error)) {
    error = "Project Gracemeria: malformed " + hints_path.string() + ": " + json_error;
    return false;
  }

  xenon::recomp::analysis::AnalysisHintSetV2 loaded;
  std::string from_json_error;
  if (!xenon::recomp::analysis::from_json(json, loaded, from_json_error)) {
    error = "Project Gracemeria: invalid schema in " + hints_path.string() + ": " + from_json_error;
    return false;
  }

  // AC6_recomp's 10,527 function starts were already migrated into this
  // revision's legacy-symbols.csv, but were previously kept reference-only
  // under the assumption Xenon automatic analysis would rediscover them.
  // Real-XEX validation showed that assumption is not currently true, so make
  // the known starts active, revision-scoped FunctionHints.
  if (!append_legacy_function_hints(legacy_symbols_path, loaded, error)) {
    return false;
  }

  // The on-disk JSON identity is intentionally zeroed until a real
  // default.xex lets migrate-ac6-recomp scope it for real use (see
  // revisions/usa-retail-tu0/revision.json's notes). Stamp the EXACT
  // identity this provider was asked about (see hint_provider.hpp's
  // doc comment): eligibility was already decided above by title/media ID,
  // not by a fixed hash this revision does not have yet.
  loaded.module_name = match.revision_id;
  loaded.identity = identity;

  // Validate the complete, final set after the CSV function starts have been
  // activated and after the exact effective identity has been stamped.
  std::vector<std::string> schema_errors;
  if (!xenon::recomp::analysis::validate_hint_set(loaded, schema_errors)) {
    error = "Project Gracemeria: revision analysis data failed hint-set validation:";
    for (const auto& schema_error : schema_errors) {
      error += " " + schema_error + ";";
    }
    return false;
  }

  out_hint_set = std::move(loaded);
  return true;
}

}  // namespace gracemeria
