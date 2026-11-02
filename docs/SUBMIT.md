# 如何被 TrafficMonitor 官方插件页收录

官方插件列表在 [zhongyang219/TrafficMonitorPlugins](https://github.com/zhongyang219/TrafficMonitorPlugins)
的 `download/plugin_download.md`（中文）和 `download/plugin_download_en.md`（英文）；
插件更新检查读同一仓库根目录的 `plugins_version.xml`。

维护者（zhongyang219）说明的提交方式有两种，**邮件**或**直接提 PR**：

> 如果你想提交你制作的插件，请发送电子邮件到 zhongyang219@hotmail.com，并附上下载链接，
> 邮件请注明"TrafficMonitor插件"。也可以直接向此仓库提交 pull request 来更新插件下载页面。

推荐 **PR**：一次改三处，可以在同一个 PR 里把中/英文条目和版本号都改好。

---

## 一、前置条件（本仓库已满足）

| 要求 | 我们的状态 |
|---|---|
| 有一个能直接下载 dll 的地址 | ✅ [Releases](https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin/releases/latest)（`.../releases/latest` 这种链接最稳） |
| 提供 x64 / x86 两个包 | ✅ `MouseJiggler_V1.0_x64.zip` / `MouseJiggler_V1.0_x86.zip`（官方插件也是按位数分包） |
| dll 文件名唯一、不与其他插件冲突 | ✅ `MouseJiggler.dll`，`plugins_version.xml` 里没有同名项 |
| 插件信息完整（插件管理里能显示名称/版本/作者） | ✅ `TMI_NAME=MouseJiggler`、`TMI_VERSION=1.0` |
| 三处版本号一致 | ✅ 插件 `TMI_VERSION=1.0`、release 标题 `V1.0`、xml `version="1.0"` |
| 一张列表用截图 | ✅ `docs/options-dialog.png` |
| 说明支持的 TrafficMonitor 版本 | TrafficMonitor 1.8.6（插件接口 v7）实测 |

> `plugins_version.xml` 里的 version 必须和插件 `TMI_VERSION` 完全一致，
> 否则 TrafficMonitor 的"插件管理"会误报有新版本（它用 `PluginVersion` 解析后比较）。

---

## 二、PR 要改的三处（可直接复制的片段）

### 1. `download/plugin_download.md` —— 在"插件下载"一节末尾追加

```markdown
### MouseJiggler

定时晃动鼠标的插件，让系统一直处于活动状态，避免锁屏、休眠或 IM 变成"离开"。支持禅动
（位移为 0，指针不动，只重置系统空闲计时）和普通（±4 像素微动）两种方式，间隔 1–60 秒可调，
并可设置只在工作时间生效（默认周一~周五 08:30–17:30）。

![](images/mousejiggler-options.png)

* 作者：[TiAmo-chen](https://github.com/TiAmo-chen)
* 主页：[TiAmo-chen/TrafficMonitor-MouseJigglerPlugin](https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin)
* 下载链接：https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin/releases/latest

---
```

### 2. `download/plugin_download_en.md` —— 追加对应英文条目

```markdown
### MouseJiggler

A plugin that periodically jiggles the mouse so the machine never looks idle (no screen
lock, no sleep, no "away" status in chat apps). Two modes: **zen** (zero-delta movement,
the pointer stays still) and **normal** (±4 px). Configurable 1–60 s interval, optional
work-hours schedule (Mon–Fri 08:30–17:30 by default). Requires TrafficMonitor 1.8.6+ (plugin
API v7).

![](images/mousejiggler-options.png)

* Author: [TiAmo-chen](https://github.com/TiAmo-chen)
* Homepage: [TiAmo-chen/TrafficMonitor-MouseJigglerPlugin](https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin)
* Download url: https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin/releases/latest

---
```

### 3. `plugins_version.xml` —— 加一行

```xml
    <plugin file_name="MouseJiggler.dll" version="1.0"/>
```

### 4. 图片

把 `docs/options-dialog.png` 复制成 `images/mousejiggler-options.png` 一起提交
（官方仓库的截图都放在 `images/` 下）。

---

## 三、邮件版（想用邮件提交时）

- 收件人：`zhongyang219@hotmail.com`
- 主题：`TrafficMonitor插件`（维护者要求的字样，别改）
- 正文：

```
钟洋你好，

提交一个自己写的 TrafficMonitor 插件：

名称：MouseJiggler
功能：定时晃动鼠标，让系统保持活动状态。禅动模式（指针不动，只重置空闲计时）/ 普通模式（±4 像素），
      间隔 1–60 秒可调，可只在工作时间生效（默认周一~周五 08:30–17:30）。
位数：x64 + x86 各一个包
源码/主页：https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin
下载链接：https://github.com/TiAmo-chen/TrafficMonitor-MouseJigglerPlugin/releases/latest
许可证：MIT
测试环境：TrafficMonitor 1.8.6 x64（Windows 11）
dll 文件名：MouseJiggler.dll（版本 1.0，与 plugins_version.xml 一致）

截图见附件。
```

---

## 四、常见被退回的原因（自查）

1. **位数不匹配**：只给了 x64，用户 32 位 TrafficMonitor 加载失败 → 我们两个都给了。
2. **下载链接指向仓库主页而不是 Release**：`/releases/latest` 才是稳定入口。
3. **版本号三处不一致**（dll 里的 `TMI_VERSION` / release / `plugins_version.xml`）。
4. **`GetInfo` 返回空指针**：会让 TrafficMonitor 崩溃，我们所有分支都返回非空字符串。
5. **插件在"插件管理"里显示加载失败**：多见于缺 VC 运行库 → 我们用 `/MT` 静态链接，无依赖。
6. **图片用外链且会失效**：官方仓库惯例是把截图放到 `images/` 里一起提交。
