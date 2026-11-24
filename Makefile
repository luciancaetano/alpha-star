# alpha-star build shortcuts (wraps CMake presets).
# Machine-specific paths (Visual Studio, debugger, exe location) are resolved
# by tools/toolchain.py, so nothing here is tied to one PC.

PYTHON ?= python

.PHONY: build-debug build-prod run-debug debug

build-debug:
	cmake --preset dev
	cmake --build --preset dev

build-prod:
	cmake --preset release
	cmake --build --preset release

run-debug: build-debug
	$(PYTHON) tools/run.py --preset dev

debug: build-debug
	$(PYTHON) tools/run.py --preset dev --debug
