#!/bin/bash
# build.sh - Компилира audio_tool_gui.c с автоматична проверка за зависимости

check_and_install() {
    PACKAGE=$1
    dpkg -s "$PACKAGE" &> /dev/null

    if [ $? -ne 0 ]; then
        echo "📦 Пакетът '$PACKAGE' не е инсталиран. Инсталираме..."
        sudo apt-get install -y "$PACKAGE"
    else
        echo "✅ '$PACKAGE' е вече инсталиран."
    fi
}

echo "🔍 Проверка за нужните библиотеки..."

REQUIRED_PACKAGES=(
    libgtk-3-dev
    portaudio19-dev
)

for pkg in "${REQUIRED_PACKAGES[@]}"; do
    check_and_install "$pkg"
done

echo "⚙️ Компилиране..."
gcc audio_tool_gui.c -o audio_tool_gui `pkg-config --cflags --libs gtk+-3.0` -lportaudio -lpthread -lm

if [ $? -eq 0 ]; then
    echo "✅ Успешно компилирано: ./audio_tool_gui"
else
    echo "❌ Грешка при компилация"
fi
