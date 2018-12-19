import json
import sys

from collections import OrderedDict
 
def adjust_material(material):

    material["water_diffusion_speed"] = 0.5

    pass


def main():
    
    if len(sys.argv) != 3:
        print("Usage: adjust_materials.py <input_json> <output_json>")
        sys.exit(-1)

    with open(sys.argv[1], "r") as in_file:
        json_obj = json.load(in_file)

    for material in json_obj:
        adjust_material(material)

    with open(sys.argv[2], "w") as out_file:
        out_file.write(json.dumps(json_obj, indent=4, sort_keys=True))

main()
