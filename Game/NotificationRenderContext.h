/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Font.h"
#include "GlobalRenderContext.h"
#include "RenderParameters.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"

#include <GameOpenGL/ShaderManager.h>

#include <GameCore/GameTypes.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Render
{

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
        ResourceLocator const & resourceLocator,
        ShaderManager<ShaderManagerTraits> & shaderManager,
		GlobalRenderContext const & globalRenderContext);

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
		TextureFrameId<GenericLinearTextureGroups> const & textureFrameId,
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

		// Triangle 1

		mHeatBlasterFlameVertexBuffer[0].vertexPosition = vec2f(left, bottom);
		mHeatBlasterFlameVertexBuffer[0].flameSpacePosition = vec2f(-0.5f, -0.5f);

		mHeatBlasterFlameVertexBuffer[1].vertexPosition = vec2f(left, top);
		mHeatBlasterFlameVertexBuffer[1].flameSpacePosition = vec2f(-0.5f, 0.5f);

		mHeatBlasterFlameVertexBuffer[2].vertexPosition = vec2f(right, bottom);
		mHeatBlasterFlameVertexBuffer[2].flameSpacePosition = vec2f(0.5f, -0.5f);

		// Triangle 2

		mHeatBlasterFlameVertexBuffer[3].vertexPosition = vec2f(left, top);
		mHeatBlasterFlameVertexBuffer[3].flameSpacePosition = vec2f(-0.5f, 0.5f);

		mHeatBlasterFlameVertexBuffer[4].vertexPosition = vec2f(right, bottom);
		mHeatBlasterFlameVertexBuffer[4].flameSpacePosition = vec2f(0.5f, -0.5f);

		mHeatBlasterFlameVertexBuffer[5].vertexPosition = vec2f(right, top);
		mHeatBlasterFlameVertexBuffer[5].flameSpacePosition = vec2f(0.5f, 0.5f);

		//
		// Store shader
		//

		switch (action)
		{
			case HeatBlasterActionType::Cool:
			{
				mHeatBlasterFlameShaderToRender = Render::ProgramType::HeatBlasterFlameCool;
				break;
			}

			case HeatBlasterActionType::Heat:
			{
				mHeatBlasterFlameShaderToRender = Render::ProgramType::HeatBlasterFlameHeat;
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

		mFireExtinguisherSprayVertexBuffer[0].vertexPosition = vec2f(left, bottom);
		mFireExtinguisherSprayVertexBuffer[0].spraySpacePosition = vec2f(-0.5f, -0.5f);

		mFireExtinguisherSprayVertexBuffer[1].vertexPosition = vec2f(left, top);
		mFireExtinguisherSprayVertexBuffer[1].spraySpacePosition = vec2f(-0.5f, 0.5f);

		mFireExtinguisherSprayVertexBuffer[2].vertexPosition = vec2f(right, bottom);
		mFireExtinguisherSprayVertexBuffer[2].spraySpacePosition = vec2f(0.5f, -0.5f);

		// Triangle 2

		mFireExtinguisherSprayVertexBuffer[3].vertexPosition = vec2f(left, top);
		mFireExtinguisherSprayVertexBuffer[3].spraySpacePosition = vec2f(-0.5f, 0.5f);

		mFireExtinguisherSprayVertexBuffer[4].vertexPosition = vec2f(right, bottom);
		mFireExtinguisherSprayVertexBuffer[4].spraySpacePosition = vec2f(0.5f, -0.5f);

		mFireExtinguisherSprayVertexBuffer[5].vertexPosition = vec2f(right, top);
		mFireExtinguisherSprayVertexBuffer[5].spraySpacePosition = vec2f(0.5f, 0.5f);

		//
		// Store shader
		//

		mFireExtinguisherSprayShaderToRender = Render::ProgramType::FireExtinguisherSpray;
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

		mBlastToolHaloVertexBuffer.emplace_back(
			vec2f(left, bottom),
			vec2f(-1.0f, -1.0f),
			renderProgress,
			personalitySeed);

		mBlastToolHaloVertexBuffer.emplace_back(
			vec2f(left, top),
			vec2f(-1.0f, 1.0f),
			renderProgress,
			personalitySeed);

		mBlastToolHaloVertexBuffer.emplace_back(
			vec2f(right, bottom),
			vec2f(1.0f, -1.0f),
			renderProgress,
			personalitySeed);

		// Triangle 2

		mBlastToolHaloVertexBuffer.emplace_back(
			vec2f(left, top),
			vec2f(-1.0f, 1.0f),
			renderProgress,
			personalitySeed);

		mBlastToolHaloVertexBuffer.emplace_back(
			vec2f(right, bottom),
			vec2f(1.0f, -1.0f),
			renderProgress,
			personalitySeed);

		mBlastToolHaloVertexBuffer.emplace_back(
			vec2f(right, top),
			vec2f(1.0f, 1.0f),
			renderProgress,
			personalitySeed);
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

		mPressureInjectionHaloVertexBuffer.emplace_back(
			vec2f(left, bottom),
			vec2f(-1.0f, -1.0f),
			flowMultiplier);

		mPressureInjectionHaloVertexBuffer.emplace_back(
			vec2f(left, top),
			vec2f(-1.0f, 1.0f),
			flowMultiplier);

		mPressureInjectionHaloVertexBuffer.emplace_back(
			vec2f(right, bottom),
			vec2f(1.0f, -1.0f),
			flowMultiplier);

		// Triangle 2

		mPressureInjectionHaloVertexBuffer.emplace_back(
			vec2f(left, top),
			vec2f(-1.0f, 1.0f),
			flowMultiplier);

		mPressureInjectionHaloVertexBuffer.emplace_back(
			vec2f(right, bottom),
			vec2f(1.0f, -1.0f),
			flowMultiplier);

		mPressureInjectionHaloVertexBuffer.emplace_back(
			vec2f(right, top),
			vec2f(1.0f, 1.0f),
			flowMultiplier);
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

		mWindSphereVertexBuffer.emplace_back(
			vec2f(left, bottom),
			centerPosition,
			preFrontRadius,
			preFrontIntensityMultiplier,
			mainFrontRadius,
			mainFrontIntensityMultiplier);

		mWindSphereVertexBuffer.emplace_back(
			vec2f(left, top),
			centerPosition,
			preFrontRadius,
			preFrontIntensityMultiplier,
			mainFrontRadius,
			mainFrontIntensityMultiplier);

		mWindSphereVertexBuffer.emplace_back(
			vec2f(right, bottom),
			centerPosition,
			preFrontRadius,
			preFrontIntensityMultiplier,
			mainFrontRadius,
			mainFrontIntensityMultiplier);

		// Triangle 2

		mWindSphereVertexBuffer.emplace_back(
			vec2f(left, top),
			centerPosition,
			preFrontRadius,
			preFrontIntensityMultiplier,
			mainFrontRadius,
			mainFrontIntensityMultiplier);

		mWindSphereVertexBuffer.emplace_back(
			vec2f(right, bottom),
			centerPosition,
			preFrontRadius,
			preFrontIntensityMultiplier,
			mainFrontRadius,
			mainFrontIntensityMultiplier);

		mWindSphereVertexBuffer.emplace_back(
			vec2f(right, top),
			centerPosition,
			preFrontRadius,
			preFrontIntensityMultiplier,
			mainFrontRadius,
			mainFrontIntensityMultiplier);
	}

	void UploadLaserCannon(
		DisplayLogicalCoordinates const & screenCenter,
		std::optional<float> strength,
		ViewModel const & viewModel);

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
		float const desiredNumberOfPixels =
			static_cast<float>(std::min(viewModel.GetCanvasPhysicalSize().width, viewModel.GetCanvasPhysicalSize().height))
			/ 30.0f;

		// Multiplier for dimensions
		float const wDimMultiplier = std::max(
			viewModel.PhysicalDisplayOffsetToWorldOffset(desiredNumberOfPixels) / (smallestDimension + worldOffset),
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

		float constexpr BorderSizeWorld = 0.4f;
		vec2f const borderSizeInVertexSpace = vec2f(
			BorderSizeWorld * wDimMultiplier / halfActualW,
			BorderSizeWorld * wDimMultiplier / halfActualH);

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

	void UploadLineGuide(
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

	inline void RenderPrepareHeatBlasterFlame();
	inline void RenderDrawHeatBlasterFlame();

	inline void RenderPrepareFireExtinguisherSpray();
	inline void RenderDrawFireExtinguisherSpray();

	inline void RenderPrepareBlastToolHalo();
	inline void RenderDrawBlastToolHalo();

	inline void RenderPreparePressureInjectionHalo();
	inline void RenderDrawPressureInjectionHalo();

	inline void RenderPrepareWindSphere();
	inline void RenderDrawWindSphere();

	inline void RenderPrepareLaserCannon();
	inline void RenderDrawLaserCannon();

	inline void RenderPrepareLaserRay();
	inline void RenderDrawLaserRay();

	inline void RenderPrepareRectSelection();
	inline void RenderDrawRectSelection();

	inline void RenderPrepareLineGuide();
	inline void RenderDrawLineGuide();

private:

	static inline constexpr NotificationAnchorPositionType TranslateAnchorPosition(AnchorPositionType anchor)
	{
		switch (anchor)
		{
			case Render::AnchorPositionType::TopLeft:
				return NotificationAnchorPositionType::TopLeft;
			case Render::AnchorPositionType::TopRight:
				return NotificationAnchorPositionType::TopRight;
			case Render::AnchorPositionType::BottomLeft:
				return NotificationAnchorPositionType::BottomLeft;
			case Render::AnchorPositionType::BottomRight:
				return NotificationAnchorPositionType::BottomRight;
		}

		return NotificationAnchorPositionType::BottomLeft;
	}

	struct FontTextureAtlasMetadata;
	struct TextNotificationTypeContext;

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

	void GenerateTextVertices(TextNotificationTypeContext & context) const;

	void GenerateTextureNotificationVertices();

private:

	GlobalRenderContext const & mGlobalRenderContext;

    ShaderManager<ShaderManagerTraits> & mShaderManager;

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

	struct HeatBlasterFlameVertex
	{
		vec2f vertexPosition;
		vec2f flameSpacePosition;

		HeatBlasterFlameVertex()
		{}
	};

	struct FireExtinguisherSprayVertex
	{
		vec2f vertexPosition;
		vec2f spraySpacePosition;

		FireExtinguisherSprayVertex()
		{}
	};

	struct BlastToolHaloVertex
	{
		vec2f vertexPosition;
		vec2f haloSpacePosition;
		float renderProgress;
		float personalitySeed;

		BlastToolHaloVertex(
			vec2f _vertexPosition,
			vec2f _haloSpacePosition,
			float _renderProgress,
			float _personalitySeed)
			: vertexPosition(_vertexPosition)
			, haloSpacePosition(_haloSpacePosition)
			, renderProgress(_renderProgress)
			, personalitySeed(_personalitySeed)
		{}
	};

	struct PressureInjectionHaloVertex
	{
		vec2f vertexPosition;
		vec2f haloSpacePosition;
		float flowMultiplier;

		PressureInjectionHaloVertex(
			vec2f _vertexPosition,
			vec2f _haloSpacePosition,
			float _flowMultiplier)
			: vertexPosition(_vertexPosition)
			, haloSpacePosition(_haloSpacePosition)
			, flowMultiplier(_flowMultiplier)
		{}
	};

	struct WindSphereVertex
	{
		vec2f vertexPosition;
		vec2f centerPosition;
		float preFrontRadius;
		float preFrontIntensity;
		float mainFrontRadius;
		float mainFrontIntensity;

		WindSphereVertex(
			vec2f _vertexPosition,
			vec2f _centerPosition,
			float _preFrontRadius,
			float _preFrontIntensity,
			float _mainFrontRadius,
			float _mainFrontIntensity)
			: vertexPosition(_vertexPosition)
			, centerPosition(_centerPosition)
			, preFrontRadius(_preFrontRadius)
			, preFrontIntensity(_preFrontIntensity)
			, mainFrontRadius(_mainFrontRadius)
			, mainFrontIntensity(_mainFrontIntensity)
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

	struct LineGuideVertex
	{
		vec2f ndcPosition;
		float pixelCoord; //  PixelSpace

		LineGuideVertex() = default;

		LineGuideVertex(
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

	TextureAtlasMetadata<GenericLinearTextureGroups> const & mGenericLinearTextureAtlasMetadata;
	TextureAtlasMetadata<GenericMipMappedTextureGroups> const & mGenericMipMappedTextureAtlasMetadata;

    //
    // Text notifications
    //

	/*
	 * Metadata for each font in the single font atlas.
	 */
	struct FontTextureAtlasMetadata
	{
		vec2f CellTextureAtlasSize; // Size of one cell of the font, in texture atlas space coordinates
		std::array<vec2f, 256> GlyphTextureAtlasBottomLefts; // Bottom-left of each glyph, in texture atlas space coordinates
		std::array<vec2f, 256> GlyphTextureAtlasTopRights; // Top-right of each glyph, in texture atlas space coordinates
		FontMetadata OriginalFontMetadata;

		FontTextureAtlasMetadata(
			vec2f cellTextureAtlasSize,
			std::array<vec2f, 256> glyphTextureAtlasBottomLefts,
			std::array<vec2f, 256> glyphTextureAtlasTopRights,
			FontMetadata originalFontMetadata)
			: CellTextureAtlasSize(cellTextureAtlasSize)
			, GlyphTextureAtlasBottomLefts(std::move(glyphTextureAtlasBottomLefts))
			, GlyphTextureAtlasTopRights(std::move(glyphTextureAtlasTopRights))
			, OriginalFontMetadata(originalFontMetadata)
		{}

	};

	std::vector<FontTextureAtlasMetadata> mFontTextureAtlasMetadata; // This vector is storage, allowing for N:1 between contextes and fonts

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
		FontTextureAtlasMetadata const & NotificationFontTextureAtlasMetadata; // The metadata of the font to be used for this notification type

		std::vector<TextLine> TextLines;
		bool AreTextLinesDirty; // When dirty, we'll re-build the quads for this notification type
		std::vector<TextQuadVertex> TextQuadVertexBuffer;

		explicit TextNotificationTypeContext(FontTextureAtlasMetadata const & notificationFontTextureAtlasMetadata)
			: NotificationFontTextureAtlasMetadata(notificationFontTextureAtlasMetadata)
			, TextLines()
			, AreTextLinesDirty(false)
			, TextQuadVertexBuffer()
		{}
	};

	std::vector<TextNotificationTypeContext> mTextNotificationTypeContexts;

	GameOpenGLVAO mTextVAO;
	size_t mCurrentTextQuadVertexBufferSize; // Number of elements (vertices)
	size_t mAllocatedTextQuadVertexBufferSize; // Number of elements (vertices)
	GameOpenGLVBO mTextVBO;

	// Fonts
	GameOpenGLTexture mFontAtlasTextureHandle;

	//
	// Texture notifications
	//

	struct TextureNotification
	{
		TextureFrameId<GenericLinearTextureGroups> FrameId;
		AnchorPositionType Anchor;
		vec2f ScreenOffset; // In texture-size fraction (0.0 -> 1.0)
		float Alpha;

		TextureNotification(
			TextureFrameId<GenericLinearTextureGroups> const & frameId,
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

	GameOpenGLVAO mHeatBlasterFlameVAO;
	std::array<HeatBlasterFlameVertex, 6> mHeatBlasterFlameVertexBuffer;
	GameOpenGLVBO mHeatBlasterFlameVBO;
	std::optional<Render::ProgramType> mHeatBlasterFlameShaderToRender;

	GameOpenGLVAO mFireExtinguisherSprayVAO;
	std::array<FireExtinguisherSprayVertex, 6> mFireExtinguisherSprayVertexBuffer;
	GameOpenGLVBO mFireExtinguisherSprayVBO;
	std::optional<Render::ProgramType> mFireExtinguisherSprayShaderToRender;

	GameOpenGLVAO mBlastToolHaloVAO;
	std::vector<BlastToolHaloVertex> mBlastToolHaloVertexBuffer;
	GameOpenGLVBO mBlastToolHaloVBO;

	GameOpenGLVAO mPressureInjectionHaloVAO;
	std::vector<PressureInjectionHaloVertex> mPressureInjectionHaloVertexBuffer;
	GameOpenGLVBO mPressureInjectionHaloVBO;

	GameOpenGLVAO mWindSphereVAO;
	std::vector<WindSphereVertex> mWindSphereVertexBuffer;
	GameOpenGLVBO mWindSphereVBO;

	GameOpenGLVAO mLaserCannonVAO;
	std::vector<LaserCannonVertex> mLaserCannonVertexBuffer;
	GameOpenGLVBO mLaserCannonVBO;

	GameOpenGLVAO mLaserRayVAO;
	std::vector<LaserRayVertex> mLaserRayVertexBuffer;
	GameOpenGLVBO mLaserRayVBO;

	GameOpenGLVAO mRectSelectionVAO;
	std::vector<RectSelectionVertex> mRectSelectionVertexBuffer;
	GameOpenGLVBO mRectSelectionVBO;

	GameOpenGLVAO mLineGuideVAO;
	std::vector<LineGuideVertex> mLineGuideVertexBuffer;
	GameOpenGLVBO mLineGuideVBO;
};

}