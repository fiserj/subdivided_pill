add_executable(gensvg gensvg.c)

if(MSVC)
  target_compile_options(gensvg PRIVATE /W4)
else()
  target_compile_options(gensvg PRIVATE -Wall -Wextra)
endif()

set_target_properties(gensvg PROPERTIES
  C_STANDARD 99
  C_EXTENSIONS OFF
  C_STANDARD_REQUIRED ON
)
