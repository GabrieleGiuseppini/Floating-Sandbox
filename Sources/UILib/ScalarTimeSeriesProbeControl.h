/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/CircularList.h>

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
template<typename...TElement>
class ScalarTimeSeriesProbeControl : public wxPanel
{
public:

    using ValueTuple = std::tuple<TElement...>;

    // TODO: see if can use ...TElement
    using PenTuple = typename change_tuple_element_type<wxPen, ValueTuple>::type;

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width);

    virtual ~ScalarTimeSeriesProbeControl() = default;

    void SetPens(PenTuple pens);

    void RegisterSample(ValueTuple values);

    void UpdateSimulation();

    void Reset();

private:

    void OnMouseClick(wxMouseEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void Render(wxDC& dc);

    // Tuple kung-fu

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

    static int MapValueToY(float value, float minValue, float maxValue);

    // TODOTEST
    //using IntTuple = typename change_tuple_element_type<int, TTuple>::type;

    //template<size_t... Is>
    //IntTuple MapValueToY(TTuple const & t, std::integer_sequence<size_t, Is...>) const
    //{
    //    return TTuple{ MapValueToY(std::get<Is>(t), std::get<Is>(mMinValues), std::get<Is>(mMaxValues))... };
    //}

    //IntTuple MapValueToY(TTuple const & t) const
    //{
    //    return MapValueToY(t, std::make_index_sequence<sizeof...(TTuple)>{});
    //}

private:

    int const mWidth;

    std::unique_ptr<wxBitmap> mBufferedDCBitmap;
    PenTuple mTimeSeriesPens;
    wxPen mGridPen;

    ValueTuple mMaxValues;
    ValueTuple mMinValues;
    float mGridValueSize;

    CircularList<ValueTuple, 200> mSamples;
};
