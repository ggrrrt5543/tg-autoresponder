# TG Autoresponder (C++ / TDLib)

Telegram автоответчик на C++ через TDLib (MTProto). Минимальная задержка ~20-50мс.

## Структура

```
src/
├── main.cpp       — TDLib JSON client, авторизация, event loop
├── config.h/cpp   — конфигурация (JSON, потокобезопасная)
├── handler.h/cpp  — обработка команд и автоответ
├── utils.h/cpp    — извлечение суммы из текста (regex)
└── json.hpp       — nlohmann/json (single-header)
tdjson.dll         — prebuilt TDLib (загружается динамически)
tg-autoresponder.exe — готовый бинарник
build.bat          — скрипт сборки
```

## Перед запуском

1. Получи `api_id` и `api_hash` на https://my.telegram.org/apps
2. Открой `src/main.cpp`, найди строки:
   ```cpp
   {"api_id", 0},           // <-- YOUR API_ID HERE
   {"api_hash", ""},        // <-- YOUR API_HASH HERE
   ```
   Замени на свои значения.
3. Пересобери: запусти `build.bat`

## Запуск

```
tg-autoresponder.exe
```

При первом запуске попросит номер телефона, код, и 2FA пароль (если есть).
Сессия сохраняется в папке `tdlib_session/`.

## Команды (пишешь в любом чате Telegram от своего аккаунта)

| Команда | Описание |
|---------|----------|
| `/start_bot` | Включить автоответчик |
| `/stop_bot` | Выключить |
| `/status` | Показать состояние |
| `/upchat <chat_id>` | Добавить чат для мониторинга |
| `/ans_chat <текст>` | Задать текст ответа |
| `/limit <min> <max>` | Установить диапазон суммы |

## Как работает

1. Мониторит сообщения в указанных чатах
2. Ищет паттерн "Итого: X USDT" или "Количество: X USDT"
3. Если X попадает в лимиты — мгновенно отправляет заданный ответ
4. Polling каждые 10мс — реакция практически real-time
