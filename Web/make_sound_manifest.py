#!/usr/bin/env python3
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

SOUND_BITRATE = "40k"
MUSIC_BITRATE = "56k"


def transcode(source, destination, bitrate, channels):
    if os.path.exists(destination) and os.path.getmtime(destination) >= os.path.getmtime(source):
        return True
    command = [
        "ffmpeg", "-y", "-loglevel", "error", "-i", source,
        "-vn", "-ac", str(channels), "-ar", "48000",
        "-c:a", "libopus", "-b:a", bitrate, "-application", "audio",
        destination,
    ]
    return subprocess.run(command).returncode == 0


def process_directory(source_dir, out_dir, bitrate, channels):
    os.makedirs(out_dir, exist_ok=True)
    jobs = []
    for file_name in sorted(os.listdir(source_dir)):
        stem, extension = os.path.splitext(file_name)
        if extension.lower() not in (".flac", ".ogg", ".wav", ".mp3"):
            continue
        jobs.append((stem, os.path.join(source_dir, file_name), os.path.join(out_dir, stem + ".ogg")))

    with ThreadPoolExecutor(max_workers=os.cpu_count() or 2) as pool:
        results = list(pool.map(lambda job: transcode(job[1], job[2], bitrate, channels), jobs))

    groups = {}
    for (stem, _, _), ok in zip(jobs, results):
        if not ok:
            print("failed: " + stem, file=sys.stderr)
            continue
        match = re.match(r"^(.*)_(\d+)$", stem)
        base = match.group(1) if match else stem
        groups.setdefault(base, []).append(stem)
    return groups


def main():
    if len(sys.argv) != 4:
        print("usage: make_sound_manifest.py SOUNDS_DIR MUSIC_DIR OUT_DIR", file=sys.stderr)
        return 2

    if shutil.which("ffmpeg") is None:
        print("ffmpeg not found, skipping sound generation", file=sys.stderr)
        return 0

    sounds_dir, music_dir, out_dir = [os.path.abspath(p) for p in sys.argv[1:4]]
    sounds = process_directory(sounds_dir, os.path.join(out_dir, "Sounds"), SOUND_BITRATE, 1)
    music = process_directory(music_dir, os.path.join(out_dir, "Music"), MUSIC_BITRATE, 2)

    with open(os.path.join(out_dir, "sounds.json"), "w", encoding="utf-8") as handle:
        json.dump({"sounds": sounds, "music": music}, handle, separators=(",", ":"))

    print("sounds.json: %d sounds, %d music groups" % (len(sounds), len(music)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
