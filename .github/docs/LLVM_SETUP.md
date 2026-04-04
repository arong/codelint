# LLVM 18 安装指南（Ubuntu 22.04）

## 问题说明

Ubuntu 22.04 默认的 apt 仓库只包含 LLVM 14。但本项目需要 LLVM 18 才能正常构建和运行。

如果直接使用默认 apt 安装：

```bash
# ❌ 错误方式：会安装 LLVM 14（版本过旧）
sudo apt install llvm-dev clang
```

这将导致版本不匹配问题。

## 解决方案：使用 apt.llvm.org 官方脚本

apt.llvm.org 提供了自动安装脚本，可以方便地安装指定版本的 LLVM。

### 完整安装步骤

```bash
# 1. 更新 apt 源
sudo apt-get update

# 2. 安装基础依赖
sudo apt-get install -y build-essential cmake git wget

# 3. 下载并执行 LLVM 安装脚本
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18

# 4. 安装额外的开发库
sudo apt-get install -y \
  llvm-18-dev \
  clang-18 \
  libzstd-dev \
  libxml2-dev \
  libedit-dev \
  libncurses-dev \
  zlib1g-dev
```

## 验证安装

安装完成后，确认版本：

```bash
clang-18 --version
llvm-config-18 --version
```

应该显示 LLVM 18.x.x。

## 构建项目

安装完 LLVM 18 后即可构建项目：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## 参考资料

- 官方脚本：https://apt.llvm.org/
- GitHub Actions 配置：[.github/workflows/release.yml](../workflows/release.yml)
