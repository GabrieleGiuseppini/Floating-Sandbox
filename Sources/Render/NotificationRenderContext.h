/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameShaderSet.h"
#include "GlobalRenderContext.h"
#include "RenderParameters.h"

#include <OpenGLCore/ShaderManager.h>

#include <Core/FontSet.h>
#include <Core/GameTextureDatabases.h>
#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/TextureAtlas.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/*
 * This class implements the machinery for rendering UI notifications.
 *
 * The class is fully owned by the RenderContext class.
 *
 * The geometry produced by this class is highly dependent on the screen
 * (canvas) size; for this reason, this context has to remember enough data
 * about the primitives it renders so to be able to re-calculate vertex
 * buffers when the screen size changes.
 */
class NotificationRenderContext
{

private:

	enum class TextNotificationType
	{
		StatusText = 0,
		NotificationText = 1,
		PhysicsProbeReading = 2,

		_Last = PhysicsProbeReading
	};

	enum class NotificationAnchorPositionType
	{
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
		PhysicsProbeReadingDepth,
		PhysicsProbeReadingPressure,
		PhysicsProbeReadingSpeed,
		PhysicsProbeReadingTemperature,
	};

public:

	NotificationRenderContext(
		IAssetManager const & assetManager,
		ShaderManager<GameShaderSet::ShaderSet> & shaderManager,
		GlobalRenderContext & globalRenderContext);

public:

	void UploadStart();

	inline void UploadStatusTextStart()
	{
		UploadTextStart(TextNotificationType::StatusText);
	}

	inline void UploadStatusTextLine(
		std::string const & text,
		AnchorPositionType anchor,
		vec2f const & screenOffset, // In font cell-size fraction (0.0 -> 1.0)
		float alpha)
	{
		auto & textNotificationContext = mTextNotificationTypeContexts[static_cast<size_t>(TextNotificationType::StatusText)];

		textNotificationContext.TextLines.emplace_back(
			text,
			TranslateAnchorPosition(anchor),
			screenOffset,
			alpha);
	}

	inline void UploadStatusTextEnd()
	{
		// Nop
	}

	inline void UploadNotificationTextStart()
	{
		UploadTextStart(TextNotificationType::NotificationText);
	}

	inline void UploadNotificationTextLine(
		std::string const & text,
		AnchorPositionType anchor,
		vec2f const & screenOffset, // In font cell-size fraction (0.0 -> 1.0)
		float alpha)
	{
		auto & textNotificationContext = mTextNotificationTypeContexts[static_cast<size_t>(TextNotificationType::NotificationText)];

		textNotificationContext.TextLines.emplace_back(
			text,
			TranslateAnchorPosition(anchor),
			screenOffset,
			alpha);
	}

	inline void UploadNotificationTextEnd()
	{
		// Nop
	}

	inline void UploadTextureNotificationStart()
	{
		//
		// Texture notifications are sticky: we upload them once in a while and
		// continue drawing the same buffer
		//

		// Cleanup buffers
		mTextureNotifications.clear();
		mIsTextureNotificationDataDirty = true;
	}

	inline void UploadTextureNotification(
		TextureFrameId<GameTextureDatabases::GenericLinearTextureGroups> const & textureFrameId,
		AnchorPositionType anchor,
		vec2f const & screenOffset, // In texture-size fraction (0.0 -> 1.0)
		float alpha)
	{
		//
		// Store notification data
		//

		mTextureNotifications.emplace_back(
			textureFrameId,
			anchor,
			screenOffset,
			alpha);
	}

	inline void UploadTextureNotificationEnd()
	{
		// Nop
	}

	inline void UploadPhysicsProbePanel(float open, bool isOpening)
	{
		if (open != 0.0f)
		{
			mPhysicsProbePanel.emplace(open, isOpening);
		}
		else
		{
			mPhysicsProbePanel.reset();
		}

		// Remember panel is dirty
		mIsPhysicsProbeDataDirty = true;
	}

