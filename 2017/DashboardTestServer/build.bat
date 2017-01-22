@echo off
pushd Source

call vcvarsall x86
cl -Zi -Od -Fedashboard_test_server_win32 dashboard_test_server_win32.cpp ws2_32.lib
copy dashboard_test_server_win32.exe "../build/dashboard_test_server_win32.exe"
copy dashboard_test_server_win32.pdb "../build/dashboard_test_server_win32.pdb"
del *.obj *.pdb *.ilk *.exe

popd
pause
exit