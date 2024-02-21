cmake_minimum_required(VERSION 2.6)

project(Oriented_RCNN_xml2txt)

find_package(OpenCV REQUIRED)

include_directories(.
${OpenCV_INCLUDE_DIRS}
${CMAKE_CURRENT_SOURCE_DIR}/zupply
${CMAKE_CURRENT_SOURCE_DIR}/inc
${CMAKE_CURRENT_SOURCE_DIR}/tinyxml
)

macro(SCANDIR SCANMODE FILE_LISTS SEARCH)

	#主要用于匹配规则在指定的目录内匹配到所需要的文件
	#GLOB 当前文件夹下的文件    GLOB_RECURSE 当前以及子目录的文件
	# file(模式 变量名 文件路径)
    file(${SCANMODE} RES_LISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SEARCH} )
	
    foreach(f ${RES_LISTS})
        list(APPEND ${FILE_LISTS} ${f})
    endforeach()
endmacro()


SET(SUB_FOLDERS inc src )

foreach( f ${SUB_FOLDERS} )
    SCANDIR(GLOB CURRENT_HEADERS ${f}/*.h)
    SCANDIR(GLOB CURRENT_HEADERS ${f}/*.hpp)  
    
    SCANDIR(GLOB CURRENT_SOURCES ${f}/*.cpp)
    SCANDIR(GLOB CURRENT_SOURCES ${f}/*.c)
endforeach()

file(GLOB RES_LISTS ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml/* )
foreach(f ${RES_LISTS})
	list(APPEND CURRENT_TINYXML ${f})
endforeach()

file(GLOB RES_LISTS ${CMAKE_CURRENT_SOURCE_DIR}/zupply/* )
foreach(f ${RES_LISTS})
	list(APPEND CURRENT_ZUPPLY ${f})
endforeach()

message(STATUS "header: ${CURRENT_HEADERS}")
message(STATUS "source: ${CURRENT_SOURCES}")
message(STATUS "xml: ${CURRENT_TINYXML}")
message(STATUS "zupply: ${CURRENT_ZUPPLY}")

source_group("inc" FILES ${CURRENT_HEADERS}) 
source_group("src" FILES ${CURRENT_SOURCES}) 
source_group("tinyxml" FILES ${CURRENT_TINYXML}) 
source_group("zupply" FILES ${CURRENT_ZUPPLY}) 

add_definitions(-D_ENABLE_ATOMIC_ALIGNMENT_FIX)

ADD_EXECUTABLE(${PROJECT_NAME} ${CURRENT_HEADERS} ${CURRENT_SOURCES} ${CURRENT_TINYXML} ${CURRENT_ZUPPLY})

target_link_libraries(
	${PROJECT_NAME}
	${OpenCV_DIR}/x64/vc14/lib/opencv_world310d.lib
	)


