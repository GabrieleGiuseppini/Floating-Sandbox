import codecs
import json
import os
import sys

JSON_FIELD_NAMES = [
    "structure_image",
    "texture_image",
    "electrical_image",
    "ropes_image",
    "auto_texturization",
    "offset",
    "ship_name",
    "created_by",
    "year_built",
    "description",
    "electrical_panel",
    "do_hide_electricals_in_preview",
    "do_hide_hd_in_preview"
    ]

MANDATORY_JSON_FIELD_NAMES = [
    "structure_image"
    ]

FILE_JSON_FIELD_NAMES = [
    "structure_image",
    "texture_image",
    "electrical_image",
    "ropes_image"
    ]

def parse_ship_file(ship_folder_path, ship_file_name):

    ship_file_path = os.path.join(ship_folder_path, ship_file_name)
    with open(ship_file_path, "rt") as json_file:  
        try:
            data = json.load(json_file, "ascii")
            return data
        except Exception as e:
            print("ERROR: {}: {}".format(ship_file_name, str(e)))
            return None

def check_file_exists(ship_folder_path, ship_file_name, ship_definition, field_name):

    if field_name in ship_definition:
        if not os.path.exists(os.path.join(ship_folder_path, ship_definition[field_name])):
            print("ERROR: {}: '{}' file '{}' does not exist".format(ship_file_name, field_name, ship_definition[field_name]))
            return False
        else:
            return True
    else:
        return True

def check_file_valid(ship_folder_path, ship_file_name, ship_definition, field_name):

    if field_name in ship_definition:
        file_name = ship_definition[field_name]
        file_name_root, file_name_ext = os.path.splitext(file_name)
        if not file_name_ext in ".dat":
            print("ERROR: {}: '{}' file '{}' has an invalid extension".format(ship_file_name, field_name, file_name))
            return False
        else:
            return True
    else:
        return True

def main():

    ships_path = "../Ships";

    if len(sys.argv) == 2:
        ships_path = sys.argv[1]
    elif len(sys.argv) > 2:
        print("Usage: verify_ships.py [<path_to_ships>]")
        sys.exit(-1)

    #
    # Initialize stats
    #

    extraneous_files = 0
    invalid_json = 0        
    unrecognized_json_fields = 0
    invalid_schema = 0
    missing_files = 0
    invalid_files = 0
    mismatched_ship_names = 0
    orphan_dat_files = 0


    # 
    # Enumerate files
    #

    file_names = os.listdir(ships_path)

    dat_files = set()
    referenced_dat_files = set()

    for file_name in file_names:

        ### Check extension
        file_name_root, file_name_ext = os.path.splitext(file_name)
        if not file_name_root:
            print("ERROR: {}: invalid file type".format(file_name))
            extraneous_files += 1
        elif not file_name_ext in [".dat", ".shp", ".png"]:
            print("ERROR: {}: unrecognized file type".format(file_name))
            extraneous_files += 1

        ### Update set of dat files
        if file_name_ext == ".dat":
            dat_files.add(file_name)

        if file_name_ext != ".shp":
            continue

        ### Load and check json
        ship_definition = parse_ship_file(ships_path, file_name)
        if not ship_definition:
            print("ERROR: {}: invalid json".format(file_name))
            invalid_json += 1
            continue

        ### Check json fields
        for k in ship_definition.iterkeys():
            if not k in JSON_FIELD_NAMES:
                print("ERROR: {}: unrecognized json field '{}'".format(file_name, k))
                unrecognized_json_fields += 1
        for f in MANDATORY_JSON_FIELD_NAMES:
            if not f in ship_definition:
                print("ERROR: {}: missing '{}' field".format(file_name, f))
                invalid_schema += 1

        ### Check files
        for file_field_name in FILE_JSON_FIELD_NAMES:
            if not check_file_exists(ships_path, file_name, ship_definition, file_field_name):
                    missing_files += 1
            if not check_file_valid(ships_path, file_name, ship_definition, file_field_name):
                    invalid_files += 1
            if file_field_name in ship_definition:
                    referenced_dat_files.add(ship_definition[file_field_name])

        ### Check ship name
        if "ship_name" in ship_definition:
            ship_name = ship_definition["ship_name"]
            if ship_name != file_name_root:
                print("ERROR: {}: ship name '{}' does not match file name '{}'".format(file_name, ship_name, file_name_root))
                mismatched_ship_names += 1


    #
    # Make sure all dat files are referenced
    #

    for orphan_dat_file in dat_files - referenced_dat_files:
        print("ERROR: dat file '{}' is orphan".format(orphan_dat_file))
        orphan_dat_files += 1


    #
    # Final results
    #

    print("------------------------------------------")
    print("Total files: {}".format(len(file_names)))
    print("  extraneous_files         : {}".format(extraneous_files))
    print("  invalid_json             : {}".format(invalid_json))
    print("  unrecognized_json_fields : {}".format(unrecognized_json_fields))
    print("  invalid_schema           : {}".format(invalid_schema))
    print("  missing_files            : {}".format(missing_files))
    print("  invalid_files            : {}".format(invalid_files))
    print("  mismatched_ship_names    : {}".format(mismatched_ship_names))
    print("  orphan_dat_files         : {}".format(orphan_dat_files))

main()
