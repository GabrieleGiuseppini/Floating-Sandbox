/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/CircularList.h>
#include <Core/EnumFlags.h>

#include <wx/wx.h>

#include <memory>
#include <tuple>

// Machinery to define tuple of given cardinality and different type

template <typename TSource, typename TNew>
struct type_coercer
{
    using type = TNew;
};

template <typename TNewElement, typename TSourceElement>
struct change_tuple_element_type;

template <typename TNewElement, typename... TSourceElement>
struct change_tuple_element_type<TNewElement, std::tuple<TSourceElement...>>
{
    using type = std::tuple<typename type_coercer<TSourceElement, TNewElement>::type ...>;
};


/*
 * Multi-series time-based graph; performs scroll automatically.
 */

enum class ScalarTimeSeriesProbeControlOptions
{
    None = 0,
    ZeroLine = 1
};

template <> struct is_flag<ScalarTimeSeriesProbeControlOptions> : std::true_type {
};


template<typename...TElement>
class ScalarTimeSeriesProbeControl : public wxPanel
{
public:

    using ValueTuple = std::tuple<TElement...>;

    using PenTuple = typename change_tuple_element_type<wxPen, ValueTuple>::type;
    using ColorTuple = typename change_tuple_element_type<wxColor, ValueTuple>::type;

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width,
        PenTuple pens)
        : ScalarTimeSeriesProbeControl(
            parent,
            width,
            pens,
            ScalarTimeSeriesProbeControlOptions::None)
    { }

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width,
        PenTuple pens,
        ScalarTimeSeriesProbeControlOptions flags);

    virtual ~ScalarTimeSeriesProbeControl() = default;

    void RegisterSample(ValueTuple values);

    void UpdateSimulation();

    void Reset();

private:

    void OnMouseClick(wxMouseEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void Render(wxDC& dc);

    template<size_t... Is>
    void DrawCharts(wxDC& dc, std::integer_sequence<size_t, Is...>)
    {
        (DrawChart<Is>(dc), ...);
    }

    template<size_t IElement>
    void DrawChart(wxDC& dc);

    // Tuple kung-fu

    template<size_t... Is>
    static ColorTuple MakeDarkerColors(PenTuple const & t, std::integer_sequence<size_t, Is...>)
    {
        return ColorTuple{ std::get<Is>(t).GetColour().ChangeLightness(50)... };
    }

    static ColorTuple MakeDarkerColors(PenTuple const & t)
    {
        return MakeDarkerColors(t, std::make_index_sequence<sizeof...(TElement)>{});
    }

    static wxPen MakeZeroLinePen(wxPen const & pen)
    {
        return wxPen(pen.GetColour(), 1, wxPENSTYLE_SHORT_DASH);
    }

    template<size_t... Is>
    static PenTuple MakeZeroLinePens(PenTuple const & t, std::integer_sequence<size_t, Is...>)
    {
        return PenTuple{ MakeZeroLinePen(std::get<Is>(t))... };
    }

    static PenTuple MakeZeroLinePens(PenTuple const & t)
    {
        return MakeZeroLinePens(t, std::make_index_sequence<sizeof...(TElement)>{});
    }

    static ValueTuple InitTuple(float initValue)
    {
        return ValueTuple{ TElement(initValue)... };
    }

    template<size_t... Is>
    static ValueTuple Min(ValueTuple const & t1, ValueTuple const & t2, std::integer_sequence<size_t, Is...>)
    {
        return ValueTuple{ std::min(std::get<Is>(t1), std::get<Is>(t2))... };
    }

    static ValueTuple Min(ValueTuple const & t1, ValueTuple const & t2)
    {
        return Min(t1, t2, std::make_index_sequence<sizeof...(TElement)>{});
    }

    template<size_t... Is>
    static ValueTuple Max(ValueTuple const & t1, ValueTuple const & t2, std::integer_sequence<size_t, Is...>)
    {
        return ValueTuple{ std::max(std::get<Is>(t1), std::get<Is>(t2))... };
    }

    static ValueTuple Max(ValueTuple const & t1, ValueTuple const & t2)
    {
        return Max(t1, t2, std::make_index_sequence<sizeof...(TElement)>{});
    }

    template<size_t IElement>
    int MapValueToY(ValueTuple const & t) const;

    template<size_t IElement>
    int MapValueToY(float value) const;

private:

    int const mWidth;

    std::unique_ptr<wxBitmap> mBufferedDCBitmap;
    PenTuple const mTimeSeriesPens;
    ColorTuple const mLabelColors;
    PenTuple const mZeroLinePens;
    wxPen const mGridPen;
    ScalarTimeSeriesProbeControlOptions const mFlags;

    ValueTuple mMaxValues;
    ValueTuple mMinValues;
    float mGridValueSize;

    CircularList<ValueTuple, 200> mSamples;
};
