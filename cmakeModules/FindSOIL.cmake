#The places to look for the tinyxml2 folders
#set(FIND_GLWF3_PATHS
#        D:/_Projects/OpenglCLionSetup                         #On Windows, this is where my tinyxml2 folder is
#        )

#The location of the include folder (and thus the header files)
#find_path uses the paths we defined above as places to look
#Saves the location of the header files in a variable called TINYXML2_INCLUDE_DIR
find_path(SOIL_INCLUDE_DIR SOIL/SOIL2.h   #The variable to store the path in and the name of the header files
        PATH_SUFFIXES include              #The folder name containing the header files
        PATHS ${PROJECT_SOURCE_DIR}/include/)       #Where to look (defined above)
message("Running FindSOIL")

IF(APPLE)
    message("Finding SOIL2 for APPLE")
    find_library(SOIL2_LIBRARY
            NAMES SOIL2 SOIL
            PATHS ${PROJECT_SOURCE_DIR}/libs/SOIL/osx
                  ${PROJECT_SOURCE_DIR}/libs/SOIL2/osx)
ELSE(APPLE) #Windows PS
    find_library(SOIL2_LIBRARY_RELEASE
            NAMES soil2 SOIL2
            PATHS ${PROJECT_SOURCE_DIR}/libs/SOIL
                  ${PROJECT_SOURCE_DIR}/libs/SOIL2
            NO_DEFAULT_PATH)
    find_library(SOIL2_LIBRARY_DEBUG
            NAMES soil2-debug SOIL2-debug
            PATHS ${PROJECT_SOURCE_DIR}/libs/SOIL
                  ${PROJECT_SOURCE_DIR}/libs/SOIL2
            NO_DEFAULT_PATH)

    if(SOIL2_LIBRARY_DEBUG AND SOIL2_LIBRARY_RELEASE)
        set(SOIL2_LIBRARY
                optimized ${SOIL2_LIBRARY_RELEASE}
                debug ${SOIL2_LIBRARY_DEBUG})
    elseif(SOIL2_LIBRARY_RELEASE)
        set(SOIL2_LIBRARY ${SOIL2_LIBRARY_RELEASE})
    elseif(SOIL2_LIBRARY_DEBUG)
        set(SOIL2_LIBRARY ${SOIL2_LIBRARY_DEBUG})
    endif()
ENDIF(APPLE)

message("SOIL2_LIBRARY = ${SOIL2_LIBRARY}")

SET(SOIL_FOUND "NO")
IF(SOIL2_LIBRARY AND SOIL_INCLUDE_DIR)
    SET(SOIL_FOUND "YES")
ENDIF(SOIL2_LIBRARY AND SOIL_INCLUDE_DIR)