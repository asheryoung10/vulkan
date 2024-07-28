@echo off
cd shaders
call compile.bat
cd ..
clang -DNDEBUG -O3  -Iinclude -Llib -I%VULKAN_SDK%/Include -L%VULKAN_SDK%/Lib vulkan.c -lglfw3 -lgdi32 -lvulkan-1 -luser32 -lshell32 -o Vulkan.exe
vulkan
