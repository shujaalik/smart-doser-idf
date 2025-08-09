idf_component_register(
    SRCS main.c         # list the source files of this component
    INCLUDE_DIRS "../components" "include"
    PRIV_INCLUDE_DIRS "."   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
set(EXTRA_COMPONENTS, "components")
spiffs_create_partition_image(storage ../db FLASH_IN_PROJECT)
