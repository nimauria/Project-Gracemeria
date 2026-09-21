#include "gracemeria/hint_provider.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>

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
  // directory name since provide() uses it directly to locate
  // <revisions_root>/<revision_id>/analysis/hints.v2.json.
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

  const auto hints_path = revisions_root_ / match.revision_id / "analysis" / "hints.v2.json";
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

  std::vector<std::string> schema_errors;
  if (!xenon::recomp::analysis::validate_hint_set(loaded, schema_errors)) {
    error = "Project Gracemeria: " + hints_path.string() + " failed hint-set validation:";
    for (const auto& schema_error : schema_errors) error += " " + schema_error + ";";
    return false;
  }

  // The on-disk file's identity is intentionally zeroed until a real
  // default.xex lets migrate-ac6-recomp scope it for real use (see
  // revisions/usa-retail-tu0/revision.json's notes). Stamp the EXACT
  // identity this provider was asked about instead (see hint_provider.hpp's
  // doc comment): eligibility was already decided above by title/media ID,
  // not by a fixed hash this revision does not have yet.
  loaded.module_name = match.revision_id;
  loaded.identity = identity;
  out_hint_set = std::move(loaded);
  return true;
}

}  // namespace gracemeria
