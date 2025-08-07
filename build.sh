#!/bin/bash
# build.sh - Компилира audio_tool_gui.c и инсталира нужните зависимости при необходимост (Ubuntu/Debian)

set -e

echo "🔍 Проверка за операционна система..."

if command -v apt-get >/dev/null 2>&1; then
    echo "🟢 Ubuntu/Debian система установена. Проверка и инсталация на нужните пакети..."

    REQUIRED_PACKAGES=(libgtk-3-dev portaudio19-dev libsndfile1-dev pkg-config build-essential)

    for pkg in "${REQUIRED_PACKAGES[@]}"; do
        if ! dpkg -s "$pkg" &> /dev/null; then
            echo "📦 Инсталиране на '$pkg'..."
            sudo apt-get install -y "$pkg"
        else
            echo "✅ '$pkg' вече е инсталиран."
        fi
    done
else
    echo "⚠️ Не е открита Ubuntu/Debian система."
    echo "Моля, инсталирайте ръчно следните пакети преди компилация:"
    echo " - libgtk-3-dev"
    echo " - portaudio19-dev"
    echo " - libsndfile1-dev"
    echo " - pkg-config"
    echo " - build-essential"
fi

echo "⚙️ Компилиране..."
gcc audio_tool_gui.c -o audio_tool_gui \
    `pkg-config --cflags --libs gtk+-3.0` \
    -lportaudio -lsndfile -lm

if [ $? -eq 0 ]; then
    echo "✅ Успешна компилация! Стартирайте с ./audio_tool_gui"
else
    echo "❌ Грешка при компилация"
    exit 1
fi
