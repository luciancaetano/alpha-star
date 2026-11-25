# alpha-star build shortcuts (wraps CMake presets).
# Machine-specific paths (Visual Studio, debugger, exe location) are resolved
# by tools/toolchain.py, so nothing here is tied to one PC.

PYTHON ?= python
FLAPPY := examples/flappy
# Game pack the run/debug targets open (override: make run-debug GAME=path/to/pack).
GAME ?= $(FLAPPY)/dist

.PHONY: build-debug build-prod run-debug debug flappy flappy-build sdk-engine sdk-engine-linux deploy-engine linux-gate gate test

# Full Windows gate (build, tests, coverage, limits, file access, CRAP, duplication); resource-guarded.
gate:
	$(PYTHON) tools/gate.py

# Tests only (dev preset, after build-debug), under the resource guard.
test:
	$(PYTHON) tools/resource_guard.py -- ctest --preset dev

build-debug:
	cmake --preset dev
	cmake --build --preset dev

build-prod:
	cmake --preset release
	cmake --build --preset release

run-debug: build-debug flappy-build
	$(PYTHON) tools/run.py --preset dev $(GAME)

debug: build-debug flappy-build
	$(PYTHON) tools/run.py --preset dev --debug $(GAME)

# Export the Flappy example as a pack (main.js + assets) with the alpha CLI.
flappy-build:
	cd $(FLAPPY) && npm install --silent && npm run export

# Build engine + example, then play Flappy.
flappy: build-debug flappy-build
	$(PYTHON) tools/run.py --preset dev $(FLAPPY)/dist

# Stage release engines into alpha-sdk/engine/<platform>-<arch> (shipped with the SDK).
sdk-engine:
	$(PYTHON) tools/stage_engine.py

sdk-engine-linux:
	$(PYTHON) tools/stage_engine.py --linux

# Prepare alpha-sdk/ for use and release in one step: build + stage the release/shipping engines
# for this OS and for Linux (Linux ones in the podman/docker container), mirror the examples into
# templates, copy skills/ in, regenerate the docs site, check the third-party notices, then run
# the SDK tests (they verify templates and docs are in sync).
deploy-engine: sdk-engine sdk-engine-linux
	npm --prefix alpha-sdk ci
	npm --prefix alpha-sdk run templates
	npm --prefix alpha-sdk run prepack
	npm --prefix alpha-sdk run docs
	$(PYTHON) tools/make_notices.py --check
	npm --prefix alpha-sdk test

# Full gate on Linux (podman/docker container): for the dedicated Linux PC, not the Windows dev PC.
linux-gate:
	$(PYTHON) tools/linux_gate.py
