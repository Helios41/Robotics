@echo off
pushd Source

call vcvarsall x86
cl -Zi -Od -Fedashboard_win32 dashboard_win32.cpp user32.lib gdi32.lib opengl32.lib ws2_32.lib
copy dashboard_win32.exe "../build/dashboard_win32.exe"
copy dashboard_win32.pdb "../build/dashboard_win32.pdb"
del *.obj *.pdb *.ilk *.exe

popd
pause
exit