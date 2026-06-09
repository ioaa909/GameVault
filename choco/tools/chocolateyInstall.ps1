$ErrorActionPreference = 'Stop'

$toolsDir   = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
$zipUrl     = 'https://github.com/ioaa909/GameVault/releases/download/v1.0.0/GameVault-v1.0.0.zip'
$zipHash    = '80bdf1e388c4c0445a06833548c6bf817f3e34f2fa18d71187b17049a77d4fdb'

$packageArgs = @{
  packageName   = 'gamevault'
  unzipLocation = $toolsDir
  url           = $zipUrl
  checksum      = $zipHash
  checksumType  = 'sha256'
}

Install-ChocolateyZipPackage @packageArgs
