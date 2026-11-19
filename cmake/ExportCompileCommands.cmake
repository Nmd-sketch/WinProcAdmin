# Expand @response-file references in CMake's compilation database. Editors do
# not read rsp files, so the exported copy must carry literal -I paths.

file(READ "${DB_INPUT}" db)

# rsp paths are relative to each entry's working directory, which varies for
# dependencies added with add_subdirectory(). Collect every possible base.
set(bases "${DB_BUILD_DIR}")
string(JSON entry_count LENGTH "${db}")
math(EXPR last_index "${entry_count} - 1")
foreach(index RANGE 0 ${last_index})
    string(JSON entry GET "${db}" ${index})
    string(JSON directory GET "${entry}" directory)
    list(APPEND bases "${directory}")
endforeach()
list(REMOVE_DUPLICATES bases)

string(REGEX MATCHALL "@[^\" ]+\\.rsp" refs "${db}")
list(REMOVE_DUPLICATES refs)

foreach(ref IN LISTS refs)
    string(SUBSTRING "${ref}" 1 -1 rsp_relative)
    set(rsp_text "")
    foreach(base IN LISTS bases)
        if(EXISTS "${base}/${rsp_relative}")
            file(READ "${base}/${rsp_relative}" rsp_text)
            break()
        endif()
    endforeach()
    if(rsp_text STREQUAL "")
        message(FATAL_ERROR "Could not resolve response file: ${ref}")
    endif()

    string(STRIP "${rsp_text}" rsp_text)
    string(REPLACE "\\" "\\\\" rsp_text "${rsp_text}")
    string(REPLACE "\"" "\\\"" rsp_text "${rsp_text}")
    string(REPLACE "${ref}" "${rsp_text}" db "${db}")
endforeach()

file(WRITE "${DB_OUTPUT}" "${db}")
