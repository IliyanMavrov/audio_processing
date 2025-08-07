#!/bin/bash
# build.sh - Компилира audio_tool_gui.c и инсталира нужните зависимости при необходимост

set -e

echo "🔍 Проверка за нужните библиотеки..."

REQUIRED_PACKAGES=(libgtk-3-dev portaudio19-dev libsndfile1-dev)

for pkg in "${REQUIRED_PACKAGES[@]}"; do
    if ! dpkg -s "$pkg" &> /dev/null; then
        echo "📦 Инсталиране на '$pkg'..."
        sudo apt-get install -y "$pkg"
    else
        echo "✅ '$pkg' е вече инсталиран."
    fi
done

echo "⚙️ Компилиране..."
gcc audio_tool_gui.c -o audio_tool_gui \\
    `pkg-config --cflags --libs gtk+-3.0` \\
    -lportaudio -lsndfile -lm

if [ $? -eq 0 ]; then
    echo "✅ Успешна компилация! Стартирай с ./audio_tool_gui"
else
    echo "❌ Грешка при компилация"
    exit 1
fi
