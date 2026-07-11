set(TPFPC_NAME SREES_2026_Bajramovic_TPFPC)
set(TPFPC_PLUGIN_NAME SREES_2026_Bajramovic_TPFPC_Plugin)
set(TPFPC_DTWIN_PLUGIN_NAME SREES_2026_Bajramovic_TPFPC_dTwinPlugin)
set(TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME SREES_2026_Bajramovic_TPFPC_dTwinPluginLoaderTest)
set(TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME SREES_2026_Bajramovic_TPFPC_dTwinPluginGuiTest)
set(TPFPC_MATPOWER_REFERENCE_TEST_NAME MatpowerReferenceTest)
set(TPFPC_MATPOWER_TO_DTWIN_TEST_NAME MatpowerToDmodlTest)

set(TPFPC_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MainTabbedView.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MatricesView.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/PowerFlowView.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MatpowerPowerFlowView.cpp)
set(TPFPC_PLUGIN_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/PolarConverter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/PowerFlowMatrixModel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/TpfpcPluginApi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MatpowerPowerFlow.cpp)
set(TPFPC_DTWIN_PLUGIN_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/PolarConverter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/PowerFlowMatrixModel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/TpfpcDTwinPlugin.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MatpowerPowerFlow.cpp)
set(TPFPC_DTWIN_PLUGIN_LOADER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/TpfpcDTwinPluginLoaderTest.cpp)
set(TPFPC_DTWIN_PLUGIN_GUI_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/TpfpcDTwinPluginGuiTest.cpp)
set(TPFPC_MATPOWER_REFERENCE_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/MatpowerReferenceTest.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/PolarConverter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/PowerFlowMatrixModel.cpp)
set(TPFPC_MATPOWER_TO_DTWIN_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/MatpowerToDmodlTest.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MatpowerPowerFlow.cpp)
file(GLOB TPFPC_INCS ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
set(TPFPC_PLIST ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)
file(GLOB TPFPC_INC_TD ${NATID_SDK_INC}/td/*.h)
file(GLOB TPFPC_INC_GUI ${NATID_SDK_INC}/gui/*.h)
file(GLOB TPFPC_INC_DENSE ${NATID_SDK_INC}/dense/*.h)
file(GLOB TPFPC_INC_SPARSE ${NATID_SDK_INC}/sparse/*.h)
file(GLOB TPFPC_INC_CNT ${NATID_SDK_INC}/cnt/*.h)
file(GLOB TPFPC_INC_MU ${NATID_SDK_INC}/mu/*.h)
file(GLOB TPFPC_INC_MEM ${NATID_SDK_INC}/mem/*.h)
file(GLOB TPFPC_INC_ARCH ${NATID_SDK_INC}/arch/*.h)
file(GLOB TPFPC_INC_SC ${NATID_SDK_INC}/sc/*.h)
file(GLOB TPFPC_INC_SYST ${NATID_SDK_INC}/syst/*.h)

function(tpfpc_link_natid_libraries TPFPC_TARGET_NAME)
    foreach(TPFPC_LIB_PREFIX IN LISTS ARGN)
        if(WIN32)
            if(${TPFPC_LIB_PREFIX}_LIB_DEBUG)
                target_link_libraries(${TPFPC_TARGET_NAME} debug ${${TPFPC_LIB_PREFIX}_LIB_DEBUG})
            endif()
            if(${TPFPC_LIB_PREFIX}_LIB_RELEASE)
                target_link_libraries(${TPFPC_TARGET_NAME} optimized ${${TPFPC_LIB_PREFIX}_LIB_RELEASE})
            endif()
        else()
            if(${TPFPC_LIB_PREFIX}_LIB_RELEASE)
                target_link_libraries(${TPFPC_TARGET_NAME} ${${TPFPC_LIB_PREFIX}_LIB_RELEASE})
            elseif(${TPFPC_LIB_PREFIX}_LIB_DEBUG)
                target_link_libraries(${TPFPC_TARGET_NAME} ${${TPFPC_LIB_PREFIX}_LIB_DEBUG})
            endif()
        endif()
    endforeach()
endfunction()

add_library(${TPFPC_PLUGIN_NAME} SHARED ${TPFPC_PLUGIN_SOURCES} ${TPFPC_INCS}
                                      ${TPFPC_INC_TD} ${TPFPC_INC_DENSE} ${TPFPC_INC_SPARSE})

target_compile_definitions(${TPFPC_PLUGIN_NAME} PRIVATE TPFPC_PLUGIN_EXPORTS)

tpfpc_link_natid_libraries(${TPFPC_PLUGIN_NAME} MU MATRIX)

setIDEPropertiesForLib(${TPFPC_PLUGIN_NAME})

add_library(${TPFPC_DTWIN_PLUGIN_NAME} SHARED ${TPFPC_DTWIN_PLUGIN_SOURCES} ${TPFPC_INCS}
                                            ${TPFPC_INC_TD} ${TPFPC_INC_GUI}
                                            ${TPFPC_INC_CNT} ${TPFPC_INC_MU}
                                            ${TPFPC_INC_MEM} ${TPFPC_INC_ARCH}
                                            ${TPFPC_INC_SC} ${TPFPC_INC_DENSE}
                                            ${TPFPC_INC_SPARSE})

target_compile_definitions(${TPFPC_DTWIN_PLUGIN_NAME} PRIVATE PLUGIN_EXPORTS
    TPFPC_SOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}")

tpfpc_link_natid_libraries(${TPFPC_DTWIN_PLUGIN_NAME} MU MATRIX NATGUI)

setIDEPropertiesForLib(${TPFPC_DTWIN_PLUGIN_NAME})

add_executable(${TPFPC_MATPOWER_REFERENCE_TEST_NAME} ${TPFPC_MATPOWER_REFERENCE_TEST_SOURCES}
                                                      ${TPFPC_INCS}
                                                      ${TPFPC_INC_TD}
                                                      ${TPFPC_INC_DENSE}
                                                      ${TPFPC_INC_SPARSE})

tpfpc_link_natid_libraries(${TPFPC_MATPOWER_REFERENCE_TEST_NAME} MU MATRIX)

setIDEPropertiesForExecutable(${TPFPC_MATPOWER_REFERENCE_TEST_NAME})
add_executable(${TPFPC_MATPOWER_TO_DTWIN_TEST_NAME} ${TPFPC_MATPOWER_TO_DTWIN_TEST_SOURCES}
                                                    ${TPFPC_INCS})

target_compile_definitions(${TPFPC_MATPOWER_TO_DTWIN_TEST_NAME} PRIVATE
    TPFPC_SOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}")

setIDEPropertiesForExecutable(${TPFPC_MATPOWER_TO_DTWIN_TEST_NAME})

add_executable(${TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME} ${TPFPC_DTWIN_PLUGIN_LOADER_TEST_SOURCES}
                                                       ${TPFPC_INCS}
                                                       ${TPFPC_INC_TD}
                                                       ${TPFPC_INC_GUI}
                                                       ${TPFPC_INC_SYST}
                                                       ${TPFPC_INC_SC}
                                                       ${TPFPC_INC_ARCH})

target_compile_definitions(${TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME} PRIVATE
    TPFPC_DTWIN_PLUGIN_PATH="$<TARGET_FILE:${TPFPC_DTWIN_PLUGIN_NAME}>")

tpfpc_link_natid_libraries(${TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME} MU)

if(WIN32)
    set(TPFPC_RUNTIME_DLL_DIR ${HOME_ROOT}/other_bin/bin)
    foreach(TPFPC_RUNTIME_DLL mainUtils.dll Matrix.dll)
        add_custom_command(TARGET ${TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${TPFPC_RUNTIME_DLL_DIR}/${TPFPC_RUNTIME_DLL}"
                "$<TARGET_FILE_DIR:${TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME}>/${TPFPC_RUNTIME_DLL}")
    endforeach()
endif()

add_dependencies(${TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME} ${TPFPC_DTWIN_PLUGIN_NAME})

setIDEPropertiesForExecutable(${TPFPC_DTWIN_PLUGIN_LOADER_TEST_NAME})

add_executable(${TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME} ${TPFPC_DTWIN_PLUGIN_GUI_TEST_SOURCES}
                                                    ${TPFPC_INCS}
                                                    ${TPFPC_INC_TD}
                                                    ${TPFPC_INC_GUI}
                                                    ${TPFPC_INC_SYST}
                                                    ${TPFPC_INC_SC}
                                                    ${TPFPC_INC_ARCH}
                                                    ${TPFPC_INC_CNT})

target_compile_definitions(${TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME} PRIVATE
    TPFPC_DTWIN_PLUGIN_PATH="$<TARGET_FILE:${TPFPC_DTWIN_PLUGIN_NAME}>")

tpfpc_link_natid_libraries(${TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME} MU NATGUI)

add_dependencies(${TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME} ${TPFPC_DTWIN_PLUGIN_NAME})

setTargetPropertiesForGUIApp(${TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME} ${TPFPC_PLIST})
setIDEPropertiesForGUIExecutable(${TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME} ${CMAKE_CURRENT_LIST_DIR})
setPlatformDLLPath(${TPFPC_DTWIN_PLUGIN_GUI_TEST_NAME})

add_executable(${TPFPC_NAME} ${TPFPC_SOURCES} ${TPFPC_INCS} ${TPFPC_INC_TD} ${TPFPC_INC_GUI})
target_compile_definitions(${TPFPC_NAME} PRIVATE
    TPFPC_SOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}")

source_group("inc" FILES ${TPFPC_INCS})
source_group("inc\\td" FILES ${TPFPC_INC_TD})
source_group("inc\\gui" FILES ${TPFPC_INC_GUI})
source_group("inc\\dense" FILES ${TPFPC_INC_DENSE})
source_group("inc\\sparse" FILES ${TPFPC_INC_SPARSE})
source_group("inc\\cnt" FILES ${TPFPC_INC_CNT})
source_group("inc\\mu" FILES ${TPFPC_INC_MU})
source_group("inc\\mem" FILES ${TPFPC_INC_MEM})
source_group("inc\\arch" FILES ${TPFPC_INC_ARCH})
source_group("inc\\sc" FILES ${TPFPC_INC_SC})
source_group("inc\\syst" FILES ${TPFPC_INC_SYST})
source_group("src" FILES ${TPFPC_SOURCES})
source_group("src\\plugin" FILES ${TPFPC_PLUGIN_SOURCES})
source_group("src\\dTwinPlugin" FILES ${TPFPC_DTWIN_PLUGIN_SOURCES})
source_group("src\\dTwinPluginLoaderTest" FILES ${TPFPC_DTWIN_PLUGIN_LOADER_TEST_SOURCES})
source_group("src\\dTwinPluginGuiTest" FILES ${TPFPC_DTWIN_PLUGIN_GUI_TEST_SOURCES})
source_group("src\\tests" FILES ${TPFPC_MATPOWER_REFERENCE_TEST_SOURCES})

tpfpc_link_natid_libraries(${TPFPC_NAME} MU NATGUI MATRIX)
target_link_libraries(${TPFPC_NAME} ${TPFPC_PLUGIN_NAME})

setTargetPropertiesForGUIApp(${TPFPC_NAME} ${TPFPC_PLIST})

setIDEPropertiesForGUIExecutable(${TPFPC_NAME} ${CMAKE_CURRENT_LIST_DIR})

setPlatformDLLPath(${TPFPC_NAME})








