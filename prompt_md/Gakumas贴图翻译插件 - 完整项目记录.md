# Gakumas贴图翻译插件 - 完整项目记录

## 📋 项目状态概览

**项目状态**: ✅ XInput版本已完成并可运行  
**当前版本**: XInput1_3.dll + General Report 支持  
**最后更新**: 2025年1月  
**核心文件**: `xinput1_3_with_general_report.cpp` (890行)

## 🎯 项目目标与实现

### 主要目标
通过DLL劫持技术实现Unity IL2CPP游戏"gakumas"的图像资源替换系统，同时与chinosk的version.dll汉化补丁兼容运行。

### 已实现功能
- ✅ **漫画图像替换**: img_general_comic_* 系列
- ✅ **General Report图像替换**: img_general_report_* 系列  
- ✅ **XInput DLL劫持**: 安全的xinput1_3.dll代理
- ✅ **双DLL兼容**: version.dll (chinosk) + xinput1_3.dll (自制)
- ✅ **多格式支持**: PNG、JPG、JPEG
- ✅ **纹理缓存**: 避免重复加载
- ✅ **进程过滤**: 只在gakumas.exe中激活

### 计划新增功能
- 🔄 **UI图像替换**: img_general_ui_produce-1_btn 等UI元素
- 🔄 **Tutorial图像替换**: img_tutorial_event_story_event-story-006-02 等教程图像
- 🔄 **更多资源类型**: 音频、字体、动画等

## 🏗️ 技术架构

### 核心技术栈
- **DLL劫持**: XInput1_3.dll代理技术
- **Hook框架**: MinHook库 (libMinHook.x64.lib)
- **目标引擎**: Unity IL2CPP
- **编译环境**: Visual Studio 2022 (MSVC)
- **目标架构**: x64

### 关键技术突破

#### 1. IL2CPP方法地址解析
```cpp
// 成功方法: +0偏移直接获取可执行地址
void* executableAddr = *((void**)((uintptr_t)method + 0));
// chinosk使用+64偏移，但在此游戏中+0偏移更可靠
```

#### 2. 安全的资源替换策略
```cpp
// 延迟替换机制避免游戏崩溃
std::map<void*, void*> g_pendingComicReplacements;

// 步骤1: LoadAssetAsync中记录请求
g_pendingComicReplacements[originalRequest] = customTexture;

// 步骤2: GetResult中执行替换
return g_pendingComicReplacements[request];
```

#### 3. Unity纹理创建
```cpp
// IL2CPP数组内存布局: [klass][monitor][bounds][max_length][data...]
unsigned char* arrayData = (unsigned char*)((uintptr_t)byteArray + sizeof(void*) * 4);
memcpy(arrayData, imageData.data(), imageData.size());

// ImageConversion.LoadImage加载实际图像数据
il2cpp_runtime_invoke(loadImageMethod, nullptr, params, &exception);
```

## 📁 当前文件结构

### 核心文件 (保留)
```
dump_dll_research/
├── xinput1_3_with_general_report.cpp    # 主实现文件 (890行)
├── xinput1_3.vcxproj                     # Visual Studio项目文件  
├── xinput1_3.def                         # XInput API导出定义
├── MinHook/                              # Hook库依赖
│   ├── include/MinHook.h
│   └── lib/libMinHook.x64.lib
└── [已清理不需要的文件]
```

### 游戏部署结构
```
gakumas.exe目录/
├── gakumas.exe
├── version.dll                    # chinosk汉化补丁
├── xinput1_3.dll                  # 自制贴图翻译插件
└── gakumas-local-texture/         # 统一资源目录
    ├── asset_mapping.txt          # 图像替换配置
    ├── logs/                      # 日志文件目录（自动生成带时间戳）
    ├── comic/                     # 漫画图像
    ├── general_report/            # 报告图像
    ├── ui/                        # UI图像
    └── tutorial/                  # 教程图像
```

## 🔄 实现历程

### 阶段1: 基础系统建立 (已完成)
- **version.dll实现**: 首个可工作版本 (~800行)
- **DLL劫持**: 成功实现DLL代理和进程过滤
- **Hook系统**: 基于MinHook的函数拦截框架
- **chinosk分析**: 深度分析IDA Pro函数和实现方法

