
# 绝对路径转成相对路径
function(path_to_relative ret)
    set(RELATIVE_PATHS "")

    foreach(PATH_ ${ARGN})
        # 获取相对于项目根目录的路径
        if(IS_ABSOLUTE "${PATH_}")
            file(RELATIVE_PATH REL_FILE "${CMAKE_CURRENT_SOURCE_DIR}" "${PATH_}")
            list(APPEND RELATIVE_PATHS "${REL_FILE}")
        else()
            list(APPEND RELATIVE_PATHS "${PATH_}")
        endif()
    endforeach()

    set(${ret} ${RELATIVE_PATHS} PARENT_SCOPE)
endfunction(path_to_relative)

# 相对路径转成绝对路径
function(path_to_absolute ret)
    set(abs_paths "")

    foreach(path_ ${ARGN})
        if(NOT IS_ABSOLUTE "${path_}")
            get_filename_component(abs_path "${CMAKE_CURRENT_SOURCE_DIR}/${path_}" ABSOLUTE)
            list(APPEND abs_paths "${abs_path}")
        else()
            list(APPEND abs_paths "${path_}")
        endif()
    endforeach()

    set(${ret} ${abs_paths} PARENT_SCOPE)
endfunction(path_to_absolute)

# 自动收集源文件
# ret:      若 ret 已经存在, 则会将收集到的源文件追加到 ret 中
# pattern:  匹配的文件模式
# dir:      指定的目录, 递归查找
# RELATIVE: 可选参数，指定时返回相对路径，否则返回绝对路径
function(recurse_auto_sources ret pattern dir)
    path_to_absolute(dir ${dir}) # 确保 dir 是绝对路径

    file(GLOB subdir_files "${dir}/${pattern}")
    list(APPEND ${ret} ${subdir_files})

    file(GLOB subdirs RELATIVE ${dir} ${dir}/*)

    foreach(DIR ${subdirs})
        if (IS_DIRECTORY ${dir}/${DIR})
            if (NOT "${DIR}" STREQUAL "CMakeFiles")
                file(GLOB_RECURSE subdir_files "${dir}/${DIR}/${pattern}")
                list(APPEND ${ret} ${subdir_files})
            endif()
        endif()
    endforeach()

    if("RELATIVE" IN_LIST ARGN)
        path_to_relative(${ret} ${${ret}})
    endif()
    
    set(${ret} ${${ret}} PARENT_SCOPE)
endfunction(recurse_auto_sources)

# Remove all files matching a set of patterns, and,
# optionally, not matching a second set of patterns,
# from a set of lists.
#
# Example:
# This will remove all files in the CPP_SOURCES list
# matching "/test/" or "Test.cpp$", but not matching
# "BobTest.cpp$".
# remove_matches_from_lists(CPP_SOURCES MATCHES "/test/" "Test.cpp$" IGNORE_MATCHES "BobTest.cpp$")
#
# Parameters:
#
# [...]:
# The names of the lists to remove matches from.
#
# [MATCHES ...]:
# The matches to remove from the lists.
#
# [IGNORE_MATCHES ...]:
# The matches not to remove, even if they match
# the main set of matches to remove.
function(remove_matches_from_lists)
    set(LISTS_TO_SEARCH)
    set(MATCHES_TO_REMOVE)
    set(MATCHES_TO_IGNORE)
    set(argumentState 0)
    foreach (arg ${ARGN})
        if ("x${arg}" STREQUAL "xMATCHES")
            set(argumentState 1)
        elseif ("x${arg}" STREQUAL "xIGNORE_MATCHES")
            set(argumentState 2)
        elseif (argumentState EQUAL 0)
            list(APPEND LISTS_TO_SEARCH ${arg})
        elseif (argumentState EQUAL 1)
            list(APPEND MATCHES_TO_REMOVE ${arg})
        elseif (argumentState EQUAL 2)
            list(APPEND MATCHES_TO_IGNORE ${arg})
        else()
            message(FATAL_ERROR "Unknown argument state!")
        endif()
    endforeach()

    foreach (theList ${LISTS_TO_SEARCH})
        foreach (entry ${${theList}})
            foreach (match ${MATCHES_TO_REMOVE})
                if (${entry} MATCHES ${match})
                    set(SHOULD_IGNORE OFF)
                    foreach (ign ${MATCHES_TO_IGNORE})
                        if (${entry} MATCHES ${ign})
                            set(SHOULD_IGNORE ON)
                            break()
                        endif()
                    endforeach()

                    if (NOT SHOULD_IGNORE)
                        list(REMOVE_ITEM ${theList} ${entry})
                    endif()
                endif()
            endforeach()
        endforeach()
        set(${theList} ${${theList}} PARENT_SCOPE)
    endforeach()
endfunction()

# 返回文件所在目录
# ret:      返回的目录列表
# files:    文件列表
# RELATIVE: 可选参数，指定时返回相对路径，否则返回绝对路径
function(find_files_dirs ret files)
    foreach(file_ ${files})
        if(NOT IS_ABSOLUTE "${file_}")
            get_filename_component(file_ "${CMAKE_CURRENT_SOURCE_DIR}/${file_}" ABSOLUTE)
        endif()

        get_filename_component(dir_ ${file_} DIRECTORY)
        if (EXISTS "${dir_}")
            if(RETURN_RELATIVE)
                # 转换为相对路径
                file(RELATIVE_PATH rel_dir_ "${CMAKE_CURRENT_SOURCE_DIR}" "${dir_}")
                list(APPEND ${ret} ${rel_dir_})
            else()
                list(APPEND ${ret} ${dir_})
            endif()
        endif()
    endforeach()

    # 去除重复的路径
    if(${ret})
        list(REMOVE_DUPLICATES ${ret})
    endif()

    if("RELATIVE" IN_LIST ARGN)
        path_to_relative(${ret} ${${ret}})
    endif()

    set(${ret} ${${ret}} PARENT_SCOPE)
endfunction(find_files_dirs ret files)

