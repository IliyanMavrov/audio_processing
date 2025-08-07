Brief explanation


Инсталирай нужните библиотеки:

sudo apt install libgtk-3-dev portaudio19-dev

Компилирай:

gcc audio_tool_gui.c -o audio_tool_gui `pkg-config --cflags --libs gtk+-3.0` -lportaudio -lpthread -lm

✅ Самостоятелен и компилируем с gcc

gcc audio_tool_gui.c -o audio_tool_gui -lportaudio -lfftw3 -lm -lpthread

▶️ Стартирай:

./audio_tool_gui


🎛️ Аргументи: --echo, --delay, --lowpass, --distortion, --gain, --reverb, --record, --spectrum

🔉 Ефекти: echo, delay, low-pass filter, distortion, reverb

💾 Запис към .wav файл с валиден хедър

📊 Спектрален анализ в реално време (терминал)

🔒 Кръгов буфер с mutex защита


./audio_tool_gui	Просто преминаване на аудио без ефекти
./audio_tool_gui    --echo --delay=750	Echo ефект с 750ms закъснение
./audio_tool_gui    --lowpass=0.1	Филтър с меко изглаждане
./audio_tool_gui    --record=mic.wav	Записва какво чуваш в mic.wav
./audio_tool_gui    --spectrum	Спектрален анализ в реално време

Примерна употреба
./audio_tool --echo --delay=700 --lowpass=0.2 --distortion --gain=3.5 --reverb --record=output.wav --spectrum
