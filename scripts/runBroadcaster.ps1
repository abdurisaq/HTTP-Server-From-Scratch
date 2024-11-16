# Set the parent directory for builds
Set-Location -Path ..

# Check if the 'build_win' directory exists, create it if not
if (-not (Test-Path -Path "build_win")) {
    New-Item -Path "build_win" -ItemType Directory -ErrorAction Stop
}

# Change to the 'build_win' directory
Set-Location -Path "build_win" -ErrorAction Stop

# Configure and build the server target
try {
    Write-Host "Configuring and building the server..."
    cmake -DCMAKE_C_COMPILER="C:\\msys64\\ucrt64\\bin\\gcc.exe" -DCMAKE_CXX_COMPILER="C:\\msys64\\ucrt64\\bin\\g++.exe" .. 
    cmake --build . --target server_win
} catch {
    Write-Host "Server build failed. Exiting with error code $($LASTEXITCODE)."
    exit $LASTEXITCODE
}

# Configure and build the client target
try {
    Write-Host "Configuring and building the client..."
    cmake -DCMAKE_C_COMPILER="C:\\msys64\\ucrt64\\bin\\gcc.exe" -DCMAKE_CXX_COMPILER="C:\\msys64\\ucrt64\\bin\\g++.exe" .. 
    cmake --build . --target client_win
} catch {
    Write-Host "Client build failed. Exiting with error code $($LASTEXITCODE)."
    exit $LASTEXITCODE
}

# Change back to the parent directory
Set-Location -Path .. -ErrorAction Stop

# Execute the built server and client as separate processes
try {
    Write-Host "Starting the server process..."
    Start-Process ".\build_win\debug\server_win"

    Write-Host "Starting the client process..."
    Start-Process ".\build_win\debug\client_win"
} catch {
    Write-Host "Execution failed. Exiting with error code $($LASTEXITCODE)."
    exit $LASTEXITCODE
}

# Exit with success
Write-Host "Build and execution completed successfully."
exit 0

