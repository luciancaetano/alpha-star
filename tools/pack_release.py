"""Build the npm tarballs of a release locally (never publishes) and verify them.

Usage: python tools/pack_release.py [--install-test] [--require linux-x64|win32-x64|android-arm64 ...]

Writes dist/release/:
  alpha-star-sdk-<v>.tgz                 @alpha-star/sdk (CLI, types, templates, notices; no engines)
  alpha-star-engine-<target>-<v>.tgz     @alpha-star/engine-<target>: release + shipping engine
                                         from alpha-sdk/engine/<target>/ (os/cpu set: npm installs
                                         only the one for the user's machine, like esbuild)
  MANIFEST.txt                           every file, size and sha256

Engines are packed only when both flavors are staged and carry the same bytecode build id;
others are listed as skipped with the reason (--require TARGET makes that an error).
Checks: one version everywhere (CMake project, SDK package.json, its optionalDependencies,
CHANGELOG, `alpha_star --version` of this machine's engine), required files present (LICENSE,
third-party notices), no build junk, size budgets, engines byte-identical to the staged ones.
--install-test then runs alpha-sdk/test/release.test.mjs: installs the tarballs into a temp
project from local files and runs alpha init + alpha test + alpha export --standalone there.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

import harness_config as cfg
import resource_guard

SDK = cfg.ROOT / "alpha-sdk"
OUT = cfg.ROOT / "dist" / "release"
TARGETS = {"win32-x64": ("win32", "x64", "alpha_star.exe"), "linux-x64": ("linux", "x64", "alpha_star")}
# Shader compilers next to the release engine (tools/stage_engine.py SHADER_TOOLS): GLSL -> SPIR-V -> MSL.
SHADER_TOOLS = {"win32-x64": ["glslang.exe", "shadercross.exe"], "linux-x64": ["glslang", "shadercross"]}
ANDROID = "android-arm64"  # prebuilt APK engine (tools/stage_android.py), installed on every OS
# npm provenance requires repository.url to match the GitHub repo that builds the package.
REPOSITORY = {"type": "git", "url": "git+https://github.com/luciancaetano/alpha-star.git"}
ANDROID_FLAVORS, ANDROID_LIBS = ("debug", "shipping"), ("libmain.so", "libSDL3.so")
ALL_TARGETS = [*TARGETS, ANDROID]
JUNK = re.compile(r"\.(pdb|ilk|obj|o|lib|exp|log|tmp|map)$|node_modules/|CMakeFiles|__pycache__|\.DS_Store"
                  r"|^package/(test|engine|scripts)/")
BUDGET_MB = {"sdk": 8, "engine": 40}
BUILD_ID = re.compile(rb"[0-9a-f]{12}-(?:32|64)")


def npm(args, cwd):
    run = subprocess.run([shutil.which("npm") or "npm", *args], cwd=cwd, capture_output=True, text=True)
    if run.returncode != 0:
        sys.exit(f"pack_release: npm {' '.join(args)} failed in {cwd}:\n{run.stderr}")
    return run.stdout


def packed(folder):
    """npm pack of a staged package folder into dist/release/; the tarball's path."""
    return OUT / npm(["pack", "--pack-destination", str(OUT), "--ignore-scripts"], folder).strip().splitlines()[-1]


def version_problems(version):
    sdk = json.loads((SDK / "package.json").read_text(encoding="utf-8"))
    cmake = re.search(r"project\(alpha_star VERSION (\S+)", (cfg.ROOT / "CMakeLists.txt").read_text()).group(1)
    problems = [] if cmake == version else [f"CMakeLists.txt project VERSION {cmake} != SDK {version}"]
    for target in ALL_TARGETS:
        dep = sdk.get("optionalDependencies", {}).get(f"@alpha-star/engine-{target}")
        if dep != version:
            problems.append(f"SDK optionalDependencies @alpha-star/engine-{target} is {dep!r}, must be exactly {version}")
    if f"## [{version}]" not in (cfg.ROOT / "CHANGELOG.md").read_text(encoding="utf-8"):
        problems.append(f"CHANGELOG.md has no '## [{version}]' section")
    return problems


def engine_state(target, version):
    """(release, shipping, None) when packable, else (None, None, reason)."""
    exe = TARGETS[target][2]
    release, shipping = SDK / "engine" / target / exe, SDK / "engine" / target / "shipping" / exe
    if not release.is_file() or not shipping.is_file():
        return None, None, "release and shipping are not both staged (tools/stage_engine.py)"
    missing = [t for t in SHADER_TOOLS[target] if not (release.parent / t).is_file()]
    if missing:
        return None, None, f"{', '.join(missing)} not staged next to the release engine (tools/stage_engine.py)"
    ids = set(BUILD_ID.findall(release.read_bytes())) & set(BUILD_ID.findall(shipping.read_bytes()))
    if not ids:
        return None, None, "release and shipping share no bytecode build id (stale or mixed staging; restage)"
    if target == host_target():
        said = subprocess.run([str(release), "--version"], capture_output=True, text=True).stdout.strip()
        if said != f"alpha-star {version}":
            return None, None, f"engine --version says {said!r}, SDK is {version}"
    return release, shipping, None


