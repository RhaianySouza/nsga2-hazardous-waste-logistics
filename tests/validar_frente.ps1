param(
    [int]$Populacao = 100,
    [int]$Geracoes = 200,
    [uint32]$Semente = 20261102
)

$ErrorActionPreference = 'Stop'
$projeto = Split-Path $PSScriptRoot -Parent
$executavel = Join-Path $env:TEMP 'ica_nsga2-c-build\programa_nsga2.exe'
$saida = Join-Path $env:TEMP ("ica_nsga2-validar-frente-" + [Guid]::NewGuid().ToString('N'))
$pesos = @{
    case_A = [double[]]@(85, 40, 55, 28, 110, 130, 35)
    case_B = [double[]]@(60, 3.5, 10, 50, 20, 96, 3.8)
}
$tipos = @{
    case_A = [int[]]@(0, 1, 1, 2, 3, 4, 0)
    case_B = [int[]]@(0, 1, 1, 1, 2, 3, 1)
}
$capacidades = [double[]]@(450, 350, 200, 400, 500, 450)
$destinosPermitidosPorTipo = @(
    [int[]]@(0, 4), [int[]]@(1, 4), [int[]]@(2, 4),
    [int[]]@(3, 4), [int[]]@(4, 5)
)
$incompativeis = @('0:2', '0:3', '1:2', '1:4', '2:3', '2:4')

function Test-Individuo {
    param(
        [object]$Individuo,
        [double[]]$Massas,
        [int[]]$Tipos
    )

    $destinos = @($Individuo.facility_genes -split ';' | ForEach-Object { [int]$_ })
    $prioridades = @($Individuo.priority_genes -split ';' | ForEach-Object { [int]$_ })
    if ($destinos.Count -ne 7 -or $prioridades.Count -ne 7) {
        throw "Cromossomo com tamanho inesperado: geração $($Individuo.generation), etapa $($Individuo.stage)."
    }
    if (@($prioridades | Sort-Object -Unique).Count -ne 7 -or
        ($prioridades | Measure-Object -Minimum).Minimum -ne 0 -or
        ($prioridades | Measure-Object -Maximum).Maximum -ne 6) {
        throw "Prioridades não formam permutação: geração $($Individuo.generation), etapa $($Individuo.stage)."
    }

    $cargas = [double[]]::new(6)
    for ($i = 0; $i -lt 7; $i++) {
        if ($destinos[$i] -lt 0 -or $destinos[$i] -ge 6 -or
            $destinosPermitidosPorTipo[$Tipos[$i]] -notcontains $destinos[$i]) {
            throw "Destino inelegível para lote ${i}: geração $($Individuo.generation), etapa $($Individuo.stage)."
        }
        if ($Massas[$i] -gt 150) {
            throw "Lote excede capacidade do veículo."
        }
        $cargas[$destinos[$i]] += $Massas[$i]
    }
    for ($f = 0; $f -lt 6; $f++) {
        if ($cargas[$f] -gt $capacidades[$f] + 1e-9) {
            throw "Capacidade do destinador $f excedida: geração $($Individuo.generation), etapa $($Individuo.stage)."
        }
    }

    $viagensCalculadas = 0
    for ($f = 0; $f -lt 6; $f++) {
        $lotes = @(0..6 | Where-Object { $destinos[$_] -eq $f } |
            Sort-Object { $prioridades[$_] })
        $viagens = [System.Collections.Generic.List[object]]::new()
        foreach ($lote in $lotes) {
            $colocado = $false
            foreach ($viagem in $viagens) {
                if ($viagem.carga + $Massas[$lote] -gt 150 + 1e-9) { continue }
                $compativel = $true
                foreach ($outroTipo in $viagem.tipos) {
                    $par = if ($Tipos[$lote] -lt $outroTipo) {
                        "$($Tipos[$lote]):$outroTipo"
                    } else {
                        "${outroTipo}:$($Tipos[$lote])"
                    }
                    if ($incompativeis -contains $par) {
                        $compativel = $false
                        break
                    }
                }
                if ($compativel) {
                    $viagem.carga += $Massas[$lote]
                    $viagem.tipos += $Tipos[$lote]
                    $colocado = $true
                    break
                }
            }
            if (-not $colocado) {
                $viagens.Add(@{ carga = $Massas[$lote]; tipos = @($Tipos[$lote]) })
            }
        }
        $viagensCalculadas += $viagens.Count
    }
    if ($viagensCalculadas -ne [int]$Individuo.trips) {
        throw "Contagem de viagens divergente: CSV=$($Individuo.trips), recalculado=$viagensCalculadas."
    }
    if ([double]$Individuo.violation -ge 1e-8) {
        throw "Cromossomo marcado inviável: geração $($Individuo.generation), etapa $($Individuo.stage)."
    }
}

