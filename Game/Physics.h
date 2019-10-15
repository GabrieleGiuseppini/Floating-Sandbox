/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

namespace Physics
{
    class Bombs;    
    class Clouds;
    class ElectricalElements;
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

#include "Points.h"
#include "Springs.h"
#include "Triangles.h"
#include "ElectricalElements.h"

#include "Storm.h"
#include "Clouds.h"
#include "Stars.h"
#include "OceanFloor.h"
#include "OceanSurface.h"
#include "Wind.h"
#include "World.h"

#include "Bomb.h"
#include "AntiMatterBomb.h"
#include "ImpactBomb.h"
#include "RCBomb.h"
#include "TimerBomb.h"
#include "Bombs.h"

#include "ForceFields.h"
#include "PinnedPoints.h"

#include "Ship.h"
