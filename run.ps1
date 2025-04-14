& .\build.ps1 $args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
.\main.exe
exit $LASTEXITCODE