### 阶段2: 网络安全问题解决 (已完成)
- **WinHTTP版本**: 尝试使用winhttp.dll，但导致网络错误
- **问题根因**: WinHTTP是关键网络API，简化代理导致连接失败
- **解决方案**: 弃用WinHTTP，转向XInput

### 阶段3: XInput版本成功实现 (已完成)
- **安全选择**: XInput只处理手柄输入，不影响关键系统功能
- **功能迁移**: 完整迁移所有漫画替换功能到XInput版本
- **双DLL兼容**: 成功实现与chinosk version.dll的并行运行
- **稳定性**: 经过验证的稳定运行系统

### 阶段4: General Report扩展 (已完成)
- **分析集成**: 添加General Report相关图像分析和替换
- **功能分类**: 区分文本图像、照片名称、背景图表等类型
- **统一Hook**: 在同一Hook系统中处理comic和general_report替换

## 🎯 Hook目标函数

### 成功Hook的Unity函数
1. **AssetBundle.LoadAssetAsync** 
   - 程序集: UnityEngine.AssetBundleModule.dll
   - 参数: 2个 (bundle, name, type)
   - 作用: 检测资源加载请求

2. **AssetBundleRequest.GetResult**
   - 程序集: UnityEngine.AssetBundleModule.dll  
   - 参数: 0个 (实例方法)
   - 作用: 执行实际资源替换

3. **Resources.Load**
   - 程序集: UnityEngine.CoreModule.dll
   - 参数: 2个 (path, type)
   - 作用: 备用资源加载路径

### 资源命名规律
```
# 漫画资源
img_general_comic_0001, img_general_comic_0002, ...

# 报告资源
img_general_report_text_ttmr-001          # 文本图像
img_general_report_photo-name_ttmr-001    # 照片名称
img_general_report_bg_*                   # 背景图表
img_general_report_akapen_*               # 角色头像
img_general_report_compare-graph_*        # 对比图表

# [计划] UI资源
img_general_ui_produce-1_btn              # 制作按钮
img_general_ui_*                          # 其他UI元素

# [计划] 教程资源  
img_tutorial_event_story_event-story-006-02   # 教程剧情
img_tutorial_*                            # 其他教程图像
```

## ⚙️ 编译与部署

### 编译命令 (Visual Studio 2022)
```powershell
# 在Visual Studio中打开xinput1_3.vcxproj
# 选择Release|x64配置
# 生成 -> 重新生成解决方案
# 输出: x64\Release\xinput1_3.dll
```

### 手动编译 (备用)
```powershell
cl /LD /EHsc /std:c++17 xinput1_3_with_general_report.cpp /Fe:xinput1_3.dll /link /DEF:xinput1_3.def kernel32.lib user32.lib psapi.lib MinHook/lib/libMinHook.x64.lib
```

### 配置文件格式
```ini
# gakumas-local-texture/asset_mapping.txt
# 漫画替换
img_general_comic_0001=comic/my_comic_01.png
img_general_comic_0002=comic/my_comic_02.jpg

# General Report替换
img_general_report_text_ttmr-001=general_report/text/ttmr-001_cn.png
img_general_report_photo-name_ttmr-001=general_report/names/ttmr-001_cn.png

# UI替换
img_general_ui_produce-1_btn=ui/produce_btn_cn.png

# Tutorial替换
img_tutorial_event_story_event-story-006-02=tutorial/story_006_02_cn.png
```

## 🔍 调试信息

