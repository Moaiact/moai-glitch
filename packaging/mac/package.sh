#!/bin/bash
# Crea "Moai Glitch 1.1.0 (macOS).pkg" (instala VST3 + AU) y un .zip con la guía.
set -euo pipefail
VER=1.1.0
A="build/MoaiGlitch_artefacts/Release"
W=pkgwork; rm -rf "$W" dist; mkdir -p "$W/vst3" "$W/au" dist

cp -R "$A/VST3/Moai Glitch.vst3" "$W/vst3/"
cp -R "$A/AU/Moai Glitch.component" "$W/au/"

for kind in vst3 au; do
  pkgbuild --analyze --root "$W/$kind" "$W/$kind.plist"
  plutil -replace 0.BundleIsRelocatable -bool NO "$W/$kind.plist"
done

pkgbuild --root "$W/vst3" --component-plist "$W/vst3.plist" --identifier com.moaiact.moaiglitch.vst3 \
         --version $VER --install-location "/Library/Audio/Plug-Ins/VST3" "$W/vst3.pkg"
pkgbuild --root "$W/au" --component-plist "$W/au.plist" --identifier com.moaiact.moaiglitch.au \
         --version $VER --install-location "/Library/Audio/Plug-Ins/Components" "$W/au.pkg"

OUT="$W/Moai Glitch $VER (macOS)"; mkdir -p "$OUT"
productbuild --package "$W/vst3.pkg" --package "$W/au.pkg" "$OUT/Moai Glitch $VER (macOS).pkg"
cp packaging/mac/LEEME-README.txt packaging/LICENCIA.txt packaging/CREDITOS.txt "$OUT/"

(cd "$W" && ditto -c -k --keepParent "Moai Glitch $VER (macOS)" "../dist/Moai Glitch $VER (macOS).zip")
ls -la dist
