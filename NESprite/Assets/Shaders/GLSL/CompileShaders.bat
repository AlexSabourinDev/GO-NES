cd /D "%~dp0"
glslangValidator -V -o ../SPIR-V/default.fspv default.frag
glslangValidator -V -o ../SPIR-V/default.vspv default.vert
pause