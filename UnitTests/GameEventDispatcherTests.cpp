#include <GameLib/GameEventDispatcher.h>

#include "gmock/gmock.h"

class _MockHandler : public IGameEventHandler
{
public:

    MOCK_METHOD3(OnDestroy, void(StructuralMaterial const & material, bool isUnderwater, unsigned int size));
    MOCK_METHOD3(OnBreak, void(StructuralMaterial const & material, bool isUnderwater, unsigned int size));
    MOCK_METHOD2(OnPinToggled, void(bool isPinned, bool isUnderwater));
    MOCK_METHOD3(OnStress, void(StructuralMaterial const & material, bool isUnderwater, unsigned int size));
    MOCK_METHOD1(OnSinkingBegin, void(ShipId shipId));
};

using namespace ::testing;

using MockHandler = StrictMock<_MockHandler>;

/////////////////////////////////////////////////////////////////

TEST(GameEventDispatcherTests, Aggregates_OnDestroy)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterSink(&handler);

    StructuralMaterial const & pm1 = *(reinterpret_cast<StructuralMaterial *>(7));

    EXPECT_CALL(handler, OnDestroy(_, _, _)).Times(0);

    dispatcher.OnDestroy(pm1, true, 3);
    dispatcher.OnDestroy(pm1, true, 2);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnDestroy(_, true, 5)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, Aggregates_OnDestroy_MultipleKeys)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterSink(&handler);

    StructuralMaterial * pm1 = reinterpret_cast<StructuralMaterial *>(7);
    StructuralMaterial * pm2 = reinterpret_cast<StructuralMaterial *>(21);

    EXPECT_CALL(handler, OnDestroy(_, _, _)).Times(0);

    dispatcher.OnDestroy(*pm2, false, 1);
    dispatcher.OnDestroy(*pm1, false, 3);
    dispatcher.OnDestroy(*pm2, false, 2);
    dispatcher.OnDestroy(*pm1, false, 9);
    dispatcher.OnDestroy(*pm1, false, 1);
    dispatcher.OnDestroy(*pm2, true, 2);
    dispatcher.OnDestroy(*pm2, true, 2);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnDestroy(_, false, 13)).Times(1);
    EXPECT_CALL(handler, OnDestroy(_, false, 3)).Times(1);
    EXPECT_CALL(handler, OnDestroy(_, true, 4)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, Aggregates_OnPinToggled)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterSink(&handler);

    EXPECT_CALL(handler, OnPinToggled(_, _)).Times(0);

    dispatcher.OnPinToggled(true, false);
    dispatcher.OnPinToggled(true, false);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnPinToggled(true, false)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, Aggregates_OnPinToggled_MultipleKeys)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterSink(&handler);

    EXPECT_CALL(handler, OnPinToggled(_, _)).Times(0);

    dispatcher.OnPinToggled(true, false);
    dispatcher.OnPinToggled(true, false);
    dispatcher.OnPinToggled(false, false);
    dispatcher.OnPinToggled(true, true);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnPinToggled(true, false)).Times(1);
    EXPECT_CALL(handler, OnPinToggled(false, false)).Times(1);
    EXPECT_CALL(handler, OnPinToggled(true, true)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, Aggregates_OnSinkingBegin)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterSink(&handler);

    EXPECT_CALL(handler, OnSinkingBegin(_)).Times(0);

    dispatcher.OnSinkingBegin(7);
    dispatcher.OnSinkingBegin(7);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnSinkingBegin(7)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, Aggregates_OnSinkingBegin_MultipleShips)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterSink(&handler);

    EXPECT_CALL(handler, OnSinkingBegin(_)).Times(0);

    dispatcher.OnSinkingBegin(7);
    dispatcher.OnSinkingBegin(3);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnSinkingBegin(3)).Times(1);
    EXPECT_CALL(handler, OnSinkingBegin(7)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}

TEST(GameEventDispatcherTests, ClearsStateAtUpdate)
{
    MockHandler handler;

    GameEventDispatcher dispatcher;
    dispatcher.RegisterSink(&handler);

    StructuralMaterial * pm1 = reinterpret_cast<StructuralMaterial *>(7);

    EXPECT_CALL(handler, OnDestroy(_, _, _)).Times(0);

    dispatcher.OnDestroy(*pm1, false, 3);
    dispatcher.OnDestroy(*pm1, false, 2);

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnDestroy(_, false, 5)).Times(1);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);

    EXPECT_CALL(handler, OnDestroy(_, _, _)).Times(0);

    dispatcher.Flush();

    Mock::VerifyAndClear(&handler);
}