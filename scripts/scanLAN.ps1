# Run file with:
# pwsh -ExecutionPolicy Bypass -File .\scanLAN.ps1

Write-Host "Starting scanning local network"

$baseIP = "192.168.1"
$outputFile = "ipaddress.txt"

# Clear the output file
Set-Content -Path $outputFile -Value $null
Start-Sleep -Seconds 1

# List of IPs to ping
$ipAddresses = 0..255 | ForEach-Object { "$baseIP.$_" }

# Create a thread-safe collection to store successful pings
$successfulPings = [System.Collections.Generic.List[string]]::new()

# Perform parallel pinging using ForEach-Object -Parallel
$ipAddresses | ForEach-Object -ThrottleLimit 20 -Parallel {
    $localResults = @()  # Temporary array to store results in each thread

    # Ping the IP
    $pingResult = Test-Connection -ComputerName $_ -Count 1 -Quiet
    # If the ping is successful, add the IP to the local array
    if ($pingResult) {
        $localResults += $_
    }

    # Return results from this thread
    $localResults
} | ForEach-Object {
    # Append results to the shared collection after parallel execution
    $successfulPings.Add($_)
}

# Write all successful pings to the file
$successfulPings | Out-File -FilePath $outputFile -Append

Write-Output "Completed pings. Output written to $outputFile."

