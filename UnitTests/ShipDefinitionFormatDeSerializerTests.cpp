#include <Game/ShipDefinitionFormatDeSerializer.h>

#include "gtest/gtest.h"

TEST(ShipDefinitionFormatDeSerializerTests, Metadata_Full_WithoutElectricalPanel)
{
    DeSerializationBuffer<BigEndianess> buffer(256);

    // Write

    ShipMetadata md("Test ship");
    md.ArtCredits = "KillerWhale";
    md.Author = "Gabriele Giuseppini";
    md.Description = "Supercaligragilisticexpiralidocius";
    md.Password = 0x1122334455667788;
    md.ShipName = "Best ship";
    md.YearBuilt = "2020-2021";

    ShipDefinitionFormatDeSerializer::AppendMetadata(md, buffer);

    // Read

    // TODOHERE
}