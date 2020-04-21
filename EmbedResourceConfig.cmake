set(EmbedResource_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "Path for EmbedResource")

if (NOT EXISTS ${EmbedResource_DIR}/embedresource.cpp) 
    message(FATAL_ERROR "Cannot find embedresource code at  ${EmbedResource_DIR}")
endif()
if (NOT EXISTS ${EmbedResource_DIR}/EmbeddedResource.h) 
    message(FATAL_ERROR "Cannot find embedresource header at  ${EmbedResource_DIR}")
endif()

function(target_add_resource target name)
    if (NOT TARGET embedresource)
        add_executable(embedresource ${EmbedResource_DIR}/embedresource.cpp)
        set_target_properties(embedresource PROPERTIES PUBLIC_HEADER "${EmbedResource_DIR}/EmbeddedResource.h")
    endif()

    set(outdir ${CMAKE_CURRENT_BINARY_DIR})
    set(out_f ${outdir}/${name}.cpp)
    
    if ("${ARGN}" STREQUAL "")
        message(FATAL_ERROR "No args supplied to embed_Resource")
    endif()

    foreach (f ${ARGN})
        get_filename_component(tmp ${f} ABSOLUTE)
        list(APPEND inputs ${tmp})
    endforeach()
    
    add_custom_command(OUTPUT ${out_f}
        COMMAND embedresource ${out_f} ${inputs}
        DEPENDS embedresource ${inputs}
        COMMENT "Building binary file for embedding ${out_f}")

    target_sources(${target} PRIVATE ${out_f})
    target_include_directories(${target} PRIVATE ${EmbedResource_DIR})
endfunction()
