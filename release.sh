#!/bin/bash
# Gakumas 贴图翻译插件 - 发布脚本
# 使用方法: ./release.sh v1.0.0

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}===============================================${NC}"
echo -e "${BLUE}Gakumas Texture Replacement System - Release${NC}"
echo -e "${BLUE}===============================================${NC}"

# 检查参数
if [ $# -eq 0 ]; then
    echo -e "${RED}❌ 请提供版本标签！${NC}"
    echo -e "${YELLOW}💡 使用方法: $0 v1.0.0${NC}"
    exit 1
fi

VERSION_TAG=$1

# 验证标签格式
if [[ ! $VERSION_TAG =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo -e "${RED}❌ 版本标签格式错误！${NC}"
    echo -e "${YELLOW}💡 正确格式: v1.0.0 (v开头，语义化版本)${NC}"
    exit 1
fi

echo -e "${BLUE}🏷️  准备发布版本: ${VERSION_TAG}${NC}"

# 检查工作目录是否干净
if [ -n "$(git status --porcelain)" ]; then
    echo -e "${YELLOW}⚠️  工作目录有未提交的变更:${NC}"
    git status --porcelain
    echo
    read -p "是否继续？ (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${RED}❌ 发布取消${NC}"
        exit 1
    fi
fi

# 检查标签是否已存在
if git rev-parse "$VERSION_TAG" >/dev/null 2>&1; then
    echo -e "${RED}❌ 标签 $VERSION_TAG 已存在！${NC}"
    echo -e "${YELLOW}💡 请使用不同的版本号或删除现有标签${NC}"
    exit 1
fi

# 获取当前分支
CURRENT_BRANCH=$(git branch --show-current)
echo -e "${BLUE}📍 当前分支: ${CURRENT_BRANCH}${NC}"

# 确认发布
echo
echo -e "${YELLOW}📋 发布信息:${NC}"
echo -e "   🏷️  标签: ${VERSION_TAG}"
echo -e "   🌿 分支: ${CURRENT_BRANCH}"
echo -e "   📝 提交: $(git log -1 --pretty=format:'%h - %s')"
echo

read -p "确认发布？ (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${RED}❌ 发布取消${NC}"
    exit 1
fi

echo -e "${GREEN}🚀 开始发布流程...${NC}"

# 创建标签
echo -e "${BLUE}🏷️  创建标签...${NC}"
git tag -a "$VERSION_TAG" -m "Release $VERSION_TAG

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
- 学园偶像大师 (Gakumas) 游戏"

echo -e "${GREEN}✅ 标签创建成功${NC}"

# 推送标签
echo -e "${BLUE}⬆️  推送标签到远程仓库...${NC}"
git push origin "$VERSION_TAG"

echo -e "${GREEN}✅ 标签推送成功${NC}"

echo
echo -e "${GREEN}🎉 发布流程完成！${NC}"
echo
echo -e "${BLUE}📋 接下来会发生什么:${NC}"
echo -e "   1. ${YELLOW}GitHub Actions会自动触发构建${NC}"
echo -e "   2. ${YELLOW}编译 xinput1_3.dll${NC}"
echo -e "   3. ${YELLOW}打包发布文件${NC}"
echo -e "   4. ${YELLOW}创建GitHub Release${NC}"
echo -e "   5. ${YELLOW}上传构建产物${NC}"
echo
echo -e "${BLUE}🔗 查看构建状态:${NC}"
echo -e "   https://github.com/$(git config --get remote.origin.url | sed 's/.*:\([^.]*\).*/\1/')/actions"
echo
echo -e "${BLUE}🔗 发布页面 (构建完成后):${NC}"
echo -e "   https://github.com/$(git config --get remote.origin.url | sed 's/.*:\([^.]*\).*/\1/')/releases/tag/$VERSION_TAG"
echo
