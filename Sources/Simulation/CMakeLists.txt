
#
# Simulation library
#

set  (SIMULATION_SOURCES
	ElectricalPanel.h
	EventRecorder.h
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
	ShipFactory.cpp
	ShipFactory.h
	ShipFactoryTypes.h
	ShipFloorplanizer.cpp
	ShipFloorplanizer.h
	ShipLoadOptions.h
	ShipMetadata.h
	ShipOverlays.cpp
	ShipOverlays.h
	ShipPhysicsData.h
	ShipPreviewData.h
	ShipStrengthRandomizer.cpp
	ShipStrengthRandomizer.h
	ShipTexturizer.cpp
	ShipTexturizer.h
	SimulationEventDispatcher.h
	SimulationParameters.cpp
	SimulationParameters.h
)

set  (PHYSICS_SOURCES
	Physics/AntiMatterBombGadget.cpp
	Physics/AntiMatterBombGadget.h
	Physics/Clouds.cpp
	Physics/Clouds.h
	Physics/ElectricalElements.cpp
	Physics/ElectricalElements.h
	Physics/FireExtinguishingBombGadget.cpp
	Physics/FireExtinguishingBombGadget.h
	Physics/Fishes.cpp
	Physics/Fishes.h
	Physics/Formulae.h
	Physics/Frontiers.cpp
	Physics/Frontiers.h
	Physics/Gadget.h
	Physics/Gadgets.cpp
	Physics/Gadgets.h
	Physics/ImpactBombGadget.cpp
	Physics/ImpactBombGadget.h
	Physics/IShipPhysicsHandler.h
	Physics/Npcs/NpcParticles.cpp
	Physics/Npcs/NpcParticles.h
	Physics/Npcs/Npcs.cpp
	Physics/Npcs/Npcs.h
	Physics/Npcs/Npcs_HumanSimulation.cpp
	Physics/Npcs/Npcs_Simulation.cpp
	Physics/OceanFloor.cpp
	Physics/OceanFloor.h
	Physics/OceanSurface.cpp
	Physics/OceanSurface.h
	Physics/Physics.h
	Physics/PhysicsProbeGadget.cpp
	Physics/PhysicsProbeGadget.h
	Physics/PinnedPoints.cpp
	Physics/PinnedPoints.h
	Physics/Points.cpp
	Physics/Points.h
	Physics/RCBombGadget.cpp
	Physics/RCBombGadget.h
	Physics/Ship.cpp
	Physics/Ship.h
	Physics/Ship_Interactions.cpp
	Physics/Ship_Interactions_Repair.cpp
	Physics/Ship_SpringRelaxation.cpp
	Physics/Ship_StateMachines.cpp
	Physics/Ship_StateMachines.h
	Physics/ShipElectricSparks.cpp
	Physics/ShipElectricSparks.h
	Physics/Springs.cpp
	Physics/Springs.h
	Physics/Stars.cpp
	Physics/Stars.h
	Physics/Storm.cpp
	Physics/Storm.h
	Physics/TimerBombGadget.cpp
	Physics/TimerBombGadget.h
	Physics/Triangles.cpp
	Physics/Triangles.h
	Physics/Wind.cpp
	Physics/Wind.h
	Physics/World.cpp
	Physics/World.h
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
