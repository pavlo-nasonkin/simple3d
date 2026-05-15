#The places to look for the tinyxml2 folders
#set(FIND_GLWF3_PATHS
#        D:/_Projects/OpenglCLionSetup                         #On Windows, this is where my tinyxml2 folder is
#        )

#The location of the include folder (and thus the header files)
#find_path uses the paths we defined above as places to look
#Saves the location of the header files in a variable called TINYXML2_INCLUDE_DIR
find_path(GLFW3_INCLUDE_DIR GLFW/glfw3.h   #The variable to store the path in and the name of the header files
        PATH_SUFFIXES include              #The folder name containing the header files
        PATHS ${PROJECT_SOURCE_DIR}/include/)       #Where to look (defined above)
message("Running FindGLFW3")

IF(APPLE)
    message("Finding GLFW for APPLE")
    find_library(GLFW3_LIBRARY
            NAMES glfw3
            PATHS ${PROJECT_SOURCE_DIR}/libs/glfw-3.2.1/osx)
ELSE(APPLE) #Windows PS
    find_library(GLFW3_LIBRARY
            NAMES glfw3
            PATHS ${PROJECT_SOURCE_DIR}/libs/glfw-3.2.1)
ENDIF(APPLE)


SET(GLFW3_FOUND "NO")
IF(GLFW3_LIBRARIES AND GLFW3_INCLUDE_DIRS)
    SET(GLFW3_FOUND "YES")
ENDIF(GLFW3_LIBRARIES AND GLFW3_INCLUDE_DIRS)