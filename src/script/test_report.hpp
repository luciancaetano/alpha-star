#pragma once

#include <string>

#include "core/cli.hpp"

// `alpha_star --test <tests.js>` (ALPHA_DEBUG_TOOLS only; not compiled otherwise).
namespace alpha::script {

// Runs every exported test function of the module headlessly and prints one
// PASS/FAIL line each (errors indented under it) and a summary. Exit code:
// 0 iff there were tests and all passed.
int run_test_file(const std::string& path, const std::string& asset_dir);

// The whole-command modes: --version, --test, --compile. Their exit code, or -1 when
// the options ask to run a game instead.
int run_command(const core::CliOptions& options);

}  // namespace alpha::script
