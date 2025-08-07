#!/bin/bash
# build.sh - Скрипт за компилация на audio_tool_gui.c

echo "🔧 Проверка за нужните библиотеки..."
sudo apt update
sudo apt install -y libgtk-3-dev portaudio19-dev

echo "⚙️ Компилиране..."
gcc audio_tool_gui.c -o audio_tool_gui `pkg-config --cflags --libs gtk+-3.0` -lportaudio -lpthread -lm

if [ $? -eq 0 ]; then
    echo "✅ Успешно компилирано: ./audio_tool_gui"
else
    echo "❌ Грешка при компилация"
fi
