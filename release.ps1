# Gakumas 图像替换系统 - 发布脚本 (PowerShell版本)
# 使用方法: .\release.ps1 v1.0.0

param(
    [Parameter(Mandatory=$true)]
    [string]$VersionTag
)

# 颜色函数
function Write-ColorText {
    param($Text, $Color = "White")
    Write-Host $Text -ForegroundColor $Color
}

Write-ColorText "===============================================" "Blue"
Write-ColorText "Gakumas Texture Replacement System - Release" "Blue"
Write-ColorText "===============================================" "Blue"

# 验证标签格式
if ($VersionTag -notmatch '^v\d+\.\d+\.\d+$') {
    Write-ColorText "❌ 版本标签格式错误！" "Red"
    Write-ColorText "💡 正确格式: v1.0.0 (v开头，语义化版本)" "Yellow"
    exit 1
}

Write-ColorText "🏷️  准备发布版本: $VersionTag" "Blue"

# 检查工作目录是否干净
$gitStatus = git status --porcelain
if ($gitStatus) {
    Write-ColorText "⚠️  工作目录有未提交的变更:" "Yellow"
    $gitStatus | ForEach-Object { Write-Host "  $_" }
    Write-Host
    
    $continue = Read-Host "是否继续？ (y/N)"
    if ($continue -ne 'y' -and $continue -ne 'Y') {
        Write-ColorText "❌ 发布取消" "Red"
        exit 1
    }
}

# 检查标签是否已存在
try {
    git rev-parse $VersionTag 2>$null | Out-Null
    Write-ColorText "❌ 标签 $VersionTag 已存在！" "Red"
    Write-ColorText "💡 请使用不同的版本号或删除现有标签" "Yellow"
    exit 1
} catch {
    # 标签不存在，继续
}

# 获取当前分支和提交信息
$currentBranch = git branch --show-current
$lastCommit = git log -1 --pretty=format:'%h - %s'

Write-ColorText "📍 当前分支: $currentBranch" "Blue"

# 确认发布
Write-Host
Write-ColorText "📋 发布信息:" "Yellow"
Write-Host "   🏷️  标签: $VersionTag"
Write-Host "   🌿 分支: $currentBranch"  
Write-Host "   📝 提交: $lastCommit"
Write-Host

$confirm = Read-Host "确认发布？ (y/N)"
if ($confirm -ne 'y' -and $confirm -ne 'Y') {
    Write-ColorText "❌ 发布取消" "Red"
    exit 1
}

Write-ColorText "🚀 开始发布流程..." "Green"

# 创建标签消息
$tagMessage = @"
Release $VersionTag

🎯 功能特性:
- ✅ 漫画图像替换 (img_general_comic_*)
- ✅ General Report图像替换 (img_general_report_*)  
- ✅ UI按钮图像替换 (img_general_ui_*_btn*)
- ✅ 教程图像替换 (img_tutorial_*)
- ✅ 与chinosk version.dll兼容运行

📦 构建说明:
- 基于XInput1_3.dll的安全DLL劫持
- 智能缓存和内存管理优化
- 只监控已配置资源，性能优化

🔧 系统要求:
- Windows 10/11 x64
- Visual C++ Redistributable 2022 x64
- 学园偶像大师 (Gakumas) 游戏
"@

# 创建标签
Write-ColorText "🏷️  创建标签..." "Blue"
git tag -a $VersionTag -m $tagMessage

if ($LASTEXITCODE -ne 0) {
    Write-ColorText "❌ 标签创建失败" "Red"
    exit 1
}

Write-ColorText "✅ 标签创建成功" "Green"

# 推送标签
Write-ColorText "⬆️  推送标签到远程仓库..." "Blue"
git push origin $VersionTag

if ($LASTEXITCODE -ne 0) {
    Write-ColorText "❌ 标签推送失败" "Red"
    exit 1
}

Write-ColorText "✅ 标签推送成功" "Green"

# 获取仓库URL
$remoteUrl = git config --get remote.origin.url
$repoPath = ""
if ($remoteUrl -match 'github\.com[:/](.+?)(?:\.git)?$') {
    $repoPath = $matches[1]
}

Write-Host
Write-ColorText "🎉 发布流程完成！" "Green"
Write-Host
Write-ColorText "📋 接下来会发生什么:" "Blue"
Write-ColorText "   1. GitHub Actions会自动触发构建" "Yellow"
Write-ColorText "   2. 编译 xinput1_3.dll" "Yellow"
Write-ColorText "   3. 打包发布文件" "Yellow"
Write-ColorText "   4. 创建GitHub Release" "Yellow"
Write-ColorText "   5. 上传构建产物" "Yellow"
Write-Host

if ($repoPath) {
    Write-ColorText "🔗 查看构建状态:" "Blue"
    Write-ColorText "   https://github.com/$repoPath/actions" "Cyan"
    Write-Host
    Write-ColorText "🔗 发布页面 (构建完成后):" "Blue"
    Write-ColorText "   https://github.com/$repoPath/releases/tag/$VersionTag" "Cyan"
}

Write-Host
Write-ColorText "💡 提示: 构建通常需要2-5分钟完成" "Gray"
