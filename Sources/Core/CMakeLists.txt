
#
# Core library
#

set  (SOURCES
	AABB.h
	AABBSet.h
	Algorithms.h
	BarycentricCoords.cpp
	BarycentricCoords.h
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
	Finalizer.h
	FixedSetSliderCore.h
	FixedSizeVector.h
	FixedTickSliderCore.cpp
	FixedTickSliderCore.h
	FloatingPoint.h
	FontSet.h
	FontSet-inl.h
	GameChronometer.h
	GameDebug.h
	GameException.h
	GameGeometry.h
	GameMath.h
	GameRandomEngine.h
	GameTypes.cpp
	GameTypes.h
	GameWallClock.h
	IAssetManager.h
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
	PerfStats.h
	PngTools.cpp
	PngTools.h
	PortableTimepoint.cpp
	PortableTimepoint.h
	PrecalculatedFunction.cpp
	PrecalculatedFunction.h
	ProgressCallback.h
	RunningAverage.h
	StockColors.h
	Streams.h
	StrongTypeDef.h
	SysSpecifics.cpp
	SysSpecifics.h
	TaskThread.cpp
	TaskThread.h
	TemporallyCoherentPriorityQueue.h
	TextureAtlas.h
	TextureAtlas-inl.h
	TextureDatabase.h
	TextureDatabase-inl.h
	ThreadSafeQueue.h
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

add_library (Core ${SOURCES})

target_compile_definitions(Core PRIVATE LIBPNG_MODE_LIB)
target_include_directories(Core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PNG_PNG_INCLUDE_DIR})
target_include_directories(Core PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(Core INTERFACE ..)

target_link_libraries (Core
	${ZLIB_LIBRARY}
	${PNG_LIBRARY}
	${ADDITIONAL_LIBRARIES})
