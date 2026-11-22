#!/usr/bin/env python3
import json
import os
import re
import shutil
import sys

IMAGE_KEYS = ("structure_image", "texture_image", "ropes_image", "electrical_image")
SHIP_EXTENSIONS = (".shp2", ".shp", ".png")


def read_text(path):
    with open(path, "rb") as handle:
        return handle.read().decode("utf-8-sig", errors="replace")


def inspect_legacy_ship(path):
    text = read_text(path)
    name = None
    references = []

    try:
        data = json.loads(text)
        if isinstance(data, dict):
            if isinstance(data.get("ship_name"), str):
                name = data["ship_name"]
            for key in IMAGE_KEYS:
                if isinstance(data.get(key), str):
                    references.append(data[key])
            return name, references
    except ValueError:
        pass

    match = re.search(r'"ship_name"\s*:\s*"((?:[^"\\]|\\.)*)"', text)
    if match:
        try:
            name = json.loads('"' + match.group(1) + '"')
        except ValueError:
            name = match.group(1)

    for key in IMAGE_KEYS:
        match = re.search(r'"' + key + r'"\s*:\s*"((?:[^"\\]|\\.)*)"', text)
        if match:
            references.append(match.group(1))

    return name, references


def copy_if_changed(source, destination):
    if os.path.exists(destination):
        source_stat = os.stat(source)
        destination_stat = os.stat(destination)
        if source_stat.st_size == destination_stat.st_size and destination_stat.st_mtime >= source_stat.st_mtime:
            return
    shutil.copy2(source, destination)


def main():
    if len(sys.argv) != 3:
        print("usage: make_ship_manifest.py SHIPS_DIR OUT_DIR", file=sys.stderr)
        return 2

    ships_dir = os.path.abspath(sys.argv[1])
    out_dir = os.path.abspath(sys.argv[2])
    out_ships_dir = os.path.join(out_dir, "Ships")
    os.makedirs(out_ships_dir, exist_ok=True)

    available = set(os.listdir(ships_dir))
    entries = []
    needed = set()
    skipped = 0

    for file_name in sorted(available, key=str.lower):
        stem, extension = os.path.splitext(file_name)
        if extension.lower() not in SHIP_EXTENSIONS:
            continue

        display_name = stem
        dependencies = []

        if extension.lower() == ".shp":
            declared_name, references = inspect_legacy_ship(os.path.join(ships_dir, file_name))
            if declared_name:
                display_name = declared_name
            missing = [reference for reference in references if reference not in available]
            if missing:
                print("skipping %s: missing %s" % (file_name, ", ".join(missing)), file=sys.stderr)
                skipped += 1
                continue
            dependencies = references

        size = os.path.getsize(os.path.join(ships_dir, file_name))
        for dependency in dependencies:
            size += os.path.getsize(os.path.join(ships_dir, dependency))

        needed.add(file_name)
        needed.update(dependencies)

        entries.append({
            "name": display_name,
            "file": file_name,
            "format": extension.lower().lstrip("."),
            "deps": dependencies,
            "size": size,
        })

    for file_name in sorted(needed):
        copy_if_changed(os.path.join(ships_dir, file_name), os.path.join(out_ships_dir, file_name))

    entries.sort(key=lambda entry: (entry["name"].lower(), entry["file"].lower()))

    with open(os.path.join(out_dir, "ships.json"), "w", encoding="utf-8") as handle:
        json.dump({"ships": entries}, handle, ensure_ascii=False, separators=(",", ":"))

    print("ships.json: %d ships (%d skipped), %d files copied to %s" % (len(entries), skipped, len(needed), out_ships_dir))
    return 0


if __name__ == "__main__":
    sys.exit(main())
