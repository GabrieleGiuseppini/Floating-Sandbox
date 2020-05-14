# Overview
Each materials file is a (json) *array* of materials; json arrays start with:
```
    [
```
and end with:
```
    ]
```
Materials are separated by commas, as follows:
```
    {
	...
        "name": "Iron Hull",
        ...
    },
    {
	...
        "name": "Iron",
        ...
    },
    {
	...
        "name": "Wood",
        ...
    }
```

Note: the last material must NOT be followed by a comma!

# Structural Materials

Structural materials are as follows:
```
    {	
        "color_key": "#404050",
	"buoyancy_volume_fill": 0.0,
	"combustion_type": "Combustion", 
        "ignition_temperature": 1588.15, 
        "is_hull": true, 
        "mass": {
            "density": 0.0935, 
            "nominal_mass": 7950
        }, 
	"melting_temperature": 1783.15, 
        "name": "Iron Hull", 
        "render_color": "#404050", 
        "rust_receptivity": 1.0, 
        "sound_type": "Metal", 
	"specific_heat": 449.0, 
        "stiffness": 1.0, 
        "strength": 0.055, 
        "template": {
            "column": 0, 
            "row": "0|Iron Hull"
        }, 
	"thermal_conductivity": 79.5,
	"thermal_expansion_coefficient": 0.0000106,
        "water_diffusion_speed": 0.5, 
        "water_intake": 1.0, 
        "water_retention": 0.05,         
	"wind_receptivity": 0.0
    }, 
```

Here's an explanation of the elements:

- _color key_: the RGB color to use in the structural layer image to tell the game which material to use for a particle.
- _buoyancy volume fill_: the fraction of 1m3 of this material that may be occupied by air or water, and at the same time, the fraction of 1m3 of air or water that this particle displaces. This parameter basically controls buoyancy.
   - For example, 0.5 means that this particle may take in up to half a cubic meter of air or water, and that this particle also displaces half a cubic meter of air or water.
- _combustion type_: the type of combustion; one of "Combustion" and "Explosion".
- _explosive combustion radius_: the radius of explosions, when the material is characterized by explosive combustion.
- _explosive combustion strength_: the strength of explosions, when the material is characterized by explosive combustion.
- _ignition temperature_: the temperature, in Kelvin, at which the material starts burning.
- _is hull_: whether or not a point or a spring of this material is permeable to water.
- _mass_: the mass of the material, in Kg. The mass is really the product of its nominal mass (the real physical mass of the material) with its density (how much of that material is a in a cubic meter). 
   - For example, iron has a mass of 7950 Kg/m3, but the "Iron Hull" material does not represent a cubic meter of iron (that would be insane), but rather some *structure* made of iron (think, a truss or a beam) which only fills 9.35% of that cubic meter.
- _melting temperature_: the temperature, in Kelvin, at which the material starts melting.
- _name_: the name of the material; only useful to carbon-based entities.
- _render color_: the RGB color to use when rendering certain particles of this material.
- _rust receptivity_: the degree to which this material rusts when wet; when not set, assumed to be 1.0. A value of 0.0 turns off rusting for this material altogether.
- _sound type_: the type of sound to play when a spring or point of this material is stressed or breaks. Valid values are: "AirBubble", "Cable", "Cloth", "Glass", "Metal", "Plastic", "Rubber", and "Wood".
- _specific heat_: how much heat one kilogram of this material needs in order to raise its temperature by one Kelvin, in J/(Kg*K)
- _stiffness_: how quickly a spring of this material wants to return to its original length. This is not really free, as values higher than 1.2 tend to make springs explode. Lower values will though make the spring quite soft.
- _strength_: the maximum relative stretch (length difference wrt rest length, over rest length) before a spring of this material breaks. 
   - For example, 0.01 means that a spring will break after it gets shorter or longer by 1% of its rest length.
- _template_: coordinates of the material in the automatically-generated materials template.
- _thermal conductivity_: the speed with which heat propagates along this material, in W/(m*K).
- _thermal_expansion_coefficient_: the amount by which the volume of the material changes with temperature, in 1/K.
- _water diffusion speed_: the speed with which water at this particle spreads out of it. Technically, it's the fraction of water at this particle that is allowed to leave the particle towards its neighbors.
- _water intake_: the amount of water that enters (i.e. is absorbed) or leaves this particle when the particle is leaking; when not set, assumed to be 1.0.
- _water retention_: the amount of water that will remain in this particle when the particle is leaking and finds itself at a pressure point lower than the pressure of the water it contains.
- _wind receptivity_: the amount of wind force that this material feels when it is above the water line; when not set, assumed to be 0.0.

