import json
import sys

from collections import OrderedDict
 
def adjust_material(material):

    name = material["name"]

    if "Steel" in name:
        material["ignition_temperature"] = 1089.15
        material["melting_temperature"] = 1643.15
        material["thermal_conductivity"] = 50.2
        material["specific_heat"] = 490.0
    elif "Iron" in name:
        material["ignition_temperature"] = 1588.15
        material["melting_temperature"] = 1783.15
        material["thermal_conductivity"] = 79.5
        material["specific_heat"] = 449.0
    elif "Titanium" in name:
        material["ignition_temperature"] = 1473.15
        material["melting_temperature"] = 1941.15
        material["thermal_conductivity"] = 17.0
        material["specific_heat"] = 523.0
    elif "Aluminium" in name:
        material["ignition_temperature"] = 1000000.0
        material["melting_temperature"] = 933.45
        material["thermal_conductivity"] = 205.0
        material["specific_heat"] = 897.0
    elif "Wood" in name:
        material["ignition_temperature"] = 453.0
        material["melting_temperature"] = 1000000.0
        material["thermal_conductivity"] = 0.1
        material["specific_heat"] = 1800.0
    elif ("Glass" in name) or ("Lamp" in name):
        material["ignition_temperature"] = 1000000.0
        material["melting_temperature"] = 1773.15
        material["thermal_conductivity"] = 0.8
        material["specific_heat"] = 700.0
    elif ("Cloth" in name) or ("Rope" in name):
        material["ignition_temperature"] = 393.15
        material["melting_temperature"] = 1000000.0
        material["thermal_conductivity"] = 0.04
        material["specific_heat"] = 1400.0
    elif "Carbon" in name:
        material["ignition_temperature"] = 973.15
        material["melting_temperature"] = 3823.15
        material["thermal_conductivity"] = 1.7
        material["specific_heat"] = 717.0
    elif "Cardboard" in name:
        material["ignition_temperature"] = 491.15
        material["melting_temperature"] = 1000000.0
        material["thermal_conductivity"] = 0.05
        material["specific_heat"] = 1336.0
    elif "Rubber" in name:
        material["ignition_temperature"] = 553.15
        material["melting_temperature"] = 1000000.0
        material["thermal_conductivity"] = 0.1
        material["specific_heat"] = 1250.0
    elif "Fiberglass" in name:
        material["ignition_temperature"] = 1000000.0
        material["melting_temperature"] = 1394.15
        material["thermal_conductivity"] = 0.04
        material["specific_heat"] = 700.0
    elif ("(ABS)" in name) or ("Cable" in name):
        material["ignition_temperature"] = 689.15
        material["melting_temperature"] = 393.15
        material["thermal_conductivity"] = 0.25
        material["specific_heat"] = 1423.512
    elif ("Tin" in name) or ("Nails" in name):
        material["ignition_temperature"] = 1213.0
        material["melting_temperature"] = 505.05
        material["thermal_conductivity"] = 66.8
        material["specific_heat"] = 228.0
    elif "Air" in name:
        material["ignition_temperature"] = 1000000.0
        material["melting_temperature"] = 1000000.0
        material["thermal_conductivity"] = 0.0262
        material["specific_heat"] = 1005.0
    elif "Generator" in name: # Arbitrary: from Aluminium
        material["ignition_temperature"] = 1000000.0
        material["melting_temperature"] = 933.45
        material["thermal_conductivity"] = 205.0
        material["specific_heat"] = 897.0
    else:
        raise Exception("No rules for material '" + name + "'")

    material["combustion_type"] = "Combustion"

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