def compiled_key(stage):
    """The game.jsbc key id of this machine's staged release engine (what `alpha export` encrypts with)."""
    import stage_engine
    engine = SDK / "engine" / stage_engine.target() / TARGETS.get(stage_engine.target(), ("", "", "alpha_star"))[2]
    src, out = stage / "key.js", stage / "key.jsbc"
    src.write_text("export default 1;\n", encoding="utf-8")
    if not engine.is_file() or subprocess.run([str(engine), "--compile", str(src), str(out)], capture_output=True).returncode:
        return None
    return out.read_bytes()[16:32].rstrip(b"\0")


def android_state(stage):
    """(folder, None) when the Android engine is packable, else (None, reason). Its libmain.so files must
    hold the key id the desktop release engines compile with: else every exported APK refuses its game."""
    root = SDK / "engine" / ANDROID
    libs = [root / f / lib for f in ANDROID_FLAVORS for lib in ANDROID_LIBS]
    if not all(p.is_file() for p in libs):
        return None, "debug and shipping libraries are not both staged (tools/stage_android.py)"
    mains = [(root / f / "libmain.so").read_bytes() for f in ANDROID_FLAVORS]
    if not set(BUILD_ID.findall(mains[0])) & set(BUILD_ID.findall(mains[1])):
        return None, "debug and shipping share no bytecode build id (stale or mixed staging; restage)"
    key = compiled_key(stage)
    if key is None:
        return None, "no release engine on this machine to check the game.jsbc key against"
    if not all(key in m for m in mains):
        return None, (f"libmain.so lacks the release engine's game.jsbc key id {key.decode()}: build every engine "
                      "with one ALPHA_JSBC_SECRET")
    return root, None


def android_package(version, root, stage):
    dest = stage / f"engine-{ANDROID}"
    shutil.copytree(root, dest, ignore=shutil.ignore_patterns("*.tmp"))
    shutil.copy2(cfg.ROOT / "LICENSE", dest / "LICENSE")
    (dest / "THIRD_PARTY_NOTICES.txt").write_text(notices_text(), encoding="utf-8")
    (dest / "README.md").write_text(
        f"# @alpha-star/engine-{ANDROID}\n\nThe prebuilt alpha-star engine for Android (arm64-v8a), used by "
        f"`alpha export --target {ANDROID}` (optional dependency of `@alpha-star/sdk`, exact version {version}; "
        "installed on every OS, since APKs build anywhere).\n\n- `debug/`: debug APKs (dev tools)\n"
        "- `shipping/`: release APKs and AABs\n", encoding="utf-8")
    files = [f"{f}/{lib}" for f in ANDROID_FLAVORS for lib in ANDROID_LIBS]
    pkg = {"name": f"@alpha-star/engine-{ANDROID}", "version": version, "license": "MIT", "repository": REPOSITORY,
           "description": "alpha-star prebuilt Android engine (arm64-v8a) for @alpha-star/sdk",
           "files": [*files, "LICENSE", "THIRD_PARTY_NOTICES.txt", "README.md"]}
    (dest / "package.json").write_text(json.dumps(pkg, indent=2) + "\n", encoding="utf-8")
    return dest, {f: root / f for f in files}


def host_target():
    import stage_engine
    return stage_engine.target()


def notices_text():
    parts = ["alpha-star engine: third-party software it includes. Each part keeps its own license."]
    parts += [p.read_text(encoding="utf-8").strip() for p in sorted((SDK / "notices").glob("*.txt"))]
    return "\n\n\n".join(parts) + "\n"


def engine_package(target, version, release, shipping, stage):
    os_name, cpu, exe = TARGETS[target]
    dest = stage / f"engine-{target}"
    (dest / "shipping").mkdir(parents=True)
    shutil.copy2(release, dest / exe)
    shutil.copy2(shipping, dest / "shipping" / exe)
    for tool in SHADER_TOOLS[target]:
        shutil.copy2(release.parent / tool, dest / tool)
    shutil.copy2(cfg.ROOT / "LICENSE", dest / "LICENSE")
    (dest / "THIRD_PARTY_NOTICES.txt").write_text(notices_text(), encoding="utf-8")
    (dest / "README.md").write_text(
        f"# @alpha-star/engine-{target}\n\nThe alpha-star engine for {target}, installed by `@alpha-star/sdk` "
        f"(optional dependency, exact version {version}).\n\n- `{exe}`: release engine (`alpha run`, `alpha test`, "
        f"`alpha mcp`)\n- `{SHADER_TOOLS[target][0]}`, `{SHADER_TOOLS[target][1]}`: shader compilers (`alpha export`: GLSL -> SPIR-V -> MSL)\n- `shipping/{exe}`: player engine (`alpha export --standalone`)\n", encoding="utf-8")
    pkg = {"name": f"@alpha-star/engine-{target}", "version": version, "license": "MIT", "repository": REPOSITORY,
           "description": f"alpha-star engine binaries for {target} (used by @alpha-star/sdk)",
           "os": [os_name], "cpu": [cpu], "preferUnplugged": True,
           "files": [exe, *SHADER_TOOLS[target], f"shipping/{exe}", "LICENSE", "THIRD_PARTY_NOTICES.txt", "README.md"]}
    (dest / "package.json").write_text(json.dumps(pkg, indent=2) + "\n", encoding="utf-8")
    return dest


