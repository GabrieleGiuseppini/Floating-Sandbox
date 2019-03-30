
#
# GameCore library
#

set  (SOURCES
	AABB.h
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
	FixedSizeVector.h
	FixedTickSliderCore.cpp
	FixedTickSliderCore.h
	FloatingPoint.h
	GameDebug.h
	GameException.h
	GameMath.h
	GameRandomEngine.h
	GameTypes.cpp
	GameTypes.h
	GameWallClock.h
	ImageData.h
	ImageSize.h
	ImageTools.cpp
	ImageTools.h
	ISliderCore.h
	LinearSliderCore.cpp
	LinearSliderCore.h
	Log.cpp
	Log.h
	ProgressCallback.h
	RunningAverage.h
	Segment.h
	SysSpecifics.h
	TupleKeys.h
	Utils.cpp
	Utils.h	
	Vectors.cpp
	Vectors.h)

source_group(" " FILES ${SOURCES})

add_library (GameCoreLib ${SOURCES})

target_include_directories(GameCoreLib PUBLIC ${LIBSIMDPP_INCLUDE_DIRS})
target_include_directories(GameCoreLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(GameCoreLib INTERFACE ..)

target_link_libraries (GameCoreLib
	${ADDITIONAL_LIBRARIES})

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

	target_link_libraries (GameCoreLib
		"stdc++fs" 
		"c++experimental")

endif()

