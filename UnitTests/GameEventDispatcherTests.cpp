#include <Game/GameEventDispatcher.h>

#include "gmock/gmock.h"

class _MockGameEventHandler
    : public IStructuralGameEventHandler
    , public ILifecycleGameEventHandler
{
public:

    MOCK_METHOD3(OnBreak, void(StructuralMaterial const & material, bool isUnderwater, unsigned int size));
    MOCK_METHOD3(OnStress, void(StructuralMaterial const & material, bool isUnderwater, unsigned int size));
    MOCK_METHOD1(OnSinkingBegin, void(ShipId shipId));
};

using namespace ::testing;

using MockHandler = StrictMock<_MockGameEventHandler>;

StructuralMaterial MakeStructuralMaterial(std::string name)
{
    return StructuralMaterial(
        name,
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        std::nullopt,
        std::nullopt, // Sound
        "TestMaterial",
        // Water
        false,
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        // Heat
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        StructuralMaterial::MaterialCombustionType::Combustion,
        0.0f, // Radius
        0.0f, // Strength
        // Misc
        1.0f,
        false,
        // Palette
        vec4f::zero(),
        std::nullopt);
}

/////////////////////////////////////////////////////////////////

TEST(GameEventDispatcherTests, Aggregates_OnStress)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterStructuralEventHandler(&handler);

    StructuralMaterial sm = MakeStructuralMaterial("Foo");

    EXPECT_CALL(handler, OnStress(_, _, _)).Times(0);

    dispatcher.OnStress(sm, true, 3);
    dispatcher.OnStress(sm, true, 2);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnStress(Field(&StructuralMaterial::Name, "Foo"), true, 5)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, Aggregates_OnStress_MultipleKeys)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterStructuralEventHandler(&handler);

    StructuralMaterial sm1 = MakeStructuralMaterial("Foo1");

    StructuralMaterial sm2 = MakeStructuralMaterial("Foo2");

    EXPECT_CALL(handler, OnStress(_, _, _)).Times(0);

    dispatcher.OnStress(sm2, false, 1);
    dispatcher.OnStress(sm1, false, 3);
    dispatcher.OnStress(sm2, false, 2);
    dispatcher.OnStress(sm1, false, 9);
    dispatcher.OnStress(sm1, false, 1);
    dispatcher.OnStress(sm2, true, 2);
    dispatcher.OnStress(sm2, true, 2);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnStress(Field(&StructuralMaterial::Name, "Foo1"), false, 13)).Times(1);
    EXPECT_CALL(handler, OnStress(Field(&StructuralMaterial::Name, "Foo2"), false, 3)).Times(1);
    EXPECT_CALL(handler, OnStress(Field(&StructuralMaterial::Name, "Foo2"), true, 4)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, OnSinkingBegin)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterLifecycleEventHandler(&handler);

    EXPECT_CALL(handler, OnSinkingBegin(7)).Times(1);

    dispatcher.OnSinkingBegin(7);

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, OnSinkingBegin_MultipleShips)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterLifecycleEventHandler(&handler);

    EXPECT_CALL(handler, OnSinkingBegin(3)).Times(1);
    EXPECT_CALL(handler, OnSinkingBegin(7)).Times(1);

    dispatcher.OnSinkingBegin(7);
    dispatcher.OnSinkingBegin(3);

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, ClearsStateAtUpdate)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterStructuralEventHandler(&handler);

    StructuralMaterial sm = MakeStructuralMaterial("Foo");

    EXPECT_CALL(handler, OnStress(_, _, _)).Times(0);

    dispatcher.OnStress(sm, false, 3);
    dispatcher.OnStress(sm, false, 2);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnStress(Field(&StructuralMaterial::Name, "Foo"), false, 5)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnStress(_, _, _)).Times(0);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}