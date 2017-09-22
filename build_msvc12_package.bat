@echo Easy-to-use Windows build script
@echo This mirrors what Jenkins is doing
@echo Prerequirements: cmake, Visual Studio 2013 (especially msbuild.exe)

set CMAKE_BUILD_TYPE=Release
set BUILD_OPTIONS=/verbosity:d /p:Configuration=%CMAKE_BUILD_TYPE%
set CMAKE_GENERATOR="Visual Studio 12 2013"
set CMAKE_BUILD_TOOL=msbuild
set BUILD_DIR=build

mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake .. -G %CMAKE_GENERATOR% -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%  -DBUILD_FMUS=OFF && ^
cmake --build .                  -- /m %BUILD_OPTIONS% && ^
cmake --build . --target package -- %BUILD_OPTIONS%

cd ..