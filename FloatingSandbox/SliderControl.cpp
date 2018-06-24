/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SliderControl.h"

#include <wx/stattext.h>

#include <cassert>

const long ID_SLIDER = wxNewId();

SliderControl::SliderControl(
    wxWindow * parent,
    int width,
    int height,
    std::string const & label,
    float currentValue,
    std::function<void(float)> onValueChanged,
    std::unique_ptr<ISliderCore> sliderCore)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition, 
        wxSize(width, height),
        wxBORDER_NONE)
    , mOnValueChanged(std::move(onValueChanged))
    , mSliderCore(std::move(sliderCore))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    //
    // Slider
    //

    mSlider = std::make_unique<wxSlider>(
        this, 
        ID_SLIDER, 
        mSliderCore->ValueToTick(currentValue), 
        0, 
        mSliderCore->GetNumberOfTicks(), 
        wxDefaultPosition, 
        wxSize(-1, height),
        wxSL_VERTICAL | wxSL_LEFT | wxSL_INVERSE | wxSL_AUTOTICKS, wxDefaultValidator);

    mSlider->SetTickFreq(4);

    Connect(ID_SLIDER, wxEVT_SLIDER, (wxObjectEventFunction)&SliderControl::OnSliderScroll);

    sizer->Add(mSlider.get(), 1, wxALIGN_CENTRE);


    //
    // Label
    //

    wxStaticText * stiffnessLabel = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);

    sizer->Add(stiffnessLabel, 0, wxALIGN_CENTRE);


    //
    // Text control
    //

    mTextCtrl = std::make_unique<wxTextCtrl>(this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_CENTRE);

    mTextCtrl->SetValue(std::to_string(currentValue));

    sizer->Add(mTextCtrl.get(), 0, wxALIGN_CENTRE);

    this->SetSizerAndFit(sizer);
}

SliderControl::~SliderControl()
{
}

void SliderControl::OnSliderScroll(wxScrollEvent & /*event*/)
{
    float value = mSliderCore->TickToValue(mSlider->GetValue());

    mTextCtrl->SetValue(std::to_string(value));

    // Notify value
    mOnValueChanged(value);
}
