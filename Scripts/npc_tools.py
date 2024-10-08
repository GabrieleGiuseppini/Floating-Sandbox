from copy import deepcopy
import json
from pathlib import Path
from PIL import Image
import sys
from typing import Optional, Tuple


def load_json(filepath):
    with open(filepath, "r") as in_file:
        try:
            return json.load(in_file)
        except Exception as e:
            print("ERROR: {}".format(str(e)))
            sys.exit(-1)


def save_json(json_obj, filepath):
    with open(filepath, "w") as out_file:
        out_file.write(json.dumps(json_obj, indent=4))


def rgb_to_hex(rgb_tuple: Tuple[float, float, float]):
    return ("#%02x%02x%02x" % tuple(int(x) for x in rgb_tuple)).upper()


def get_average_color(root_path: Path, filename: str) -> Optional[Tuple[float, float, float]]:
    for child in root_path.iterdir():
        texture_filepath = child / (filename + ".png")
        if texture_filepath.exists():

            with Image.open(texture_filepath) as im:
                px_data = im.getdata()
            
            avg = (0, 0, 0)
            cnt = 0
            for p in px_data:
                if p[3] != 0:
                    avg = (avg[0] + p[0], avg[1] + p[1], avg[2] + p[2])
                    cnt += 1
            if cnt > 0:
                return (avg[0] / cnt, avg[1] / cnt, avg[2] / cnt)
            else:
                return None

    raise Exception(f"Cannot find {filename}")


def add_render_colors(json_filepath, npc_asset_root_directory_path):

    visited_cnt = 0
    updated_cnt = 0
   
    # Load

    root_obj = load_json(json_filepath)

    # Humans

    for kind in root_obj["humans"]["kinds"]:

        visited_cnt += 1

        kind_avg_color = None
        kind_cnt = 0
        for _, v in kind["texture_filename_stems"].items():
            texture_avg_color = get_average_color(Path(npc_asset_root_directory_path), v)
            if texture_avg_color is not None:
                if kind_avg_color is None:
                    kind_avg_color = texture_avg_color
                else:
                    kind_avg_color = (texture_avg_color[0] + kind_avg_color[0], texture_avg_color[1] + kind_avg_color[1], texture_avg_color[2] + kind_avg_color[2])
                kind_cnt += 1

        if texture_avg_color is not None:
            kind["render_color"] = rgb_to_hex(texture_avg_color)
            updated_cnt += 1

    # Furniture
    
    for kind in root_obj["furniture"]["kinds"]:
        
        visited_cnt += 1

        texture_avg_color = get_average_color(Path(npc_asset_root_directory_path), kind["texture_filename_stem"])

        if texture_avg_color is not None:
            kind["render_color"] = rgb_to_hex(texture_avg_color)
            updated_cnt += 1

    # Save

    output_json_path = Path(".") / Path(json_filepath).name 
    save_json(root_obj, output_json_path);

    print(f"Completed: {visited_cnt} visited, {updated_cnt} updated")


def print_usage():
    print("Usage: npc_tools.py add_render_colors <npc_json_filepath> <npc_asset_root_directory_path>")


def main():
    
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(-1)

    verb = sys.argv[1]
    if verb == 'add_render_colors':
        if len(sys.argv) != 4:
            print_usage()
            sys.exit(-1)
        add_render_colors(sys.argv[2], sys.argv[3])
    else:
        print("ERROR: Unrecognized verb '{}'".format(verb))
        print_usage()

main()
