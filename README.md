# threaded_irq

Використовуючи приклад onboard_io, висланий разом із матеріалами попередньої
лекції, напишіть модуль, який реагує на переривання по натисканню кнопки.

Використайте threaded irq

hardware handler повинен перемикати вихід світлодіода на протилежне значення,
інкрементувати 32-бітний лічильник натискань кнопки і піднімати потік.

thread_fn повинна зчитувати цей лічильник і друкувати його значення

Загалом робота зі змінною одночасно з двох таких обробників підпадає
під «перегони» («race conditions»). У даному випадку такі операції
з лічильником більш-менш безпечні, бо одна сторона лише читає,
а 32-бітні змінні тут зчитуються атомарно. Захист від проблем у таких
ситуаціях то тема наступних лекцій.

У функції init додайте лічильник натискань кнопки у debugfs через обгортку
    debugfs_create_u32()
Розмістіть запис у каталозі, створеному
    debugfs_create_dir(KBUILD_MODNAME, NULL)
Знайдіть цей лічильник на debugfs і виведіть його значення почергово
з натисканнями кнопки.


Додайте параметр модуля
bool simulate_busy;
по якому у функції thread_fn після друку значення лічильника робиться msleep
на декілька секунд і друк повідомлення про закінчення «роботи».