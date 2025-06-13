# Chinosk IDA Functions Reference

## 📋 总览
本文档记录了从chinosk的Gakumas Localify项目IDA Pro分析中发现的关键函数及其作用，避免重复分析。

**分析日期**: 2025年1月  
**目标**: Unity IL2CPP Hook系统实现  
**来源**: chinosk的version.dll DLL劫持项目  

---

## 🔧 核心IL2CPP方法解析函数

### `sub_18013D790` ⭐ **最重要**
```cpp
__int64 __fastcall sub_18013D790(int a1, int a2, int a3, int a4, __int64 a5)
```
**作用**: chinosk的核心IL2CPP方法解析函数  
**功能**: 
- 调用 `sub_18013D634` 查找方法
- 返回 `*(result + 64)` 获取native代码地址
- 这是chinosk获取可Hook地址的关键函数

**关键逻辑**:
```cpp
result = sub_18013D634(a1, a2, a3, a4, a5);
if (result)
    return *(_QWORD *)(result + 64);  // +64偏移是关键！
return result;
```

### `sub_18013D634`
```cpp
__int64 __fastcall sub_18013D634(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5)
```
**作用**: IL2CPP方法查找的核心实现  
**功能**:
- 调用 `sub_18013A9C8` 获取程序集
- 调用 `sub_18013A854` 查找类
- 调用 `sub_18012E00C` 查找方法
- 包含错误处理逻辑

**错误消息**:
- `"GetMethod error: assembly %s not found."`
- `"GetMethod error: Class %s::%s not found."`  
- `"GetMethod error: method %s::%s.%s not found."`

---

## 🔍 IL2CPP查找子系统

### `sub_18013A9C8` - 程序集查找
```cpp
__int64 __fastcall sub_18013A9C8(const void **a1)
```
**作用**: 在已加载的程序集中查找指定程序集  
**功能**: 遍历全局程序集列表，通过名称匹配查找目标程序集

### `sub_18013A854` - 类查找  
```cpp
__int64 __fastcall sub_18013A854(__int64 a1, _QWORD *a2, _QWORD *a3, __int64 a4)
```
**作用**: 在程序集中查找指定的类  
**功能**: 
- 支持通配符 `*` (ASCII 42)
- 在程序集的类列表中搜索
- 支持命名空间和类名匹配

### `sub_18012E00C` - 方法查找
```cpp
__int64 __fastcall sub_18012E00C(__int64 a1, const void **a2, __int64 *a3)
```
**作用**: 在类中查找指定的方法  
**功能**:
- 根据方法名和参数类型匹配
- 支持方法重载识别
- 支持通配符参数匹配

---

## 🎯 Hook安装主函数

### `sub_18014301C` ⭐ **Hook安装核心**
```cpp
__int64 sub_18014301C()
```
**作用**: chinosk的主要Hook安装函数  
**特点**: 
- **5620行** 超大函数，包含所有Hook逻辑
- 多次调用 `sub_18013D790` 获取函数地址
- 包含完整的Hook安装和错误处理

**关键调用模式**:
```cpp
// 1. 构建方法查找参数
sub_180134AAC(v790, "LoadFromCacheOrDownload");
sub_180134AAC(v789, "OctoResourceLoader");  
sub_180134AAC(v788, "Octo.Loader");
sub_180134AAC(v787, "Octo.dll");

// 2. 获取函数地址
qword_1805618A0 = sub_18013D790(
    (unsigned int)v787,  // "Octo.dll"
    (unsigned int)v788,  // "Octo.Loader"
    (unsigned int)v789,  // "OctoResourceLoader"  
    (unsigned int)v790,  // "LoadFromCacheOrDownload"
    (__int64)v291);      // 参数类型数组

// 3. 安装Hook
v51 = *(__int64 (__fastcall **)(_QWORD, _QWORD, _QWORD, _QWORD))(*(_QWORD *)v1 + 8LL);
v53 = v51(v1, v52, sub_1801412C0, &qword_1805618A8);
```

