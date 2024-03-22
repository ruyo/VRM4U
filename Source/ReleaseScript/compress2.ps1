
Remove-Item -Recurse ./_out/Intermediate
Remove-Item -Recurse ./_out/Binaries/Win64/*.pdb
Remove-Item -Recurse ./_out/Source/ReleaseScript

Copy-Item -Path ../../ThirdParty -Destination ./_out -Recurse -Container

Move-Item -Path ./_out -Destination ./Plugins

Compress-Archive -Force -Path ./Plugins -DestinationPath $Args[0]


Move-Item -Path ./Plugins -Destination ./_out

Remove-Item -Recurse ./_out