# Electrical Materials

Each material is as follows:
``` 
    {
        "name": "High Lamp",	
        "template": {
                "row": "0|Lamps",
                "column": "0|High Lamp"
        },
        "color_key": "#FFE010",
        "electrical_type": "Lamp",
	"heat_generated": 900.0,
        "is_self_powered": false,
        "luminiscence": 2.0,
        "light_spread": 9.0,
	"minimum_operating_temperature": 233.15,
	"maximum_operating_temperature": 398.15, 
        "wet_failure_rate": 0.0
    },
``` 
Here's an explanation of the elements:

- _name_: the name of the material, only useful to humans.
- _template_: coordinates of the material in the automatically-generated materials template.
- _color key_: the RGB colour to use in the structural or electrical image to tell the game which electrical characteristics to use for the vertex.
- _conducts electricity_: whether by default allows current to flow through the material. Used by switches to indicate whether they are "on" or "off" by default.
- _electrical type_: the type of electrical behaviour of points and springs of this material. Valid values are:
   - "Generator": injects an electrical current into all connected cables.
   - "Cable": propagates an electrical current through its endpoints.
   - "Engine": exherts thrust when enabled via an immediately nearby EngineController element.
   - "EngineController": controls the power and direction of all nearby Engine elements.
   - "InteractiveSwitch": a switch that may be controlled via the electrical panel.
   - "Lamp": emits light.
   - "OtherSink": produces heat while operating.
   - "PowerMonitor": displays a light on the electrical panel when current flows through it.
   - "ShipSound": emits a sound when turned on; see _ship sound type_ property below for a list of the sounds that are available.
   - "SmokeEmitter": emits smoke - ideal for funnels and engine exhaust pipes.
   - "WaterPump": when powered, takes water either in or out, depending on the nominal force of the pump.
   - "WaterSensingSwitch": a switch that toggles automatically when wet.
   - "WatertightDoor": when powered, inverts the "hull-ness" of the underlying particle - i.e. it either allows water to flow through the particle, or prevents water from flowing through it.
- _engine direction_: when the element is of the Engine type, describes the main axis of the engine thrust, in radians.
- _engine power_: when the element is of the Engine type, describes the maximum engine power, in HP.
- _engine responsiveness_: when the element is of the Engine type, describes how quickly the engine responds to throttle changes; between 0.0 (totally unresponsive) and 1.0 (immediate response).
- _engine type_: when the element is of the Engine type, describes the type of engine. Valid values are:
   - "Outboard": an outboard engine, suitable for boats and small ships.
   - "Diesel": a diesel engine, suitable for small and mid-size ships.
   - "Steam": a steam engine, suitable for large ships.
- _heat generated_: the amount of heat generated when functioning, in KJ/s.
- _interactive switch type_: when the element is of the InteractiveSwitch type, describes the type of switch. Valid values are:
   - "Push": the switch returns to its rest position when released.
   - "Toggle": the switch can be toggled on and off.
- _is self powered_: whether a lamp emits light on its own (when *true*) or only when it's powered by an electrical current (when *false*).
- _luminiscence_: the amount of light emitted by a lamp; between 0.0 and 1.0.
- _light spread_: the distance from a lamp, in metres, beyond which that lamp's light is zero.
- _minimum operating temperature_: the minimum temperature, in Kelvin, below which the material/device will stop working.
- _maximum operating temperature_: the maximum temperature, in Kelvin, above which the material/device will stop working.
- _particle emission rate_: when the element is of the SmokeEmitter type, dictates the average interval - in simulation time seconds - between two particle emissions.
   - For example, 0.5 means that on average two particles will be emitted each second.
- _ship sound type_: when the element is of the ShipSound type, describes the type of sound that is played when the element is activated. Valid values are:
   - "Bell1", "Bell2": alarm bell sounds.
   - "Horn1", "Horn2", "Horn3": steam horn sounds.
   - "Klaxon1": klaxon sounds (submarine alarm).
- _wet failure rate_: the average number of lamps of this material that will fail in a minute when wet.
   - For example, 2.0 means that a wet lamp will most likely turn off after 30 seconds of becoming wet.
- _water pump nominal force_: when the element is of the WaterPump type, describes the force with which the pump takes water in (when positive) or out (when negative).
