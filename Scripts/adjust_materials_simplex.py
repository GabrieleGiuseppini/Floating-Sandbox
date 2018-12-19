import re
import sys


def adjust_density(factor, input_file, output_file):

    print "adjust_density: {}".format(factor)

    def replace_density(m):
        density = float(m.group(1))
        density *= factor
        return "\"density\": " + str(density)

    for line in input_file:
        new_line = re.sub("\"density\":\s+(\d*\.\d+)", replace_density, line)
        output_file.write(new_line)


def main():
    
    if len(sys.argv) != 5 or sys.argv[1] != "density":
        print("Usage: adjust_materials_simplex.py density <alpha> <input_json> <output_json>")
        sys.exit(-1)

    alpha = float(sys.argv[2])

    with open(sys.argv[3], "r") as inFile:
        with open(sys.argv[4], "w") as outFile:
            adjust_density(alpha, inFile, outFile)

main()
