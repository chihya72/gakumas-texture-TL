# 🎯 视频角色信息输出功能 (自动启用)

## ✅ 已完成集成

视频角色信息输出功能已经**自动集成**到现有的纹理替换系统中！

## 🔧 工作原理

- 利用现有的 `AssetBundle.LoadAssetAsync` Hook
- 检测包含视频相关关键词的资源加载
- 从资源名称中自动解析角色ID信息
- 无需额外的函数地址设置

## 📋 检测关键词

系统会自动检测包含以下关键词的资源：
- `live`
- `scene`
- `character`
- `costume`
- `idol`

## 📤 输出格式

日志文件中：
```
=== VIDEO/LIVE SCENE ASSET DETECTED ===
Asset Name: [资源名称]
*** VIDEO/LIVE SCENE INFO EXTRACTED ***
Character ID: [角色ID]
Idol Card ID: [偶像卡片ID]
Costume ID: [服装ID]
========================================
```

控制台中：
```
*** VIDEO INFO: CharID=xxx, IdolCardID=xxx, CostumeID=xxx ***
```

## 🚀 使用方法

1. 重新编译项目
2. 运行游戏
3. 开始 Live Scene
4. 查看日志输出

## 📊 状态

- ✅ 功能已集成
- ✅ 无需手动设置
- ✅ 基于成功的Hook架构
- ✅ 与现有系统兼容
