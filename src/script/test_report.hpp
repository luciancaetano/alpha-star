#pragma once

#include <string>

// `alpha_star --test <tests.js>` (ALPHA_DEBUG_TOOLS only; not compiled otherwise).
namespace alpha::script {

// Runs every exported test function of the module headlessly and prints one
// PASS/FAIL line each (errors indented under it) and a summary. Exit code:
// 0 iff there were tests and all passed.
int run_test_file(const std::string& path, const std::string& asset_dir);

}  // namespace alpha::script
