
Remove-Item -Recurse ../../../../Plugins/VRM4U/Intermediate
Remove-Item -Recurse ../../../../Plugins/VRM4U/Binaries/Win64/*.pdb

Compress-Archive -Force -Path ../../../../Plugins -DestinationPath $Args[0]


