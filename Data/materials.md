Each materials file is a (json) *array* of materials; json arrays start with:

    [

and end with:

    ]

Materials are separated by commas, as follows:

    {
        "name": "Iron Hull",
        ...
    },
    {
        "name": "Iron",
        ...
    },
    {
        "name": "Wood",
        ...
    }

Note: the last material must NOT be followed by a comma!

# Structural Materials

Structural materials are as follows:
        
    {
        "name": "Iron Hull",
        "template": {
                "row": "0|Iron Hull",
                "column": 0
        },
        "strength": 0.055,
        "mass": {
                "nominal_mass": 7950,
                "density": 0.17
        },
        "is_hull": true,
        "stiffness": 1,
        "color_key": "#404050",
        "render_color": "#404050",
	"sound_type": "Metal"
    }


Here's an explanation of the elements:

- _name_: the name of the material, only useful to humans.
- _template_: coordinates of the material in the automatically-generated materials template.
- _strength_: the maximum relative stretch (length difference wrt rest length, over rest length) before a spring of this material breaks. 
            For example: 0.01 means that a spring will break after it gets shorter or longer by 1% of its rest length.
- _mass_: the mass of the material, in Kg. The mass is really the product of its nominal mass (the real physical mass of the material) with its density (how much of that material is a in a cubic meter). 
        For example, iron has a mass of 7950 Kg/m3, but the "Iron Hull" material does not represent a cubic meter of iron (that would be insane), but rather some *structure* made of iron (think, a truss or a beam)	which only uses 17% of the cubic meter.
- _is hull_: whether or not a point or a spring of this material is permeable to water
- _stiffness_: how strongly a spring of this material wants to return to its original length. This is not really free, as values higher than 1.2 tend to make springs explode. Lower values will though make the spring quite soft.
- _color key_: the RGB colour to use in the structural layer image to tell the game which material to use for a vertex.
- _render color_: the RGB colour to use when rendering the material, when there is not separate texture layer image.
- _sound type_: the type of sound to play when a spring or point of this material is stressed or breaks. Valid values are: "Cable", "Glass", "Metal", "Wood".

# Electrical Materials

Each material is as follows:
    
    {
        "name": "Self-Powered Lamp",	
	"template": {
		"row": "0|Lamps",
		"column": "0|Self-Powered Lamp"
	},
        "color_key": "#FFFF00",
	"electrical_type": "Lamp",
	"is_self_powered": true
    },
    
Here's an explanation of the elements:

- _name_: the name of the material, only useful to humans.
- _template_: coordinates of the material in the automatically-generated materials template.
- _color key_: the RGB colour to use in the structural or electrical image to tell the game which electrical characteristics to use for the vertex.
- _electrical type_: the type of electrical behaviour of points and springs of this material. Valid values are:
	- "Generator": propagates an electrical current through all connected cables.
	- "Cable": propagates an electrical current through its endpoints.
	- "Lamp": emits light.
- _is self powered_: whether a lamp emits light on its own (when *true*) or only when it's powered by an electrical current (when *false*).