& (Join-Path $projeto 'compilar.ps1') -Otimizado
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $executavel)) {
    throw 'Falha ao compilar a versão C para validar a frente.'
}
& $executavel both $Populacao $Geracoes --seed $Semente --log quiet --individuals --output $saida
if ($LASTEXITCODE -ne 0) {
    throw 'Falha na execução usada para validar a frente.'
}

foreach ($cenario in @('case_A', 'case_B')) {
    $pasta = Join-Path $saida $cenario
    $individuos = Import-Csv (Join-Path $pasta 'individuals.csv')
    $populacaoFinal = @($individuos | Where-Object {
        $_.stage -eq 'populacao_final_geracao' -and [int]$_.generation -eq $Geracoes
    })
    if ($populacaoFinal.Count -ne $Populacao) {
        throw "${cenario}: população final tem $($populacaoFinal.Count) indivíduos; esperado $Populacao."
    }
    foreach ($individuo in $populacaoFinal) {
        Test-Individuo $individuo $pesos[$cenario] $tipos[$cenario]
    }

    $descendentes = @($individuos | Where-Object { $_.stage -eq 'descendente' })
    if ($descendentes.Count -ne $Populacao * $Geracoes) {
        throw "${cenario}: quantidade de descendentes registrada não corresponde a população × gerações."
    }
    foreach ($individuo in $descendentes) {
        if ([double]$individuo.violation -ge 1e-8) {
            throw "${cenario}: descendente inviável em geração $($individuo.generation)."
        }
        $destinos = @($individuo.facility_genes -split ';' | ForEach-Object { [int]$_ })
        $prioridades = @($individuo.priority_genes -split ';' | ForEach-Object { [int]$_ })
        if ($destinos.Count -ne 7 -or $prioridades.Count -ne 7) {
            throw "${cenario}: cromossomo descendente com tamanho inesperado."
        }
        $vistos = [bool[]]::new(7)
        for ($i = 0; $i -lt 7; $i++) {
            if ($destinos[$i] -lt 0 -or $destinos[$i] -ge 6 -or
                $destinosPermitidosPorTipo[$tipos[$cenario][$i]] -notcontains $destinos[$i]) {
                throw "${cenario}: descendente com destinador inelegível."
            }
            if ($prioridades[$i] -lt 0 -or $prioridades[$i] -ge 7 -or $vistos[$prioridades[$i]]) {
                throw "${cenario}: prioridades do descendente não formam permutação."
            }
            $vistos[$prioridades[$i]] = $true
        }
    }

    $frente = @($populacaoFinal | Where-Object { [int]$_.rank -eq 0 })
    foreach ($candidato in $frente) {
        foreach ($outro in $populacaoFinal) {
            if ([object]::ReferenceEquals($candidato, $outro)) { continue }
            $naoPior = $true
            $estritamenteMelhor = $false
            foreach ($indice in 0..2) {
                $objetivos = @('cost', 'risk_transport', 'risk_facility')
                $a = [double]$candidato.($objetivos[$indice])
                $b = [double]$outro.($objetivos[$indice])
                if ($a -gt $b + 1e-9) { $naoPior = $false; break }
                if ($a + 1e-9 -lt $b) { $estritamenteMelhor = $true }
            }
            if ($naoPior -and $estritamenteMelhor) {
                throw "${cenario}: solução rank zero dominada na população final."
            }
        }
    }

    $arquivoPareto = Join-Path $pasta 'pareto.csv'
    $quantidadePareto = (Import-Csv $arquivoPareto).Count
    if ($Populacao -eq 100 -and $Geracoes -eq 200 -and $Semente -eq 20261102) {
        $esperado = if ($cenario -eq 'case_A') { 55 } else { 68 }
        if ($quantidadePareto -ne $esperado) {
            throw "${cenario}: exportou $quantidadePareto soluções; esperado $esperado."
        }
    }
    Write-Host "PASS: $cenario - $($descendentes.Count) descendentes factíveis; $($populacaoFinal.Count) indivíduos finais; $($frente.Count) rank zero não dominados; $quantidadePareto exportadas."
}

Write-Host 'PASS: factibilidade e não dominância verificadas sobre a população final.'
