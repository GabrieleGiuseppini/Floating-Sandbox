import codecs
import json
import os
import sys

MANDATORY_JSON_FIELD_NAMES = [
    "color_key",
    "name",
    "render_color",
    "texture_name"
    ]


def check_structural_materials_file(root_folder_path):

    structural_materials_path = os.path.join(root_folder_path, "materials_structural.json")
    with open(structural_materials_path, "rt") as json_file:  
        try:
            materials_db = json.load(json_file, "ascii")
        except Exception as e:
            print("ERROR: {}".format(str(e)))
            return

    for material in materials_db:

        if not 'name' in material:
            print("ERROR: missing 'name' field")
            continue

        material_name = material['name']

        for f in MANDATORY_JSON_FIELD_NAMES:
            if not f in material:
                if f != "texture_name" or 'is_legacy_electrical' not in material or material['is_legacy_electrical'] == False:
                    print("ERROR: {}: missing '{}' field".format(material_name, f))

        # Color key vs Render color
        color_key = material["color_key"]
        render_color = material["render_color"]
        if color_key != render_color:
            print("WARNING: {}: color_key '{}' != render_color '{}'".format(material_name, color_key, render_color))

    # TODOHERE

def main():

    root_folder_path = "../Data";

    if len(sys.argv) == 2:
        root_folder_path = sys.argv[1]
    elif len(sys.argv) > 2:
        print("Usage: verify_structural_materials.py [<path_to_structural_materials_folder>]")
        sys.exit(-1)

    # 
    # Check
    #

    check_structural_materials_file(root_folder_path)

main()
