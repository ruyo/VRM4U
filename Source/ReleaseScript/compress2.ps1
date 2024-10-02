
Remove-Item -Recurse ./_out/Intermediate
Remove-Item -Recurse ./_out/Binaries/Win64/*.pdb
Remove-Item -Recurse ./_out/Source/ReleaseScript

Copy-Item -Path ../../ThirdParty -Destination ./_out -Recurse -Container

New-Item -ItemType Directory -Path "Plugins"

Move-Item -Path ./_out -Destination ./Plugins/VRM4U

Compress-Archive -Force -Path ./Plugins -DestinationPath $Args[0]


Remove-Item -Recurse ./Plugins
Remove-Item -Recurse ./_out