---

## 🪝 Hook相关函数

### `sub_1801412C0` - Hook函数
**作用**: chinosk的实际Hook函数实现  
**功能**: 拦截 `OctoResourceLoader.LoadFromCacheOrDownload` 调用

### Hook管理器函数

#### `sub_1801A0574` - Hook系统初始化
**作用**: 初始化Hook系统  
**调用位置**: `v0 = sub_1801A0574();`

#### `sub_1801A0570` - Hook管理器创建
**作用**: 创建Hook管理器实例  
**调用位置**: `v1 = sub_1801A0570(v0);`

---

## 🛠 辅助工具函数

### 字符串处理函数

#### `sub_180134AAC` - 字符串构建
**作用**: 构建用于方法查找的字符串参数  
**使用**: 所有方法名、类名、程序集名的构建

#### `sub_180134BDC` - 参数类型处理
**作用**: 处理方法参数类型信息  
**使用**: 构建参数类型数组用于精确方法匹配

#### `sub_180134BE8` - 参数处理
**作用**: 处理方法查找的参数信息

### 内存管理函数

#### `sub_1801351B4` - 资源清理
**作用**: 清理字符串和内存资源  
**使用**: 在所有字符串使用后进行清理

#### `sub_180156BE4` - 内存清理
**作用**: 清理内存和资源  
**使用**: 清理参数数组等复杂结构

### 日志函数

#### `sub_1801848C4` - 错误日志
**作用**: 输出错误信息  
**消息格式**: `"ADD_HOOK: %s at %p failed: %s"`

#### `sub_180184A1C` - 成功日志  
**作用**: 输出成功信息
**消息格式**: `"ADD_HOOK: %s at %p"`

---

## 📊 函数调用关系图

```
sub_18014301C (主Hook安装)
├── sub_1801A0574 (初始化Hook系统)
├── sub_1801A0570 (创建Hook管理器)
├── sub_180134AAC (构建字符串) ×多次
├── sub_180134BDC (参数类型处理)
├── sub_180134BE8 (参数处理)
├── sub_18013D790 (获取函数地址) ×多次
│   └── sub_18013D634 (方法查找核心)
│       ├── sub_18013A9C8 (程序集查找)
│       ├── sub_18013A854 (类查找)
│       └── sub_18012E00C (方法查找)
├── Hook管理器.CreateHook (安装Hook)
├── sub_1801412C0 (Hook函数)
├── sub_1801351B4 (清理) ×多次
├── sub_180156BE4 (清理) ×多次
├── sub_1801848C4 (错误日志)
└── sub_180184A1C (成功日志)
```

---

## 🎯 关键发现总结

### 1. **核心技术**
- **+64偏移**: `*(methodInfo + 64)` 是获取native代码地址的关键
- **3参数版本**: chinosk实际Hook的是3参数版本的 `LoadFromCacheOrDownload`
- **默认调用约定**: 不是 `__fastcall`，使用默认约定

### 2. **Hook目标**
- **程序集**: `"Octo.dll"`
- **命名空间**: `"Octo.Loader"`  
- **类**: `"OctoResourceLoader"`
- **方法**: `"LoadFromCacheOrDownload"`

### 3. **实现要点**
- 多重尝试不同参数数量的方法重载
- 完整的错误处理和日志记录
- 资源管理和内存清理
- 使用自定义Hook库（不是MinHook）

---

## 💡 实际应用指导

### 复现chinosk方法的关键步骤:

1. **使用IL2CPP API查找方法**
2. **应用+64偏移获取native地址**  
3. **Hook 3参数版本的函数**
4. **使用默认调用约定**
5. **实现完整的错误处理**

### 常见问题解决:

- **Hook不被调用**: 检查参数数量（应该是3个）
- **地址无效**: 确保使用+64偏移方法
- **函数找不到**: 检查程序集、类、方法名称的精确匹配

---

**注意**: 这些函数名是IDA Pro的自动命名，实际的符号名可能不同。重要的是理解其功能和调用关系。 