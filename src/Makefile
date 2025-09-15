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


mode_x.lib : vga_ctl.obj palette.obj screen.obj image.obj sprite.obj font.obj gdi.obj
  # Make Library
  @del mode_x.lib > NUL
  @tlib mode_x.lib /c +vga_ctl +palette +screen +image +sprite +font +gdi
  @del mode_x.bak > NUL


# Common Module
vga_ctl.obj  : vga_ctl.cpp  mode_x.h vga_reg.h
palette.obj  : palette.cpp  mode_x.h
screen.obj   : screen.cpp   mode_x.h
image.obj    : image.cpp    mode_x.h
sprite.obj   : sprite.cpp   mode_x.h
font.obj     : font.cpp     mode_x.h
gdi.obj      : gdi.cpp      mode_x.h