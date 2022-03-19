from copy import deepcopy
import json
import sys
 

MANDATORY_JSON_FIELD_NAMES = [
    "color_key",
    "name",
    ]

class VariantConstants:
    HULL_KEY = "Hull"
    # TODO
    THIN_IBEAM_KEY = "Thin I-Beam" # was: Base
    THICK_IBEAM_KEY = "Thick I-Beam" # was: Structural
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


def get_normalized_color_keys(material):
    color_keys = material["color_key"]
    if not isinstance(color_keys, list):
        color_keys = [color_keys]
    return color_keys


def hex_to_rgb(hex_value):
    if len(hex_value) == 0 or hex_value[0] != "#":
        print("ERROR: invalid color '{};".format(hex_value))
        sys.exit(-1)
    hex_value = hex_value.lstrip("#")
    lv = len(hex_value)
    return tuple(int(hex_value[i:i + lv // 3], 16) for i in range(0, lv, lv // 3))


def rgb_to_hex(rgb_tuple):
    return ("#%02x%02x%02x" % rgb_tuple).upper()


def make_color_set(json_obj):
    colors = set()
    for material in json_obj["materials"]:
        color_keys = get_normalized_color_keys(material)
        for color in color_keys:
            if color in colors:
                print("ERROR: material '{}' has color '{}' which is a duplicate".format(material["name"], color))
            else:
                colors.add(color)
    return colors


def verify(filename):
    json_obj = load_json(filename)

    # Verify colors
    make_color_set(json_obj)

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


def make_color(base_rgb, rgb_offset, color_set):
    extra_offset = 0
    while True:
        new_color = tuple(b + o + extra_offset for b, o in zip(base_rgb, rgb_offset))
        new_color = tuple(c + 256 if c < 0 else c for c in new_color)
        #print("make_color: " + str(base_rgb) + " + " + str(rgb_offset) + " = " + str(new_color))
        new_color_str = rgb_to_hex(new_color)
        if new_color_str not in color_set:
            color_set.add(new_color_str)
            return new_color
        else:
            extra_offset = extra_offset + 1


def make_color_keys(base_color_keys, rgb_offset, color_set):
    # Normalize colors
    if not isinstance(base_color_keys, list):
        base_color_keys = [base_color_keys]
    # Process colors
    new_colors = []
    for base_color_key_str in base_color_keys:
        base_rgb = hex_to_rgb(base_color_key_str)
        new_rgb = make_color(base_rgb, rgb_offset, color_set)
        new_colors.append(rgb_to_hex(new_rgb))
    return new_colors


def make_variant_material(material_name_stem, variant_key, ideal_mass_offset, ideal_density_multiplier, ideal_is_impermeable, ideal_rgb_offset_over_base, variants, color_set):
    variant_name = VariantConstants.names[variant_key]
    
    # Get base material
    if VariantConstants.THIN_IBEAM_KEY not in variants:
        print("ERROR: cannot find base material for material stem '{}'".format(material_name_stem))
        sys.exit(-1)
    base_material = variants[VariantConstants.THIN_IBEAM_KEY]

    # Calculate targets
    ideal_name = "{} {}".format(material_name_stem, variant_name)
    ideal_mass = float(base_material["mass"]["nominal_mass"] + ideal_mass_offset)
    ideal_density = float(base_material["mass"]["density"] * ideal_density_multiplier)
    if ideal_is_impermeable:
        ideal_buoyancy_volume_fill = 0.0
    else:
        ideal_buoyancy_volume_fill = 1.0

    # Check if variant exists
    if variant_key not in variants:

        # Variant needs to be synthesized
        print("    Synthesize: {} ('{}')".format(VariantConstants.names[variant_key], ideal_name))
        material = deepcopy(base_material)

        # Name
        material["name"] = ideal_name

        # Mass and Density
        material["mass"]["nominal_mass"] = ideal_mass
        material["mass"]["density"] = ideal_density

        # Color keys
        material["color_key"] = make_color_keys(base_material["color_key"], ideal_rgb_offset_over_base, color_set)

        # Hullness
        material["is_hull"] = ideal_is_impermeable
        material["buoyancy_volume_fill"] = ideal_buoyancy_volume_fill

        variants[variant_key] = material

    else:

        # Variant exists
        material = variants[variant_key]

        # Normalize name
        if material["name"] != ideal_name:
            print("    Rename: {} ('{}' -> '{}')".format(VariantConstants.names[variant_key], material["name"], ideal_name))
            material["name"] = ideal_name


def dump_variant(variant_key, variants):
    material = variants[variant_key]
    print("  {}: '{}' n_mass={} density={} is_hull={} buoyancy_volume_fill={} color_keys=[{}]".format(
        VariantConstants.names[variant_key], 
        material["name"], 
        material["mass"]["nominal_mass"],
        material["mass"]["density"], 
        material["is_hull"], 
        material["buoyancy_volume_fill"],
        ", ".join(c for c in material["color_key"])))


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
        if material_name_stem in material_name_parts:
            # Determine variant
            if len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.HULL_KEY]:
                add_variant_material(material, VariantConstants.HULL_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im            
            elif len(material_name_parts) == 1 or (len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.THIN_IBEAM_KEY]): # Default
                add_variant_material(material, VariantConstants.THIN_IBEAM_KEY, variants)
                if first_existing_material_index is None:
                    first_existing_material_index = im
            elif len(material_name_parts) == 2 and (material_name_parts[1] == VariantConstants.names[VariantConstants.THICK_IBEAM_KEY] or "Structural" in material_name_parts):
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

    #
    # Make variants
    #

    color_set = make_color_set(json_obj)

    make_variant_material(material_name_stem, VariantConstants.HULL_KEY, 100.0, 10.0, True, [-64, -64, -64], variants, color_set)
    make_variant_material(material_name_stem, VariantConstants.THIN_IBEAM_KEY, 0.0, 1.0, False, [0, 0, 0], variants, color_set)

    # Calculate actual range between hull and base
    base_rgb = hex_to_rgb(get_normalized_color_keys(variants[VariantConstants.THIN_IBEAM_KEY])[0])
    hull_rgb = hex_to_rgb(get_normalized_color_keys(variants[VariantConstants.HULL_KEY])[0])
    color_key_range = tuple(h - b for b, h in zip(base_rgb, hull_rgb))

    make_variant_material(material_name_stem, VariantConstants.THICK_IBEAM_KEY, 100.0, 10.0, False,
        tuple(int(r * 2.0 / 5.0) for r in color_key_range),
        variants, color_set)
    make_variant_material(material_name_stem, VariantConstants.THIN_BULKHEAD_KEY, 0.0, 1.0, True,
        tuple(int(r * 3.0 / 5.0) for r in color_key_range),
        variants, color_set)
    make_variant_material(material_name_stem, VariantConstants.THICK_BULKHEAD_KEY, 0.0, 4.0, True, 
        tuple(int(r * 4.0 / 5.0) for r in color_key_range),
        variants, color_set)
    make_variant_material(material_name_stem, VariantConstants.CHEAP_KEY, 0.0, 1.0, False, 
        tuple(int(r * 1.0 / 5.0) for r in color_key_range),
        variants, color_set)

    # Dump all variants
    print("All variants:")
    dump_variant(VariantConstants.HULL_KEY, variants)
    dump_variant(VariantConstants.THIN_IBEAM_KEY, variants)
    dump_variant(VariantConstants.THICK_IBEAM_KEY, variants)
    dump_variant(VariantConstants.THIN_BULKHEAD_KEY, variants)
    dump_variant(VariantConstants.THICK_BULKHEAD_KEY, variants)
    dump_variant(VariantConstants.CHEAP_KEY, variants)


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
