require 'mkmf'

pkg_config("librsvg-2.0")

$LIBS << " -ljpeg"

extension_name = 'ffimage'
dir_config(extension_name)
create_makefile(extension_name)