def sdk_package(stage):
    """A copy of exactly what `npm pack` would take from alpha-sdk/, plus the repo LICENSE."""
    dest = stage / "sdk"
    listing = json.loads(npm(["pack", "--dry-run", "--json", "--ignore-scripts"], SDK))[0]["files"]
    for entry in listing:
        target = dest / entry["path"]
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(SDK / entry["path"], target)
    shutil.copy2(cfg.ROOT / "LICENSE", dest / "LICENSE")
    return dest


def members(tgz):
    with tarfile.open(tgz) as tar:
        return {m.name: m.size for m in tar.getmembers() if m.isfile()}


def tarball_problems(tgz, required, budget_mb, identical):
    files = members(tgz)
    problems = [f"missing {r}" for r in required if f"package/{r}" not in files]
    problems += [f"build junk {n}" for n in files if JUNK.search(n)]
    size = sum(files.values()) / 2 ** 20
    if size > budget_mb:
        problems.append(f"{size:.1f} MB unpacked > budget {budget_mb} MB")
    with tarfile.open(tgz) as tar:
        for name, source in identical.items():
            data = tar.extractfile(f"package/{name}").read() if f"package/{name}" in files else b""
            if data != source.read_bytes():
                problems.append(f"{name} differs from {cfg.rel(source)}")
    return [f"{tgz.name}: {p}" for p in problems]


def manifest(tarballs):
    lines = []
    for tgz in tarballs:
        lines.append(f"{tgz.name}  {tgz.stat().st_size} bytes  sha256 {hashlib.sha256(tgz.read_bytes()).hexdigest()}")
        lines += [f"  {size:>10}  {name}" for name, size in sorted(members(tgz).items())]
    (OUT / "MANIFEST.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")


def pack(stage, version, require):
    tarballs, problems, skipped = [], version_problems(version), []
    sdk_required = ["package.json", "README.md", "LICENSE", "bin/alpha.mjs", "lib/engine.mjs", "types/alpha.d.ts",
                    "templates/blank/package.json", "skills/alpha-harness/alpha-harness.md"] +[f"notices/{p.name}" for p in (SDK / "notices").glob("*.txt")]
    sdk = packed(sdk_package(stage))
    tarballs.append(sdk)
    problems += tarball_problems(sdk, sdk_required, BUDGET_MB["sdk"], {})
    for target in TARGETS:
        release, shipping, reason = engine_state(target, version)
        if reason:
            skipped.append(f"{target}: {reason}")
            if target in require:
                problems.append(f"required engine {target} not packable: {reason}")
            continue
        exe = TARGETS[target][2]
        tgz = packed(engine_package(target, version, release, shipping, stage))
        tarballs.append(tgz)
        problems += tarball_problems(tgz, ["package.json", "README.md", "LICENSE", "THIRD_PARTY_NOTICES.txt"],
                                     BUDGET_MB["engine"], {exe: release, f"shipping/{exe}": shipping})
    root, reason = android_state(stage)
    if reason:
        skipped.append(f"{ANDROID}: {reason}")
        if ANDROID in require:
            problems.append(f"required engine {ANDROID} not packable: {reason}")
    else:
        dest, identical = android_package(version, root, stage)
        tgz = packed(dest)
        tarballs.append(tgz)
        problems += tarball_problems(tgz, ["package.json", "README.md", "LICENSE", "THIRD_PARTY_NOTICES.txt"],
                                     BUDGET_MB["engine"], identical)
    return tarballs, problems, skipped


def main(argv):
    resource_guard.limit()
    ap = argparse.ArgumentParser()
    ap.add_argument("--install-test", action="store_true")
    ap.add_argument("--require", action="append", default=[], choices=sorted(ALL_TARGETS))
    a = ap.parse_args(argv)
    version = json.loads((SDK / "package.json").read_text(encoding="utf-8"))["version"]
    shutil.rmtree(OUT, ignore_errors=True)
    stage = OUT / "stage"
    stage.mkdir(parents=True)
    try:
        tarballs, problems, skipped = pack(stage, version, a.require)
    finally:
        shutil.rmtree(stage, ignore_errors=True)
    manifest(tarballs)
    for t in tarballs:
        print(f"pack_release: {cfg.rel(t)} ({t.stat().st_size / 2 ** 20:.1f} MB packed, {len(members(t))} files)")
    for s in skipped:
        print(f"pack_release: skipped {s}")
    for p in problems:
        print(f"pack_release: FAIL {p}")
    if problems:
        return 1
    if a.install_test:
        run = subprocess.run(["node", "--test", "test/release.test.mjs"], cwd=SDK,
                             env={**os.environ, "ALPHA_RELEASE_DIR": str(OUT)})
        if run.returncode != 0:
            return 1
    print("pack_release: PASS (nothing published)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
