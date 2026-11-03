param(
    [int]$Populacao = 100,
    [int]$Geracoes = 200,
    [uint32]$Semente = 20261102
)

$ErrorActionPreference = 'Stop'
$projetoC = Split-Path $PSScriptRoot -Parent
$referenciaCpp = Join-Path (Split-Path $projetoC -Parent) 'v2 - c++'
$pastaTemporaria = Join-Path $env:TEMP 'ica_nsga2-paridade'
$executavelCpp = Join-Path $pastaTemporaria 'ica_nsga2_cpp.exe'
$executavelC = Join-Path $env:TEMP 'ica_nsga2-c-build\programa_nsga2.exe'
$saidaCpp = Join-Path $pastaTemporaria 'cpp'
$saidaC = Join-Path $pastaTemporaria 'c'

New-Item -ItemType Directory -Force $pastaTemporaria | Out-Null

Push-Location $referenciaCpp
try {
    $fontesCpp = Get-ChildItem (Join-Path $referenciaCpp 'src') -Filter '*.cpp' |
        Sort-Object Name |
        ForEach-Object FullName
    if ($fontesCpp.Count -eq 0) {
        throw "Nenhum fonte C++ encontrado em $referenciaCpp\src."
    }
    & g++ -std=c++17 -O2 "-I$(Join-Path $referenciaCpp 'include')" $fontesCpp -o $executavelCpp
    if ($LASTEXITCODE -ne 0) {
        throw 'Falha ao compilar a referencia C++.'
    }
} finally {
    Pop-Location
}

& (Join-Path $projetoC 'compilar.ps1') -Otimizado
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $executavelC)) {
    throw 'Falha ao compilar a versao C.'
}

& $executavelCpp both $Populacao $Geracoes --seed $Semente --log quiet --output $saidaCpp
if ($LASTEXITCODE -ne 0) {
    throw 'A referencia C++ falhou durante a execucao.'
}

& $executavelC both $Populacao $Geracoes --seed $Semente --log quiet --output $saidaC
if ($LASTEXITCODE -ne 0) {
    throw 'A versao C falhou durante a execucao.'
}

$esperados = @{ case_A = 55; case_B = 68 }
foreach ($cenario in @('case_A', 'case_B')) {
    $arquivoCpp = Join-Path $saidaCpp "$cenario\pareto.csv"
    $arquivoC = Join-Path $saidaC "$cenario\pareto.csv"
    $linhasCpp = Get-Content $arquivoCpp
    $linhasC = Get-Content $arquivoC
    $solucoes = (Import-Csv $arquivoC).Count
    $diferencas = @(Compare-Object $linhasCpp $linhasC)

    if ($Populacao -eq 100 -and $Geracoes -eq 200 -and $Semente -eq 20261102 -and
        $solucoes -ne $esperados[$cenario]) {
        throw "$cenario produziu $solucoes solucoes; esperado: $($esperados[$cenario])."
    }
    if ($diferencas.Count -ne 0) {
        Write-Host "Primeiras diferencas de ${cenario}:"
        $diferencas | Select-Object -First 10 | Format-Table -AutoSize
        throw "$cenario divergiu da referencia C++ em $($diferencas.Count) linhas."
    }
    Write-Host "PASS: $cenario - $solucoes solucoes e nenhuma diferenca de conteudo"
}

Write-Host 'PASS: paridade completa entre C e C++'
