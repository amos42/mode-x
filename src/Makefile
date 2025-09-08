#
#  Mode-X Library MAKE File
#           Ju, Gyeong-min


!ifndef tc_path
tc_path = c:\tc
!endif

!ifndef bin_path
bin_path = bin
!endif

!ifndef compiler
compiler = tcc
!endif

.cpp.obj:
  $(tc_path)\$(bin_path)\$(compiler) -c -ml -1 -f- -O -I$(tc_path)\include -E$(ta_path)\tasm.exe $<


mode_x.lib : mode_x.obj screen.obj image.obj sprite.obj font.obj
  # Make Library
  @del mode_x.lib > NUL
  @tlib mode_x.lib /c +mode_x +screen +image +sprite +font
  @del mode_x.bak > NUL


# Common Module
mode_x.obj   : mode_x.cpp  mode_x.h vga_reg.h
screen.obj   : screen.cpp  mode_x.h
image.obj    : image.cpp   mode_x.h
sprite.obj   : sprite.cpp  mode_x.h
font.obj     : font.cpp    mode_x.h
