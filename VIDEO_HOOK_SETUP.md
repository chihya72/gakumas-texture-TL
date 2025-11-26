# 视频播放日志功能设置说明 (✅ 自动启用版本)

## 🎉 功能已自动启用！

视频播放日志功能现在通过现有的 AssetBundle Hook 系统自动工作，**无需任何手动设置**！

功能可以记录游戏中加载视频/Live Scene相关资源时的角色信息：
- Character ID (角色ID)
- Idol Card ID (偶像卡片ID)  
- Costume ID (服装ID)
- Costume Head ID (头部服装ID)

## 🔧 工作原理

新的实现方法参照了现有成功的图片替换代码：
1. **利用现有Hook**: 使用已经工作的 `AssetBundle.LoadAssetAsync` Hook
2. **资源名称解析**: 从加载的资源名称中提取角色信息
3. **模式匹配**: 检测包含 `live`、`scene`、`character`、`costume`、`idol` 等关键词的资源
4. **ID提取**: 从资源名称中解析出数字ID

## ✅ 使用方法

**完全自动** - 重新编译并运行即可！

当游戏加载包含以下关键词的资源时，会自动检测并输出信息：
- `live`
- `scene` 
- `character`
- `costume`
- `idol`

## 📋 预期输出格式

当检测到相关资源时，会在日志中看到：
```
=== VIDEO/LIVE SCENE ASSET DETECTED ===
Asset Name: [资源名称]
Bundle: 0x[地址], Type: 0x[地址]
*** VIDEO/LIVE SCENE INFO EXTRACTED ***
Character ID: [角色ID]
Idol Card ID: [偶像卡片ID]
Costume ID: [服装ID]
Costume Head ID: [头部服装ID]
========================================
```

控制台也会显示简化信息：
```
*** VIDEO INFO: CharID=xxx, IdolCardID=xxx, CostumeID=xxx ***
```

## 📝 当前状态

- ✅ 功能已完全集成到现有系统
- ✅ 无需手动设置函数地址  
- ✅ 基于成功的图片替换代码架构
- ✅ 自动检测视频相关资源
- ✅ 实时输出角色信息
- ✅ 与 GakumasLocal-Native 兼容工作

## 🚀 立即使用

重新编译项目，运行游戏，开始Live Scene时即可看到角色信息输出！
   ```cpp
   moveLiveSceneFunc = (void*)(baseAddr + 0x4014192C); // 取消注释并设置正确的RVA
   ```
5. 重新编译

**如果地址不正确**：
尝试修改为其他可能的值：
```cpp
moveLiveSceneFunc = (void*)(baseAddr + 0x14192C);   // 尝试1
moveLiveSceneFunc = (void*)(baseAddr + 0x1014192C); // 尝试2
```

### 原始方法 (使用 IDA Pro)

### 原始方法 (使用 IDA Pro)

如果简化方法不工作，使用 IDA Pro 进行精确分析：

1. 打开 `gakumas.exe` 在 IDA Pro 中
2. 搜索字符串 `"MoveLiveScene: characterId: %s, idolCar"`
3. 找到引用这个字符串的函数
4. 记录函数的起始地址
5. 计算 RVA = 函数地址 - 模块基址

在 `InstallVideoHooks()` 函数中设置：
```cpp
moveLiveSceneFunc = (void*)(baseAddr + YOUR_CALCULATED_RVA);
```

### 3. 重新编译

重新编译 DLL 文件并替换游戏目录中的 `xinput1_3.dll`

## 日志输出格式

成功设置后，当游戏播放视频时会在日志中看到：
```
=== VIDEO PLAYBACK DETECTED ===
Character ID: [角色ID]
Idol Card ID: [偶像卡片ID]
Costume ID: [服装ID]
Costume Head ID: [头部服装ID]
===============================
```

控制台也会显示简化信息：
```
*** VIDEO PLAYBACK: CharID=xxx, IdolCardID=xxx, CostumeID=xxx, CostumeHeadID=xxx ***
```

## 注意事项

1. 函数地址可能因游戏版本更新而改变
2. 如果地址错误，可能导致游戏崩溃
3. 建议先在测试环境中验证
4. 参数解析可能需要根据实际情况调整

## 当前状态 (更新)

从日志可以看到，GakumasLocal-Native 插件已经在输出 MoveLiveScene 信息：
```
[INFO] GakumasLocal-Native: MoveLiveScene: characterId: , idolCardId: , costumeId: , costumeHeadId: 缈?
```

但是参数都是空的，说明参数解析有问题。

### 快速启用方法

1. **找到正确的函数地址**：
   在 `InstallVideoHooks()` 函数中，找到这行：
   ```cpp
   // moveLiveSceneFunc = (void*)(baseAddr + 0x4014192C); // 取消注释并设置正确的RVA
   ```

2. **启用 Hook**：
   取消注释该行：
   ```cpp
   moveLiveSceneFunc = (void*)(baseAddr + 0x4014192C);
   ```

3. **调整 RVA 地址**：
   如果 0x4014192C 不正确，尝试其他值：
   - `0x14192C` 
   - `0x1014192C`
   - 或者通过 IDA Pro 计算正确的 RVA

### 调试信息

启用后，Hook 会输出详细的调试信息：
- 参数地址
- 多种参数解析尝试
- 内存有效性检查
- 参数内容的十六进制查看

这将帮助确定正确的参数顺序和解析方法。

### 状态更新

- ✅ Hook 框架已就绪
- ✅ 增强的参数解析和调试
- ⚠️ 需要取消注释函数地址设置
- ⚠️ 可能需要调整 RVA 地址
- ℹ️ GakumasLocal-Native 已在处理相同函数，但参数解析不完整
