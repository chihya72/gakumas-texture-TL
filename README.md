# Gakumas 贴图翻译插件 (XInput版本)

一个基于DLL劫持技术的Unity IL2CPP贴图翻译插件，专为学园偶像大师 (Gakumas) 游戏设计，目前支持漫画、培育报告、UI按钮和教程图像的本地化替换。

## 🎯 项目特色

- ✅ **非侵入式替换**: 无需修改游戏文件，通过DLL劫持实现
- ✅ **多类型支持**: 漫画、General Report、UI按钮、教程图像
- ✅ **兼容性优秀**: 与chinosk的version.dll汉化补丁完美并行运行  
- ✅ **安全稳定**: 使用XInput1_3.dll，不影响关键系统功能
- ✅ **性能优化**: 智能缓存和内存管理，只监控已配置资源
- ✅ **实时日志**: 详细的替换日志，便于调试和监控
- 🔄 **持续更新**: 本仓库会定期更新翻译资源，Release版本包含最新翻译

## 🚀 快速开始

### 安装步骤

1. **下载Release版本**
   ```
   下载完整Release包，包含：
   - xinput1_3.dll (插件本体)
   - gakumas-local-texture/ (预置翻译资源)
   将全部文件解压到游戏根目录
   ```

2. **确认资源目录结构**
   ```
   gakumas.exe所在目录/
   ├── gakumas.exe
   ├── version.dll              # chinosk汉化补丁 (可选)
   ├── xinput1_3.dll            # 贴图翻译插件
   └── gakumas-local-texture/   # 资源目录 (Release中已包含)
       ├── asset_mapping.txt    # 配置文件 (已预配置)
       ├── comic/              # 漫画图像 (含作者翻译)
       ├── general_report/     # 培育报告图像 (含作者翻译)
       ├── ui/                 # UI按钮图像 (含作者翻译)
       └── tutorial/           # 教程图像 (含作者翻译)
   ```

3. **自定义图像映射 (可选)**
   
   **注意**: Release版本已包含作者翻译的图像资源和预配置文件，可直接使用。
   如需自定义或添加更多翻译，可编辑 `gakumas-local-texture/asset_mapping.txt`:
   ```ini
   # 漫画替换
   img_general_comic_0001=comic/1ko/my_comic_01.png
   img_general_comic_0002=comic/1ko/my_comic_02.png
   
   # General Report替换
   img_general_report_text_ttmr-001=general_report/text_ttmr_001_cn.png
   img_general_report_photo-name_ttmr-001=general_report/name_ttmr_001_cn.png
   
   # UI按钮替换 (基于实际检测到的资源名)
   img_general_ui_produce-1_btn-small=ui/produce_1_btn_small_cn.png
   img_general_ui_produce-nia_btn=ui/produce_nia_btn_cn.png
   
   # 教程图像替换
   img_tutorial_produce_01_first-001=tutorial/produce_tutorial_001_cn.png
   ```

4. **启动游戏**
   
   正常启动游戏，系统将自动：
   - 检测目标进程 (gakumas.exe)
   - 加载配置文件
   - 安装Hook系统
   - 开始图像替换

## 📁 项目结构

```
gakumas-texture-TL/
├── xinput1_3_with_general_report.cpp    # 主实现文件 (890行)
├── xinput1_3.vcxproj                     # Visual Studio项目
├── xinput1_3.def                         # XInput API导出定义
├── MinHook/                              # Hook库依赖
│   ├── include/MinHook.h
│   └── lib/libMinHook.x64.lib
├── gakumas-local-texture/                # 游戏资源目录 (持续更新中)
│   ├── asset_mapping.txt                 # 配置文件 (预配置作者翻译)
│   ├── comic/                           # 漫画图像 (含作者翻译)
│   │   ├── 1ko/                         # 一格漫画
│   │   └── 4ko/                         # 四格漫画
│   ├── general_report/                  # 培育报告图像 (含作者翻译)                                               
│   ├── ui/button/                       # UI按钮图像 (含作者翻译)
│   ├── tutorial/                        # 教程图像 (含作者翻译)
│   └── logs/                            # 自动生成日志
└── prompt_md/                           # 开发文档
    ├── Gakumas贴图翻译插件 - 完整项目记录.md
    ├── UI_Tutorial_扩展开发记录.md
    └── Chinosk_ida_functions_reference.md
```

## 🔧 技术原理

### 核心技术架构

1. **DLL劫持**: 使用XInput1_3.dll代理实现安全劫持
2. **IL2CPP Hook**: 基于MinHook库Hook Unity IL2CPP函数
3. **延迟替换**: LoadAssetAsync记录请求，GetResult执行替换
4. **智能缓存**: 纹理缓存和过期清理机制

### Hook目标函数

- `AssetBundle.LoadAssetAsync` - 检测资源加载请求
- `AssetBundleRequest.GetResult` - 执行实际替换
- `Resources.Load` - 备用资源加载路径

### 资源类型支持

| 类型 | 资源前缀 | 示例 | 状态 |
|------|----------|------|------|
| 漫画 | `img_general_comic` | `img_general_comic_0001` | ✅ 完整支持 |
| 培育报告 | `img_general_report` | `img_general_report_text_ttmr-001` | ✅ 完整支持 |
| UI按钮 | `img_general_ui` | `img_general_ui_produce-1_btn-small` | ✅ 完整支持 |
| 教程 | `img_tutorial` | `img_tutorial_produce_01_first-001` | ✅ 完整支持 |

