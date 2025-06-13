@echo off
REM Gakumas 贴图翻译插件 - 本地构建脚本
REM 使用方法: build.bat [clean]

echo ===============================================
echo Gakumas Texture Replacement System - Builder
echo ===============================================

if "%1"=="clean" (
    echo [CLEAN] 清理构建目录...
    if exist "x64\Release" rmdir /s /q "x64\Release"
    if exist "x64\Debug" rmdir /s /q "x64\Debug"
    if exist "build" rmdir /s /q "build"
    echo [SUCCESS] 清理完成
    goto :eof
)

echo [INFO] 检查构建环境...

REM 检查MSBuild
where msbuild >nul 2>&1
if errorlevel 1 (
    echo [ERROR] 未找到MSBuild，请确保已安装Visual Studio 2022
    echo [TIP] 或者运行 "Developer Command Prompt for VS 2022"
    pause
    exit /b 1
)

REM 检查项目文件
if not exist "xinput1_3.vcxproj" (
    echo [ERROR] 未找到项目文件 xinput1_3.vcxproj
    pause
    exit /b 1
)

echo [SUCCESS] 构建环境检查通过

echo.
echo [BUILD] 开始编译 Release x64...
msbuild xinput1_3.vcxproj /p:Configuration=Release /p:Platform=x64 /p:OutputPath=..\build\ /verbosity:minimal

if errorlevel 1 (
    echo [ERROR] 编译失败！
    pause
    exit /b 1
)

REM 检查输出文件
if not exist "build\xinput1_3.dll" (
    echo [ERROR] 编译完成但未找到输出文件
    pause
    exit /b 1
)

echo [SUCCESS] 编译成功！

echo.
echo [PACKAGE] 准备发布包...

REM 创建发布目录
if not exist "dist" mkdir "dist"
if exist "dist\release-package" rmdir /s /q "dist\release-package"
mkdir "dist\release-package"

REM 复制文件
copy "build\xinput1_3.dll" "dist\release-package\"
xcopy "gakumas-local-texture" "dist\release-package\gakumas-local-texture\" /s /e /i

REM 创建使用说明
echo # Gakumas 贴图翻译插件 Release Package > "dist\release-package\README.txt"
echo. >> "dist\release-package\README.txt"
echo ## 安装说明 >> "dist\release-package\README.txt"
echo 1. 将 xinput1_3.dll 复制到 gakumas.exe 所在目录 >> "dist\release-package\README.txt"
echo 2. 将 gakumas-local-texture 文件夹复制到 gakumas.exe 所在目录 >> "dist\release-package\README.txt"
echo 3. 可自由编辑 gakumas-local-texture/asset_mapping.txt 配置要替换的图像 >> "dist\release-package\README.txt"
echo 4. 启动游戏即可生效 >> "dist\release-package\README.txt"
echo. >> "dist\release-package\README.txt"
echo 构建时间: %date% %time% >> "dist\release-package\README.txt"

echo [SUCCESS] 发布包准备完成！

echo.
echo [RESULT] 构建结果:
echo    DLL文件: build\xinput1_3.dll
echo    发布包: dist\release-package\
echo    说明文件: dist\release-package\README.txt

REM 获取文件大小
for %%F in ("build\xinput1_3.dll") do echo    DLL大小: %%~zF bytes

echo.
echo [SUCCESS] 构建完成！
echo [TIP] 可以运行 'build.bat clean' 清理构建文件
echo.
pause
