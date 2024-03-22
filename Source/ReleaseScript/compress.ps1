
Remove-Item -Recurse ../../../../Plugins/VRM4U/Intermediate
Remove-Item -Recurse ../../../../Plugins/VRM4U/Binaries/Win64/*.pdb
Remove-Item -Recurse ../../../../Plugins/VRM4U/Source/_zip
Remove-Item -Recurse ../../../../Plugins/VRM4U/Source/_out
Remove-Item -Recurse ../../../../Plugins/VRM4U/Source/Plugins

Compress-Archive -Force -Path ../../../../Plugins -DestinationPath $Args[0]


