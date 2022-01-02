/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-10-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/event.h>
#include <wx/valnum.h>

#include <limits>
#include <memory>
#include <type_traits>

struct TextValidatorFactory
{
	template<typename TValue,
		typename std::enable_if_t<
		!std::is_integral<TValue>::value
		&& !std::is_floating_point<TValue>::value> * = nullptr>
	static std::unique_ptr<wxValidator> CreateInstance(TValue const & minValue, TValue const & maxValue);

	template<typename TValue, typename std::enable_if_t<std::is_floating_point<TValue>::value> * = nullptr>
	static std::unique_ptr<wxValidator> CreateInstance(TValue const & minValue, TValue const & /*maxValue*/)
	{
		auto validator = std::make_unique<wxFloatingPointValidator<TValue>>();

		float minRange;
		if (minValue >= 0.0f)
			minRange = 0.0f;
		else
			minRange = std::numeric_limits<TValue>::lowest();

		validator->SetRange(minRange, std::numeric_limits<TValue>::max());

		return validator;
	}

	template<typename TValue, typename std::enable_if_t<std::is_integral<TValue>::value> * = nullptr>
	static std::unique_ptr<wxValidator> CreateInstance(TValue const & minValue, TValue const & /*maxValue*/)
	{
		auto validator = std::make_unique<wxIntegerValidator<TValue>>();

		TValue minRange;
		if (minValue >= 0)
			minRange = 0;
		else
			minRange = std::numeric_limits<TValue>::lowest();

		validator->SetRange(minRange, std::numeric_limits<TValue>::max());

		return validator;
	}
};
