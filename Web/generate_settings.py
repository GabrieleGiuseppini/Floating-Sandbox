#!/usr/bin/env python3
import re
import sys

SCALAR_TYPES = {
    "float": "float",
    "bool": "bool",
    "size_t": "int",
    "unsigned int": "int",
    "std::chrono::minutes": "minutes",
    "std::chrono::seconds": "seconds",
}

SKIP = {
    "SimulationParallelism",
    "NumberOfStars",
    "NumberOfClouds",
}


def read(path):
    with open(path, encoding="utf-8") as handle:
        return handle.read()


def main():
    settings_header, options_header, output = sys.argv[1:4]
    settings_text = read(settings_header)
    options_text = read(options_header)

    min_max = set(re.findall(r"virtual\s+[\w:<> ]+?\s+GetM(?:in|ax)(\w+)\(\) const", options_text))

    group = "Mechanics"
    entries = []
    getters = {}
    setters = {}
    order = []

    for line in settings_text.splitlines():
        stripped = line.strip()
        heading = re.match(r"^//\s+([A-Z][A-Za-z ]+)$", stripped)
        if heading:
            group = heading.group(1).strip()
            continue
        getter = re.match(r"virtual\s+([\w:<> ]+?)\s+Get(\w+)\(\) const = 0;", stripped)
        if getter:
            kind = SCALAR_TYPES.get(getter.group(1).strip())
            if kind:
                getters[getter.group(2)] = (kind, group)
                order.append(getter.group(2))
            continue
        setter = re.match(r"virtual\s+void\s+Set(\w+)\(([\w:<> ]+?)\s+value\) = 0;", stripped)
        if setter:
            setters[setter.group(1)] = setter.group(2).strip()

    lines = []
    for name in order:
        if name not in setters or name in SKIP:
            continue
        kind, section = getters[name]
        has_range = (("Min" + name) in min_max) or (name in min_max)
        has_range = bool(re.search(r"Get(?:Min|Max)" + name + r"\(\)", options_text))
        label = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", name)
        if kind == "float":
            get_expr = "static_cast<double>(c.Get%s())" % name
            set_expr = "c.Set%s(static_cast<float>(v))" % name
            min_expr = "static_cast<double>(o.GetMin%s())" % name if has_range else "0.0"
            max_expr = "static_cast<double>(o.GetMax%s())" % name if has_range else "1.0"
        elif kind == "bool":
            get_expr = "c.Get%s() ? 1.0 : 0.0" % name
            set_expr = "c.Set%s(v >= 0.5)" % name
            min_expr = "0.0"
            max_expr = "1.0"
        elif kind == "int":
            cpp = setters[name]
            get_expr = "static_cast<double>(c.Get%s())" % name
            set_expr = "c.Set%s(static_cast<%s>(v + 0.5))" % (name, cpp)
            min_expr = "static_cast<double>(o.GetMin%s())" % name if has_range else "0.0"
            max_expr = "static_cast<double>(o.GetMax%s())" % name if has_range else "100.0"
        else:
            cpp = setters[name]
            get_expr = "static_cast<double>(c.Get%s().count())" % name
            set_expr = "c.Set%s(%s(static_cast<long long>(v + 0.5)))" % (name, cpp)
            min_expr = "static_cast<double>(o.GetMin%s().count())" % name if has_range else "0.0"
            max_expr = "static_cast<double>(o.GetMax%s().count())" % name if has_range else "100.0"
        step = {"float": "0", "bool": "1", "int": "1", "minutes": "1", "seconds": "1"}[kind]
        lines.append(
            '    { "%s", "%s", "%s", "%s", %s, [](IGameControllerSettings const & c, IGameControllerSettingsOptions const & o) { (void)o; return %s; }, '
            '[](IGameControllerSettings const & c, IGameControllerSettingsOptions const & o) { (void)c; (void)o; return %s; }, '
            '[](IGameControllerSettings const & c, IGameControllerSettingsOptions const & o) { (void)c; (void)o; return %s; }, '
            '[](IGameControllerSettings & c, double v) { %s; } },'
            % (name, label, section, kind, step, get_expr, min_expr, max_expr, set_expr)
        )

    with open(output, "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")

    print("%d settings" % len(lines))


if __name__ == "__main__":
    main()
