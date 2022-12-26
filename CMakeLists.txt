cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0042 NEW)
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project (zorm)
set(VERSION 1.0.0.0)
SET(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX17_ABI=0")
# set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(MSVC)
    include_directories(D:\\codes\\zorm\\src\\include)
    include_directories(D:\\codes\\zorm\\thirds\\sqlit3)
    include_directories(D:\\codes\\zorm\\thirds\\zjson)

    link_directories(D:\\codes\\zorm\\out\\build\\x64-debug)
elseif(UNIX)
    include_directories(/home/roch/codes/zorm/src/include)
    include_directories(/home/roch/codes/zorm/thirds/sqlit3)
    include_directories(/home/roch/codes/zorm/thirds/zjson)
    include_directories(/home/roch/codes/zorm/thirds/pgsql/include/)
    include_directories(/home/roch/codes/zorm/thirds/MySQL-C-Api-6.1/include/)
endif()

add_definitions(-DZORM_LIB)
add_definitions(-D__LINUX__)

set(NODE_LINK_LIBS "")
set(NODE_EXTERNAL_INCLUDES "")

FILE(GLOB_RECURSE SOURCE_FILES "./addon/*.cc") 
FILE(GLOB_RECURSE HEADER_FILES "./addon/*.h") 

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
message("-------- CMAKE_JS_INC -------" ${CMAKE_JS_INC})
# Include Node-API wrappers
target_include_directories(${PROJECT_NAME} PRIVATE  
    ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api 
    ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api/src
    ${CMAKE_JS_INC})
#target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_JS_LIB})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} )

# define NPI_VERSION
add_definitions(-DNAPI_VERSION=8)
add_definitions(-DNAPI_DISABLE_CPP_EXCEPTIONS)
