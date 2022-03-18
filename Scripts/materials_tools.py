import json
import sys
 

MANDATORY_JSON_FIELD_NAMES = [
    "color_key",
    "name",
    ]


def load_json(filename):
    with open(filename, "r") as in_file:
        try:
            return json.load(in_file)
        except Exception as e:
            print("ERROR: {}".format(str(e)))
            sys.exit(-1)


def save_json(json_obj, filename):
    with open(filename, "w") as out_file:
        out_file.write(json.dumps(json_obj, indent=4, sort_keys=True))


def get_color_set(json_obj):
    colors = set()
    for material in json_obj["materials"]:

        # Normalize colors
        color_keys = material["color_key"]
        if not isinstance(color_keys, list):
            color_keys = [color_keys]

        # Build set
        for color in color_keys:
            if color in colors:
                print("ERROR: material '{}' has color '{}' which is a duplicate".format(material["name"], color))
            else:
                colors.add(color)

    return colors


def verify(filename):
    json_obj = load_json(filename)

    # Verify colors
    get_color_set(json_obj)

    for material in json_obj["materials"]:
        if not 'name' in material:
            print("ERROR: missing 'name' field")
            continue

        material_name = material['name']

        # Verify mandatory fields
        for f in MANDATORY_JSON_FIELD_NAMES:
            if not f in material:
                if f != "texture_name" or 'is_legacy_electrical' not in material or material['is_legacy_electrical'] == False:
                    print("ERROR: {}: missing '{}' field".format(material_name, f))


def add_variants(material_name_stem, input_filename, output_filename):
    json_obj = load_json(input_filename)

    # Scoop up variants
    existing_variants = []
    for material in json_obj["materials"]:
        material_name_parts = material["name"].split(' ')
        assert(len(material_name_parts) >= 1)
        if material_name_parts[0] == material_name_stem:
            existing_variants.append(material)

    if len(existing_variants) == 0:
        print("ERROR: cannot find any variants for material stem '{}'".format(material_name_stem))
        sys.exit(-1)

    print("Found {} variants: {}".format(len(existing_variants), ", ".join(m["name"] for m in existing_variants)))

    # TODOHERE


def print_usage():
    print("Usage: materials_tools.py verify <input_json>")
    print("Usage: materials_tools.py add_variants <material_name> <input_json> <output_json>")


def main():
    
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(-1)

    verb = sys.argv[1]
    if verb == 'verify':
        if len(sys.argv) != 3:
            print_usage()
            sys.exit(-1)
        verify(sys.argv[2])
    elif verb == 'add_variants':
        if len(sys.argv) != 5:
            print_usage()
            sys.exit(-1)
        add_variants(sys.argv[2], sys.argv[3], sys.argv[4])
    else:
        print("ERROR: Unrecognized verb '{}'".format(verb))
        print_usage()

main()
