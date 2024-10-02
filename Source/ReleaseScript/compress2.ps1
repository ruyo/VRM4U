
Remove-Item -Recurse ($Args[1] + "/Intermediate")
Remove-Item -Recurse ($Args[1] + "/Binaries/Win64/*.pdb")
Remove-Item -Recurse ($Args[1] + "/Source/ReleaseScript")

Copy-Item -Path ../../ThirdParty -Destination $Args[1] -Recurse -Container

New-Item -ItemType Directory -Path "Plugins"

Move-Item -Path $Args[1] -Destination ./Plugins/VRM4U

Compress-Archive -Force -Path ./Plugins -DestinationPath $Args[0]


Remove-Item -Recurse ./Plugins
Remove-Item -Recurse $Args[1]

