find_package (Subversion)
if (Subversion_FOUND AND EXISTS "${SOURCE_DIR}/.svn")
  # the FindSubversion.cmake module is part of the standard distribution
  include (FindSubversion)
  # extract working copy information for SOURCE_DIR into MY_XXX variables
  Subversion_WC_INFO (${SOURCE_DIR} MY)
  message ("${MY_WC_INFO}")
  # determine if the working copy has outstanding changes
  execute_process (COMMAND ${Subversion_SVN_EXECUTABLE} status ${SOURCE_DIR}
    OUTPUT_FILE "${OUTPUT_DIR}/svn_status.txt"
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  file (STRINGS "${OUTPUT_DIR}/svn_status.txt" __svn_changes
    REGEX "^[^?].*$"
    )
  if (__svn_changes)
    set (MY_WC_REVISION "${MY_WC_REVISION}-dirty")
    foreach (__svn_change ${__svn_changes})
      message (STATUS "${__svn_change}")
    endforeach (__svn_change ${__svn_changes})
  endif (__svn_changes)
  # write a file with the SVNVERSION define
  file (WRITE svnversion.h.txt "#define SVNVERSION r${MY_WC_REVISION}\n")
else (Subversion_FOUND AND EXISTS "${SOURCE_DIR}/.svn")
  file (WRITE svnversion.h.txt "#define SVNVERSION local\n")
endif (Subversion_FOUND AND EXISTS "${SOURCE_DIR}/.svn")

# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different svnversion.h.txt svnversion.h)