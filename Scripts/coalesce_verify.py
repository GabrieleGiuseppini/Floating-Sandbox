import json
import re
import sys

def coalesce_verify(json_obj, selector_pattern):
 
    template_properties = None
    color_keys = []
    for material in json_obj:
        name = material["name"]
        result = re.search(selector_pattern, name, flags=re.I)
        if result:

            # Create core properties
            core_properties = {}
            for k,v in material.items():
                if k != "name" and k != "template" and k != "color_key" and k != "render_color":
                    core_properties[k] = v

            if not template_properties:
                print("{}: TEMPLATE".format(name))
                template_properties = core_properties
                color_keys.append(material["color_key"])
            else:
                if core_properties == template_properties:
                    if material["render_color"] == material["color_key"]:
                        print("{}: OK {}".format(name, material["color_key"]))
                        color_keys.append(material["color_key"])
                    else:
                        print("{}: NO RENDER COLOR".format(name))                        
                else:
                    print("{}: NOT MATCHING".format(name))

    print(', '.join('"' + c + '"' for c in color_keys))

def main():
    
    if len(sys.argv) != 3:
        print("Usage: coalesce_verify.py <input_json> <selector_pattern>")
        sys.exit(-1)

    with open(sys.argv[1], "r") as in_file:
        json_obj = json.load(in_file)

    coalesce_verify(json_obj, sys.argv[2])

main()
