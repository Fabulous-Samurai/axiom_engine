# Fix EngineResult return statement pattern in all source files
$files = @(
    "src/unit_manager.cpp",
    "src/statistics_engine.cpp",
    "src/symbolic_engine.cpp",
    "src/linear_system_parser.cpp",
    "src/dynamic_calc.cpp"
)

foreach ($file in $files) {
    $fullPath = "c:\Users\fabulous_samurai\OneDrive\Documents\GitHub\axiom_engine\$file"
    if (Test-Path $fullPath) {
        Write-Host "Processing $file..." -ForegroundColor Cyan
        $content = Get-Content $fullPath -Raw
        
        # Replace {EngineSuccessResult(...), {}} with just EngineSuccessResult(...)
        $content = $content -replace 'return \{EngineSuccessResult\(([^}]+)\), \{\}\};', 'return EngineSuccessResult($1);'
        
        # Replace {{}, EngineErrorResult(...)} with error result
        $content = $content -replace 'return \{\{\}, EngineErrorResult\(([^}]+)\)\};', 'EngineResult err_result; err_result.error = EngineErrorResult($1); return err_result;'
        $content = $content -replace 'return \{\{\}, \{EngineErrorResult\(([^}]+)\)\}\};', 'EngineResult err_result; err_result.error = EngineErrorResult($1); return err_result;'
        
        Set-Content $fullPath $content -NoNewline
        Write-Host "  OK Fixed $file" -ForegroundColor Green
    }
}

Write-Host "All files processed" -ForegroundColor Green
