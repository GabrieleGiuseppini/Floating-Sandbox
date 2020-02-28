
#
# GameCore library
#

set  (SOURCES
	AABB.h
	Algorithms.h
	BoundedVector.h
	Buffer.h
	BufferAllocator.h
	CircularList.h
	Colors.cpp
	Colors.h
	ElementContainer.h
	ElementIndexRangeIterator.h
	EnumFlags.h
	ExponentialSliderCore.cpp
	ExponentialSliderCore.h
	FileSystem.h
	FixedSizeVector.h
	FixedTickSliderCore.cpp
	FixedTickSliderCore.h
	FloatingPoint.h
	GameChronometer.h
	GameDebug.h
	GameException.h
	GameGeometry.h
	GameMath.h
	GameRandomEngine.h
	GameTypes.cpp
	GameTypes.h
	GameWallClock.h
	ImageData.h
	ImageSize.h
	ImageTools.cpp
	ImageTools.h
	IntegralLinearSliderCore.h
	ISliderCore.h
	LinearSliderCore.cpp
	LinearSliderCore.h
	Log.cpp
	Log.h
	MemoryStreams.h
	ParameterSmoother.h
	PrecalculatedFunction.cpp
	PrecalculatedFunction.h
	ProgressCallback.h
	RunningAverage.h
	Settings.cpp
	Settings.h
	SysSpecifics.h
	TaskThreadPool.cpp
	TaskThreadPool.h
	TemporallyCoherentPriorityQueue.h
	TruncatedPriorityQueue.h
	TupleKeys.h
	UniqueBuffer.h
	Utils.cpp
	Utils.h	
	Vectors.cpp
	Vectors.h
	Version.h	
)

source_group(" " FILES ${SOURCES})

add_library (GameCoreLib ${SOURCES})

target_include_directories(GameCoreLib PUBLIC ${LIBSIMDPP_INCLUDE_DIRS})
target_include_directories(GameCoreLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(GameCoreLib INTERFACE ..)

target_link_libraries (GameCoreLib
	${ADDITIONAL_LIBRARIES})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

	target_link_libraries (GameCoreLib
		"stdc++fs")

endif()

