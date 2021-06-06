/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-06-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

// TODOTEST
#include <GameCore/Log.h>

#include <wx/wx.h>

#include <cassert>
#include <memory>

/*
 * This wrapper injects a handler in the event chain of its base class, forwarding
 * keyboard events to a specific target window.
 */
template<typename TBase>
class KeyEventForwardingControl : public TBase
{
public:

    template<typename...TArgs>
    KeyEventForwardingControl(TArgs&&... args)
        : TBase(std::forward<TArgs>(args)...)
        , mKeyEventHandler()
    {
    }

    virtual ~KeyEventForwardingControl()
    {
        if (!!mKeyEventHandler)
        {
            auto const * oldEventHandler = this->PopEventHandler(false);
            assert(oldEventHandler == mKeyEventHandler.get());
            (void)oldEventHandler;
        }
    }

    void ForwardKeyEventsTo(wxWindow * keyEventTarget)
    {
        assert(!mKeyEventHandler);

        mKeyEventHandler = std::make_unique<KeyEventHandler>(keyEventTarget);
        this->PushEventHandler(mKeyEventHandler.get());
    }

    void ForwardKeyEventsToParent()
    {
        assert(!mKeyEventHandler);

        mKeyEventHandler = std::make_unique<KeyEventHandler>(this->GetParent());
        this->PushEventHandler(mKeyEventHandler.get());
    }

private:

    class KeyEventHandler : public wxEvtHandler
    {
    public:

        KeyEventHandler(wxWindow * target)
            : wxEvtHandler()
            , mTarget(target)
        {
            this->Bind(wxEVT_KEY_UP, &KeyEventHandler::OnKeyUp, this);
            this->Bind(wxEVT_KEY_DOWN, &KeyEventHandler::OnKeyDown, this);
        }

        // TODO: just for diagnostics
        ~KeyEventHandler()
        {
            LogMessage("TODO");
        }

    private:

        void OnKeyUp(wxKeyEvent & event)
        {
            LogMessage("TODOTEST: Intercepted: KeyUp");
            mTarget->ProcessWindowEvent(event);
        }

        void OnKeyDown(wxKeyEvent & event)
        {
            LogMessage("TODOTEST: Intercepted: KeyDown");
            mTarget->ProcessWindowEvent(event);
        }

        wxWindow * const mTarget;
    };

    std::unique_ptr<KeyEventHandler> mKeyEventHandler;
};