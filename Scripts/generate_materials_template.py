from copy import deepcopy
import json
from operator import itemgetter
import re
import sys

def get_subcategory_materials(sub_category_name, root_obj):  # [material]
    materials = [m for m in root_obj["materials"] if ("palette_coordinates" in m and m["palette_coordinates"]["sub_category"] == sub_category_name)]
    return sorted(materials, key=lambda m: m["palette_coordinates"]["sub_category_ordinal"])

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

    # palette element: sub-category name, [material]
    palette = []

    for category_obj in root_obj["palette_categories"]:
        category_name = category_obj["category"]
        for sub_category_name in category_obj["sub_categories"]:
            materials = get_subcategory_materials(sub_category_name, root_obj)
            palette.append((sub_category_name, materials))
            
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

    html += "<table style='border: 1px solid black' cellpadding=0 cellspacing={}>".format(spacing)

    # Visit all rows
    for sub_category in palette:

        color_row_html = ""
        data_row_html = ""

        # Title
        color_row_html += "<td valign='middle' align='right' style='padding-right:5px;font-size:10px;'>" + sub_category[0] + "</td>"
        data_row_html += "<td></td>"
        
        for m in sub_category[1]:

            # Normalize color key
            color_key = m["color_key"]
            if not isinstance(color_key, list):
                color_key = [color_key]

            # Process all colors
            for c in color_key:
                # --- Color ---
                color_row_html += "<td bgcolor='" + c + "'class='border_top' style='width: 50px;'>&nbsp;</td>"
                # --- Data ---
                data_row_html += "<td style='font-size:9px;vertical-align:top'>"
                if is_structural:
                    data_row_html += "{:.2f}".format(m["mass"]["nominal_mass"] * m["mass"]["density"]) + "|" + str(m["strength"]) + "|" + str(m["stiffness"])
                else:
                    data_row_html += m["name"]
                data_row_html += "</td>"

        html += "<tr>" + color_row_html + "</tr><tr>" + data_row_html + "</tr>"
        
    html += "</table>";
    html += "</body></html>";

    with open("materials_template.html", "w") as html_file:
        html_file.write(html)

main()
