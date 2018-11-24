set  (UI_LIB_SOURCES
	ExponentialSliderCore.cpp
	ExponentialSliderCore.h
	FixedTickSliderCore.cpp
	FixedTickSliderCore.h
	ISliderCore.h
	LinearSliderCore.cpp
	LinearSliderCore.h
	)

source_group(" " FILES ${UI_LIB_SOURCES})

add_library (UILib ${UI_LIB_SOURCES})

target_include_directories(GameLib PUBLIC ${PICOJSON_INCLUDE_DIRS})
target_include_directories(GameLib INTERFACE ..)

target_link_libraries (UILib
	${ADDITIONAL_LIBRARIES})
