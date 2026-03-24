# AI Personal Access Token (PAT) 配置指南

本文档说明如何为 AI 助手创建和配置 GitHub Personal Access Token，使 AI 能够自动提交代码和创建 Pull Request。

## 为什么需要 PAT

AI 助手需要 PAT 来执行以下操作：
- 提交代码变更
- 创建 Pull Request
- 推送分支到远程仓库
- 触发 GitHub Actions

## 创建 PAT

### 步骤 1：进入 Token 设置

1. 登录 GitHub
2. 点击右上角头像，选择 **Settings**
3. 滚动到页面底部，点击 **Developer settings**
4. 在左侧菜单，点击 **Personal access tokens** → **Tokens (classic)**
5. 点击右上角 **Generate new token** → **Generate new token (classic)**

### 步骤 2：配置 Token 信息

- **Note**: 输入易识别的描述，例如 `AI Assistant - codelint`
- **Expiration**: 建议设置为 30 天或 90 天（定期轮换更安全）

### 步骤 3：选择权限范围

勾选以下权限：

- ✅ **repo**（完整控制）
  - 包括 `public_repo`、`repo:status`、`repo_deployment`、`read:org`
  - 允许读写私有仓库

- ✅ **workflow**
  - 更新 GitHub Actions 工作流文件
  - 允许 AI 修改 `.github/workflows/` 目录

- ✅ **actions:write**（在 workflow 或 repo 下）
  - 触发 GitHub Actions
  - 允许 AI 运行 CI/CD

**不要勾选 admin 权限**，保留人工审核控制权。

### 步骤 4：生成 Token

1. 点击页面底部的 **Generate token**
2. **立即复制 Token**（格式：`ghp_xxxxxxxxxxxx`）
3. 将 Token 保存到安全位置（一旦离开页面就无法再查看）

## 添加到仓库 Secrets

### 步骤 1：进入 Secrets 设置

1. 打开仓库主页
2. 点击 **Settings** 标签
3. 在左侧菜单，展开 **Secrets and variables**
4. 点击 **Actions**

### 步骤 2：创建 Repository Secret

1. 点击右上角 **New repository secret**
2. 填写以下信息：
   - **Name**: `AI_PAT`
   - **Secret**: 粘贴刚才复制的 Token 值
3. 点击 **Add secret**

### 步骤 3：验证配置

确认 Secret 已添加到列表：
```
Name: AI_PAT
Last updated: [日期]
```

## 在 GitHub Actions 中使用

在工作流文件中使用 `secrets.AI_PAT`：

```yaml
jobs:
  ai-commit:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          token: ${{ secrets.AI_PAT }}
```

## 安全建议

### Token 有效期

- **设置过期时间**: 不要创建永久 Token
- **定期轮换**: 每 30-90 天更换一次
- **最小权限**: 只授予必要权限，不使用 admin

### 访问控制

- **专用账户**: 考虑为 AI 创建专用 GitHub 账户
- **受限范围**: 只授予目标仓库的访问权限
- **审计日志**: 定期检查 Token 使用记录

### 紧急处理

如果 Token 泄露：
1. 立即在 **Developer settings** 撤销 Token
2. 重新生成新 Token
3. 更新仓库 Secrets

## AI Commit 标记

为防止 AI commit 触发 CI 循环，在 commit message 中包含 skip 标记：

### 可用标记

以下任一标记会跳过 CI 触发：

- `[ci skip]`
- `[skip ci]`
- `[bot skip]`
- `[no ci]`

### 示例

```bash
git commit -m "feat: add new functionality [ci skip]"
git commit -m "fix: resolve bug [bot skip]"
```

### GitHub Actions 配置

在工作流中添加条件判断：

```yaml
on:
  push:
    paths-ignore:
      - '**.md'
    # 或使用 commit message 过滤
```

## 故障排除

### 问题：Permission denied

**原因**: Token 权限不足

**解决方案**:
1. 检查是否勾选了 `repo`、`workflow`、`actions` 权限
2. 确认 Secret 名称正确（`AI_PAT`）
3. 验证 token 未过期

### 问题：Token 过期

**解决方案**:
1. 撤销旧 Token
2. 重新生成新 Token
3. 更新仓库 Secrets

### 问题：CI 循环触发

**原因**: AI commit 又触发了 CI，CI 又触发 AI commit

**解决方案**:
1. 在 commit message 添加 `[ci skip]`
2. 在工作流中排除 AI 账户的提交
3. 使用条件判断避免循环

## 相关文档

- [GitHub PAT 官方文档](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens)
- [GitHub Secrets 使用指南](https://docs.github.com/en/actions/security-guides/encrypted-secrets)
