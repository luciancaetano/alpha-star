"""Run the gate (or any command) inside the Linux build container.

Usage: python tools/linux_gate.py                 # full gate on Linux (preset dev)
       python tools/linux_gate.py -- <command...>  # run a command in the container

Needs podman (or docker). The repo is mounted at /src; the Linux build lives in
a named volume over /src/build, so it never mixes with the host's build/.
Host networking: the podman machine's default container network may have no
route out (apt/git time out), while the VM itself does.
"""
import shutil
import subprocess
import os
import sys

import harness_config as cfg

IMAGE = "alpha-star-linux"
BUILD_VOLUME = "alpha-star-linux-build"


def engine():
    for name in ("podman", "docker"):
        if shutil.which(name):
            return name
    sys.exit("linux_gate: needs podman or docker")


def main(argv):
    tool = engine()
    build = [tool, "build", "--network=host", "-t", IMAGE, "-f", "tools/linux/Containerfile", "."]
    if subprocess.run(build, cwd=cfg.ROOT).returncode != 0:
        return 1
    command = argv[argv.index("--") + 1:] if "--" in argv else ["python3", "tools/gate.py"]
    # Capped so a gate never takes over the host: 8 CPUs, 8 parallel compile jobs.
    os.environ.setdefault("CMAKE_BUILD_PARALLEL_LEVEL", "8")
    run = [tool, "run", "--rm", "--init", "--network=host", "--cpus", "8", "-e", "CMAKE_BUILD_PARALLEL_LEVEL", "-v", f"{cfg.ROOT}:/src", "-v", f"{BUILD_VOLUME}:/src/build",
           IMAGE, "xvfb-run", "-a", *command]
    return subprocess.run(run, cwd=cfg.ROOT).returncode


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
