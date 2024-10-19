/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

namespace Physics
{
    class Clouds;
    class ElectricalElements;
    class Fishes;
    class Frontiers;
    class Gadgets;
    class Npcs;
    class NpcParticles;
    class OceanFloor;
    class OceanSurface;
    class PinnedPoints;
	class Points;
	class Ship;
	class Springs;
    class Stars;
    class Storm;
	class Triangles;
    class Wind;
	class World;
}

#include <GameCore/ElementContainer.h>

#include "IShipPhysicsHandler.h"

#include "Storm.h"
#include "Wind.h"

#include "Formulae.h"

#include "Points.h"
#include "Springs.h"
#include "Triangles.h"
#include "ElectricalElements.h"
#include "Frontiers.h"
#include "NpcSimulation/NpcParticles.h"

#include "Clouds.h"
#include "Fishes.h"
#include "Stars.h"
#include "OceanFloor.h"
#include "OceanSurface.h"
#include "World.h"

#include "Gadget.h"
#include "AntiMatterBombGadget.h"
#include "ImpactBombGadget.h"
#include "PhysicsProbeGadget.h"
#include "RCBombGadget.h"
#include "TimerBombGadget.h"
#include "Gadgets.h"
#include "PinnedPoints.h"
#include "Ship.h"

#include "NpcSimulation/Npcs.h"