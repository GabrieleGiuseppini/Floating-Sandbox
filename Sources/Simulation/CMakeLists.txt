
#
# Simulation library
#

set  (SIMULATION_SOURCES
	ElectricalPanel.h
	FishSpeciesDatabase.cpp
	FishSpeciesDatabase.h
	ISimulationEventHandlers.h
	LayerElements.h
	Layers.cpp
        Layers.h
	Materials.cpp
	Materials.h
	MaterialDatabase.cpp
	MaterialDatabase.h
	NpcDatabase.cpp
	NpcDatabase.h
	OceanFloorHeightMap.cpp
	OceanFloorHeightMap.h
	RopeBuffer.h
	ShipAutoTexturizationSettings.cpp
	ShipAutoTexturizationSettings.h
	ShipDefinition.h
	ShipDefinitionFormatDeSerializer.cpp
	ShipDefinitionFormatDeSerializer.h
	ShipFactoryTypes.h
	ShipLoadOptions.h
	ShipMetadata.h
	ShipOverlays.cpp
	ShipOverlays.h
	ShipPhysicsData.h
	ShipPreviewData.h
	ShipStrengthRandomizer.cpp
	ShipStrengthRandomizer.h
	SimulationEventDispatcher.h
	SimulationParameters.cpp
	SimulationParameters.h
)

set  (PHYSICS_SOURCES
	Physics/ElectricalElements.cpp
	Physics/ElectricalElements.h
	Physics/Formulae.h
	Physics/Frontiers.cpp
	Physics/Frontiers.h
	Physics/IShipPhysicsHandler.h
	Physics/Npcs/NpcParticles.cpp
	Physics/Npcs/NpcParticles.h
	Physics/Npcs/Npcs.cpp
	Physics/Npcs/Npcs.h
	Physics/Npcs/Npcs_HumanSimulation.cpp
	Physics/Npcs/Npcs_Simulation.cpp
	Physics/Physics.h
	Physics/Points.cpp
	Physics/Points.h
	Physics/Springs.cpp
	Physics/Springs.h
	Physics/Storm.cpp
	Physics/Storm.h
	Physics/Triangles.cpp
	Physics/Triangles.h
	Physics/Wind.cpp
	Physics/Wind.h
)

source_group(" " FILES ${SIMULATION_SOURCES})
source_group("Physics" FILES ${PHYSICS_SOURCES})

add_library (Simulation ${SIMULATION_SOURCES} ${PHYSICS_SOURCES})

target_include_directories(Simulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Simulation PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Simulation INTERFACE ..)

target_link_libraries (Simulation
	Core
	Render
	${ADDITIONAL_LIBRARIES})
