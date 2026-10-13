<# Build a T21 Development test package and record the actual working-tree inputs.
    Existing Blueprint edits are read, never saved or staged. Shipping candidate
    packaging and platform submission remain separate T25/T27 work. #>
[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\Program Files\Epic\UE_5.8',
    [string]$ArchiveDirectory,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$taskProject = Join-Path $taskRoot 'MothEffect.uproject'
$taskUAT = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
if (!(Test-Path -LiteralPath $taskUAT)) { throw "Missing UAT: $taskUAT" }
if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) { throw 'Save your editor work and close Unreal Editor before this package build.' }
$taskCommit = (git -C $taskRoot rev-parse HEAD).Trim()
$taskStamp = [TimeZoneInfo]::ConvertTimeBySystemTimeZoneId([DateTime]::UtcNow, 'China Standard Time').ToString('yyyyMMdd_HHmmss')
if (!$ArchiveDirectory) { $ArchiveDirectory = Join-Path $taskRoot "Builds/T21/MothEffect_${taskStamp}_$($taskCommit.Substring(0,7))" }
$taskArchive = [IO.Path]::GetFullPath($ArchiveDirectory)
$taskLogDirectory = Join-Path $taskRoot "Saved/Automation/T21/Package_$taskStamp"
New-Item -ItemType Directory -Path $taskArchive, $taskLogDirectory -Force | Out-Null

function Get-TaskInputHashes {
    $taskPaths = git -C $taskRoot ls-files -- Source Config Content/MothEffect MothEffect.uproject
    foreach ($taskRelative in $taskPaths) {
        $taskFile = Join-Path $taskRoot $taskRelative
        if (Test-Path -LiteralPath $taskFile -PathType Leaf) {
            [pscustomobject]@{ path=$taskRelative.Replace('\','/'); sha256=(Get-FileHash -LiteralPath $taskFile -Algorithm SHA256).Hash }
        }
    }
}

$taskBefore = @(Get-TaskInputHashes)
$taskArgs = @('BuildCookRun', "-project=$taskProject", '-nop4', '-platform=Win64', '-clientconfig=Development',
    '-cook', '-stage', '-pak', '-iostore', '-archive', "-archivedirectory=$taskArchive",
    '-map=/Game/MothEffect/Maps/L_Arena_01', '-nocompileeditor', '-skipeditorcontent',
    '-nodebuginfo', '-prereqs', '-unattended', '-utf8output')
if (!$SkipBuild) { $taskArgs += '-build' }
$taskManifest = [ordered]@{
    task='T21'; local_time_hong_kong=$taskStamp; git_commit=$taskCommit
    configuration='Win64 Development'; default_map='/Game/MothEffect/Maps/L_Arena_01'
    gameplay_parameters='v0.18'; editor_closed=$true
    git_status=@(git -C $taskRoot status --short)
    archive_directory=$taskArchive; uat_arguments=$taskArgs; input_hashes=$taskBefore
    external_dependencies='Read from this project; original third-party package version/license/full restoration evidence pending.'
    user_pie_acceptance='Pending'; complete_G3='Pending external TC26'
}
$taskManifest | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $taskLogDirectory 'BuildInputs.json') -Encoding utf8
Write-Output "T21 archive: $taskArchive"
Write-Output "Input evidence: $taskLogDirectory"
& $taskUAT @taskArgs *> (Join-Path $taskLogDirectory 'UATConsole.log')
$taskExit = $LASTEXITCODE
$taskAfter = @(Get-TaskInputHashes)
$taskChanges = @(Compare-Object -ReferenceObject $taskBefore -DifferenceObject $taskAfter -Property path,sha256)
[ordered]@{ uat_exit_code=$taskExit; input_changes=$taskChanges; archive_directory=$taskArchive } |
    ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $taskLogDirectory 'BuildResult.json') -Encoding utf8
Get-Content -LiteralPath (Join-Path $taskLogDirectory 'UATConsole.log') -Tail 35
if ($taskExit -ne 0) { exit $taskExit }
if ($taskChanges.Count) { throw 'Source/asset inputs changed during the build; see BuildResult.json before treating the package as a stable snapshot.' }
Write-Output "T21 build succeeded: $taskArchive"