	inline void UploadPhysicsProbeReading(
		std::string const & speed,
		std::string const & temperature,
		std::string const & depth,
		std::string const & pressure)
	{
		auto & textNotificationContext = mTextNotificationTypeContexts[static_cast<size_t>(TextNotificationType::PhysicsProbeReading)];

		textNotificationContext.TextLines.clear();

		textNotificationContext.TextLines.emplace_back(
			speed,
			NotificationAnchorPositionType::PhysicsProbeReadingSpeed,
			vec2f::zero(),
			1.0f);

		textNotificationContext.TextLines.emplace_back(
			temperature,
			NotificationAnchorPositionType::PhysicsProbeReadingTemperature,
			vec2f::zero(),
			1.0f);

		textNotificationContext.TextLines.emplace_back(
			depth,
			NotificationAnchorPositionType::PhysicsProbeReadingDepth,
			vec2f::zero(),
			1.0f);

		textNotificationContext.TextLines.emplace_back(
			pressure,
			NotificationAnchorPositionType::PhysicsProbeReadingPressure,
			vec2f::zero(),
			1.0f);

		textNotificationContext.AreTextLinesDirty = true;
	}

	inline void UploadPhysicsProbeReadingClear()
	{
		auto & textNotificationContext = mTextNotificationTypeContexts[static_cast<size_t>(TextNotificationType::PhysicsProbeReading)];

		textNotificationContext.TextLines.clear();
		textNotificationContext.AreTextLinesDirty = true;
	}

