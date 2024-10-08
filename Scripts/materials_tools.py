from copy import deepcopy
import json
import sys

 
MANDATORY_JSON_FIELD_NAMES = [
    "color_key",
    "name",
    ]

class VariantConstants:
    HULL_KEY = "Hull"                       # Hull (most of the times, density being 10 times that of base)
    LIGHT_IBEAM_KEY = "Light I-Beam"        # base
    SOLID_IBEAM_KEY = "Solid I-Beam"        # Non-hull (permeable) with 10 times the density of base; was: Structural
    LIGHT_BULKHEAD_KEY = "Light Bulkhead"   # Hull (impermeable), with same density as base
    SOLID_BULKHEAD_KEY = "Solid Bulkhead"   # Hull (impermeable), with ~5 times the density of base
    LOWGRADE_KEY = "Low-Grade"              # base, but weak; was: Cheap

    names = {
        HULL_KEY: "Hull",
        LIGHT_IBEAM_KEY: "Light I-Beam",
        SOLID_IBEAM_KEY: "Solid I-Beam",
        LIGHT_BULKHEAD_KEY: "Light Bulkhead",
        SOLID_BULKHEAD_KEY: "Solid Bulkhead",
        LOWGRADE_KEY: "Low-Grade"
    }

    material_names = {
        HULL_KEY: "{} Hull",
        LIGHT_IBEAM_KEY: "Light {} I-Beam",
        SOLID_IBEAM_KEY: "Solid {} I-Beam",
        LIGHT_BULKHEAD_KEY: "Light {} Bulkhead",
        SOLID_BULKHEAD_KEY: "Solid {} Bulkhead",
        LOWGRADE_KEY: "Low-Grade {}"
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


def clamp(num, min_value, max_value):
   return max(min(num, max_value), min_value)


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


def make_name_set(json_obj):
    names = set()
    for material in json_obj["materials"]:
        if 'name' not in material:
            print("ERROR: missing 'name' field")
            continue
        name = material['name']
        if name in names:
            print("ERROR: material name '{}' is duplicate".format(name))
        else:
            names.add(name)
    return names


def find_palette_group(json_obj, material_group_name):
    for palette_name in ["structural_palette", "ropes_palette", "electrical_palette"]:
        palettes_obj = json_obj["palettes"]
        if palette_name in palettes_obj:
            categories_obj = palettes_obj[palette_name]
            for category_obj in categories_obj:
                for group_obj in category_obj["groups"]:
                    if group_obj["name"] == material_group_name:
                        return group_obj
    print("ERROR: cannot find group '{}' among palettes".format(material_group_name))
    sys.exit(-1)


def verify(filename):
    json_obj = load_json(filename)

    # Verify colors
    make_color_set(json_obj)

    # Verify names
    names = make_name_set(json_obj)

    for material in json_obj["materials"]:
        
        assert 'name' in material
        material_name = material['name']

        # Verify mandatory fields
        for f in MANDATORY_JSON_FIELD_NAMES:
            if not f in material:
                if f != "texture_name" or 'is_legacy_electrical' not in material or material['is_legacy_electrical'] == False:
                    print("ERROR: {}: missing '{}' field".format(material_name, f))

        # Verify color keys
        color_keys = get_normalized_color_keys(material)
        for hex_color in color_keys:
            rgb = hex_to_rgb(hex_color)
            if (not "unique_type" in material or material["unique_type"] != "Rope") and rgb[0] == 0 and rgb[1] < 16:
                print("ERROR: {}: color key clashes with legacy ropes's color keys".format(material_name))

        # Verify palette
        if not 'palette_coordinates' in material:
            if not 'is_exempt_from_palette' in material or material['is_exempt_from_palette'] == False:
                print("ERROR: {}: has no 'palette_coordinates' but is not exempt from palette".format(material_name))


def add_variant_material(material, variant_key, variants):
    if variant_key in variants:
        print("ERROR: variant '{}' already found for material '{}'".format(VariantConstants.names[variant_key], material["name"]))
        sys.exit(-1)
    variants[variant_key] = material


def make_color(base_rgb, offset_rgb, color_set):
    extra_offset = 0
    while True:
        new_color = tuple(clamp(b + o + extra_offset, 0, 255) for b, o in zip(base_rgb, offset_rgb))
        new_color_str = rgb_to_hex(new_color)
        if new_color_str not in color_set:
            color_set.add(new_color_str)
            return new_color
        else:
            extra_offset = extra_offset + 1


def make_color_keys(base_color_keys, rgb_offsets, color_set):
    # Normalize colors
    if not isinstance(base_color_keys, list):
        base_color_keys = [base_color_keys]
    # Process colors
    new_colors = []
    for base_color_key_str, rgb_offset in zip(base_color_keys, rgb_offsets):
        base_rgb = hex_to_rgb(base_color_key_str)
        new_rgb = make_color(base_rgb, rgb_offset, color_set)
        new_colors.append(rgb_to_hex(new_rgb))
    return new_colors


def make_variant_material(material_group_name, variant_key, ideal_nominal_mass_offset, ideal_density_multiplier, ideal_is_impermeable, ideal_strength_multiplier, ideal_rgb_offsets_over_base, variants, color_set):
    variant_name = VariantConstants.names[variant_key]
    
    # Get base material
    if VariantConstants.LIGHT_IBEAM_KEY not in variants:
        print("ERROR: cannot find base material for material group '{}'".format(material_group_name))
        sys.exit(-1)
    base_material = variants[VariantConstants.LIGHT_IBEAM_KEY]

    # Calculate targets
    ideal_name = VariantConstants.material_names[variant_key].format(material_group_name)
    ideal_mass = float(base_material["mass"]["nominal_mass"] + ideal_nominal_mass_offset)
    ideal_density = float(base_material["mass"]["density"] * ideal_density_multiplier)
    if ideal_is_impermeable:
        ideal_buoyancy_volume_fill = 0.0
    else:
        ideal_buoyancy_volume_fill = 1.0
    ideal_strength = float(base_material["strength"] * ideal_strength_multiplier)

    # Check if variant exists
    if variant_key not in variants:

        # Variant needs to be synthesized
        print("    Synthesize: {} ('{}')".format(VariantConstants.names[variant_key], ideal_name))
        material = deepcopy(base_material)

        # Name
        material["name"] = ideal_name

        # Palette sub-category
        material["palette_coordinates"]["sub_category"] = ideal_name

        # Mass and Density
        material["mass"]["nominal_mass"] = ideal_mass
        material["mass"]["density"] = ideal_density

        # Color keys
        material["color_key"] = make_color_keys(base_material["color_key"], ideal_rgb_offsets_over_base, color_set)

        # Hullness
        material["is_hull"] = ideal_is_impermeable
        material["buoyancy_volume_fill"] = ideal_buoyancy_volume_fill

        # Strength
        material["strength"] = ideal_strength

        variants[variant_key] = material

    else:

        # Variant exists
        material = variants[variant_key]

        # ...just normalize name in this case
        if material["name"] != ideal_name:
            print("    Rename: {} ('{}' -> '{}')".format(VariantConstants.names[variant_key], material["name"], ideal_name))
            material["name"] = ideal_name
        material["palette_coordinates"]["sub_category"] = ideal_name


def dump_variant(variant_key, variants):
    material = variants[variant_key]
    print("  {}: n_mass={} density={} mass={} is_hull={} buoyancy_volume_fill={} strength={}".format(
        material["name"], 
        material["mass"]["nominal_mass"],
        material["mass"]["density"], 
        material["mass"]["nominal_mass"] * material["mass"]["density"],
        material["is_hull"], 
        material["buoyancy_volume_fill"],
        material["strength"]))


def add_color(material_name, input_filename, output_filename, base_reference_color, target_reference_color, base_colors):    
    base_reference_rgb = hex_to_rgb(base_reference_color) # From reference material
    target_reference_rgb = hex_to_rgb(target_reference_color) # From material
    base_rgbs = list(hex_to_rgb(bc) for bc in base_colors) # Target, from reference

    json_obj = load_json(input_filename)
    color_set = make_color_set(json_obj)

    material = list(filter(lambda m : m["name"] == material_name, json_obj["materials"]))
    if not material or len(material) > 1:
        print("ERROR: cannot find material '{}', or too many matching materials found".format(material_name))
        sys.exit(-1)
    material = material[0]

    offset_rgb = tuple(tr - br for tr, br in zip(target_reference_rgb, base_reference_rgb))
    new_color_rgbs = list(make_color(base_rgb, offset_rgb, color_set) for base_rgb in base_rgbs)
    new_colors = list(rgb_to_hex(new_color_rgb) for new_color_rgb in new_color_rgbs)

    # Add
    color_keys = get_normalized_color_keys(material)
    color_keys.extend(new_colors)
    material["color_key"] = color_keys

    # Write
    save_json(json_obj, output_filename)


def add_variants(material_group_name, input_filename, output_filename):
    json_obj = load_json(input_filename)

    # Find group in palette
    palette_group = find_palette_group(json_obj, material_group_name)

    # Scoop up variants
    variants = {} # {Key, Material}
    first_existing_material_index = None
    im = 0
    while im < len(json_obj["materials"]):
        material = json_obj["materials"][im]
        material_name_parts = material["name"].split(' ')
        assert(len(material_name_parts) >= 1)
        has_taken_material = False
        if material_group_name in material_name_parts:
            # Determine variant
            if len(material_name_parts) == 2 and material_name_parts[1] == VariantConstants.names[VariantConstants.HULL_KEY]:
                add_variant_material(material, VariantConstants.HULL_KEY, variants)
                has_taken_material = True            
            elif len(material_name_parts) == 1 or material["name"] == VariantConstants.material_names[VariantConstants.LIGHT_IBEAM_KEY].format(material_group_name): # Default
                add_variant_material(material, VariantConstants.LIGHT_IBEAM_KEY, variants)
                has_taken_material = True
            elif material["name"] == VariantConstants.material_names[VariantConstants.SOLID_IBEAM_KEY].format(material_group_name) or "Structural" in material_name_parts:
                add_variant_material(material, VariantConstants.SOLID_IBEAM_KEY, variants)
                has_taken_material = True
            elif material["name"] == VariantConstants.material_names[VariantConstants.LIGHT_BULKHEAD_KEY].format(material_group_name):
                add_variant_material(material, VariantConstants.LIGHT_BULKHEAD_KEY, variants)
                has_taken_material = True
            elif material["name"] == VariantConstants.material_names[VariantConstants.SOLID_BULKHEAD_KEY].format(material_group_name):
                add_variant_material(material, VariantConstants.SOLID_BULKHEAD_KEY, variants)
                has_taken_material = True
            elif material["name"] == VariantConstants.material_names[VariantConstants.LOWGRADE_KEY].format(material_group_name) or "Cheap" in material_name_parts:
                add_variant_material(material, VariantConstants.LOWGRADE_KEY, variants)
                has_taken_material = True
            else:
                print("WARNING: found unrecognized variant at material '{}'".format(material["name"]))
        if has_taken_material:
            if first_existing_material_index is None:
                    first_existing_material_index = im
            # Remove material
            del json_obj["materials"][im]
            # Remove sub-category
            palette_group["sub_categories"].remove(material["name"])
        else:
            im = im + 1

    if len(variants) == 0:
        print("ERROR: cannot find any variants for material group '{}'".format(material_group_name))
        sys.exit(-1)

    print("Found {} variants: {}".format(len(variants), ", ".join(m["name"] for m in variants.values())))

    #
    # Make variants
    #

    color_set = make_color_set(json_obj)

    # Get number of colors in base and hull
    base_color_keys = get_normalized_color_keys(variants[VariantConstants.LIGHT_IBEAM_KEY])
    base_color_key_counts = len(base_color_keys)

    #
    # First pass on hull and base (eventually creating hull)
    #
    
    make_variant_material(
        material_group_name, VariantConstants.HULL_KEY, 
        ideal_nominal_mass_offset = 100.0,  # Hull nominal mass is 100Kg more than non-hull
        ideal_density_multiplier = 10.0,    # Hull density is 10 times non-hull
        ideal_is_impermeable = True,
        ideal_strength_multiplier = 1.0,
        ideal_rgb_offsets_over_base = [-64, -64, -64] * base_color_key_counts, 
        variants = variants, 
        color_set = color_set)

    make_variant_material(
        material_group_name, VariantConstants.LIGHT_IBEAM_KEY, 
        ideal_nominal_mass_offset = 0.0, 
        ideal_density_multiplier = 1.0, 
        ideal_is_impermeable = False, 
        ideal_strength_multiplier = 1.0,
        ideal_rgb_offsets_over_base = [0, 0, 0] * base_color_key_counts, 
        variants = variants, 
        color_set = color_set)

    # Calculate ranges between Hull and base   

    # Colors
    base_rgbs = [hex_to_rgb(base_color_keys[i]) for i in range(0, len(base_color_keys))]
    hull_color_keys = get_normalized_color_keys(variants[VariantConstants.HULL_KEY])
    hull_rgbs = [hex_to_rgb(hull_color_keys[i]) for i in range(0, len(hull_color_keys))]
    base_color_key_counts = min(len(base_color_keys), len(hull_color_keys))
    color_key_ranges = [tuple(h - b for b, h in zip(base_rgbs[i], hull_rgbs[i])) for i in range(0, base_color_key_counts)]

    #
    # All other variants
    #

    # Non-hull (permeable) with 10 times the density of base; was: Structural
    make_variant_material(material_group_name, VariantConstants.SOLID_IBEAM_KEY, 
        ideal_nominal_mass_offset = 100.0,
        ideal_density_multiplier = 10.0, 
        ideal_is_impermeable = False,
        ideal_strength_multiplier = 1.07,
        ideal_rgb_offsets_over_base = [tuple(int(r * 2.0 / 5.0) for r in color_key_ranges[i]) for i in range(0, base_color_key_counts)],
        variants = variants, 
        color_set = color_set)

    # Hull (impermeable), with same density as base
    make_variant_material(material_group_name, VariantConstants.LIGHT_BULKHEAD_KEY, 
        ideal_nominal_mass_offset = 0.0, 
        ideal_density_multiplier = 1.0, 
        ideal_is_impermeable = True,
        ideal_strength_multiplier = 1.0,
        ideal_rgb_offsets_over_base = [tuple(int(r * 3.0 / 5.0) for r in color_key_ranges[i]) for i in range(0, base_color_key_counts)],
        variants = variants, 
        color_set = color_set)

    # Hull (impermeable), with ~5 times the density of base
    make_variant_material(material_group_name, VariantConstants.SOLID_BULKHEAD_KEY, 
        ideal_nominal_mass_offset = 0.0, 
        ideal_density_multiplier = 4.0, 
        ideal_is_impermeable = True,
        ideal_strength_multiplier = 1.07,
        ideal_rgb_offsets_over_base = [tuple(int(r * 4.0 / 5.0) for r in color_key_ranges[i]) for i in range(0, base_color_key_counts)],
        variants = variants, 
        color_set = color_set)

    # base, but weak; was: Cheap
    make_variant_material(material_group_name, VariantConstants.LOWGRADE_KEY, 
        ideal_nominal_mass_offset = 0.0, 
        ideal_density_multiplier = 1.0, 
        ideal_is_impermeable = False,
        ideal_strength_multiplier = 0.3,
        ideal_rgb_offsets_over_base = [tuple(int(r * 1.0 / 5.0) for r in color_key_ranges[i]) for i in range(0, base_color_key_counts)],
        variants = variants, 
        color_set = color_set)

    # Dump all variants
    print("All variants:")
    dump_variant(VariantConstants.HULL_KEY, variants)
    dump_variant(VariantConstants.LIGHT_IBEAM_KEY, variants)
    dump_variant(VariantConstants.SOLID_IBEAM_KEY, variants)
    dump_variant(VariantConstants.LIGHT_BULKHEAD_KEY, variants)
    dump_variant(VariantConstants.SOLID_BULKHEAD_KEY, variants)
    dump_variant(VariantConstants.LOWGRADE_KEY, variants)

    #
    # Produce json
    #

    # Insert materials
    assert(first_existing_material_index is not None)
    json_obj["materials"].insert(first_existing_material_index, variants[VariantConstants.HULL_KEY])
    json_obj["materials"].insert(first_existing_material_index + 1, variants[VariantConstants.SOLID_BULKHEAD_KEY])
    json_obj["materials"].insert(first_existing_material_index + 2, variants[VariantConstants.LIGHT_BULKHEAD_KEY])
    json_obj["materials"].insert(first_existing_material_index + 3, variants[VariantConstants.SOLID_IBEAM_KEY])
    json_obj["materials"].insert(first_existing_material_index + 4, variants[VariantConstants.LIGHT_IBEAM_KEY])
    json_obj["materials"].insert(first_existing_material_index + 5, variants[VariantConstants.LOWGRADE_KEY])

    # Insert palette sub-categories
    palette_group["sub_categories"].insert(0, variants[VariantConstants.HULL_KEY]["name"])
    palette_group["sub_categories"].insert(1, variants[VariantConstants.SOLID_BULKHEAD_KEY]["name"])
    palette_group["sub_categories"].insert(2, variants[VariantConstants.LIGHT_BULKHEAD_KEY]["name"])
    palette_group["sub_categories"].insert(3, variants[VariantConstants.SOLID_IBEAM_KEY]["name"])
    palette_group["sub_categories"].insert(4, variants[VariantConstants.LIGHT_IBEAM_KEY]["name"])
    palette_group["sub_categories"].insert(5, variants[VariantConstants.LOWGRADE_KEY]["name"])
    
    # Write
    save_json(json_obj, output_filename)


def dump_materials(filename, field_names):
    json_obj = load_json(filename)
    for m in json_obj["materials"]:
        print("{}:".format(m["name"]))
        print("   " + " ".join("{}={}".format(n, m[n]) for n in field_names))


def print_usage():
    print("Usage: materials_tools.py add_color <material_name> <input_json> <output_json> <base_reference_color> <target_reference_color> <base_color_1> <base_color_2> ...")
    print("Usage: materials_tools.py add_variants <material_name> <input_json> <output_json>")
    print("Usage: materials_tools.py dump_materials <input_json> <field_1> <field_2> ...")
    print("Usage: materials_tools.py verify <input_json>")


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
    elif verb == 'add_color':
        if len(sys.argv) < 7:
            print_usage()
            sys.exit(-1)
        if len(sys.argv) >= 8:
            base_colors = sys.argv[7:]
        else:
            # Based off 404050 base_reference_color
            base_colors = [
                "#840E00",
                "#DBCF4D",
                "#806200",
                "#467B00",
                "#006A76",
                "#1B3261",
                "#860080",
                "#A84F00",
                "#111111"
            ]
        add_color(material_name=sys.argv[2], input_filename=sys.argv[3], output_filename=sys.argv[4], base_reference_color=sys.argv[5], target_reference_color=sys.argv[6], base_colors=base_colors)
    elif verb == 'add_variants':
        if len(sys.argv) != 5:
            print_usage()
            sys.exit(-1)
        add_variants(sys.argv[2], sys.argv[3], sys.argv[4])
    elif verb == 'dump_materials':
        if len(sys.argv) < 4:
            print_usage()
            sys.exit(-1)
        fields = sys.argv[3:]
        dump_materials(sys.argv[2], fields)
    else:
        print("ERROR: Unrecognized verb '{}'".format(verb))
        print_usage()

main()
