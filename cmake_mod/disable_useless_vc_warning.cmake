
if(MSVC)
  # Disable following warnings
  # C4819: Code page mismatched warning
  # SECURE WARNINGS: Common warnings appear when using POSIX or libc.
  add_definitions ( -wd4819 -D_C_SECURE_NO_WARNINGS )
endif()