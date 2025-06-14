function(git_commit_counts _var)
    if(NOT GIT_FOUND)
        set(${_var}
            "000000"
            PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-list HEAD --count
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT res EQUAL 0)
        set(out "000000")
    endif()

    set(${_var}
        "${out}"
        PARENT_SCOPE)
endfunction()


function(git_branch_name _var)
    if(NOT GIT_FOUND)
        set(${_var}
            "main"
            PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT res EQUAL 0)
        set(out "main")
    endif()

    set(${_var}
        "${out}"
        PARENT_SCOPE)
endfunction()

function(git_describe_string _var)
    if(NOT GIT_FOUND)
        set(${_var}
            "unknown"
            PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT res EQUAL 0)
        set(out "unknown")
    endif()

    set(${_var}
        "${out}"
        PARENT_SCOPE)
endfunction()

function(git_version_string _var)
    git_commit_counts(COMMITS)
    
    # 计算版本号
    # 每100次提交，次版本号加1
    # 每1000次提交(次版本号超过9)，主版本号加1
    math(EXPR MAJOR "1 + (${COMMITS} / 1000)")
    math(EXPR MINOR "(${COMMITS} / 100) % 10")
    math(EXPR PATCH "${COMMITS} % 100")
    
    set(${_var}
        "${MAJOR}.${MINOR}.${PATCH}"
        PARENT_SCOPE)
endfunction()

function(make_version_file template_path output_path)
    find_package(Git REQUIRED)

    git_commit_counts(GIT_COMMIT_COUNT)
    git_version_string(GIT_SEMVER)
    git_describe_string(GIT_DESCRIBE)

    configure_file(
        ${template_path}
        ${output_path}
        @ONLY
    )
endfunction()
