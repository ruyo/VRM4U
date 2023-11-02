$a = Get-Content ../MyProjectBuildScript.uproject -Encoding UTF8 | ConvertFrom-Json
$a

$a.EngineAssociation = $Args[0]

if ($a.EngineAssociation -eq '4.23' -or $a.EngineAssociation -eq '4.22' -or $a.EngineAssociation -eq '4.21' -or $a.EngineAssociation -eq '4.20')
{

    $b = Get-Content ../Plugins/VRM4U/VRM4U.uplugin -Encoding UTF8 | ConvertFrom-Json
    $b

    $b.Modules[2].Type = 'Developer'
    $b.Modules[3].Type = 'Developer'

    $b | ConvertTo-Json > ../Plugins/VRM4U/VRM4U.uplugin
}


$a | ConvertTo-Json > ../MyProjectBuildScript.uproject

