#include <Simulation/Materials.h>
#include <Simulation/Physics/Physics.h>
#include <Simulation/SimulationEventDispatcher.h>
#include <Simulation/SimulationParameters.h>

#include <initializer_list>
#include <iterator>

#include "gtest/gtest.h"

#include <algorithm>
#include <memory>

class ShipTests : public ::testing::Test
{
    virtual void SetUp() override
    {
        std::shared_ptr<SimulationEventDispatcher> dispatcher = std::make_shared<SimulationEventDispatcher>();
        mWorld = std::make_unique<Physics::World>(dispatcher);
    }

    virtual void TearDown() override
    {
    }

protected:

	static MaterialDatabase MakeMaterials(std::vector<Material> && materials)
	{
		std::vector<std::unique_ptr<Material const>> res;
		for (Material const & m : materials)
			res.emplace_back(new Material(m));

		return MaterialDatabase::Create(std::move(res));
	}

    std::unique_ptr<Physics::World> mWorld;
};


///////////////////////////////////////////////////////////////////////
// Points
///////////////////////////////////////////////////////////////////////

TEST_F(ShipTests, BuildsPoints_OnePoint)
{
	auto materials = MakeMaterials(
		{
			{ "Mat1", 1.0f, 1.0f, { 0, 0, 0 }, { 1, 1, 1 }, false, true, std::nullopt, std::nullopt },
			{ "Mat2", 1.0f, 1.0f, { 25, 30, 35 }, { 25, 30, 35 }, false, false, std::nullopt, std::nullopt }
		}
	);

	unsigned char imageDataSrc[] = {
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	25, 30, 35,			0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
	};

    auto imageData = new unsigned char [sizeof(imageDataSrc)];
    memcpy(imageData, imageDataSrc, sizeof(imageDataSrc));

	auto ship = Physics::Ship::Create(
        0,
		mWorld.get(),
        ShipDefinition(
            ImageData(4, 5, std::unique_ptr<unsigned char const []>(imageData)),
            std::nullopt,
            "Test",
            vec2f()),
		materials);

	ASSERT_TRUE(!!ship);

	ASSERT_EQ(1u, ship->GetPoints().size());

	Physics::Point const & pt = *(ship->GetPoints().begin());
	EXPECT_EQ(vec2f(-1.0f, 3.0f), pt.GetPosition());
    EXPECT_EQ(materials.Get({ 25, 30, 35 }), pt.GetMaterial());
    EXPECT_EQ(0u, pt.GetConnectedSprings().size());
	EXPECT_EQ(0u, pt.GetConnectedTriangles().size());
    EXPECT_EQ(nullptr, pt.GetConnectedElectricalElement());
	EXPECT_TRUE(pt.IsLeaking());
}

TEST_F(ShipTests, BuildsPoints_TwoPoints)
{

    auto materials = MakeMaterials(
        {
            { "Mat1", 1.0f, 1.0f, { 0, 0, 0 }, { 40, 45, 50 }, true, true, std::nullopt, std::nullopt },
            { "Mat2", 1.0f, 1.0f, { 25, 30, 35 }, { 25, 30, 35 }, false, false, std::nullopt, std::nullopt }
        }
    );

    unsigned char imageDataSrc[] = {
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	25, 30, 35,			0, 0, 0,			0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
    };

    auto imageData = new unsigned char[sizeof(imageDataSrc)];
    memcpy(imageData, imageDataSrc, sizeof(imageDataSrc));

    auto ship = Physics::Ship::Create(
        0,
        mWorld.get(),
        ShipDefinition(
            ImageData(4, 5, std::unique_ptr<unsigned char const[]>(imageData)),
            std::nullopt,
            "Test",
            vec2f()),
        materials);

    ASSERT_TRUE(!!ship);

    ASSERT_EQ(2u, ship->GetPoints().size());

    auto pointIt = ship->GetPoints().begin();

    Physics::Point const & pt1 = *pointIt;
    EXPECT_EQ(vec2f(-1.0f, 3.0f), pt1.GetPosition());
    EXPECT_EQ(materials.Get({ 25, 30, 35 }), pt1.GetMaterial());
    EXPECT_EQ(1u, pt1.GetConnectedSprings().size());
    EXPECT_EQ(0u, pt1.GetConnectedTriangles().size());
    EXPECT_TRUE(pt1.IsLeaking());

    ++pointIt;

    Physics::Point const & pt2 = *pointIt;
    EXPECT_EQ(vec2f(0.0f, 3.0f), pt2.GetPosition());
    EXPECT_EQ(materials.Get({ 0, 0, 0 }), pt2.GetMaterial());
    EXPECT_EQ(1u, pt2.GetConnectedSprings().size());
	EXPECT_EQ(0u, pt2.GetConnectedTriangles().size());
	EXPECT_FALSE(pt2.IsLeaking());
}

