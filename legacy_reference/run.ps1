# platformio-run.ps1
$ErrorActionPreference = 'Stop'

$containerName = 'platformio_run'

# 1) Require GITHUB_TOKEN
if (-not $Env:GITHUB_TOKEN -or [string]::IsNullOrWhiteSpace($Env:GITHUB_TOKEN)) {
    Write-Error "GITHUB_TOKEN is not set. Please set it to continue."
    exit 1
}

# 2) Docker login using stdin
Write-Host "Logging into GitHub Container Registry..."
Write-Output "$Env:GITHUB_TOKEN" | docker login ghcr.io -u "__token__" --password-stdin | Out-Null

# 3) Remove existing container if present
$exists = docker ps -a --format '{{.Names}}' | Where-Object { $_ -eq $containerName }
if ($exists) {
    Write-Host "Container $containerName already exists. Removing..."
    docker rm --force $containerName | Out-Null
}

# 4) Run container (mount current dir and load .env)

Write-Host "Running container $containerName..."
docker run -d -it --name $containerName `
  --env-file .\.env `
  ghcr.io/goldenm-software/platformio-builder:uv-3.13 | Out-Null

Write-Host "Copying source code into container..."
docker cp .\ ${containerName}:/app

# 5) Enter the container
Write-Host "Entering container $containerName..."
docker exec -it $containerName /bin/bash