	inline void UploadHeatBlasterFlame(
		vec2f const & centerPosition,
		float radius,
		HeatBlasterActionType action)
	{
		//
		// Populate vertices
		//

		float const quadHalfSize = (radius * 1.5f) / 2.0f; // Add some slack for transparency
		float const left = centerPosition.x - quadHalfSize;
		float const right = centerPosition.x + quadHalfSize;
		float const top = centerPosition.y + quadHalfSize;
		float const bottom = centerPosition.y - quadHalfSize;

		switch (action)
		{
			case HeatBlasterActionType::Cool:
			{
				// Triangle 1

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameCool(
						vec2f(left, bottom),
						vec2f(-1.0f, -1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameCool(
						vec2f(left, top),
						vec2f(-1.0f, 1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameCool(
						vec2f(right, bottom),
						vec2f(1.0f, -1.0f)));

				// Triangle 2

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameCool(
						vec2f(left, top),
						vec2f(-1.0f, 1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameCool(
						vec2f(right, bottom),
						vec2f(1.0f, -1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameCool(
						vec2f(right, top),
						vec2f(1.0f, 1.0f)));

				break;
			}

			case HeatBlasterActionType::Heat:
			{
				// Triangle 1

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameHeat(
						vec2f(left, bottom),
						vec2f(-1.0f, -1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameHeat(
						vec2f(left, top),
						vec2f(-1.0f, 1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameHeat(
						vec2f(right, bottom),
						vec2f(1.0f, -1.0f)));

				// Triangle 2

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameHeat(
						vec2f(left, top),
						vec2f(-1.0f, 1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameHeat(
						vec2f(right, bottom),
						vec2f(1.0f, -1.0f)));

				mMultiNotificationVertexBuffer.emplace_back(
					MultiNotificationVertex::MakeHeatBlasterFlameHeat(
						vec2f(right, top),
						vec2f(1.0f, 1.0f)));

				break;
			}
		}
	}

	inline void UploadFireExtinguisherSpray(
		vec2f const & centerPosition,
		float radius)
	{
		//
		// Populate vertices
		//

		float const quadHalfSize = (radius * 3.5f) / 2.0f; // Add some slack to account for transparency
		float const left = centerPosition.x - quadHalfSize;
		float const right = centerPosition.x + quadHalfSize;
		float const top = centerPosition.y + quadHalfSize;
		float const bottom = centerPosition.y - quadHalfSize;

		// Triangle 1

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeFireExtinguisherSpray(
				vec2f(left, bottom),
				vec2f(-1.0f, -1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeFireExtinguisherSpray(
				vec2f(left, top),
				vec2f(-1.0f, 1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeFireExtinguisherSpray(
				vec2f(right, bottom),
				vec2f(1.0f, -1.0f)));

		// Triangle 2

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeFireExtinguisherSpray(
				vec2f(left, top),
				vec2f(-1.0f, 1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeFireExtinguisherSpray(
				vec2f(right, bottom),
				vec2f(1.0f, -1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeFireExtinguisherSpray(
				vec2f(right, top),
				vec2f(1.0f, 1.0f)));
	}

	inline void UploadBlastToolHalo(
		vec2f const & centerPosition,
		float radius,
		float renderProgress,
		float personalitySeed)
	{
		//
		// Populate vertices
		//

		float const left = centerPosition.x - radius;
		float const right = centerPosition.x + radius;
		float const top = centerPosition.y + radius;
		float const bottom = centerPosition.y - radius;

		// Triangle 1

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeBlastToolHalo(
				vec2f(left, bottom),
				renderProgress,
				vec2f(-1.0f, -1.0f),
				personalitySeed));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeBlastToolHalo(
				vec2f(left, top),
				renderProgress,
				vec2f(-1.0f, 1.0f),
				personalitySeed));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeBlastToolHalo(
				vec2f(right, bottom),
				renderProgress,
				vec2f(1.0f, -1.0f),
				personalitySeed));

		// Triangle 2

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeBlastToolHalo(
				vec2f(left, top),
				renderProgress,
				vec2f(-1.0f, 1.0f),
				personalitySeed));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeBlastToolHalo(
				vec2f(right, bottom),
				renderProgress,
				vec2f(1.0f, -1.0f),
				personalitySeed));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeBlastToolHalo(
				vec2f(right, top),
				renderProgress,
				vec2f(1.0f, 1.0f),
				personalitySeed));
	}

	inline void UploadPressureInjectionHalo(
		vec2f const & centerPosition,
		float flowMultiplier)
	{
		//
		// Populate vertices
		//

		float const quadHalfSize = 9.0f / 2.0f; // Add some slack to account for transparency
		float const left = centerPosition.x - quadHalfSize;
		float const right = centerPosition.x + quadHalfSize;
		float const top = centerPosition.y + quadHalfSize;
		float const bottom = centerPosition.y - quadHalfSize;

		// Triangle 1

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakePressureInjectionHalo(
				vec2f(left, bottom),
				vec2f(-1.0f, -1.0f),
				flowMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakePressureInjectionHalo(
				vec2f(left, top),
				vec2f(-1.0f, 1.0f),
				flowMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakePressureInjectionHalo(
				vec2f(right, bottom),
				vec2f(1.0f, -1.0f),
				flowMultiplier));

		// Triangle 2

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakePressureInjectionHalo(
				vec2f(left, top),
				vec2f(-1.0f, 1.0f),
				flowMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakePressureInjectionHalo(
				vec2f(right, bottom),
				vec2f(1.0f, -1.0f),
				flowMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakePressureInjectionHalo(
				vec2f(right, top),
				vec2f(1.0f, 1.0f),
				flowMultiplier));
	}

	inline void UploadWindSphere(
		vec2f const & centerPosition,
		float preFrontRadius,
		float preFrontIntensityMultiplier,
		float mainFrontRadius,
		float mainFrontIntensityMultiplier)
	{
		//
		// Populate vertices
		//

		float const left = centerPosition.x - preFrontRadius;
		float const right = centerPosition.x + preFrontRadius;
		float const top = centerPosition.y + preFrontRadius;
		float const bottom = centerPosition.y - preFrontRadius;

		// Triangle 1

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeWindSphere(
				vec2f(left, bottom),
				vec2f(-preFrontRadius, -preFrontRadius),
				preFrontRadius,
				preFrontIntensityMultiplier,
				mainFrontRadius,
				mainFrontIntensityMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeWindSphere(
				vec2f(left, top),
				vec2f(-preFrontRadius, preFrontRadius),
				preFrontRadius,
				preFrontIntensityMultiplier,
				mainFrontRadius,
				mainFrontIntensityMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeWindSphere(
				vec2f(right, bottom),
				vec2f(preFrontRadius, -preFrontRadius),
				preFrontRadius,
				preFrontIntensityMultiplier,
				mainFrontRadius,
				mainFrontIntensityMultiplier));

		// Triangle 2

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeWindSphere(
				vec2f(left, top),
				vec2f(-preFrontRadius, preFrontRadius),
				preFrontRadius,
				preFrontIntensityMultiplier,
				mainFrontRadius,
				mainFrontIntensityMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeWindSphere(
				vec2f(right, bottom),
				vec2f(preFrontRadius, -preFrontRadius),
				preFrontRadius,
				preFrontIntensityMultiplier,
				mainFrontRadius,
				mainFrontIntensityMultiplier));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeWindSphere(
				vec2f(right, top),
				vec2f(preFrontRadius, preFrontRadius),
				preFrontRadius,
				preFrontIntensityMultiplier,
				mainFrontRadius,
				mainFrontIntensityMultiplier));
	}

	void UploadLaserCannon(
		DisplayLogicalCoordinates const & screenCenter,
		std::optional<float> strength,
		ViewModel const & viewModel);

	void UploadGripCircle(
		vec2f const & worldCenterPosition,
		float worldRadius)
	{
		//
		// Populate vertices
		//

		float const left = worldCenterPosition.x - worldRadius;
		float const right = worldCenterPosition.x + worldRadius;
		float const top = worldCenterPosition.y + worldRadius;
		float const bottom = worldCenterPosition.y - worldRadius;

		// Triangle 1

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeGripCircle(
				vec2f(left, bottom),
				vec2f(-1.0f, -1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeGripCircle(
				vec2f(left, top),
				vec2f(-1.0f, 1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeGripCircle(
				vec2f(right, bottom),
				vec2f(1.0f, -1.0f)));

		// Triangle 2

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeGripCircle(
				vec2f(left, top),
				vec2f(-1.0f, 1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeGripCircle(
				vec2f(right, bottom),
				vec2f(1.0f, -1.0f)));

		mMultiNotificationVertexBuffer.emplace_back(
			MultiNotificationVertex::MakeGripCircle(
				vec2f(right, top),
				vec2f(1.0f, 1.0f)));
	}

	void UploadRectSelection(
		vec2f const & centerPosition,
		vec2f const & verticalDir,
		float width,
		float height,
		rgbColor const & color,
		float elapsedSimulationTime,
		ViewModel const & viewModel)
	{
		float const smallestDimension = std::min(width, height);

		// Offset around quad is calc'd as a percentage of smallest dimension
		float const worldOffset = smallestDimension * 8.0f;

		//
		// Calculate world dimension multiplier so that the smallest dimension _plus_
		// a world offset is at least a desired number of pixels
		//

		// Desired number of pixels is calc'd as a fraction of the smallest canvas physical dimension size
		float const minDesiredNumberOfPixels =
			static_cast<float>(std::min(viewModel.GetCanvasPhysicalSize().width, viewModel.GetCanvasPhysicalSize().height))
			/ 30.0f;

		// Multiplier for world dimensions
		float const wDimMultiplier = std::max(
			viewModel.PhysicalDisplayOffsetToWorldOffset(minDesiredNumberOfPixels) / (smallestDimension + worldOffset),
			1.0f);

		//
		// Calculate quad
		//

		float const halfActualW = (width + worldOffset) / 2.0f * wDimMultiplier;
		float const halfActualH = (height + worldOffset) / 2.0f * wDimMultiplier;

		vec2f const centerTop = centerPosition + verticalDir * halfActualH;
		vec2f const leftTop = centerTop + verticalDir.to_perpendicular() * halfActualW;
		vec2f const rightTop = centerTop - verticalDir.to_perpendicular() * halfActualW;

		vec2f const centerBottom = centerPosition - verticalDir * halfActualH;
		vec2f const leftBottom = centerBottom + verticalDir.to_perpendicular() * halfActualW;
		vec2f const rightBottom = centerBottom - verticalDir.to_perpendicular() * halfActualW;

		//
		// Calculate the fraction of the quad's dimensions occupied by one pixel
		//

		float const onePixelWorldSize = viewModel.PhysicalDisplayOffsetToWorldOffset(1.0f);

		// VrtxSpaceSize = 2; pixelSizeInVertexSpace = VrtxSpaceSize * (onePixelWorldSize/WidthWorld)
		vec2f const pixelSizeInVertexSpace = vec2f(
			onePixelWorldSize / halfActualW,
			onePixelWorldSize / halfActualH);

		//
		// Calculate border size in vertex space
		//
		// We want the border to be a fixed number of pixels
		//

		float const borderSizePixels = minDesiredNumberOfPixels / 5.0f;

		// Convert to world size
		float const borderSizeWorld = viewModel.PhysicalDisplayOffsetToWorldOffset(borderSizePixels);

		// Convert to vertex space
		vec2f const borderSizeInVertexSpace = vec2f(
			borderSizeWorld / halfActualW,
			borderSizeWorld / halfActualH);

		//
		// Create vertices
		//

		vec3f const colorF = color.toVec3f();

		// Left, top
		mRectSelectionVertexBuffer.emplace_back(
			leftTop,
			vec2f(-1.0f, 1.0f),
			pixelSizeInVertexSpace,
			borderSizeInVertexSpace,
			colorF,
			elapsedSimulationTime);

		// Left, bottom
		mRectSelectionVertexBuffer.emplace_back(
			leftBottom,
			vec2f(-1.0f, -1.0f),
			pixelSizeInVertexSpace,
			borderSizeInVertexSpace,
			colorF,
			elapsedSimulationTime);

		// Right, top
		mRectSelectionVertexBuffer.emplace_back(
			rightTop,
			vec2f(1.0f, 1.0f),
			pixelSizeInVertexSpace,
			borderSizeInVertexSpace,
			colorF,
			elapsedSimulationTime);

		// Left, bottom
		mRectSelectionVertexBuffer.emplace_back(
			leftBottom,
			vec2f(-1.0f, -1.0f),
			pixelSizeInVertexSpace,
			borderSizeInVertexSpace,
			colorF,
			elapsedSimulationTime);

		// Right, top
		mRectSelectionVertexBuffer.emplace_back(
			rightTop,
			vec2f(1.0f, 1.0f),
			pixelSizeInVertexSpace,
			borderSizeInVertexSpace,
			colorF,
			elapsedSimulationTime);

		// Right, bottom
		mRectSelectionVertexBuffer.emplace_back(
			rightBottom,
			vec2f(1.0f, -1.0f),
			pixelSizeInVertexSpace,
			borderSizeInVertexSpace,
			colorF,
			elapsedSimulationTime);
	}

	void UploadInteractiveToolDashedLine(
		DisplayLogicalCoordinates const & screenStart,
		DisplayLogicalCoordinates const & screenEnd,
		ViewModel const & viewModel);

	void UploadEnd();

	void ProcessParameterChanges(RenderParameters const & renderParameters);

	void RenderPrepare();

	void RenderDraw();

private:

	void ApplyViewModelChanges(RenderParameters const & renderParameters);
	void ApplyCanvasSizeChanges(RenderParameters const & renderParameters);
	void ApplyEffectiveAmbientLightIntensityChanges(RenderParameters const & renderParameters);
	void ApplyDisplayUnitsSystemChanges(RenderParameters const & renderParameters);

	inline void RenderPrepareTextNotifications();
	inline void RenderDrawTextNotifications();

	inline void RenderPrepareTextureNotifications();
	inline void RenderDrawTextureNotifications();

	inline void RenderPreparePhysicsProbePanel();
	inline void RenderDrawPhysicsProbePanel();

	inline void RenderPrepareLaserCannon();
	inline void RenderDrawLaserCannon();

	inline void RenderPrepareLaserRay();
	inline void RenderDrawLaserRay();

	inline void RenderPrepareMultiNotification();
	inline void RenderDrawMultiNotification();

	inline void RenderPrepareRectSelection();
	inline void RenderDrawRectSelection();

	inline void RenderPrepareInteractiveToolDashedLines();
	inline void RenderDrawInteractiveToolDashedLines();

private:

	static inline constexpr NotificationAnchorPositionType TranslateAnchorPosition(AnchorPositionType anchor)
	{
		switch (anchor)
		{
			case AnchorPositionType::TopLeft:
				return NotificationAnchorPositionType::TopLeft;
			case AnchorPositionType::TopRight:
				return NotificationAnchorPositionType::TopRight;
			case AnchorPositionType::BottomLeft:
				return NotificationAnchorPositionType::BottomLeft;
			case AnchorPositionType::BottomRight:
				return NotificationAnchorPositionType::BottomRight;
		}

		return NotificationAnchorPositionType::BottomLeft;
	}

	inline void UploadTextStart(TextNotificationType textNotificationType)
	{
		//
		// Text notifications are sticky: we upload them once in a while and
		// continue drawing the same buffer
		//

		// Cleanup line buffers for this notification type
		auto & textContext = mTextNotificationTypeContexts[static_cast<size_t>(textNotificationType)];
		textContext.TextLines.clear();
		textContext.AreTextLinesDirty = true;
	}

	struct TextNotificationTypeContext;

	void GenerateTextVertices(TextNotificationTypeContext & context) const;

	void GenerateTextureNotificationVertices();

private:

    ShaderManager<GameShaderSet::ShaderSet> & mShaderManager;
	GlobalRenderContext & mGlobalRenderContext;

    float mScreenToNdcX;
    float mScreenToNdcY;

private:

	//
	// Types
	//

#pragma pack(push, 1)

	struct TextureNotificationVertex
	{
		vec2f vertexPositionNDC;
		vec2f textureCoordinate;
		float alpha;

		TextureNotificationVertex(
			vec2f const & _vertexPositionNDC,
			vec2f const & _textureCoordinate,
			float _alpha)
			: vertexPositionNDC(_vertexPositionNDC)
			, textureCoordinate(_textureCoordinate)
			, alpha(_alpha)
		{}
	};

	struct PhysicsProbePanelVertex
	{
		vec2f vertexPositionNDC;
		vec2f textureFrameOffset;
		vec2f xLimitsNDC;
		float vertexIsOpening;

		PhysicsProbePanelVertex(
			vec2f const & _vertexPositionNDC,
			vec2f const & _textureFrameOffset,
			vec2f const & _xLimitsNDC,
			float _vertexIsOpening)
			: vertexPositionNDC(_vertexPositionNDC)
			, textureFrameOffset(_textureFrameOffset)
			, xLimitsNDC(_xLimitsNDC)
			, vertexIsOpening(_vertexIsOpening)
		{}
	};

	struct LaserCannonVertex
	{
		vec2f vertexPositionNDC;
		vec2f textureCoordinate;
		float planeId;
		float alpha;
		float ambientLightSensitivity;

		LaserCannonVertex(
			vec2f const & _vertexPositionNDC,
			vec2f const & _textureCoordinate,
			float _planeId,
			float _alpha,
			float _ambientLightSensitivity)
			: vertexPositionNDC(_vertexPositionNDC)
			, textureCoordinate(_textureCoordinate)
			, planeId(_planeId)
			, alpha(_alpha)
			, ambientLightSensitivity(_ambientLightSensitivity)
		{}
	};

	struct LaserRayVertex
	{
		vec2f vertexPositionNDC;
		vec2f vertexSpacePosition;
		float strength;

		LaserRayVertex(
			vec2f const & _vertexPositionNDC,
			vec2f const & _vertexSpacePosition,
			float _strength)
			: vertexPositionNDC(_vertexPositionNDC)
			, vertexSpacePosition(_vertexSpacePosition)
			, strength(_strength)
		{}
	};

	struct MultiNotificationVertex
	{
		enum VertexKindType : int
		{
			// Note: enum values are to be kept in sync with shader
			BlastToolHalo = 1,
			FireExtinguisherSpray = 2,
			GripCircle = 3,
			HeatBlasterFlameCool = 4,
			HeatBlasterFlameHeat = 5,
			PressureInjectionHalo = 6,
			WindSphere = 7
		};
		float vertexKind;

		vec2f vertexPosition;
		float float1; // FlowMultiplier | Progress | PreFrontRadius
		vec2f auxPosition; // VirtualSpacePosition | CenterPosition
		float float2; // PersonalitySeed | PreFrontIntensity
		float float3; // MainFrontRadius
		float float4; // MainFrontIntensity

		static MultiNotificationVertex MakeBlastToolHalo(
			vec2f vertexPosition,
			float progress,
			vec2f virtualSpacePosition,
			float personalitySeed)
		{
			return MultiNotificationVertex(
				static_cast<float>(VertexKindType::BlastToolHalo),
				vertexPosition,
				progress,
				virtualSpacePosition,
				personalitySeed,
				0.0f,
				0.0f);
		}

		static MultiNotificationVertex MakeFireExtinguisherSpray(
			vec2f vertexPosition,
			vec2f virtualSpacePosition)
		{
			return MultiNotificationVertex(
				static_cast<float>(VertexKindType::FireExtinguisherSpray),
				vertexPosition,
				0.0f,
				virtualSpacePosition,
				0.0f,
				0.0f,
				0.0f);
		}

		static MultiNotificationVertex MakeGripCircle(
			vec2f vertexPosition,
			vec2f virtualSpacePosition)
		{
			return MultiNotificationVertex(
				static_cast<float>(VertexKindType::GripCircle),
				vertexPosition,
				0.0f,
				virtualSpacePosition,
				0.0f,
				0.0f,
				0.0f);
		}

		static MultiNotificationVertex MakeHeatBlasterFlameCool(
			vec2f vertexPosition,
			vec2f virtualSpacePosition)
		{
			return MultiNotificationVertex(
				static_cast<float>(VertexKindType::HeatBlasterFlameCool),
				vertexPosition,
				0.0f,
				virtualSpacePosition,
				0.0f,
				0.0f,
				0.0f);
		}

		static MultiNotificationVertex MakeHeatBlasterFlameHeat(
			vec2f vertexPosition,
			vec2f virtualSpacePosition)
		{
			return MultiNotificationVertex(
				static_cast<float>(VertexKindType::HeatBlasterFlameHeat),
				vertexPosition,
				0.0f,
				virtualSpacePosition,
				0.0f,
				0.0f,
				0.0f);
		}

		static MultiNotificationVertex MakePressureInjectionHalo(
			vec2f vertexPosition,
			vec2f virtualSpacePosition,
			float flowMultiplier)
		{
			return MultiNotificationVertex(
				static_cast<float>(VertexKindType::PressureInjectionHalo),
				vertexPosition,
				flowMultiplier,
				virtualSpacePosition,
				0.0f,
				0.0f,
				0.0f);
		}

		static MultiNotificationVertex MakeWindSphere(
			vec2f vertexPosition,
			vec2f virtualSpacePosition, // In world dimensions
			float preFrontRadius,
			float preFrontIntensity,
			float mainFrontRadius,
			float mainFrontIntensity)
		{
			return MultiNotificationVertex(
				static_cast<float>(VertexKindType::WindSphere),
				vertexPosition,
				preFrontRadius,
				virtualSpacePosition,
				preFrontIntensity,
				mainFrontRadius,
				mainFrontIntensity);
		}

	private:

		MultiNotificationVertex(
			float _vertexKind,
			vec2f const & _vertexPosition,
			float _float1,
			vec2f const & _auxPosition,
			float _float2,
			float _float3,
			float _float4)
			: vertexKind(_vertexKind)
			, vertexPosition(_vertexPosition)
			, float1(_float1)
			, auxPosition(_auxPosition)
			, float2(_float2)
			, float3(_float3)
			, float4(_float4)
		{}
	};

	struct RectSelectionVertex
	{
		vec2f vertexPositionNDC;
		vec2f vertexSpacePosition;
		vec2f pixelSizeInVertexSpace;
		vec2f borderSizeInVertexSpace;
		vec3f rectColor;
		float elapsed;

		RectSelectionVertex(
			vec2f const & _vertexPositionNDC,
			vec2f const & _vertexSpacePosition,
			vec2f const & _pixelSizeInVertexSpace,
			vec2f const & _borderSizeInVertexSpace,
			vec3f const & _rectColor,
			float _elapsed)
			: vertexPositionNDC(_vertexPositionNDC)
			, vertexSpacePosition(_vertexSpacePosition)
			, pixelSizeInVertexSpace(_pixelSizeInVertexSpace)
			, borderSizeInVertexSpace(_borderSizeInVertexSpace)
			, rectColor(_rectColor)
			, elapsed(_elapsed)
		{}
	};

	struct InteractiveToolDashedLineVertex
	{
		vec2f ndcPosition;
		float pixelCoord; //  PixelSpace

		InteractiveToolDashedLineVertex() = default;

		InteractiveToolDashedLineVertex(
			vec2f _ndcPosition,
			float _pixelCoord)
			: ndcPosition(_ndcPosition)
			, pixelCoord(_pixelCoord)
		{}
	};

#pragma pack(pop)

	//
	// Textures
	//

	TextureAtlasMetadata<GameTextureDatabases::GenericLinearTextureDatabase> const & mGenericLinearTextureAtlasMetadata;
	TextureAtlasMetadata<GameTextureDatabases::GenericMipMappedTextureDatabase> const & mGenericMipMappedTextureAtlasMetadata;

    //
    // Text notifications
    //

	/*
	 * Describes a vertex of a text quad, with all the information necessary to the shader.
	 */
	#pragma pack(push, 1)
	struct TextQuadVertex
	{
		float positionNdcX;
		float positionNdcY;
		float textureCoordinateX;
		float textureCoordinateY;
		float alpha;

		TextQuadVertex(
			float _positionNdcX,
			float _positionNdcY,
			float _textureCoordinateX,
			float _textureCoordinateY,
			float _alpha)
			: positionNdcX(_positionNdcX)
			, positionNdcY(_positionNdcY)
			, textureCoordinateX(_textureCoordinateX)
			, textureCoordinateY(_textureCoordinateY)
			, alpha(_alpha)
		{}
	};
	#pragma pack(pop)

	/*
	 * State per-line-of-text.
	 */
	struct TextLine
	{
		std::string Text;
		NotificationAnchorPositionType Anchor;
		vec2f ScreenOffset; // In font cell-size fraction (0.0 -> 1.0)
		float Alpha;

		TextLine(
			std::string const & text,
			NotificationAnchorPositionType anchor,
			vec2f const & screenOffset,
			float alpha)
			: Text(text)
			, Anchor(anchor)
			, ScreenOffset(screenOffset)
			, Alpha(alpha)
		{}
	};

	/*
	 * State per-text-notification-type.
	 */
	struct TextNotificationTypeContext
	{
		FontMetadata const * NotificationFontMetadata; // The metadata of the font to be used for this notification type

		std::vector<TextLine> TextLines;
		bool AreTextLinesDirty; // When dirty, we'll re-build the quads for this notification type
		std::vector<TextQuadVertex> TextQuadVertexBuffer;

		explicit TextNotificationTypeContext(FontMetadata const * notificationFontMetadata)
			: NotificationFontMetadata(notificationFontMetadata)
			, TextLines()
			, AreTextLinesDirty(false)
			, TextQuadVertexBuffer()
		{}

		TextNotificationTypeContext() // Just to allow array
			: NotificationFontMetadata(nullptr)
			, TextLines()
			, AreTextLinesDirty(false)
			, TextQuadVertexBuffer()
		{}
	};

	std::array<TextNotificationTypeContext, static_cast<size_t>(TextNotificationType::_Last) + 1> mTextNotificationTypeContexts;

	GameOpenGLVAO mTextVAO;
	size_t mCurrentTextQuadVertexBufferSize; // Number of elements (vertices)
	size_t mAllocatedTextQuadVertexBufferSize; // Number of elements (vertices)
	GameOpenGLVBO mTextVBO;

	// Fonts
	std::vector<FontMetadata> mFontSetMetadata; // Storage for TextNotificationTypeContext
	GameOpenGLTexture mFontAtlasTextureHandle;

	//
	// Texture notifications
	//

	struct TextureNotification
	{
		TextureFrameId<GameTextureDatabases::GenericLinearTextureGroups> FrameId;
		AnchorPositionType Anchor;
		vec2f ScreenOffset; // In texture-size fraction (0.0 -> 1.0)
		float Alpha;

		TextureNotification(
			TextureFrameId<GameTextureDatabases::GenericLinearTextureGroups> const & frameId,
			AnchorPositionType anchor,
			vec2f const & screenOffset,
			float alpha)
			: FrameId(frameId)
			, Anchor(anchor)
			, ScreenOffset(screenOffset)
			, Alpha(alpha)
		{}
	};

	std::vector<TextureNotification> mTextureNotifications;
	bool mIsTextureNotificationDataDirty; // When dirty, we'll re-build and re-upload the vertex data

	GameOpenGLVAO mTextureNotificationVAO;
	std::vector<TextureNotificationVertex> mTextureNotificationVertexBuffer;
	GameOpenGLVBO mTextureNotificationVBO;

	//
	// Physics probe panel
	//

	struct PhysicsProbePanel
	{
		float Open;
		bool IsOpening;

		PhysicsProbePanel(
			float open,
			bool isOpening)
			: Open(open)
			, IsOpening(isOpening)
		{}
	};

	std::optional<PhysicsProbePanel> mPhysicsProbePanel;
	bool mIsPhysicsProbeDataDirty; // When dirty, we'll re-build and re-upload the vertex data

	GameOpenGLVAO mPhysicsProbePanelVAO;
	std::vector<PhysicsProbePanelVertex> mPhysicsProbePanelVertexBuffer; // Just to cache allocations
	GameOpenGLVBO mPhysicsProbePanelVBO;

	//
	// Tool notifications
	//

	GameOpenGLVAO mLaserCannonVAO;
	std::vector<LaserCannonVertex> mLaserCannonVertexBuffer;
	GameOpenGLVBO mLaserCannonVBO;

	GameOpenGLVAO mLaserRayVAO;
	std::vector<LaserRayVertex> mLaserRayVertexBuffer;
	GameOpenGLVBO mLaserRayVBO;

	GameOpenGLVAO mMultiNotificationVAO;
	std::vector<MultiNotificationVertex> mMultiNotificationVertexBuffer;
	GameOpenGLVBO mMultiNotificationVBO;

	GameOpenGLVAO mRectSelectionVAO;
	std::vector<RectSelectionVertex> mRectSelectionVertexBuffer;
	GameOpenGLVBO mRectSelectionVBO;

	GameOpenGLVAO mInteractiveToolDashedLineVAO;
	std::vector<InteractiveToolDashedLineVertex> mInteractiveToolDashedLineVertexBuffer;
	GameOpenGLVBO mInteractiveToolDashedLineVBO;
};