TEST_F(ShipTests, BuildsPoints_EmptyShip)
{
	auto materials = MakeMaterials(
		{
			{ "Mat1", 1.0f, 1.0f, { 0, 0, 0 }, { 1, 1, 1 }, false, true, std::nullopt, std::nullopt },
			{ "Mat2", 1.0f, 1.0f, { 25, 30, 35 }, { 25, 30, 35 }, false, false, std::nullopt, std::nullopt }
		}
	);

	unsigned char imageDataSrc[] = {
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0x03, 0x02, 0x01,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
	};

    auto imageData = new unsigned char[sizeof(imageDataSrc)];
    memcpy(imageData, imageDataSrc, sizeof(imageDataSrc));

	auto ship = Physics::Ship::Create(
        0,
        mWorld.get(),
        ShipDefinition(
            ImageData(4, 3, std::unique_ptr<unsigned char const[]>(imageData)),
            std::nullopt,
            "Test",
            vec2f()),
		materials);

	ASSERT_TRUE(!!ship);

	EXPECT_EQ(0u, ship->GetPoints().size());
}

///////////////////////////////////////////////////////////////////////
// Springs
///////////////////////////////////////////////////////////////////////

TEST_F(ShipTests, BuildsSprings_OneSpring)
{
	auto materials = MakeMaterials(
		{
            { "Mat1", 1.0f, 1.0f, { 0, 0, 0 }, { 40, 45, 50 }, false, true, std::nullopt, std::nullopt },
            { "Mat2", 1.0f, 1.0f, { 25, 30, 35 }, { 25, 30, 35 }, false, false, std::nullopt, std::nullopt }
        }
	);

	unsigned char imageDataSrc[] = {
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	25, 30, 35,			0, 0, 0,			0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
	};

    auto imageData = new unsigned char[sizeof(imageDataSrc)];
    memcpy(imageData, imageDataSrc, sizeof(imageDataSrc));

	auto ship = Physics::Ship::Create(
        0,
        mWorld.get(),
        ShipDefinition(
            ImageData(4, 5, std::unique_ptr<unsigned char const[]>(imageData)),
            std::nullopt,
            "Test",
            vec2f()),
        materials);

	ASSERT_TRUE(!!ship);

	ASSERT_EQ(1u, ship->GetSprings().size());

	Physics::Spring const & sp1 = *(ship->GetSprings().begin());
	ASSERT_NE(nullptr, sp1.GetPointA());
	EXPECT_EQ(vec2f(-1.0f, 3.0f), sp1.GetPointA()->GetPosition());
	ASSERT_NE(nullptr, sp1.GetPointB());
	EXPECT_EQ(vec2f(0.0f, 3.0f), sp1.GetPointB()->GetPosition());
	EXPECT_EQ(materials.Get({ 0, 0, 0 }), sp1.GetMaterial());
}

///////////////////////////////////////////////////////////////////////
// Triangles
///////////////////////////////////////////////////////////////////////

TEST_F(ShipTests, BuildsTriangles_OneTriangle)
{
	auto materials = MakeMaterials(
		{
            { "Mat1", 1.0f, 1.0f, { 0, 0, 0 }, { 40, 45, 50 }, false, true, std::nullopt, std::nullopt },
            { "Mat2", 1.0f, 1.0f, { 25, 30, 35 }, { 25, 30, 35 }, false, false, std::nullopt, std::nullopt }
        }
	);

	unsigned char imageDataSrc[] = {
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	25, 30, 35,			0, 0, 0,			0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	25, 30, 35,			0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
	};

    auto imageData = new unsigned char[sizeof(imageDataSrc)];
    memcpy(imageData, imageDataSrc, sizeof(imageDataSrc));

	auto ship = Physics::Ship::Create(
        0,
        mWorld.get(),
        ShipDefinition(
            ImageData(4, 5, std::unique_ptr<unsigned char const[]>(imageData)),
            std::nullopt,
            "Test",
            vec2f()),
        materials);

	ASSERT_TRUE(!!ship);

	ASSERT_EQ(1u, ship->GetTriangles().size());

	Physics::Triangle const & tr1 = *(ship->GetTriangles().begin());

	ASSERT_NE(nullptr, tr1.GetPointA());
	ASSERT_NE(nullptr, tr1.GetPointB());
	ASSERT_NE(nullptr, tr1.GetPointC());

    ASSERT_EQ(1U, tr1.GetPointA()->GetConnectedTriangles().size());
    ASSERT_EQ(1U, tr1.GetPointB()->GetConnectedTriangles().size());
    ASSERT_EQ(1U, tr1.GetPointC()->GetConnectedTriangles().size());

	std::set<Physics::Point const *> distinctPoints;
	distinctPoints.insert(tr1.GetPointA());
	distinctPoints.insert(tr1.GetPointB());
	distinctPoints.insert(tr1.GetPointC());

	ASSERT_EQ(3u, distinctPoints.size());
}

///////////////////////////////////////////////////////////////////////
// Destroy
///////////////////////////////////////////////////////////////////////

