###VERTEX-120

#define in attribute
#define out varying

// Inputs
in vec4 inUnderwaterPlantStatic1;  // Position, PersonalitySeed, TODO
in vec4 inUnderwaterPlantStatic2; // TODO
in float inUnderwaterPlantStatic3; // speciesIndex
in float inUnderwaterPlantDynamic1; // VertexWorldOceanRelativeY, negative underneath

// Outputs
out float vertexSpeciesIndex;
out float vertexWorldOceanRelativeY;

// Parameters
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexSpeciesIndex = inUnderwaterPlantStatic3;
    vertexWorldOceanRelativeY = inUnderwaterPlantDynamic1;

    gl_Position = paramOrthoMatrix * vec4(inUnderwaterPlantStatic1.xy, -1.0, 1.0);
}

###FRAGMENT-120

#define in varying

// Inputs from previous shader
in float vertexSpeciesIndex;
in float vertexWorldOceanRelativeY;

void main()
{
    gl_FragColor = vec4(204./255., 239./255., 240./255., 1.0);
}
