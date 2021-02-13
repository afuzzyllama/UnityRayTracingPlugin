@echo off

setlocal
set GLSL_COMPILER=glslangValidator.exe
set SOURCE_FOLDER="../PluginSource~/source/PixelsForGlory/Shaders/"
set BINARIES_FOLDER="../Runtime/Plugins/x86_64/"

:: raygen shaders
%GLSL_COMPILER% --target-env vulkan1.2 -V -S rgen %SOURCE_FOLDER%ray_gen.glsl -o %BINARIES_FOLDER%ray_gen.bin

:: closest-hit shaders
%GLSL_COMPILER% --target-env vulkan1.2 -V -S rchit %SOURCE_FOLDER%ray_chit.glsl -o %BINARIES_FOLDER%ray_chit.bin
%GLSL_COMPILER% --target-env vulkan1.2 -V -S rchit %SOURCE_FOLDER%shadow_ray_chit.glsl -o %BINARIES_FOLDER%shadow_ray_chit.bin

:: miss shaders
%GLSL_COMPILER% --target-env vulkan1.2 -V -S rmiss %SOURCE_FOLDER%ray_miss.glsl -o %BINARIES_FOLDER%ray_miss.bin
%GLSL_COMPILER% --target-env vulkan1.2 -V -S rmiss %SOURCE_FOLDER%shadow_ray_miss.glsl -o %BINARIES_FOLDER%shadow_ray_miss.bin

