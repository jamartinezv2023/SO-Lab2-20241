#!/bin/bash
# ============================================================
# 🎓 UNIVERSIDAD DE ANTIOQUIA — Laboratorio de Sistemas Operativos
# Script: setup_access.sh
# Autor: José Martínez V.
# Descripción:
#   Configura accesos rápidos permanentes en Ubuntu y Windows
#   para abrir automáticamente el laboratorio SO-Lab2.
# ============================================================

set -e

PROJECT_PATH="/root/so_lab2"
POWERSHELL_PROFILE_PATH="/mnt/c/Users/Admin/Documents/WindowsPowerShell/Microsoft.PowerShell_profile.ps1"
UBUNTU_SHORTCUT_PATH="/mnt/c/Users/Admin/Desktop/Ubuntu_SO-Lab2.lnk"

echo "==========================================================="
echo "⚙️  CONFIGURACIÓN AUTOMÁTICA DE ACCESOS RÁPIDOS"
echo "==========================================================="

# ------------------------------------------------------------
# 1️⃣ Configurar entrada automática al proyecto en Ubuntu
# ------------------------------------------------------------
if ! grep -q "cd $PROJECT_PATH" ~/.bashrc; then
  echo "cd $PROJECT_PATH" >> ~/.bashrc
  echo "✅ Añadido inicio automático en ~/.bashrc"
else
  echo "ℹ️ Ya existe la configuración de inicio automático en ~/.bashrc"
fi

# ------------------------------------------------------------
# 2️⃣ Crear alias 'so' en PowerShell para abrir Ubuntu directamente
# ------------------------------------------------------------
echo "Creando alias 'so' en PowerShell..."

mkdir -p "$(dirname "$POWERSHELL_PROFILE_PATH")"
cat << 'EOF' > "$POWERSHELL_PROFILE_PATH"
# Alias para abrir Ubuntu directamente en la carpeta de laboratorio
Set-Alias so "wsl.exe -d Ubuntu --cd /root/so_lab2"
EOF

echo "✅ Alias 'so' añadido en PowerShell (Microsoft.PowerShell_profile.ps1)"

# ------------------------------------------------------------
# 3️⃣ Crear acceso directo en el escritorio de Windows
# ------------------------------------------------------------
echo "🪟 Creando acceso directo en el escritorio de Windows..."

powershell.exe -Command "
\$WScriptShell = New-Object -ComObject WScript.Shell;
\$Shortcut = \$WScriptShell.CreateShortcut('$UBUNTU_SHORTCUT_PATH');
\$Shortcut.TargetPath = 'wsl.exe';
\$Shortcut.Arguments = '-d Ubuntu --cd /root/so_lab2';
\$Shortcut.Description = 'Acceso rápido al laboratorio SO-Lab2';
\$Shortcut.IconLocation = 'C:\\Windows\\System32\\wsl.exe,0';
\$Shortcut.Save();
"

echo "✅ Acceso directo creado en el escritorio: Ubuntu_SO-Lab2.lnk"

# ------------------------------------------------------------
# 4️⃣ Confirmación final
# ------------------------------------------------------------
echo "==========================================================="
echo "🎉 CONFIGURACIÓN COMPLETADA"
echo "📂 Carpeta de trabajo: $PROJECT_PATH"
echo "🪟 Acceso directo: Ubuntu_SO-Lab2.lnk (Escritorio de Windows)"
echo "💻 Alias PowerShell: so"
echo "==========================================================="
echo "👉 Puedes abrir directamente con: so"
echo "   o hacer doble clic en el acceso directo del escritorio."
