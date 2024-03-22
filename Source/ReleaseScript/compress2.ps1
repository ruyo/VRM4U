
Remove-Item -Recurse ./_out/Intermediate
Remove-Item -Recurse ./_out/Binaries/Win64/*.pdb

Compress-Archive -Force -Path ./Plugins -DestinationPath $Args[0]


