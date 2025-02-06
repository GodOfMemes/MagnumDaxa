find_package(daxa CONFIG REQUIRED)

find_package(Corrade REQUIRED 
    Main
)
find_package(Magnum REQUIRED
    GlfwApplication
)


add_executable(${PROJECT_NAME}
    main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::Magnum

    daxa::daxa
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
