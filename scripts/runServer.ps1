# Change to the parent directory
Set-Location -Path ..

# Check if the 'build' directory exists, create it if not
if (-not (Test-Path -Path "build_win")) {
    New-Item -Path "build_win" -ItemType Directory -ErrorAction Stop
}

# Change to the 'build' directory
Set-Location -Path "build_win" -ErrorAction Stop

# Run CMake to configure the project
try {
    cmake -DCMAKE_C_COMPILER="C:\\msys64\\ucrt64\\bin\\gcc.exe" -DCMAKE_CXX_COMPILER="C:\\msys64\\ucrt64\\bin\\g++.exe" .. 
} catch {
    Write-Host "CMake configuration failed. Exiting with error code $($LASTEXITCODE)."
    exit $LASTEXITCODE
}

# Build the project target 'client'
try {
    cmake --build . --target server_win
} catch {
    Write-Host "Build failed. Exiting with error code $($LASTEXITCODE)."
    exit $LASTEXITCODE
}

# Change back to the parent directory
Set-Location -Path .. -ErrorAction Stop

# Execute the built client with any arguments passed to the script
try {
    & ".\build_win\debug\server_win" $args
} catch {
    Write-Host "Execution failed. Exiting with error code $($LASTEXITCODE)."
    exit $LASTEXITCODE
}

# Exit with no errors
exit 0


