$a = Get-Content ../../../../MyProjectBuildScript.uproject -Encoding UTF8 | ConvertFrom-Json
$a

$a.EngineAssociation = $Args[0]

if ($a.EngineAssociation -eq '4.23' -or $a.EngineAssociation -eq '4.22' -or $a.EngineAssociation -eq '4.21' -or $a.EngineAssociation -eq '4.20')
{

    $b = Get-Content ../../../VRM4U/VRM4U.uplugin -Encoding UTF8 | ConvertFrom-Json
    $b

    $b.Modules[5].Type = 'Developer'

    $b | ConvertTo-Json > ../../../VRM4U/VRM4U.uplugin
}

if ($a.EngineAssociation -eq '4.27' -or $a.EngineAssociation -eq '4.26' -or $a.EngineAssociation -eq '4.25' -or $a.EngineAssociation -eq '4.24' -or $a.EngineAssociation -eq '4.23' -or $a.EngineAssociation -eq '4.22' -or $a.EngineAssociation -eq '4.21' -or $a.EngineAssociation -eq '4.20')
{

    $b = Get-Content ../../../VRM4U/VRM4U.uplugin -Encoding UTF8 | ConvertFrom-Json
    $b

    $PluginArrayList = [System.Collections.ArrayList]$b.Plugins
    $PluginArrayList.RemoveAt(3)
    $PluginArrayList.RemoveAt(2)
    $PluginArrayList.RemoveAt(1)
    $b.Plugins = $PluginArrayList

    $b | ConvertTo-Json > ../../../VRM4U/VRM4U.uplugin
}

if ($a.EngineAssociation -eq '5.1' -or $a.EngineAssociation -eq '5.0' -or $a.EngineAssociation -eq '4.27' -or $a.EngineAssociation -eq '4.26' -or $a.EngineAssociation -eq '4.25' -or $a.EngineAssociation -eq '4.24' -or $a.EngineAssociation -eq '4.23' -or $a.EngineAssociation -eq '4.22' -or $a.EngineAssociation -eq '4.21' -or $a.EngineAssociation -eq '4.20')
{

    $b = Get-Content ../../../VRM4U/VRM4U.uplugin -Encoding UTF8 | ConvertFrom-Json
    $b

    $ModuleArrayList = [System.Collections.ArrayList]$b.Modules
    $ModuleArrayList.RemoveAt(4)
    $ModuleArrayList.RemoveAt(3)
    $ModuleArrayList.RemoveAt(2)
    $b.Modules = $ModuleArrayList

    $b | ConvertTo-Json > ../../../VRM4U/VRM4U.uplugin
}


$a | ConvertTo-Json > ../../../../MyProjectBuildScript.uproject

