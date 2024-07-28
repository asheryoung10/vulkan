%VULKAN_SDK%\bin\glslc.exe shader.vert -o vert.spv
%VULKAN_SDK%\bin\glslc.exe shader.frag -o frag.spv
cd spvToHeader/
call build.bat
call spvToHeaders.exe ../vert.spv ../frag.spv
cd ..
MOVE vert.spv.h ..
MOVE frag.spv.h ..
