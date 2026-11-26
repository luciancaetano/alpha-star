"""Encodes the frames of tools/record-showcase.mjs into the docs video (no audio):
`python -I tools/encode-showcase.py <framesDir> <outDir>` writes showcase.mp4 (H.264, plays in
every browser) and showcase.png (poster). Uses the ffmpeg binary bundled with `pip install imageio-ffmpeg`.
Kept at capture size (768x432): the page scales it with image-rendering: pixelated."""
import os
import shutil
import subprocess
import sys

import imageio_ffmpeg


def main():
    frames, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
    src = ["-y", "-loglevel", "error", "-framerate", "30", "-i", os.path.join(frames, "f%05d.png"),
           "-an", "-pix_fmt", "yuv420p"]
    subprocess.run([ffmpeg, *src, "-c:v", "libx264", "-preset", "slow", "-crf", "30", "-movflags", "+faststart",
                    os.path.join(out, "showcase.mp4")], check=True)
    names = sorted(os.listdir(frames))
    shutil.copy(os.path.join(frames, names[len(names) // 2]), os.path.join(out, "showcase.png"))
    for name in ("showcase.mp4", "showcase.png"):
        print(name, os.path.getsize(os.path.join(out, name)) // 1024, "KB")


if __name__ == "__main__":
    sys.exit(main())
