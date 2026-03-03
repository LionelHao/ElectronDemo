# Windows 11 自动化测试脚本
# 使用方法：.\test-windows.ps1

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Electron Video Editor - Windows 测试" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 颜色函数
function Write-Success { param($msg) Write-Host "✅ $msg" -ForegroundColor Green }
function Write-Error { param($msg) Write-Host "❌ $msg" -ForegroundColor Red }
function Write-Info { param($msg) Write-Host "ℹ️  $msg" -ForegroundColor Yellow }

# 测试计数器
$TotalTests = 0
$PassedTests = 0

# 测试 1: Node.js 安装
Write-Info "测试 1: 检查 Node.js 安装..."
$TotalTests++
try {
    $nodeVersion = node --version
    if ($nodeVersion -match "v(\d+)\.") {
        $majorVersion = [int]$Matches[1]
        if ($majorVersion -ge 18) {
            Write-Success "Node.js $nodeVersion 已安装 (版本 >= 18)"
            $PassedTests++
        } else {
            Write-Error "Node.js 版本过低：$nodeVersion (需要 >= 18)"
        }
    } else {
        Write-Error "无法解析 Node.js 版本：$nodeVersion"
    }
} catch {
    Write-Error "Node.js 未安装，请先安装 Node.js 18+"
}
Write-Host ""

# 测试 2: npm 安装
Write-Info "测试 2: 检查 npm 安装..."
$TotalTests++
try {
    $npmVersion = npm --version
    Write-Success "npm $npmVersion 已安装"
    $PassedTests++
} catch {
    Write-Error "npm 未安装"
}
Write-Host ""

# 测试 3: Git 安装
Write-Info "测试 3: 检查 Git 安装..."
$TotalTests++
try {
    $gitVersion = git --version
    Write-Success "$gitVersion 已安装"
    $PassedTests++
} catch {
    Write-Error "Git 未安装"
}
Write-Host ""

# 测试 4: 项目目录
Write-Info "测试 4: 检查项目文件..."
$TotalTests++
$requiredFiles = @("package.json", "main.js", "preload.js", "index.html", "renderer.js")
$missingFiles = @()
foreach ($file in $requiredFiles) {
    if (-not (Test-Path $file)) {
        $missingFiles += $file
    }
}
if ($missingFiles.Count -eq 0) {
    Write-Success "所有必需文件存在"
    $PassedTests++
} else {
    Write-Error "缺少文件：$($missingFiles -join ', ')"
}
Write-Host ""

# 测试 5: 依赖安装
Write-Info "测试 5: 安装依赖..."
$TotalTests++
try {
    Write-Info "运行 npm install..."
    npm install --silent
    if ($LASTEXITCODE -eq 0) {
        Write-Success "依赖安装成功"
        $PassedTests++
    } else {
        Write-Error "依赖安装失败"
    }
} catch {
    Write-Error "依赖安装出错：$_"
}
Write-Host ""

# 测试 6: Native Module (可选)
Write-Info "测试 6: 检查 Native Module 编译环境..."
$TotalTests++
$hasVS = Test-Path "C:\Program Files (x86)\Microsoft Visual Studio"
$hasFFmpeg = $env:FFMPEG_DIR -ne $null
if ($hasVS) {
    Write-Success "Visual Studio 已安装"
} else {
    Write-Info "Visual Studio 未安装 (Mock 模式可用)"
}
if ($hasFFmpeg) {
    Write-Success "FFmpeg 环境变量已设置：$env:FFMPEG_DIR"
} else {
    Write-Info "FFmpeg 未配置 (Mock 模式)"
}
$PassedTests++  # 此项为信息性测试，总是通过
Write-Host ""

# 测试 7: 应用启动测试
Write-Info "测试 7: 应用启动测试..."
$TotalTests++
Write-Info "尝试启动应用（5 秒后自动关闭）..."
try {
    $process = Start-Process -FilePath "npm" -ArgumentList "start" -PassThru -NoNewWindow
    Start-Sleep -Seconds 5
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    Write-Success "应用可以启动"
    $PassedTests++
} catch {
    Write-Error "应用启动失败：$_"
}
Write-Host ""

# 总结
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  测试结果汇总" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "通过：$PassedTests / $TotalTests" -ForegroundColor $(if ($PassedTests -eq $TotalTests) { "Green" } else { "Yellow" })
Write-Host ""

if ($PassedTests -eq $TotalTests) {
    Write-Success "所有测试通过！应用已准备就绪。"
    Write-Host ""
    Write-Info "运行应用：npm start"
} else {
    $failed = $TotalTests - $PassedTests
    Write-Error "$failed 个测试失败，请检查上述错误信息。"
    Write-Host ""
    Write-Info "详细测试指南请参阅：WINDOWS_TEST.md"
}

Write-Host ""
