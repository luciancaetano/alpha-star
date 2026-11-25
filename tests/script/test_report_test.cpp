#include <doctest/doctest.h>

#if ALPHA_DEBUG_TOOLS  // `alpha_star --test` exists only in dev/release builds

#include "host_fixture.hpp"
#include "script/test_report.hpp"

using alpha::test::Sandbox;

TEST_CASE("run_test_file: exit 0 only when there are tests and all pass") {
  Sandbox box;
  CHECK(alpha::script::run_test_file(box.write("ok.js", "export function a() {}\nexport function b() {}\n"), "") == 0);
  CHECK(alpha::script::run_test_file(
            box.write("bad.js", "export function a() {}\nexport function b() { throw new Error('boom\\nline 2\\n'); }\n"),
            "") == 1);
  CHECK(alpha::script::run_test_file(box.write("none.js", "export const x = 1;\n"), "") == 1);
  CHECK(alpha::script::run_test_file((box.dir / "missing.js").string(), "") == 1);
}

#endif
