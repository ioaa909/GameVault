$mingwBin = "C:\ProgramData\mingw64\mingw64\bin"
$env:Path = "$mingwBin;$env:Path"

$cflags = @("-O2", "-s", "-mwindows")
$libs = @("-lcomctl32", "-lcomdlg32", "-lgdi32", "-lshell32", "-luser32", "-ladvapi32", "-lole32", "-luuid", "-lpsapi", "-lshlwapi", "-luxtheme", "-ldwmapi", "-lversion")
$lflags = @("-static")

Write-Host "Compiling resources..."
& "$mingwBin\windres" "resource.rc" -o "resource.o"
if ($LASTEXITCODE -ne 0) { Write-Host "Resource compilation failed!"; exit 1 }

Write-Host "Compiling GameVault.cpp..."
& "$mingwBin\g++" @cflags -c "GameVault.cpp" -o "GameVault.o"
if ($LASTEXITCODE -ne 0) { Write-Host "Compilation failed!"; exit 1 }

Write-Host "Linking..."
& "$mingwBin\g++" @cflags "GameVault.o" "resource.o" -o "GameVault.exe" @libs @lflags
if ($LASTEXITCODE -ne 0) { Write-Host "Linking failed!"; exit 1 }

Write-Host "Build successful! GameVault.exe created."
Get-ChildItem "GameVault.exe" | Select-Object Name, Length