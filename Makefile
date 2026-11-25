# alpha-star build shortcuts (wraps CMake presets).
# Machine-specific paths (Visual Studio, debugger, exe location) are resolved
# by tools/toolchain.py, so nothing here is tied to one PC.

PYTHON ?= python
FLAPPY := examples/flappy
# Game module the run/debug targets open (override: make run-debug GAME=path/to/main.js).
GAME ?= $(FLAPPY)/dist/main.js

.PHONY: build-debug build-prod run-debug debug flappy flappy-build

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

# Compile the Flappy example (TypeScript -> one JS module) with the alpha SDK.
flappy-build:
	cd $(FLAPPY) && npm install --silent && npm run build

# Build engine + example, then play Flappy.
flappy: build-debug flappy-build
	$(PYTHON) tools/run.py --preset dev $(FLAPPY)/dist/main.js
