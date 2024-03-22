
Remove-Item -Recurse ./_out/Intermediate
Remove-Item -Recurse ./_out/Binaries/Win64/*.pdb

Compress-Archive -Force -Path ./_out -DestinationPath $Args[0]