### 成功运行日志
```
[ComicReplace-XInput+GeneralReport] Target process detected: gakumas.exe
[ComicReplace-XInput+GeneralReport] Original xinput1_3.dll loaded successfully
[ComicReplace-XInput+GeneralReport] Loaded 8 comic mappings (including general_report)
[ComicReplace-XInput+GeneralReport] IL2CPP API initialization: SUCCESS
[ComicReplace-XInput+GeneralReport] XINPUT Hooked AssetBundle.LoadAssetAsync at 0x...
[ComicReplace-XInput+GeneralReport] XINPUT Hooked AssetBundleRequest.GetResult at 0x...
[ComicReplace-XInput+GeneralReport] XINPUT Comic + General Report hooks installed: 2/3
[ComicReplace-XInput+GeneralReport] *** GENERAL_REPORT ASSET DETECTED! ***
[ComicReplace-XInput+GeneralReport] Type: TEXT IMAGE (需要翻译)
[ComicReplace-XInput+GeneralReport] *** GENERAL_REPORT REPLACEMENT MATCHED! ***
[ComicReplace-XInput+GeneralReport] *** SUCCESS: Image data loaded into texture! ***
```

### 常见问题解决
1. **UnityCrashHandler64.exe注入**: DllMain开始即检查进程名
2. **网络连接问题**: 使用XInput而非WinHTTP避免网络API冲突
3. **Hook失败**: 使用+0偏移获取可执行地址
4. **游戏崩溃**: 延迟替换策略，在GetResult中返回纹理

## 📈 下一步开发计划

### 高优先级 (UI和Tutorial支持)
1. **UI图像替换功能**
   - 分析UI资源命名规律
   - 添加UI专用的Hook和分析函数
   - 实现img_general_ui_*系列替换
   - 重点: img_general_ui_produce-1_btn等重要按钮

2. **Tutorial图像替换功能**  
   - 分析教程系统的资源加载机制
   - 实现img_tutorial_*系列替换
   - 重点: img_tutorial_event_story_event-story-006-02等剧情图像

### 中优先级 (功能增强)
1. **更多General Report类型**
   - 背景图表 (_bg_)
   - 角色头像 (_akapen_)  
   - 对比图表 (_compare-graph_)

2. **性能优化**
   - 纹理预加载
   - 异步图像处理
   - 内存使用优化

### 低优先级 (扩展功能)
1. **音频资源替换**
2. **字体资源替换**
3. **动画/特效替换**
4. **可视化配置工具**

## 🛠️ 关键技术参考

### chinosk实现分析
- **核心函数**: `sub_18013D790` (+64偏移方法)
- **Hook目标**: OctoResourceLoader.LoadFromCacheOrDownload
- **参数**: 3个参数版本
- **错误处理**: 完整的异常处理和日志系统

### 我们的实现差异
- **偏移方法**: +0偏移 (更适合此游戏)
- **Hook目标**: AssetBundle.LoadAssetAsync + GetResult
- **DLL选择**: XInput (更安全) vs Version (chinosk)
- **兼容性**: 双DLL并行运行

## 📚 相关资源

### 开源项目
- [chinosk/Gakumas-Localify](https://github.com/chinosk/Gakumas-Localify) - 原始参考实现
- [TsudaKageyu/minhook](https://github.com/TsudaKageyu/minhook) - Hook库

### 技术文档
- [Unity IL2CPP内部机制](https://docs.unity3d.com/Manual/IL2CPP.html)
- [Windows DLL搜索顺序](https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order)

### IDA Pro分析
- 详细的函数地址和调用关系记录在`Chinosk_ida_functions_reference.md`

## 🎉 项目成果

### 技术成就
- ✅ 成功实现Unity IL2CPP游戏的非侵入式资源替换
- ✅ 解决了与现有汉化补丁的兼容性问题
- ✅ 建立了稳定可靠的Hook系统
- ✅ 实现了多种图像格式的无损替换

### 实用价值
- ✅ 为gakumas游戏提供了完整的图像本地化解决方案
- ✅ 建立了可扩展的资源替换框架
- ✅ 为其他Unity游戏提供了技术参考

### 经验总结
- ✅ DLL选择的重要性: 避免关键系统API
- ✅ 延迟替换策略: 避免直接返回导致崩溃
- ✅ IL2CPP地址解析: 不同游戏可能需要不同偏移
- ✅ 兼容性设计: 多DLL并行的可行性

---

**重要提醒**: 本文档是项目的完整记录，新对话时加载此文档即可获得全部项目记忆和技术细节。

**最新状态**: XInput版本稳定运行，支持comic和general_report替换，等待UI和Tutorial功能扩展。 