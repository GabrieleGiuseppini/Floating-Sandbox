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
template<typename TTuple>
class ScalarTimeSeriesProbeControl : public wxPanel
{
public:

    using PenTuple = typename change_tuple_element_type<wxPen, TTuple>::type;

    ScalarTimeSeriesProbeControl(
        wxWindow * parent,
        int width);

    virtual ~ScalarTimeSeriesProbeControl() = default;

    void SetPens(PenTuple pens);

    void RegisterSample(TTuple values);

    void UpdateSimulation();

    void Reset();

private:

    void OnMouseClick(wxMouseEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnEraseBackground(wxPaintEvent & event);

    void Render(wxDC& dc);

    // Tuple kung-fu

    template<size_t... Is>
    static TTuple InitTuple(float initValue, std::integer_sequence<size_t, Is...>)
    {
        return TTuple{ initValue... };
    }

    template<typename...TElement>
    static std::tuple<TElement...> InitTuple(float initValue)
    {
        return InitTuple(initValue, std::make_index_sequence<sizeof...(TElement)>{});
    }



    //template <typename TSourceElement>
    //struct TupleHelper;

    //template <typename...TSourceElement>
    //struct TupleHelper<std::tuple<TSourceElement...>>
    //{
    //    static std::tuple<TSourceElement...> InitTuple(float value)
    //    {
    //        return std::tuple<TSourceElement...>(value...);
    //    }
    //};

    //static TTuple InitTuple(float value)
    //{
    //    return TupleHelper<TTuple>::InitTuple(value);
    //}

    template<size_t... Is>
    static TTuple Min(TTuple const & t1, TTuple const & t2, std::integer_sequence<size_t, Is...>)
    {
        return TTuple{ std::min(std::get<Is>(t1), std::get<Is>(t2))... };
    }

    template<typename...TElement>
    static TTuple Min(std::tuple<TElement...> const & t1, std::tuple<TElement...> const & t2)
    {
        static_assert(sizeof...(TElement) == std::tuple_size<TTuple>{});
        return Min(t1, t2, std::make_index_sequence<sizeof...(TElement)>{});
    }

    template<size_t... Is>
    static TTuple Max(TTuple const & t1, TTuple const & t2, std::integer_sequence<size_t, Is...>)
    {
        return TTuple{ std::max(std::get<Is>(t1), std::get<Is>(t2))... };
    }

    template<typename...TElement>
    static TTuple Max(std::tuple<TElement...> const & t1, std::tuple<TElement...> const & t2)
    {
        static_assert(sizeof...(TElement) == std::tuple_size<TTuple>{});
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

    TTuple mMaxValues;
    TTuple mMinValues;
    float mGridValueSize;

    CircularList<TTuple, 200> mSamples;
};
