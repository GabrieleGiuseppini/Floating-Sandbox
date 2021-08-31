from copy import deepcopy
from dataclasses import dataclass
import json
from operator import itemgetter
import re
import sys
from typing import List

MAX_WIDTH = 16


@dataclass
class SubCategory:
    category_name: str
    sub_category_name: str
    materials: list  # normalized


def get_subcategory(category_name, sub_category_name, root_obj) -> List[SubCategory]:
    materials = [m for m in root_obj["materials"] if ("palette_coordinates" in m and m["palette_coordinates"]["sub_category"] == sub_category_name)]
    materials_normalized = []
    for m in materials:
        # Normalize color key
        color_keys = m["color_key"]
        if not isinstance(color_keys, list):
            color_keys = [color_keys]
        for c in color_keys:
            new_material = deepcopy(m)
            new_material["color_key"] = c
            materials_normalized.append(new_material)
    # Sort by ordinal
    materials_normalized.sort(key=lambda m: m["palette_coordinates"]["sub_category_ordinal"])
    return SubCategory(category_name, sub_category_name, materials_normalized)


def main():
    
    if len(sys.argv) != 3 or (sys.argv[2] != "s" and sys.argv[2] != "e"):
        print("Usage: generate_materials_template.py <path_to_materials_json> s|e")
        sys.exit(-1)

    json_content = ""
    comment_re = re.compile(r"^(.*?)(//.+)?$")
    with open(sys.argv[1]) as f:
        for line in f:
            json_content += comment_re.sub(r"\1", line)

    root_obj = json.loads(json_content)

    is_structural = (sys.argv[2] == "s")

    #
    # Read palette
    #

    palette: List[SubCategory] = []
    for category_obj in root_obj["palette_categories"]:
        category_name = category_obj["category"]
        for sub_category_name in category_obj["sub_categories"]:
            sub_category = get_subcategory(category_name, sub_category_name, root_obj)
            palette.append(sub_category)
            
    #
    # Generate HTML
    #

    html = "<html><head><title>Floating Sandbox Materials Template</title>"
    html += "<style>tr.border_top td { border-top:1pt solid black;}</style>"
    html += "</head><body>"

    if is_structural:
        spacing = 4
    else:
        spacing = 4

    # Open HTML
    html += "<table style='border: 1px solid black' cellpadding=0 cellspacing={}>".format(spacing)

    # Visit palette and generate rows
    color_row_html = ""
    data_row_html = ""
    column_count = 0
    current_category_name = None
    for sub_category in palette:
        sc_column_count = 1 + len(sub_category.materials)
        if (column_count + sc_column_count > MAX_WIDTH or sub_category.category_name != current_category_name) and column_count > 0:
            # Close row
            html += "<tr>" + color_row_html + "</tr><tr>" + data_row_html + "</tr>"
            color_row_html = ""
            data_row_html = ""
            column_count = 0

        # Title
        color_row_html += "<td valign='middle' align='right' style='padding-right:5px;font-size:10px;'>" + sub_category.sub_category_name + "</td>"
        data_row_html += "<td></td>"

        # Process all colors
        for m in sub_category.materials:
            # --- Color ---
            color_row_html += "<td bgcolor='" + m["color_key"] + "'class='border_top' style='width:70px;height:20px'>&nbsp;</td>"
            # --- Data ---
            data_row_html += "<td style='font-size:9px;vertical-align:top'>"
            if is_structural:
                data_row_html += "{:.2f}".format(m["mass"]["nominal_mass"] * m["mass"]["density"]) + "|" + str(m["strength"]) + "|" + str(m["stiffness"])
            else:
                data_row_html += m["name"]
            data_row_html += "</td>"

        # Advance
        column_count = column_count + 1 + len(sub_category.materials)
        current_category_name = sub_category.category_name

    # Close last row, if any
    if column_count > 0:
        html += "<tr>" + color_row_html + "</tr><tr>" + data_row_html + "</tr>"

    # Close HTML
    html += "</table>";
    html += "</body></html>";

    # Write HTML
    with open("materials_template.html", "w") as html_file:
        html_file.write(html)

main()
