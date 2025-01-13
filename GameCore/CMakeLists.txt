
#
# GameCore library
#

set  (SOURCES
	AABB.h
	AABBSet.h
	Algorithms.h
	BarycentricCoords.cpp
	BarycentricCoords.h
	BootSettings.cpp
	BootSettings.h
	BoundedVector.h
	Buffer.h
	Buffer2D.h
	BufferAllocator.h
	BuildInfo.h
	CircularList.h
	Colors.cpp
	Colors.h
	Conversions.h
	DeSerializationBuffer.h
	ElementContainer.h
	ElementIndexRangeIterator.h
	Endian.h
	EnumFlags.h
	ExponentialSliderCore.cpp
	ExponentialSliderCore.h
	FileSystem.h
	Finalizer.h
	FixedSetSliderCore.h
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
	ImageFileMap.h
	ImageTools.cpp
	ImageTools.h
	IndexRemap.h
	IntegralLinearSliderCore.h
	ISliderCore.h	
	LinearSliderCore.cpp
	LinearSliderCore.h
	Log.cpp
	Log.h
	Matrix.h
	MemoryStreams.h
	Noise.cpp
	Noise.h
	ParameterSmoother.h
	PortableTimepoint.cpp
	PortableTimepoint.h
	PrecalculatedFunction.cpp
	PrecalculatedFunction.h
	ProgressCallback.h
	RunningAverage.h	
	Settings.cpp
	Settings.h
	StrongTypeDef.h
	SysSpecifics.cpp
	SysSpecifics.h
	TaskThread.cpp
	TaskThread.h
	TemporallyCoherentPriorityQueue.h
	ThreadManager.cpp
	ThreadManager.h
	ThreadPool.cpp
	ThreadPool.h
	TruncatedPriorityQueue.h
	TupleKeys.h
	UniqueBuffer.h
	UserGameException.h
	Utils.cpp
	Utils.h	
	Vectors.cpp
	Vectors.h
	Version.h	
)

source_group(" " FILES ${SOURCES})

add_library (GameCoreLib ${SOURCES})

target_include_directories(GameCoreLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(GameCoreLib INTERFACE ..)

target_link_libraries (GameCoreLib
	${ADDITIONAL_LIBRARIES})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	target_link_libraries (GameCoreLib
		"stdc++fs")
endif()