TEST_F(ShipTests, DestroyAt)
{
    GameParameters gameParameters;

	auto materials = MakeMaterials(
		{
            { "Mat1", 1.0f, 1.0f, { 40, 45, 50 }, { 40, 45, 50 }, false, false, std::nullopt, std::nullopt },
            { "Mat2", 1.0f, 1.0f, { 25, 30, 35 }, { 25, 30, 35 }, false, false, std::nullopt, std::nullopt },

            { "XXXX", 1.0f, 1.0f, { 77, 77, 77 }, { 77, 77, 77 }, false, false, std::nullopt, std::nullopt },
            { "YYYY", 1.0f, 1.0f, { 0, 0, 0 }, { 66, 66, 66 }, false, true, std::nullopt, std::nullopt }
		}
	);

	unsigned char imageDataSrc[] = {
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0, 0, 0,			40, 45, 50,			0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	77, 77, 77,			40, 45, 50,			0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	25, 30, 35,			40, 45, 50,			0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
	};

    auto imageData = new unsigned char[sizeof(imageDataSrc)];
    memcpy(imageData, imageDataSrc, sizeof(imageDataSrc));

	auto ship = Physics::Ship::Create(
        0,
        mWorld.get(),
        ShipDefinition(
            ImageData(4, 5, std::unique_ptr<unsigned char const[]>(imageData)),
            std::nullopt,
            "Test",
            vec2f()),
        materials);

	ASSERT_TRUE(!!ship);

	EXPECT_EQ(6u, ship->GetPoints().size());
	EXPECT_EQ(11u, ship->GetSprings().size());
	EXPECT_EQ(8u, ship->GetTriangles().size());

    Physics::Point const * testPoint = nullptr;
    Physics::Point const * sentinelPoint = nullptr;
    for (auto const & pt : ship->GetPoints())
    {
        if (pt.GetMaterial()->Name == "XXXX")
            testPoint = &pt;
        if (pt.GetMaterial()->Name == "YYYY")
            sentinelPoint = &pt;
    }

    ASSERT_NE(testPoint, nullptr);
    ASSERT_NE(sentinelPoint, nullptr);

    EXPECT_EQ(5u, testPoint->GetConnectedSprings().size());
    EXPECT_EQ(6u, testPoint->GetConnectedTriangles().size());

    EXPECT_EQ(3u, sentinelPoint->GetConnectedSprings().size());
    EXPECT_EQ(3u, sentinelPoint->GetConnectedTriangles().size());


    //
    // Destroy test point now
    //

	ship->DestroyAt(
        vec2f(-1.0f, 2.0f),
        0.1f,
        gameParameters);

    EXPECT_EQ(5u, std::count_if(ship->GetPoints().begin(), ship->GetPoints().end(), [](auto const & p) {return !p.IsDeleted(); }));
	EXPECT_EQ(6u, std::count_if(ship->GetSprings().begin(), ship->GetSprings().end(), [](auto const & s) {return !s.IsDeleted(); }));
	EXPECT_EQ(0u, std::count_if(ship->GetTriangles().begin(), ship->GetTriangles().end(), [](auto const & t) {return !t.IsDeleted(); }));

    EXPECT_EQ(2u, sentinelPoint->GetConnectedSprings().size());
    // These days we nuke all triangles connected to each connected point
    EXPECT_EQ(0u, sentinelPoint->GetConnectedTriangles().size());
}

///////////////////////////////////////////////////////////////////////
// Lamps
///////////////////////////////////////////////////////////////////////

TEST_F(ShipTests, BuildsLamps_OneLamp)
{
    auto materials = MakeMaterials(
        {
            { "Mat1", 1.0f, 1.0f, { 0, 0, 0 }, { 40, 45, 50 }, false, true, std::nullopt, std::nullopt },
            { "Mat2", 1.0f, 1.0f, { 25, 30, 35 }, { 25, 30, 35 }, false, false, {{Material::ElectricalProperties::ElectricalElementType::Lamp, 0.0f, 0.0f}}, std::nullopt }
        }
    );

    unsigned char imageDataSrc[] = {
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	25, 30, 35,			0, 0, 0,			0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,	0xFF, 0xFF, 0xFF,
    };

    auto imageData = new unsigned char[sizeof(imageDataSrc)];
    memcpy(imageData, imageDataSrc, sizeof(imageDataSrc));

    auto ship = Physics::Ship::Create(
        0,
        mWorld.get(),
        ShipDefinition(
            ImageData(4, 5, std::unique_ptr<unsigned char const[]>(imageData)),
            std::nullopt,
            "Test",
            vec2f()),
        materials);

    ASSERT_TRUE(!!ship);

    ASSERT_EQ(1u, ship->GetElectricalElements().size());

    Physics::ElectricalElement const * el1 = *(ship->GetElectricalElements().begin());
    EXPECT_EQ(Physics::ElectricalElement::Type::Lamp, el1->GetType());
    EXPECT_EQ(vec2f(-1.0f, 3.0f), el1->GetPoint()->GetPosition());
}
