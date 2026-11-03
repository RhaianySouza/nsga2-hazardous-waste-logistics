param(
    [switch]$Testar,
    [switch]$Otimizado
)

$ErrorActionPreference = 'Stop'
$projeto = $PSScriptRoot
$pastaBuild = Join-Path $env:TEMP 'ica_nsga2-c-build'
$pastaObjetos = Join-Path $pastaBuild 'objetos'
$include = Join-Path $projeto 'include'
$fontes = Get-ChildItem (Join-Path $projeto 'src') -Filter '*.c' | Sort-Object Name
$flags = @('-std=c11', '-Wall', '-Wextra', '-Wpedantic')

if ($Otimizado) {
    $flags += '-O2'
} else {
    $flags += @('-O0', '-g3')
}

New-Item -ItemType Directory -Force $pastaObjetos | Out-Null
$objetos = @{}

foreach ($fonte in $fontes) {
    $objeto = Join-Path $pastaObjetos ($fonte.BaseName + '.o')
    & gcc @flags "-I$include" -c $fonte.FullName -o $objeto
    if ($LASTEXITCODE -ne 0) {
        throw "Falha ao compilar $($fonte.Name)."
    }
    $objetos[$fonte.BaseName] = $objeto
}

$executavel = Join-Path $pastaBuild 'programa_nsga2.exe'
& g++ @($objetos.Values) -o $executavel
if ($LASTEXITCODE -ne 0) {
    throw 'Falha ao ligar o executavel principal.'
}
Write-Host "PASS: programa compilado em $executavel"

if ($Testar) {
    $objetoTestes = Join-Path $pastaObjetos 'testes.o'
    $fonteTestes = Join-Path $projeto 'tests\testes.c'
    & gcc @flags "-I$include" -c $fonteTestes -o $objetoTestes
    if ($LASTEXITCODE -ne 0) {
        throw 'Falha ao compilar os testes.'
    }

    $objetosTeste = @($objetos.GetEnumerator() |
        Where-Object Key -ne 'main' |
        ForEach-Object Value)
    $objetosTeste += $objetoTestes
    $executavelTestes = Join-Path $pastaBuild 'testes.exe'
    & g++ $objetosTeste -o $executavelTestes
    if ($LASTEXITCODE -ne 0) {
        throw 'Falha ao ligar os testes.'
    }
    & $executavelTestes
    if ($LASTEXITCODE -ne 0) {
        throw 'A suite de testes falhou.'
    }
}

exit 0
