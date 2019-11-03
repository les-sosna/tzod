cmake_minimum_required (VERSION 3.4)
project(WinStoreData)

file(GLOB_RECURSE data_files
    LIST_DIRECTORIES false
    "../_build/data/data/*"
)

add_library(StoreData ${data_files})

set_property(SOURCE ${data_files}
             PROPERTY VS_DEPLOYMENT_CONTENT 1)

set_target_properties(StoreData PROPERTIES LINKER_LANGUAGE CXX)
