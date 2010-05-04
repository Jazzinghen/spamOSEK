echo Executing NeXTTool to upload SonarTest.rxe...
wineconsole /cygdrive/C/cygwin/nexttool/NeXTTool.exe /COM=usb -download=SonarTest.rxe
wineconsole /cygdrive/C/cygwin/nexttool/NeXTTool.exe /COM=usb -listfiles=SonarTest.rxe
echo NeXTTool is terminated.
