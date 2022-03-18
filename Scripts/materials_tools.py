import json
import sys
 

MANDATORY_JSON_FIELD_NAMES = [
    "color_key",
    "name",
    ]

class VariantConstants:
    HULL_KEY = "Hull"
    # TODO
    THIN_IBEAM_KEY = "Thin I-Beam" # Base
    THICK_IBEAM_KEY = "Thick I-Beam"
    THIN_BULKHEAD_KEY = "Thin Bulkhead"
    THICK_BULKHEAD_KEY = "Thick Bulkhead"
    CHEAP_KEY = "Cheap"

    names = {
        HULL_KEY: "Hull",
        THIN_IBEAM_KEY: "Thin I-Beam",
        THICK_IBEAM_KEY: "Thick I-Beam",
        THIN_BULKHEAD_KEY: "Thin Bulkhead",
        THICK_BULKHEAD_KEY: "Thick Bulkhead",
        CHEAP_KEY: "Cheap"
    }


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


def add_variant_material(material, variant_key, variants):
    if variant_key in variants:
        print("ERROR: variant '{}' already found for material '{}'".format(VariantConstants.names[variant_key], material["name"]))
        sys.exit(-1)
    variants[variant_key] = material


def add_variants(material_name_stem, input_filename, output_filename):
    json_obj = load_json(input_filename)

    # Scoop up variants
    variants = {} # {Key, Material}
    first_existing_material_index = None
    im = 0
    while im < len(json_obj["materials"]):
        material = json_obj["materials"][im]
        material_name_parts = material["name"].split(' ')
        assert(len(material_name_parts) >= 1)
        if material_name_parts[0] == material_name_stem:
            # Determine variant
            if len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.HULL_KEY]:
                add_variant_material(material, VariantConstants.HULL_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im
            # Default
            elif len(material_name_parts) == 1 or (len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.THIN_IBEAM_KEY]):
                add_variant_material(material, VariantConstants.THIN_IBEAM_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im
            elif len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.THICK_IBEAM_KEY]:
                add_variant_material(material, VariantConstants.THICK_IBEAM_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im
            elif len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.THIN_BULKHEAD_KEY]:
                add_variant_material(material, VariantConstants.THIN_BULKHEAD_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im
            elif len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.THICK_BULKHEAD_KEY]:
                add_variant_material(material, VariantConstants.THICK_BULKHEAD_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im
            elif len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.CHEAP_KEY]:
                add_variant_material(material, VariantConstants.CHEAP_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im
            else:
                print("WARNING: found unrecognized variant at material '{}'".format(material["name"]))
        im = im + 1

    if len(variants) == 0:
        print("ERROR: cannot find any variants for material stem '{}'".format(material_name_stem))
        sys.exit(-1)

    print("Found {} variants: {}".format(len(variants), ", ".join(m["name"] for m in variants.values())))

    # Pre-process existing variants
    # TODOHERE: : need to remember also others
    variant_densities = {} # {variant_key, density}
    for variant_key, material in variants.items():

        material_name = material["name"]
        variant_name = VariantConstants.names[variant_key]

        print("  {} ({}):".format(material_name, variant_name))

        # Normalize name
        expected_name = "{} {}".format(material_name_stem, variant_name)
        if material_name != expected_name:
            print("    Rename: '{}' -> '{}'".format(material_name, expected_name))
            material["name"] = expected_name

        # Extract density
        assert(variant_key not in variant_densities)
        variant_densities[variant_key] = material["mass"]["density"]
        print("    {} density: {}".format(variant_name, variant_densities[variant_key]))


    # Calculate densities for all missing variants

    if VariantConstants.THIN_IBEAM_KEY not in variant_densities:
        print("ERROR: cannot find base material for material stem '{}'".format(material_name_stem))
        sys.exit(-1)

    if VariantConstants.HULL_KEY not in variant_densities:
        variant_densities[VariantConstants.HULL_KEY] = variant_densities[VariantConstants.THIN_IBEAM_KEY] * 10.0

    if VariantConstants.THICK_IBEAM_KEY not in variant_densities:
        variant_densities[VariantConstants.THICK_IBEAM_KEY] = variant_densities[VariantConstants.THIN_IBEAM_KEY] * 10.0

    if VariantConstants.THIN_BULKHEAD_KEY not in variant_densities:
        variant_densities[VariantConstants.THIN_BULKHEAD_KEY] = variant_densities[VariantConstants.THIN_IBEAM_KEY]

    if VariantConstants.THICK_BULKHEAD_KEY not in variant_densities:
        variant_densities[VariantConstants.THICK_BULKHEAD_KEY] = variant_densities[VariantConstants.THIN_IBEAM_KEY] * 4.0

    if VariantConstants.CHEAP_KEY not in variant_densities:
        variant_densities[VariantConstants.CHEAP_KEY] = variant_densities[VariantConstants.THIN_IBEAM_KEY]

    print("Densities:")
    for variant_key, density in variant_densities.items():
        print("  {}: {}".format(VariantConstants.names[variant_key],density))

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
