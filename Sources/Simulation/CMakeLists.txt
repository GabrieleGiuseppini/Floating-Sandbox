
#
# Simulation library
#

set  (SIMULATION_SOURCES
	ElectricalPanel.h
	FishSpeciesDatabase.cpp
	FishSpeciesDatabase.h
	LayerElements.h
	Layers.cpp
        Layers.h
	Materials.cpp
	Materials.h
	MaterialDatabase.cpp
	MaterialDatabase.h
	NpcDatabase.cpp
	NpcDatabase.h
	RopeBuffer.h
	ShipAutoTexturizationSettings.cpp
	ShipAutoTexturizationSettings.h
	ShipDefinition.h
	ShipDefinitionFormatDeSerializer.cpp
	ShipDefinitionFormatDeSerializer.h
	ShipFactoryTypes.h
	ShipLoadOptions.h
	ShipMetadata.h
	ShipPhysicsData.h
	ShipPreviewData.h
	ShipStrengthRandomizer.cpp
	ShipStrengthRandomizer.h
	SimulationParameters.cpp
	SimulationParameters.h
)

source_group(" " FILES ${SIMULATION_SOURCES})

add_library (Simulation ${SIMULATION_SOURCES})

target_include_directories(Simulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Simulation PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Simulation INTERFACE ..)

target_link_libraries (Simulation
	Core
	Render
	${ADDITIONAL_LIBRARIES})
