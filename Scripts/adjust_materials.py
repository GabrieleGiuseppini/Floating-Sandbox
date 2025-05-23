import json
import sys

from collections import OrderedDict
 
def adjust_material(material):

    """
    val = 0.0
    if "palette_coordinates" in material:
        cat = material["palette_coordinates"]["category"]
        if cat == "Cloth" or cat == "Cardboard and Paper" or cat == "Ropes" or cat == "Chains" or cat == "Rubber Bands":
            val = 1.0
            
    material["laser_ray_cut_receptivity"] = val
    """

    if "friction_static_coefficient" in material and material["friction_static_coefficient"] == 0.25 \
        and "friction_kinetic_coefficient" in material and material["friction_kinetic_coefficient"] == 0.25:
        material["friction_static_coefficient"] = 1.25
        material["friction_kinetic_coefficient"] = 0.69

def main():
    
    if len(sys.argv) != 3:
        print("Usage: adjust_materials.py <input_json> <output_json>")
        sys.exit(-1)

    with open(sys.argv[1], "r") as in_file:
        json_obj = json.load(in_file)

    for material in json_obj["materials"]:
        adjust_material(material)

    with open(sys.argv[2], "w") as out_file:
        out_file.write(json.dumps(json_obj, indent=4, sort_keys=True))

main()
