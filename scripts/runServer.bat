$echo off
cd ..

if not exist "build" mkdir build || goto :error

cd build || goto :error

cmake .. || goto :error

cmake --build . --target server || goto :error


cd .. || goto :error

.\build\server %* || goto :error

exit /b



:error
REM Handle the error and exit
echo An error occurred. Exiting with error code %errorlevel%.
exit /b %errorlevel%
