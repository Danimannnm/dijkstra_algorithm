cl /nologo /EHsc /Zi /std:c++17 $args main.cpp Util.cpp WinUtil.cpp Trophy.cpp /Fe:main.exe /link User32.lib
exit $LASTEXITCODE
