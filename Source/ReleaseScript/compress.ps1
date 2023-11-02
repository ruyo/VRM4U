
Remove-Item -Recurse ../Intermediate
Remove-Item -Recurse ../Plugins/VRM4U/Intermediate
Remove-Item -Recurse ../Plugins/VRM4U/Binaries/Win64/*.pdb
Remove-Item -Recurse ../Plugins/VRM4U/Source

Compress-Archive -Force -Path ../Plugins -DestinationPath $Args[0]


