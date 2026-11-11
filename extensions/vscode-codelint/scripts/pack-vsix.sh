#!/usr/bin/env bash
# Minimal VSIX packer (avoids downloading @vscode/vsce when network is slow).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

NAME="$(node -p "require('./package.json').name")"
VERSION="$(node -p "require('./package.json').version")"
PUBLISHER="$(node -p "require('./package.json').publisher")"
OUT="${NAME}-${VERSION}.vsix"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

mkdir -p "$STAGE/extension"
cp package.json README.md "$STAGE/extension/"
cp -R out "$STAGE/extension/out"
if [[ -d media ]]; then
  cp -R media "$STAGE/extension/media"
fi

cat >"$STAGE/extension.vsixmanifest" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<PackageManifest Version="2.0.0" xmlns="http://schemas.microsoft.com/developer/vsx-schema/2011" xmlns:d="http://schemas.microsoft.com/developer/vsx-schema-design/2011">
  <Metadata>
    <Identity Language="en-US" Id="${NAME}" Version="${VERSION}" Publisher="${PUBLISHER}" />
    <DisplayName>Codelint</DisplayName>
    <Description xml:space="preserve">Run codelint-core clang-tidy checks in VS Code</Description>
    <Tags>c++,clang-tidy,codelint,lint</Tags>
    <Categories>Linters,Programming Languages</Categories>
    <Icon>extension/media/icon.png</Icon>
    <Properties>
      <Property Id="Microsoft.VisualStudio.Code.Engine" Value="^1.85.0" />
      <Property Id="Microsoft.VisualStudio.Code.ExtensionDependencies" Value="" />
      <Property Id="Microsoft.VisualStudio.Code.ExtensionPack" Value="" />
      <Property Id="Microsoft.VisualStudio.Code.LocalizedLanguages" Value="" />
      <Property Id="Microsoft.VisualStudio.Services.GitHubFlavoredMarkdown" Value="true" />
    </Properties>
  </Metadata>
  <Installation>
    <InstallationTarget Id="Microsoft.VisualStudio.Code"/>
  </Installation>
  <Dependencies/>
  <Assets>
    <Asset Type="Microsoft.VisualStudio.Code.Manifest" Path="extension/package.json" Addressable="true" />
    <Asset Type="Microsoft.VisualStudio.Services.Icons.Default" Path="extension/media/icon.png" Addressable="true" />
  </Assets>
</PackageManifest>
EOF

cat >"$STAGE/[Content_Types].xml" <<'EOF'
<?xml version="1.0" encoding="utf-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension=".json" ContentType="application/json"/>
  <Default Extension=".vsixmanifest" ContentType="text/xml"/>
  <Default Extension=".md" ContentType="text/markdown"/>
  <Default Extension=".js" ContentType="application/javascript"/>
  <Default Extension=".map" ContentType="application/json"/>
  <Default Extension=".png" ContentType="image/png"/>
</Types>
EOF

rm -f "$OUT"
(
  cd "$STAGE"
  zip -rq "$ROOT/$OUT" extension.vsixmanifest "[Content_Types].xml" extension
)
echo "Wrote $ROOT/$OUT ($(wc -c <"$OUT") bytes)"