## 📊 实际发现的资源

通过游戏运行时检测，发现的真实资源名称：

### UI按钮资源
```
img_general_ui_produce-1_btn-small
img_general_ui_produce-2_btn-small  
img_general_ui_produce-3_btn-small
img_general_ui_produce-nia_btn
img_general_ui_produce-nia-2_btn
```

### Tutorial资源
```
img_tutorial_produce_01_first-001
img_tutorial_produce_01_first-002
```

### General Report资源类型
- **文本图像**: `img_general_report_text_*`
- **照片名称**: `img_general_report_photo-name_*`  
- **背景图表**: `img_general_report_bg_*`
- **角色头像**: `img_general_report_akapen_*`

## 📋 使用说明

### 配置文件格式

**注意**: Release版本已包含预配置的 `asset_mapping.txt` 文件，包含作者翻译的资源映射。

如需自定义或添加更多翻译资源，`asset_mapping.txt` 使用简单的键值对格式：
```ini
# 注释以 # 开头
资源ID=本地文件路径

# 支持的格式示例
img_general_comic_0001=comic/1ko/my_comic_01.png
img_general_comic_0002=comic/1ko/my_comic_02.jpg
```

### 支持的图像格式
- PNG (推荐)
- JPG/JPEG
- 其他Unity支持的格式

### 日志系统
- 自动生成带时间戳的日志文件
- 位置: `gakumas-local-texture/logs/`
- 格式: `gakumas_texture_replace_YYYYMMDD_HHMMSS.log`

### 性能优化设置
```cpp
// 缓存设置 (编译时常量)
MAX_CACHE_SIZE = 50          // 最大缓存纹理数量
CACHE_EXPIRE_TIME = 300000   // 5分钟过期时间
PENDING_EXPIRE_TIME = 60000  // 1分钟待处理过期时间
```

## 🔍 故障排除

### 常见问题

1. **系统未启动**
   ```
   检查日志: 是否检测到 "Target process detected: gakumas.exe"
   确认进程名: 只在gakumas.exe中激活
   ```

2. **Hook安装失败**
   ```
   检查日志: "IL2CPP API initialization: SUCCESS"
   确认游戏版本: IL2CPP结构可能变化
   ```

3. **图像不替换**
   ```
   检查配置: asset_mapping.txt 资源ID是否正确
   检查文件: 本地图像文件是否存在
   检查日志: 是否有 "REPLACEMENT MATCHED" 消息
   ```

### 调试模式

系统自动创建控制台窗口显示实时日志，标题为：
```
Gakumas - Comic + General Report + UI + Tutorial Replacement System (XInput Optimized)
```

### 安全性

- 只在目标进程中激活，拒绝Unity辅助进程
- 不影响关键系统API (使用XInput而非WinHTTP)
- 完整的异常处理，避免游戏崩溃
- 智能内存管理，防止内存泄漏

## 🤝 兼容性

### 与其他Mod的兼容性
- ✅ **chinosk version.dll**: 完美并行运行
- ✅ **其他DLL Mod**: 使用不同DLL名称，无冲突
- ❓ **其他Hook系统**: 需要具体测试

### 游戏版本兼容性
- 开发基于: Unity IL2CPP架构
- 理论兼容: 所有使用相同引擎架构的版本
- 注意事项: 重大游戏更新可能需要重新适配

## 📈 开发历程

### 技术演进路径
1. **Version.dll版本** → **XInput版本** (解决网络问题)
2. **Comic支持** → **General Report扩展** → **UI+Tutorial全功能**
3. **性能问题** → **优化监控策略** → **智能内存管理**

### 关键技术突破
- IL2CPP方法地址解析 (+0偏移方案)
- 延迟替换策略 (避免直接返回崩溃)
- Unity纹理创建 (IL2CPP数组内存布局)
- 双DLL兼容设计

## 🔮 未来计划

### 资源更新计划
- 🔄 **持续翻译**: 定期更新gakumas-local-texture中的翻译资源
- 📦 **Release更新**: 每次资源更新后发布新的Release版本
- 🌐 **社区贡献**: 欢迎社区贡献更多高质量翻译资源

### 短期目标
- [ ] 更多UI元素支持 (非按钮类型)
- [ ] 音频资源替换系统
- [ ] 配置热重载功能

### 长期目标  
- [ ] 可视化配置工具
- [ ] 资源包管理系统
- [ ] 自动化资源发现工具

## 📚 相关资源

### 技术参考
- [chinosk6/gakuen-imas-localify](https://github.com/chinosk6/gakuen-imas-localify) - 原始汉化补丁
- [TsudaKageyu/minhook](https://github.com/TsudaKageyu/minhook) - Hook库
- [Unity IL2CPP Documentation](https://docs.unity3d.com/Manual/IL2CPP.html)

### 开发文档
- `prompt_md/` 目录包含完整的开发记录
- IDA Pro分析结果和技术细节
- 详细的问题解决方案记录

## ⚖️ 许可证

本项目仅用于学习和研究目的。请遵守游戏服务条款和相关法律法规。

## 🙏 致谢

- **chinosk**: 提供了宝贵的技术参考和汉化补丁
- **TsudaKageyu**: MinHook库作者
- **Unity Technologies**: IL2CPP技术文档

---

**项目状态**: ✅ 稳定运行  
**最后更新**: 2025年6月14日  
**版本**: XInput优化版 v1.2
