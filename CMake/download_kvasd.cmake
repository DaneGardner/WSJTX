#
# CMake script to fetch kvasd binary for the current platform
#
set (kvasd_NAME https://svn.code.sf.net/p/wsjt/wsjt/trunk/kvasd-binary/${SYSTEM_NAME}/kvasd${EXECUTABLE_SUFFIX})
message (STATUS "file: ${kvasd_NAME}")
if (APPLE)
  set (__kvasd_md5sum 198dbdde1e4b04f9940f63731097ee35)
endif (APPLE)
if (WIN32)
  set (__kvasd_md5sum 7b16809e51126a01bd02aed44427510c)
endif (WIN32)
if (UNIX)
  set (__kvasd_md5sum 77d5eef0215783fa74478ab411ac32ca)
endif (UNIX)
file (
  DOWNLOAD ${kvasd_NAME} contrib/kvasd${EXECUTABLE_SUFFIX}
  TIMEOUT 10
  STATUS kvasd_STATUS
  LOG kvasd_LOG
  SHOW_PROGRESS
  EXPECTED_MD5 ${__kvasd_md5sum}
  )
list (GET kvasd_STATUS 0 kvasd_RC)
if (kvasd_RC)
  message (WARNING "${kvasd_STATUS}")
  message (FATAL_ERROR "${kvasd_LOG}")
endif (kvasd_RC)
