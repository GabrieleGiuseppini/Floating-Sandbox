/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <functional>
#include <string>

// The progress value is the progress that will be reached at the end of the operation.

struct SimpleProgressCallback final
{
	static SimpleProgressCallback Dummy()
	{
		return SimpleProgressCallback([](float) {});
	}

	SimpleProgressCallback(std::function<void(float progress)> callback)
			: mCallback(std::move(callback))
			, mMinOutputRange(0.0f)
			, mOutputRangeWidth(1.0f)
	{}

	SimpleProgressCallback(
		std::function<void(float progress)> callback, // Will be invoked with values in range
		float minOutputRange,
		float outputRangeWidth)
			: mCallback(std::move(callback))
			, mMinOutputRange(minOutputRange)
			, mOutputRangeWidth(outputRangeWidth)
	{}

	/*
	 * User provides 0.0-1.0, but the original callback
	 * is invoked on our range.
	 */
	void operator()(float progress) const
	{
		mCallback(mMinOutputRange + progress * mOutputRangeWidth);
	}

	/*
	 * User of the new callback provides 0.0-1.0, but this callback
	 * is invoked on the specified range - chained with our range.
	 */
	SimpleProgressCallback MakeSubCallback(
		float minOutputRange,
		float outputRangeWidth) const
	{
		return SimpleProgressCallback(
			[this](float progress)
			{
				(*this)(progress);
			},
			minOutputRange,
			outputRangeWidth);
	}

private:

	std::function<void(float progress)> const mCallback;
	float const mMinOutputRange;
	float const mOutputRangeWidth;
};

enum class ProgressMessageType : std::size_t
{
	None = 0,						// Used when no message is propagated
	LoadingFonts,					// "Loading fonts..."
	InitializingOpenGL,				// "Initializing OpenGL..."
	LoadingShaders,					// "Loading shaders..."
	InitializingNoise,				// "Initializing noise..."
	LoadingGenericTextures,			// "Loading generic textures..."
	LoadingExplosionTextureAtlas,	// "Loading explosion texture atlas..."
	LoadingCloudTextureAtlas,		// "Loading cloud texture atlas..."
	LoadingFishTextureAtlas,		// "Loading fish texture atlas..."
	LoadingWorldTextures,			// "Loading world textures..."
	InitializingGraphics,			// "Initializing graphics..." - 10
	InitializingUI,					// "Initializing UI..."
	LoadingSounds,					// "Loading sounds..."
	LoadingMusic,					// "Loading music..."
	LoadingElectricalPanel,			// "Loading electrical panel..." - 14
	LoadingShipBuilder,				// "Loading ShipBuilder..."
	LoadingMaterialPalette,			// "Loading materials palette..."
	Calibrating,					// "Calibrating game on the computer..." - 17
	Ready,							// "Ready!"

	_Last = Ready
};

struct ProgressCallback final
{
	ProgressCallback(std::function<void(float progress, ProgressMessageType message)> callback)
			: mCallback(std::move(callback))
			, mMinOutputRange(0.0f)
			, mOutputRangeWidth(1.0f)
	{}

	ProgressCallback(
		std::function<void(float progress, ProgressMessageType message)> callback, // Will be invoked with values in range
		float minOutputRange,
		float outputRangeWidth)
			: mCallback(std::move(callback))
		  	, mMinOutputRange(minOutputRange)
		  	, mOutputRangeWidth(outputRangeWidth)
	{}

	/*
	 * User provides 0.0-1.0, but the original callback
	 * is invoked on our range.
	 */
	void operator()(float progress, ProgressMessageType message) const
	{
		mCallback(mMinOutputRange + progress * mOutputRangeWidth, message);
	}

	/*
	 * User of the new callback provides 0.0-1.0, but this callback
	 * is invoked on the specified range - chained with our range.
	 */
	ProgressCallback MakeSubCallback(
		float minOutputRange,
		float outputRangeWidth) const
	{
		return ProgressCallback(
			[this](float progress, ProgressMessageType message)
			{
				(*this)(progress, message);
			},
			minOutputRange,
            outputRangeWidth);
	}

	/*
	 * User of the new callback provides 0.0-1.0, but this callback
	 * is invoked on the specified range - chained with our range.
	 */
	SimpleProgressCallback MakeSubCallback(
		float minOutputRange,
		float outputRangeWidth,
		ProgressMessageType message) const
	{
		return SimpleProgressCallback(
			[this, message=message](float progress)
			{
				(*this)(progress, message);
			},
			minOutputRange,
            outputRangeWidth);
	}

	/*
	 * Creates a new simple callback that outputs the same range as this callback,
	 * on the specified callback.
	 */
    SimpleProgressCallback CloneToSimpleCallback(std::function<void(float progress)> callback) const
    {
        return SimpleProgressCallback(
            callback,
            mMinOutputRange,
            mOutputRangeWidth);
    }

private:

	std::function<void(float progress, ProgressMessageType message)> mCallback;
	float const mMinOutputRange;
	float const mOutputRangeWidth;
};

