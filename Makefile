# alpha-star build shortcuts (wraps CMake presets).
# Machine-specific paths (Visual Studio, debugger, exe location) are resolved
# by tools/toolchain.py, so nothing here is tied to one PC.

PYTHON ?= python
FLAPPY := examples/flappy
# Game pack the run/debug targets open (override: make run-debug GAME=path/to/pack).
GAME ?= $(FLAPPY)/dist

.PHONY: build-debug build-prod run-debug debug flappy flappy-build sdk-engine sdk-engine-linux linux-gate

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

# Full gate on Linux (podman/docker container).
linux-gate:
	$(PYTHON) tools/linux_gate.py
