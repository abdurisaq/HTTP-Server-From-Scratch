#run file with
#pwsh -ExecutionPolicy Bypass -File .\scanLAN.ps1
$baseIP = "192.168.1"

# Output file
$outputFile = "ipaddress.txt"
Set-Content -Path $outputFile -Value $null
sleep 1
# List of IPs to ping
$ipAddresses = 0..255 | ForEach-Object { "$baseIP.$_" }

# Perform parallel pinging using ForEach-Object -Parallel
$ipAddresses | ForEach-Object -ThrottleLimit 20 -Parallel {

    # $_ | Out-File -FilePath "ipaddress.txt" -Append
    # Ping the IP
    $pingResult = Test-Connection -ComputerName $_ -Count 1 -Quiet
    #
    # # Append to file if ping is successful
    if ($pingResult) {
        $_ | Out-File -FilePath "ipaddress.txt" -Append
    }
}
Write-Output "Completed pings. Output written to $outputFile."

