#pragma once

#include <filesystem>
#include <string>

#include "xenon/recomp/driver.hpp"

namespace gracemeria {

struct RevisionMatch {
  bool matched{};
  std::string revision_id;
  std::string reason;
};

// Eligibility check independent of any exact effective-image hash (Gracemeria
// does not yet have one locked in for this revision - see revisions/
// usa-retail-tu0/revision.json's "effectiveXexSha1: null" and its notes).
// Matches on title ID + media ID + "not a title-update-patched image" only.
[[nodiscard]] RevisionMatch match_supported_revision(const xenon::xbox::XexEffectiveIdentity& identity);

// Production xenon::recomp::ModuleHintProviderV2 for Ace Combat 6 (see
// Xenon's docs/ANALYSIS_HINT_SCHEMA_V2.md for the schema/interface this
// implements). Supplies the migrated Analysis Hint Schema V2 hint set for
// the one currently address-verified revision profile (US retail, Media ID
// 0x531C30BE, base executable / no title update).
//
// This provider does not yet know a fixed effective-image SHA-1 to compare
// against (that requires a legally supplied default.xex to hash locally),
// so its eligibility test (match_supported_revision) is title/media-ID-based
// rather than hash-based. Once eligible, it stamps the returned hint set
// with the EXACT identity it was asked about (never a different/assumed
// one), so Xenon's own hint_set_matches_identity() safety check - which
// still runs after this provider returns - trivially and correctly passes;
// this provider's own matching policy is what actually decides eligibility,
// not a bypass of that check.
//
// Loads its schema-native hint data from
// `<revisions_root>/<revision_id>/analysis/hints.v2.json` and supplements the
// active FunctionHint list from the already-migrated
// `legacy-symbols.csv` beside it. AC6_recomp contains 10,527 known function
// starts and current Xenon automatic discovery does not yet recover enough of
// them from this retail XEX on its own, so these revision-scoped starts are
// authoritative analysis hints rather than reference-only documentation.
//
// The CSV is still kept separate from hints.v2.json so the hand-auditable
// migration data remains compact and so a future Xenon analysis pass can
// reduce/remove redundant starts without rewriting the generic schema file.
// A missing/malformed/schema-invalid JSON or CSV file is a hard provide()
// failure, never a silent fallback to a smaller hint set.
class Ac6HintProvider final : public xenon::recomp::ModuleHintProviderV2 {
 public:
  // `revisions_root` defaults to this source tree's own revisions/
  // directory (set at configure time via the GRACEMERIA_REVISIONS_DIR
  // compile definition - see CMakeLists.txt), which is what gracemeria-recomp
  // and the unit tests use. Packaging Gracemeria as an installed module later
  // moves this default to the installed package's own revisions/ directory;
  // callers that already know a specific root (tests exercising a malformed
  // or missing file) pass it explicitly instead.
  explicit Ac6HintProvider(std::filesystem::path revisions_root = default_revisions_root());

  [[nodiscard]] bool provide(const xenon::xbox::XexEffectiveIdentity& identity,
                             xenon::recomp::analysis::AnalysisHintSetV2& out_hint_set,
                             std::string& error) const override;

 private:
  [[nodiscard]] static std::filesystem::path default_revisions_root();

  std::filesystem::path revisions_root_;
};

}  // namespace gracemeria
