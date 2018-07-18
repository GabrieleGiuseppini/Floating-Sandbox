/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <UILib/ISliderCore.h>

#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <functional>
#include <memory>
#include <string>

/*
 * This control incorporates a slider and a textbox that shows the 
 * current mapped float value of the slider.
 *
 * The control takes a core that provides the logic that maps
 * slider positions to float vaues.
 */
class SliderControl : public wxPanel
{
public:

    SliderControl(
        wxWindow * parent,
        int width,
        int height,
        std::string const & label,
        float currentValue,
        std::function<void(float)> onValueChanged,
        std::unique_ptr<ISliderCore> sliderCore);
    
    virtual ~SliderControl();

    float GetValue() const
    {
        return mSliderCore->TickToValue(mSlider->GetValue());
    }

    void SetValue(float value)
    {
        mSlider->SetValue(mSliderCore->ValueToTick(value));
        mTextCtrl->SetValue(std::to_string(value));
    }

private:

    void OnSliderScroll(wxScrollEvent & event);

private:

    std::unique_ptr<wxSlider> mSlider;
    std::unique_ptr<wxTextCtrl> mTextCtrl;

    std::function<void(float)> const mOnValueChanged;
    std::unique_ptr<ISliderCore> const mSliderCore;
};
