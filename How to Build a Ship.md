# Overview
Ships in Floating Sandbox can be built by authoring simple image files saved in the TODO:italic:png format. The positions of pixels in the image provide
the positions of the particles (points) of the ship, while the colors of the pixels map to characteristics of the particles such as their mass, strength, 
the sound they make when they break, and so on. Floating sandbox comes with two palettes - structural_materials_template and 
electrical_materials_template - that list the colors that you may use together with the various characteristics that they represent.

There are basically two methods to author a ship: a basic method, and a more advanced method.
The basic method consists of providing a single png image containing all the information about the characteristics of the ship's particles; the 
simplicity of this method is counterbalanced by the lack of fine-grained control over the final look-and-feel of the ship, as the ship
will be rendered using the palette colors used for the materials, and by a restricted spectrum of electrical materials that may be chosen from.
The more advanced method consists of providing multiple png images, each providing characteristics specifics to a domain:
- A first image provides the "structural layer", that is, the physical characteristics of particles such as mass, strength, sound, stiffness, and so on;
- Another image provides the "electrical layer", that is, the electrical nature and electrical characteristics of particles such as whether
   they are lamps or generators, their conductivity, their luminiscence, and so on;
- Yet another image provides the "ropes layer", which contains endpoints of ropes that are conveniently automatically completed by Floating Sandbox;
- A final image provides the "texture layer", which is the image that is visible during the game.

The only mandatory layer is the "structural layer", and this is the layer that Floating Sandbox implicitly assumes it's loading when importing 
a simple png. In order to work with multiple layers instead, you would craft a simple JSON file that specifies the names of the image files
providing the individual layers. Also in this case the only mandatory layer is the "structural layer", with all the other layers being optional.

# Structural Layer
TODO

# Electrical Layer
TODO

# Ropes Layer
A ropes layer image is an image that contains pairs of pixels with the same color; for each such a pair, Floating Sandbox will automagically fill-in 
a segment (the "rope") between the two points corresponding to the pixels, each point of the segment being of the "Rope" material.
These are the rules to follow when drawing an image for the ropes layer of a ship:
- The size of the ropes layer image must match the size of the structural layer image.
- A rope is specified via a pair of pixels with matching colors; the coordinates of the pixels will be used as the coordinates of the endpoints 
 of the rope.
-- It follows that there may only exist two pixels with the same color. Having only one pixel with a given color will cause an error, as will having
   three or more pixels with the same color.
- For every rope endpoint pixel in the rope layer, there must exist a corresponding pixel in the structural layer.
- The background must be full white (#FFFFFF). Non-white pixels will be assumed to be part of a rope endpoint pair.

Note that it is also possible to create ropes without a separate ropes layer image: you may in fact draw the rope endpoints directly in the
structural layer image, and they will behave exactly as if they were in the ropes layer. In this case however you are constrained to use colors
in the range from #000000 to #000FFF, and the structural material of the endpoints themselves will have to be the "Rope" material, as opposed to
being whatever structural material specified by the structural layer.

# Texture Layer
The texture layer image is a TODO.

# .shp File
TODO
