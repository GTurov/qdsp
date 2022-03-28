# Библиотека DSP для QT
____

## Назначение
Библиотка предназначена для реализации алгоритмов цифровой обработки сигналов
(ЦОС) и включает в себя:
- специализированные контейнеры для действительных и
комплексных сигналов и их арифметику.Имеется возможность экспорта сигналов в
файлы и их импорта из них.
- базовые алгоритмы цифровой обработки сигналов.
____

## Состав
В данный момент библиотека включает в себя 2 модуля:

- **qdsp.h**. Базовые классы и алгортмы для ЦОС
- **qrswaveform.h**. Импорт и экспорт сигналов в форматы, поддерживаемые
оборудованием Rohde&Scwarz (*.wv, *wvh/wvd, *iq.tar).
____

## Требования
- QT5
- С++17
____

## Базовые принципы

### Комплексные числа
Комплексные числа в библиотеке представлены классом `QComplex`, являющегося
псевдонимом класса `std::complex<double>`. Класс содержит в себе действительную и
мнимую части числа. Числа могут быть заданы как с помощью аргументов
конструктора, так и c помощью литерала **i**.
```C++
QComplex a; // a == (0)
QComplex b = 3.0; // b == (3, 0)
QComplex c = 3.1 + 5.0_i; // c == (3.1, 5)
QComplex d(1, 2); // d == (1, 2)
d.real(2); // d == (2, 2)
```
Поддерживаются все арифметические операции над числами:
```C++
QComplex a; // a == (0)
a += QComplex(0,3); // a == (0, 3)
QComplex b = 10; // b == (10)
b /= 2.0; // b == (5);
QComplex c = a + b; // c == (5, 3)
```
Также поддерживаются тригонометрические функции, возведение в степень и логарифмирование.

### Сигналы
Основными классами библиотеки являются действительные и комплексные сигналы -
специализированные контейнеры, построенные на базе `QVector`, поддерживающие
основные арифметические операции ЦОС. Действительный сигнал может быть приведён
к комплексному. Единицей измерения отсчётов сигналов является Вольт.
Поддерживаются основные арифметические операции над сигналами: сложение,
вычитание, умножение. Операции выполняются попарно над соответствующими
отсчётами сигналов.
```C++
QRealSignal a = {1, 2, 3, 4, 5};
QComplexSignal b = {1i, 2i, 3i, 4i, 5i};
QComplexSignal c = a + b;
// c == {1+1i, 2+2i, 3+3i, 4+4i, 5+5i};
```

### Арифметика сигналов
- Результатом арифметических операций над одним или несколькими сигналами
является сигнал, комплексный или действительный.
- Одним из аргументов операции умножения может выступать число, действительное
или комплексное.
```C++
QRealSignal a = {1, 2, 3, 4, 5};
QRealSignal b = a * 2;
// b == {2, 4, 6, 8, 10};
```
- Если хотя бы один из операндов комплексный, то результат комплексный, иначе
действительный.

| Операнд 1 | Операнд 2 | Результат |
|:----------------:|:---------:|:----------------:|
| Действительный | Действительный | Действительный |
| Действительный | Комплексный | Комплексный |
| Комплексный | Действительный | Комплексный |
| Комплексный | Комплексный | Комплексный |

```C++
QRealSignal a = {1, 2, 3, 4, 5};
QComplex b = 10i;
auto c = a * b;
// c == {10i, 20i, 30i, 40i, 50i}; //QComplexSignal
```
- Если оба операнды сигналы, то размер результирующего сигнала всегда выбирается
как минимум из размеров операндов.
```C++
QRealSignal a = {1, 2, 3, 4, 5};
QRealSignal b = {10, 10, 10};
QRealSignal c = a * b;
// c.size() == 3;
// c == {10, 20, 30};
```

### Частоты дискретизации
Во многих приложениях требуется учитывать не только форму
сигнала, но и его частоту дискретизации. Для операций с частотами дискретизации
предусмотрен специальный класс `QFrequency`, который содержит в себе значение
частоты. Отдельный класс необходим для минимазации количества ошибок при
передаче аргументов в конструкторы и алгоритмы. Поддерживаются литералы **_Hz**,
**_kHz**, **_MHz**, **_GHz**, а также операции сложения, вычитания, умножения на
действительное число и деление на действительное число. Cчитается, что сигнал
(или иной контейнер) имеет частоту дискретизации, если она положительна.
```C++
QFrequency f1 = 5.5_kHz; // f1 == 5.5e3
auto f2 = 2 * f1; // f2 == 1.1e4

QFrequency f3 = 10_MHz; // f3 == 1e7
auto f4 = f3 / 2; // f4 == 5e6

QRealSignal sine = QRealSignal(10);
// sine.hasClock() == false
// sine.clock() == 0
sine = QRealSignal::harmonic(100, 100_MHz, 1_MHz, 5);
// Создан сигнал из 100 отсчётов с частотой дискретизации 100 МГц,
// представляющий собой синусоиду частотой 1МГц и пиковым значением 5В.
// sine.hasClock() == true
// sine.clock() == 1e8
```

### Арифметика частот дискретизации
- Если оба операнда не имеют частот дискретизации, результат также не имеет
частоты дискретизации.
- Если сигналы имеют одинаковую частоту дискретизации или только один из
операндов имеет частоту дискретизации, результат имеет эту же частоту
дискретизации.
- Если оба операнда имеют частоты дискретизации и они разные, результат не имеет
частоты дискретизации.

| Частота дискретизации 1 | Частота дискретизации 2 | Частота дискретизации результата |
|:----------------:|:---------:|:----------------:|
| 0 | 0 | 0 |
| F | 0 | F |
| 0 | F | F |
| F | F | F |
| F1 | F2 | 0 |

### Фреймы
Для работы с фрагментами сигналов предусмотрены классы `QRealSignalFrame` и
`QComplexSignalFrame`. Классы позволяют работать с фрагментом сигнала
соответствующего типа как с отдельным сигналом (производить арифметические
операции и передавать его в качестве аргумента в алгоритмы ЦОС), не прибегая при
этом к глубокому копированию, что очень актуально при работе с сигналами
большого размера. Эти классы играют для соответствующих типов сигналов примерно такую же
роль, как `std::string_view` для `std::string`. Можно представить фрейм как
рамку, через которую доступен фрагмент сигнала, размер и положение которой можно
менять.
```C++
QRealSignal a = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
QRealSignalFrame fr(a);
// fr == {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
fr.setWidth(3);
// fr == {1, 2, 3}
fr.shift(2);
// fr == {3, 4, 5}
fr.increaseWidth(2).shift(3);
// fr == {6, 7, 8, 9, 10}
fr.decreaseWidth(2);
// fr == {6, 7, 8}
QRealSignal b = {10, 100, 1000};
auto c = b * fr;
// c == {60, 700, 8000}
```

### Импорт и экспорт сигналов
Для импорта и экспорта сигналов в файлы предусмотрены классы `QRealWaveform` и
`QComplexWaveform`, которые унаследованы от соответствующих типов сигналов. К
сохранённому сигналу можно добавить комментарий и задать дату его создания.
Поддерживаются типы данных для хранения сигнала:
- целочисленный 8 бит (`int8_t`);
- целочисленный 16 бит (`int16_t`);
- целочисленный 32 бит (`int32_t`);
- целочисленный 64 бит (`int64_t`);
- с плавающей точкой 32 бита (`float`);
- с плавающей точкой 64 бита (`double`).
По умолчанию используется тип данных `double`. Для целочисленных типов используется
масштабирование сигнала относительно максимального уровня, поддерживаемого
соответствующим типом данных, что может привести к возрастанию шума квантования
при малой недостаточной разрядности выбранного типа данных.
```C++
QRealSignal a = QRealSignal::harmonic(44000, 44_kHz, 440_Hz);
QRealSignalWaveform wf(a, "test signal description");
wf.saveToFile("test_signal.wfr", WaveformDataType::INT32);
```
____

## Модуль qdsp.h

### Типы и классы

#### QReal
Псевдоним, характеризующий тип данных, используемый для хранения отсчётов внутри
контейнеров. По-умолчанию `double`. Может быть изменён на `float` путём объявления
`#define QDSP_FLOAT_DISCRETE`.

#### Класс QComplex
Класс, описывающий комплексные числа. Является псевдонимом класса
`std::complex<double>`(`std::complex<float>` при объявлении `#define QDSP_FLOAT_DISCRETE`).
Более подробная информация по ссылке https://en.cppreference.com/w/cpp/numeric/complex.

#### Класс QFrequency
Класс, описывающий частоту. Класс имеет единстенное приватное поле - значение
частоты (`double`). Частота может принимать как положительные, так и отрицательные значения.
##### Публичные функции

**QFrequency()**
> Конструктор по-умолчанию, создаёт экземпляр класса с нулевой частотой

**explicit constexpr QFrequency(double Hz)**
> Коструктор, создаёт экземпляр класса с частотой, передаваемой в качестве параметра. Частота выражена в Герцах.

**constexpr double mHz() const**
> Возвращает значение частоты, выраженное в мГц.

**constexpr double Hz() const**
> Возвращает значение частоты, выраженное в мГц.

**constexpr double kHz() const**
> Возвращает значение частоты, выраженное в кГц.

**constexpr double MHz() const**
> Возвращает значение частоты, выраженное в МГц.

**constexpr double GHz() const**
> Возвращает значение частоты, выраженное в ГГц.

**constexpr double value() const**
> Возвращает значение частоты. То же, что и **constexpr double Hz() const**

**constexpr bool isPositive() const**
> Возвращает `true`, если значение частоты больше `0`, иначе возвращает `false`.

**void clear()**
> Сбрасывает установленное значение частоты.

##### Константы

**inline constexpr QFrequency noFrequency = {}**
> Константа, соответствующая нулевой частоте.

##### Свободные функции

**QDebug operator<<(QDebug debug, QFrequency freq)**
> Вывод значения частоты в `qDebug()`. Частота выводится в Герцах.

**constexpr QFrequency operator+(QFrequency f)**
> Унарный `+`. Возвращает переданный аргумент без изменений.

**constexpr QFrequency operator-(QFrequency f)**
> Унарный `-`. Возвращает частоту, инвертированную относительно нуля.

**constexpr QFrequency operator+(QFrequency lhs, QFrequency rhs)**
> Возвращает сумму двух частот. Результат частота.

**constexpr QFrequency operator-(QFrequency lhs, QFrequency rhs)**
> Возвращает разность двух частот. Результат частота.

**constexpr double operator/(QFrequency lhs, QFrequency rhs)**
> Возвращает частное двух частот. Результат действительное число.

**constexpr QFrequency operator/(QFrequency lhs, double rhs)**
> Возвращает частное частоты и действительного числа. Результат частота.

__constexpr QFrequency operator*(QFrequency lhs, double rhs)__
> Возвращает произведение частоты и действительного числа. Результат частота.

__constexpr QFrequency operator*(double lhs, QFrequency rhs)__
> Возвращает произведение частоты и действительного числа. Результат частота.

**constexpr bool operator==(QFrequency lhs, QFrequency rhs)**
> Сравнение частот. Возвращет `true`, если аргументы равны, иначе возвращает `false`.

**constexpr bool operator!=(QFrequency lhs, QFrequency rhs)**
> Сравнение частот. Возвращет `true`, если аргументы не равны, иначе возвращает `false`.

**constexpr bool operator>(QFrequency lhs, QFrequency rhs)**
> Сравнение частот. Возвращет `true`, если значение первого аргумента больше второго, иначе возвращает `false`.

**constexpr bool operator>=(QFrequency lhs, QFrequency rhs)**
> Сравнение частот. Возвращет `true`, если значение первого аргумента больше или равно второму, иначе возвращает `false`.

**constexpr bool operator<(QFrequency lhs, QFrequency rhs)**
> Сравнение частот. Возвращет `true`, если значение первого аргумента меньше второго, иначе возвращает `false`.

**constexpr bool operator<=(QFrequency lhs, QFrequency rhs)**
> Сравнение частот. Возвращет `true`, если значение первого аргумента меньше или равно второму, иначе возвращает `false`.

##### Литералы

**constexpr QFrequency operator"" _Hz(long double value)**
> Создаёт экземпляр класса частоты на основе действительного числа. Аргумент выражен в Гц.

**constexpr QFrequency operator"" _Hz(unsigned long long value)**
> Создаёт экземпляр класса частоты на основе целого числа. Аргумент выражен в Гц.

**constexpr QFrequency operator"" _kHz(long double value)**
> Создаёт экземпляр класса частоты на основе действительного числа. Аргумент выражен в кГц.

**constexpr QFrequency operator"" _kHz(unsigned long long value)**
> Создаёт экземпляр класса частоты на основе целого числа. Аргумент выражен в кГц.

**constexpr QFrequency operator"" _MHz(long double value)**
> Создаёт экземпляр класса частоты на основе действительного числа. Аргумент выражен в МГц.

**constexpr QFrequency operator"" _MHz(unsigned long long value)**
> Создаёт экземпляр класса частоты на основе целого числа. Аргумент выражен в МГц.

**constexpr QFrequency operator"" _GHz(long double value)**
> Создаёт экземпляр класса частоты на основе действительного числа. Аргумент выражен в ГГц.

**constexpr QFrequency operator"" _GHz(unsigned long long value)**
> Создаёт экземпляр класса частоты на основе целого числа. Аргумент выражен в ГГц.

#### Классы QRealSignal и QComplexSignal
Классы описывают действительный (`QRealSignal`) и комплексный (`QComplexSignal`) сигналы.
Эти классы наследуюстся от `QVector<QReal>` и `QVector<QComplex>` соотвественно.
Ниже параметр шаблона `Discrete` соответствует `QReal` для действительного сигнала и
`QComplex` для комплексного.

##### Публичные типы

**typedef ConstIterator**
> Qt синоним для const_iterator.

**typedef Iterator**
> Qt синоним для iterator.

**typedef const_iterator**
> константный итератор в стиле STL.

**typedef const_pointer**
> typedef для const Discrete*. Для совместимости с STL

**typedef const_reference**
> typedef для const Discrete&. Для совместимости с STL

**typedef const_reverse_iterator**
> константный реверсный итератор в стиле STL.

**typedef difference_type**
> typedef для ptrdiff_t. Для совместимости с STL.

**typedef iterator**
> итератор в стиле STL.

**typedef pointer**
> typedef для Discrete*. Для совместимости с STL.

**typedef reference**
> typedef для Discrete&. Для совместимости с STL.

**typedef reverse_iterator**
> реверсный итератор в стиле STL.

**typedef size_type**
> Typedef для int. Для совместимости с STL.

**typedef value_type**
> Typedef для Discrete. Для совместимости с STL.

##### Общие публичные функции классов QRealSignal и QComplexSignal, являющиеся полными аналогами соответствующих методов шаблона QVector<T>

**void append(const Discrete& value)**

**void append(const QSignal<Discrete>& other)**

**const Discrete& at(int i) const**

**reference back()**

**const_reference back() const**

**iterator begin()**

**const_iterator begin() const**

**int capacity() const**

**const_iterator cbegin() const**

**const_iterator cend() const**

**void clear()**

**const_iterator constBegin() const**

**const Discrete*  constData() const**

**const_iterator constEnd() const**

**const Discrete& constFirst() const**

**const Discrete& constLast() const**

**bool contains(const Discrete& value) const**

**int count(const Discrete& value) const**

**int count() const**

**const_reverse_iterator crbegin()**

**const const_reverse_iterator crend()**

__Discrete* data()__

__const Discrete* data() const__

**bool empty() const**

**iterator end()**

**const_iterator end() const**

**iterator erase(iterator pos)**

**iterator erase(iterator begin, iterator end)**

**auto& fill(const QReal& value, int size = -1)**

**Discrete& first()**

**const Discrete& first() const**

**Discrete& front()**

**const_reference front() const**

**void insert(int i, const Discrete& value)**

**void insert(int i, int count, const Discrete& value)**

**iterator insert(iterator before, int count, const Discrete& value)**

**iterator insert(iterator before, const Discrete& value)**

**bool isEmpty() const**

**Discrete& last()**

**const Discrete& last() const**

**int length() const**

**auto mid(int pos, int length = -1)**

**void move(int from, int to)**

**void pop_back()**

**void pop_front()**

**void prepend(const Discrete& value)**

**void push_back(const Discrete& value)**

**void push_front(const Discrete& value)**

**reverse_iterator rbegin()**

**const_reverse_iterator rbegin() const**

**void remove(int i)**

**void remove(int i, int count)**

**int  removeAll(const Discrete& t)**

**void removeAt(int i)**

**void removeFirst()**

**void removeLast()**

**bool removeOne(const Discrete& t)**

**reverse_iterator rend()**

**const_reverse_iterator rend() const**

**void replace(int i, const Discrete& value)**

**void reserve(int size)**

**void resize(int size)**

**void shrink_to_fit()**

**int size() const**

**void squeeze()**

**void swap(QSignal<Discrete>& other)**

**Discrete takeAt(int i)**

**Discrete takeFirst()**

**Discrete takeLast()**

**QList<Discrete> toList() const**

**std::vector<Discrete> toStdVector() const**

**Discrete value(int i) const**

**Discrete value(int i, const Discrete& defaultValue) const**

**Discrete& operator[](int i)**

**const Discrete& operator[](int i) const**

**auto& operator=(const QSignal<Discrete>& other)**

**auto& operator=(QSignal<Discrete>&& other)**

**bool operator==(const QSignal<Discrete>& other) const**

**bool operator!=(const QSignal<Discrete>& other) const**

**auto& operator<<(const Discrete& value)**

##### Общие публичные функции классов QRealSignal и QComplexSignal

**QFrequency clock() const**
> Возвращает тактовую частоту сигнала

**double duration(int n) const**
> Возвращает длительность `n` отсчётов сигнала.

**double duration(int from, int to) const**
> Возвращает длительность сигнала от отсчёта `from` до `to`.

**double duration() const**
> Возвращает длительность всего сигнала.

**bool hasClock() const**
> Возвращает `true`, если сигнал имеет корректную тактовую частоту (больше `0`), иначе возвращает `false`.

**void resetClock()**
> Сбрасывает тактовую частоту сигнала в `0`

**void setClock(QFrequency freq)**
> Устанавливает тактовую частоту сигнала, равную `freq`.

**const QVector<Discrete>& asQVector() const**
> Возвращает ссылку на сигнал в виде вектора отсчётов.


##### Публичные функции класса QRealSignal

**QRealSignal()**
> Конструктор по-умолчанию

**explicit QRealSignal(QFrequency clock)**
> Создаёт действительный сигнал с тактовой частотой `clock`.

**explicit QRealSignal(int size)**
> Создаёт действительный сигнал размером `size`.

**explicit QRealSignal(int size, QFrequency clock)**
> Создаёт действительный сигнал размером `size` с тактовой частотой `clock`.

**explicit QRealSignal(int size, const QReal& value)**
> Создаёт действительный сигнал размером `size`, заполненный значениями, равными `value`.

**explicit QRealSignal(int size, QFrequency clock, const QReal& value)**
> Создаёт действительный сигнал размером `size` с тактовой частотой `clock`, заполненный значениями, равными `value`.

**QRealSignal(const QRealSignal& other)**
> Создаёт копию действительного сигнала `other`.

**QRealSignal(QRealSignal&& other)**
> Создаёт действительный сигнал, перемещая содержимое и тактовую частоту из `other`.

**QRealSignal(std::initializer_list<QReal> args)**
> Создаёт действительный сигнал из списка инициализации.

**explicit QRealSignal(QFrequency clock, std::initializer_list<QReal> args)**
> Создаёт действительный сигнал из списка инициализации с тактовой частотой `clock`.

**virtual ~QRealSignal() {}**
> Деструктор

**QRealSignal& operator=(const QRealSignal &other)**
> Оператор присваивания

**QRealSignal& operator=(QRealSignal &&other)**
> Оператор перемещения

**template<class OutputIt>**
**static void harmonic(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Аддитивно добавляет к контейнеру действительный гармонический сигнал размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**static QRealSignal harmonic(int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный гармонический сигнал размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**static QRealSignal harmonic(int size, QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный гармонический сигнал размером `size` с частотой `freq`, частотой дискретизации `clock`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**template<class OutputIt>**
**static void meander(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Аддитивно добавляет к контейнеру действительный меандр размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `phase`, начиная с позиции `start`.

**static QRealSignal meander(int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный меандр размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**static QRealSignal meander(int size, QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный меандр размером `size` с частотой `freq`, частотой дискретизации `clock`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**template<class OutputIt>**
**static void saw(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Аддитивно добавляет к контейнеру действительный пилообразный сигнал размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**static QRealSignal saw(int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный пилообразный сигнал размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**static QRealSignal saw(int size, QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный пилообразный сигнал размером `size` с частотой `freq`, частотой дискретизации `clock`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**template<class OutputIt>**
**static void chirp(OutputIt start, int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0**
> Аддитивно добавляет к контейнеру действительный сигнал с линейно-частотной модуляцией размером `size` с начальной нормированной частотой `startNormFreq`,
> начальной нормированной частотой `stopNormFreq`, максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**static QRealSignal chirp(int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный сигнал с линейно-частотной модуляцией размером `size` с начальной нормированной частотой `startNormFreq`,
> начальной нормированной частотой `stopNormFreq`, максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**static QRealSignal chirp(int size, QFrequency clock, QFrequency startFreq, QFrequency stopFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает действительный сигнал с линейно-частотной модуляцией размером `size` с частотой дискретизации `clock`, с начальной нормированной частотой `startFreq`,
> начальной нормированной частотой `stopFreq`, максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**template<class OutputIt>**
**static void uniformNoise(OutputIt start, int size, double deviation = 1.0, double mean = 0)**
> Аддитивно добавляет к контейнеру `size` действительных отсчётов шума с равномерным распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

**static QRealSignal uniformNoise(int size, QFrequency clock, double deviation = 1.0, double mean = 0)**
> Возвращает действительный шумовой сигнал размером `size` с равномерным распределением, тактовой частотой `clock`,
> дисперсией `deviation` и математическим ожиданием `mean`, начиная с позиции `start`.

**static QRealSignal uniformNoise(int size, double deviation = 1.0, double mean = 0)**
> Возвращает действительный шумовой сигнал размером `size` с равномерным распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

**template<class OutputIt>**
**static void gaussianNoise(OutputIt start, int size, double deviation = 1.0, double mean = 0)**
> Аддитивно добавляет к контейнеру `size` действительных отсчётов шума с нормальным (Гауссовским) распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

**static QRealSignal gaussianNoise(int size, QFrequency clock, double deviation = 1.0, double mean = 0)**
> Возвращает действительный шумовой сигнал размером `size` с нормальным (Гауссовским) распределением, тактовой частотой `clock`,
> дисперсией `deviation` и математическим ожиданием `mean`, начиная с позиции `start`.

**static QRealSignal gaussianNoise(int size, double deviation = 1.0, double mean = 0)**
> Возвращает действительный шумовой сигнал размером `size` с нормальным (Гауссовским) распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

##### Публичные функции класса QComplexSignal

**QComplexSignal()**
> Конструктор по-умолчанию

**explicit QComplexSignal(QFrequency clock)**
> Создаёт комплексный сигнал с тактовой частотой `clock`.

**explicit QComplexSignal(int size)**
> Создаёт комплексный сигнал размером `size`.

**explicit QComplexSignal(int size, QFrequency clock)**
> Создаёт комплексный сигнал размером `size` с тактовой частотой `clock`.

**explicit QComplexSignal(int size, const QComplex& value)**
> Создаёт комплексный сигнал размером `size`, заполненный значениями, равными `value`.

**explicit QComplexSignal(int size, QFrequency clock, const QComplex& value)**
> Создаёт комплексный сигнал размером `size` с тактовой частотой `clock`, заполненный значениями, равными `value`.

**QComplexSignal(const QComplexSignal& other)**
> Создаёт копию комплексного сигнала `other`.

**QComplexSignal(const QRealSignal& other)**
> Создаёт комплексный сигнал копируюя значения из действительного сигнала `other`.

**QComplexSignal(QComplexSignal&& other)**
> Создаёт комплексный сигнал, перемещая содержимое и тактовую частоту из `other`.

**QComplexSignal(std::initializer_list<QComplex> args)**
> Создаёт комплексный сигнал из списка инициализации.

**QComplexSignal(QFrequency clock, std::initializer_list<QComplex> args)**
> Создаёт комплексный сигнал из списка инициализации с тактовой частотой `clock`.

**virtual ~QComplexSignal() {}**
> Деструктор

**void append(const QRealSignal& other)**
> Добавляет в комплексный сигнал значения из действительного сигнала `other`.

**QRealSignal i() const**
> Возвращает синфазную составляющую комплексного сигнала.

**QRealSignal q() const**
> Возвращает квадратурную составляющую комплексного сигнала.

**template<class OutputIt>**
**static void harmonic(OutputIt start, int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Аддитивно добавляет к контейнеру комплексный гармонический сигнал (комплексную экспоненту) размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `phase`, начиная с позиции `start`.

**static QComplexSignal harmonic(int size, double normFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает комплексный гармонический сигнал (комплексную экспоненту) размером `size` с нормированной частотой `normFreq`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**static QComplexSignal harmonic(int size, QFrequency clock, QFrequency freq, double magnitude = 1.0, double phase = 0)**
> Возвращает комплексный гармонический сигнал (комплексную экспоненту) размером `size` с частотой `freq`, частотой дискретизации `clock`,
> максимальным значением `magnitude` и начальной фазой `phase`.

**template<class OutputIt>**
**static void chirp(OutputIt start, int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0)**
> Аддитивно добавляет к контейнеру комплексный сигнал с линейно-частотной модуляцией размером `size` с начальной нормированной частотой `startNormFreq`,
> начальной нормированной частотой `stopNormFreq`, максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**static QComplexSignal chirp(int size, double startNormFreq, double stopNormFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает комплексный сигнал с линейно-частотной модуляцией размером `size` с начальной нормированной частотой `startNormFreq`,
> начальной нормированной частотой `stopNormFreq`, максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**static QComplexSignal chirp(int size, QFrequency clock, QFrequency startFreq, QFrequency stopFreq, double magnitude = 1.0, double phase = 0)**
> Возвращает комплексный сигнал с линейно-частотной модуляцией размером `size` с частотой дискретизации `clock`, с начальной нормированной частотой `startFreq`,
> начальной нормированной частотой `stopFreq`, максимальным значением `magnitude` и начальной фазой `double`, начиная с позиции `start`.

**template<class OutputIt>**
**static void uniformNoise(OutputIt start, int size, double deviation = 1.0, double mean = 0)**
> Аддитивно добавляет к контейнеру `size` комплексных отсчётов шума с равномерным распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

**static QComplexSignal uniformNoise(int size, QFrequency clock, double deviation = 1.0, double mean = 0)**
> Возвращает комплексный шумовой сигнал размером `size` с равномерным распределением, тактовой частотой `clock`,
> дисперсией `deviation` и математическим ожиданием `mean`, начиная с позиции `start`.

**static QComplexSignal uniformNoise(int size, double deviation = 1.0, double mean = 0)**
> Возвращает комплексный шумовой сигнал размером `size` с равномерным распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

**template<class OutputIt>**
**static void gaussianNoise(OutputIt start, int size, double deviation = 1.0, double mean = 0)**
> Аддитивно добавляет к контейнеру `size` комплексных отсчётов шума с нормальным (Гауссовским) распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

**static QComplexSignal gaussianNoise(int size, QFrequency clock, double deviation = 1.0, double mean = 0)**
> Возвращает комплексный шумовой сигнал размером `size` с нормальным (Гауссовским) распределением, тактовой частотой `clock`,
> дисперсией `deviation` и математическим ожиданием `mean`, начиная с позиции `start`.

**static QComplexSignal gaussianNoise(int size, double deviation = 1.0, double mean = 0)**
> Возвращает комплексный шумовой сигнал размером `size` с нормальным (Гауссовским) распределением, дисперсией `deviation`
> и математическим ожиданием `mean`, начиная с позиции `start`.

##### Арифметические операции над классами QRealSignal и QComplexSignal

| Операнд 1 | Операнд 2 | Результат |
|:----------------:|:---------:|:----------------:|
| Действительный | Действительный | Действительный |
| Действительный | Комплексный | Комплексный |
| Комплексный | Действительный | Комплексный |
| Комплексный | Комплексный | Комплексный |

Если оба операнда не являются числами, то размер результирующего сигнала выбирается как минимум из размеров операндов.

**template <class T>**
**auto operator+(const T& lhs)**
> Унарный плюс. В качестве аргумента могут выступать сигналы, вейвформы или фреймы. Возвращаемый тип - `QRealSignal` и `QComplexSignal` в зависимости от типа аргумента (комплексный или действительный).

**template <class T>**
**auto operator-(const T& lhs)**
> Унарный минус. Инвертирует аргумент и возвращает его в виде сигнала. В качестве аргумента могут выступать сигналы, вейвформы или фреймы. Возвращаемый тип - `QRealSignal` и `QComplexSignal` в зависимости от типа аргумента (комплексный или действительный).

**template <class T1, class T2>**
**auto operator+(const T1& lhs, const T2& rhs)**
> Возвращает сумму двух аргументов. В качестве аргументов могут выступать сигналы, вейвформы или фреймы. Возвращаемый тип - `QRealSignal` и `QComplexSignal` в зависимости от типа аргумента (комплексный или действительный).

**template <class T1, class T2>**
**auto operator-(const T1& lhs, const T2& rhs)**
> Возвращает разность двух аргументов. В качестве аргументов могут выступать сигналы, вейвформы или фреймы. Возвращаемый тип - `QRealSignal` и `QComplexSignal` в зависимости от типа аргумента (комплексный или действительный).

**template <class T1, class T2>**
**auto operator*(const T1& lhs, const T2& rhs)**
> Возвращает произведение двух аргументов. В качестве аргументов могут выступать сигналы, вейвформы, фреймы или число (комплексное или действительное). Возвращаемый тип - `QRealSignal` и `QComplexSignal` в зависимости от типа аргумента (комплексный или действительный).

**template <class T1, class T2>**
**auto operator/(const T1& lhs, const T2& rhs)**
> Возвращает произведение двух аргументов. В качестве первого аргумента могут выступать сигналы, вейвформы или фреймы, в качестве второго - число (комплексное или действительное). Возвращаемый тип - `QRealSignal` и `QComplexSignal` в зависимости от типа аргумента (комплексный или действительный).

**template <class T1, class T2>**
**T1& operator+=(T1& lhs, const T2& rhs)**
> Прибавляет к первому аргументу второй и возвращает ссылку на первый аргумент. В качестве первого аргумента могут выступать сигналы или вейвформы, в качестве второго - сигналы, вейвформы или фреймы.

**template <class T1, class T2>**
**T1& operator-=(T1& lhs, const T2& rhs)**
> Отнимает от первого аргумента второй и возвращает ссылку на первый аргумент. В качестве первого аргумента могут выступать сигналы или вейвформы, в качестве второго - сигналы, вейвформы или фреймы.

**template <class T1, class T2>**
**T1& operator*=(T1& lhs, const T2& rhs)**
> Перемножает первого аргумент со вторым и возвращает ссылку на первый аргумент. В качестве первого аргумента могут выступать сигналы или вейвформы, в качестве второго - сигналы, вейвформы, фреймы или число (комплексное или действительное).

**template <class T1, class T2>**
**T1& operator/=(T1& lhs, const T2& rhs)**
> Делит первый аргумент на второй и возвращает ссылку на первый аргумент. В качестве первого аргумента могут выступать сигналы или вейвформы, в качестве второго - число (комплексное или действительное).

#### Шаблонный класс QSignalFrame<T>
Класс ссылается на сигнал и позволяет работать с его фрагментом как с целым сигналом: изменять размер, производить математические операции и передавать в качестве аргумента в алгоритмы ЦОС.
Этот шаблонный класс играет для соответствующих типов сигналов примерно такую же роль, как класс `std::string_view` для класса `std::string`. Можно представить фрейм как
рамку, через которую доступен фрагмент сигнала, размер и положение которой можно менять.

В библиотеке присутствует два псевдонима, специализирующими шаблонный клас:
- `QRealSignalFrame`, являющийся псевдонимом `QSignalFrame<QRealSignal>`;
- `QComplexSignalFrame`, являющийся псевдонимом `QSignalFrame<QComplexSignal>`.

`Discrete` является псевдонимом `double` для `QRealSignalFrame` и псевдонимом `QComplex` для `QComplexSignalFrame` соответственно.

##### Публичные функции, имеющие аналоги в классах QRealSignal и QComplexSignal

**QSignalFrame(const T& signal)**
> Создаёт фрейм, ссылающийся на сигнал `signal`. Размер фрейма соответствует размеру сигнала, начало фрейма соответствует первому отсчёту сигнала.

**QSignalFrame(const T& signal, int width, int startPos = 0)**
> Создаёт фрейм, ссылающийся на сигнал `signal`. Размер фрейма задаётся значением `width`, положение - `startPos`.

**const Discrete& at(int i) const**
> Возвращает отсчёт сигнала на позиции `i`. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const_reference back() const**
> Для совместимости с STL. Эквивалентно `last()`. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const_iterator begin() const**
> Возвращает константный STL итератор, указывающий на первый отсчёт сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const_iterator cbegin() const**
> Возвращает константный STL итератор, указывающий на первый отсчёт сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const_iterator cend() const**
> Возвращает константный STL итератор, указывающий на воображаемый отсчёт, следующий за последним отсчётом сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**QFrequency clock() const**
> Возвращает тактовую частоту сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const_iterator constBegin() const**
> Возвращает константный STL итератор, указывающий на первый отсчёт сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

__const Discrete* constData() const__
> Возвращает указатель на константые отсчёты сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const_iterator constEnd() const**
> Возвращает константный STL итератор, указывающий на воображаемый отсчёт, следующий за последним отсчётом сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const Discrete& constFirst() const**
> Возвращает константную ссылку на первый отсчёт сигнала. Предполагается, что сигнал содержит хотя бы один отсчёт. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const Discrete& constLast() const**
> Возвращает константную ссылку на последний отсчёт сигнала. Предполагается, что сигнал содержит хотя бы один отсчёт. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**int count() const**
> Возвращает количество отсчётов внутри фрейма.

**const_reverse_iterator crbegin() const**
> Возвращает константный реверсивный STL итератор, указывающий на первый отсчёт сигнала в обратном порядке.

**const_reverse_iterator crend() const**
> Возвращает константный реверсивный STL итератор, указывающий на воображаемый отсчёт, следующий за последним отсчётом сигнала в обратном порядке.

__const Discrete* data() const__
> Возвращает указатель на константые отсчёты сигнала.

**double duration(int n) const**
> Возвращает длительность `n` отсчётов фрейма в секундах.

**double duration(int from, int to) const**
> Возвращает длительность фрагмента фрейма в секундах.

**double duration() const**
> Возвращает длительность `n` отсчётов фрейма в секундах.

**bool empty() const**
> Для совместимости с STL. Возвращает `true`, если размер фрейма равен `0`. В противном случае возвращает `false`.

**const_iterator end() const**
> Возвращает константный STL итератор, указывающий на воображаемый отсчёт, следующий за последним отсчётом сигнала. Соответствует аналогичному методу классов `QRealSignal`, `QComplexSignal`.

**const Discrete& first() const**
> Возвращает константную ссылку на первый отсчёт сигнала. Предполагается, что сигнал содержит хотя бы один отсчёт.

**const_reference front() const**
> Для совместимости с STL. Возвращает константную ссылку на первый отсчёт сигнала. Предполагается, что сигнал содержит хотя бы один отсчёт.

**bool isEmpty() const**
> Возвращает `true`, если размер фрейма равен `0`. В противном случае возвращает `false`.

**bool hasClock() const**
> Возвращает `true`, если сигнал имеет положительную тактовую частоту. В противном случае возвращает `false`.

**const Discrete& last() const**
> Возвращает константную ссылку на последний отсчёт сигнала. Предполагается, что сигнал содержит хотя бы один отсчёт.

**int length() const**
> Для совместимости с `QList`. Возвращает число отсчётов внутри фрейма.

**const_reverse_iterator rbegin() const**
> Возвращает константный реверсивный STL итератор, указывающий на первый отсчёт сигнала в обратном порядке.

**const_reverse_iterator rend() const**
> Возвращает константный реверсивный STL итератор, указывающий на воображаемый отсчёт, следующий за последним отсчётом сигнала в обратном порядке.

**const QSignalFrame& resize(int size) const**
> Изменяет размер фрейма на `size` отсчётов. При выходе границ фрейма за границы сигнала возникает неопределённое поведение.

**int size() const**
> Возвращает количество отсчётов внутри фрейма.

**const QSignalFrame& shift(int num = 1) const**
> Сдвигает положение фрейма на `num` отсчётов. Аргумент может быть отрицательным. При выходе границ фрейма за границы сигнала возникает неопределённое поведение.
**void swap(QSignalFrame& other)**
> Обменивает содержимое, позицию и размер фрейма с `other`.

**Discrete value(int i) const**
> Возвращает отсчёт сигнала на позиции `i`. Если `i` выходит за границы сигнала, возвращается `0`. Если `i` в пределах границ сигнала, целесообразнее использовать `at(i)`, который немного быстрее.

**Discrete value(int i, const Discrete& defaultValue) const**
> Возвращает отсчёт сигнала на позиции `i`. Если `i` выходит за границы сигнала, возвращается `defaultValue`.

**int width() const**
> Возвращает размер фрейма. Эквивалентно `size()`.

**const Discrete& operator[](int i) const**
> Возвращает отсчёт сигнала на позиции `i`. Эквивалентно `at(i)`.

**const QSignalFrame& setPosition(int pos) const**
> Устанавливает начало фрейма на `pos` отсчёт сигнала.

**const QSignalFrame& setWidth(int width) const**
> Изменяет размер фрейма на `width` отсчётов. Эквивалентно `resize(width)`. При выходе границ фрейма за границы сигнала возникает неопределённое поведение.

**int signalSize() const**
> Возвращает число отсчётов сигнала.

##### Публичные функции

**const QSignalFrame& decreaseWidth(int decrement = 1) const**
> Уменьшает размер фрейма на `decrement` отсчётов. Возвращает ссылку на себя. Аргумент может быть отрицательным. При выходе границ фрейма за границы сигнала возникает неопределённое поведение.

**const QSignalFrame& increaseWidth(int increment = 1) const**
> Увеличивает размер фрейма на `increment` отсчётов. Возвращает ссылку на себя. Аргумент может быть отрицательным. При выходе границ фрейма за границы сигнала возникает неопределённое поведение.

**bool isSignalEmpty() const**
> Возвращает `true`, если размер сигнала равен `0`. В противном случае возвращает `false`.

**int position() const**
> Возвращает позицию начала фрейма относительно начала сигнала.

**const QSignalFrame& resetPosition() const**
> Устанавливает начало фрейма на первый отсчёт сигнала.

**const QSignalFrame& resetSize() const**
> Устанавливает размер фрейма соответствующим размеру сигнала.

**T toSignal() const**
> Преобразует содержимое фрейма в сигнал.

#### Класс QWaveformOptions
Класс описывает параметры вейвформы: комментарий, дату и время её создания или редактирования.

##### Публичные функции

**QWaveformOptions()**
> Конструктор по-умолчанию. Создаёт экземпляр класса с пустым комментарием и текущей датой и временем.

**QWaveformOptions(const QString& comment)**
> Создаёт экземпляр класса с комментарием `comment` и текущей датой и временем.

**QWaveformOptions(const QString& comment, QDateTime dateTime)**
> Создаёт экземпляр класса с комментарием `comment` и датой и временем `dateTime`.

**QWaveformOptions(QString&& comment)**
> Создаёт экземпляр класса с комментарием `comment` и текущей датой и временем. Содержимое `comment` перемещается в экземпляр класса.

**QWaveformOptions(QString&& comment, QDateTime dateTime)**
> Создаёт экземпляр класса с комментарием `comment` и датой и временем `dateTime`. Содержимое `comment` перемещается в экземпляр класса.

**const QString& comment() const**
> Возвращает комментарий.

**void setComment(const QString& comment)**
> Устанавливает комментарий `comment`.

**QDateTime dateTime() const**
> Возвращает дату и время.

**void setDateTime(QDateTime dateTime)**
> Устанавливает дату и время `dateTime`

#### Перечисляемая константа WaveformDataType

Набор констант, описывающих формат данных, используемые при записи вейвформы в файл.
1. INT8 - целочисленный 8 бит (`int8_t`);
2. INT16 - целочисленный 16 бит (`int16_t`);
3. INT32 - целочисленный 32 бит (`int32_t`);
4. INT64 - целочисленный 64 бит (`int64_t`);
5. FLOAT - с плавающей точкой 32 бита (`float`);
6. DOUBLE - с плавающей точкой 64 бита (`double`).

#### Класс QWaveformFileInfo
Описывает информацию о файле с сохранённым сигналом: тип сигнала, формат данных, размер сигнала и т.п. Публично наследуется от QWaveformOptions.

##### Публичные функции

**QWaveformFileInfo()**
> Конструктор по-умолчанию.

**QWaveformFileInfo(double clock, WaveformDataType dataType, bool complex, int64_t signalSize, int16_t refLevel, const QString& comment, QDateTime dateTime)**
> Создаёт экземпляр класса с частотой `clock`, форматом данных `dataType`, размером `signalSize`, опорным уровнем `refLevel`, комментарием `comment` и датой создания `dateTime`. Если `isComplex` `true`, используется комплексные числа, если `false` - действительные.

**QWaveformFileInfo(double clock, WaveformDataType dataType, int64_t signalSize, bool complex, int16_t refLevel = 0, const QString& comment = "")**
> Создаёт экземпляр класса с частотой `clock`, форматом данных `dataType`, размером `signalSize`, опорным уровнем `refLevel`, комментарием `comment`. Если `isComplex` `true`, используется комплексные числа, если `false` - действительные.

**const QString& comment() const**
> Возвращает комментарий.

**void setComment(const QString& comment)**
> Устанавливает комментарий `comment`.

**QDateTime dateTime() const**
> Возвращает дату и время.

**void setDateTime(QDateTime dateTime)**
> Устанавливает дату и время `dateTime`

**QFrequency clock() const**
> Возвращает тактовую частоту сохраняемого сигнала.

**void setClock(QFrequency clock)**
> Устанавливает тактовую частоту сохраняемого сигнала `clock`.

**WaveformDataType dataType() const**
> Возвращает формат данных, используемых при сохранении.

**void setDataType(WaveformDataType dataType)**
> Устанавливает формат данных, используемых при сохранении.

**bool isInteger() const**
> Возвращает `true`, если используется целочисленный формат сохранения, иначе возвращает `false`.

**bool isComplex() const**
Возвращает `true`, если сигнал комплексный, иначе возвращает `false`.

**void setComplex(bool isComplex)**
> Устанавливает тип сохраняемого сигнала. Если `isComplex` `true`, используется комплексные числа, если `false` - действительные.

**int signalSize() const**
>Возвращает размер сохраняемого сигнала.

**void setSignalSize(int64_t size)**
> Устанавливает размер сохраняемого сигнала, равный `size`.

**int16_t refLevel() const**
> Возвращает опорный уровень вейвформы, выраженный в дБВ.

**void setRefLevel(int16_t value)**
> Устанваливает опорный уровень вейвформы, выраженный в дБВ и равный `value`.

**static QWaveformFileInfo analyse(const QString& fileName)**>
Анализирует файл с сохранённым сигналом и возвращает его параметры в виде экземпляра класса QWaveformFileInfo.

#### QRealWaveform
Класс, позволяющий экспортировать действительный сигнал в файлы и загружать сигнал из файла. При сохранении имеется возможность
выбора формата сохранения и добавления тестового описания файла.

#### QComplexWaveform
Класс, позволяющий экспортировать комплексный сигнал в файлы и загружать сигнал из файла. При сохранении имеется возможность
выбора формата сохранения и добавления тестового описания файла.

#### Класс QFft
Класс, выполняющий быстрое преобразовании Фурье (БПФ). В каждом экземпляре класса
хранятся поворачивающие множители БПФ, что позволяет существенно ускорить вычисление
преобразования при частом использовании. Пересчёт поворачивающих множитетелей производится
при создании экземпляра класса или при изменении размерности БПФ.

##### Публичные функции

**explicit QFft(bool inverted = false)**
> Конструктор по-умолчанию. Параметр `inverted` задаёт тип преобразования: прямое или обратное.

**explicit QFft(int size, bool inverted = false)**
> Создаёт экземпляр класса БПФ размерностью `size`. Параметр `inverted` задаёт тип преобразования: прямое или обратное.

**QComplexSignal compute(const T& signal) const**
> Выполняет БПФ над аргументом. В качестве аргумента может выступать сигнал, вейвформа или фрейм. Если размер аргумента больше размерности БПФ, лишние отсчёты игнорируются, если размер аргумента меньше размерности БПФ, аргумент дополняется нулями.

**void setPower(int power)**
> Устанавливает размерность БПФ путём задания показателя степени с основанием 2. Вызывает пересчёт поворачивающих множителей.

**int power() const**
> Возвращает показатель степени БПФ как двоичный логарифм от размерности.

**void setSize(int size)**
> Устанавливает размерность БПФ. Если аргумент не является степенью числа 2, выбирается ближаёшее к аргументу число, являющееся степенью числа 2. Вызывает пересчёт поворачивающих множителей.

**int size() const**
> Возвращает размерность БПФ.

**void setInverted(bool inverted)**
> Устанавливает тип преобразования: прямое или обратное. Изменение значения не вызывает пересчёта поворачивающих множителей.

**bool inverted() const**
> Возвращает тип преобразования: прямое или обратное.

**QComplexSignal operator()(const T& signal) const**
> Выполняет БПФ над аргументом. Эквивалентно `QComplexSignal compute(const T& signal) const`.

#### Класс QAbstractWindow
Абстрактный класс, описывающий весовое окно. Создание отдельного класса вместо использования свободных функций обусловлено наличием задач, где необходимо
использовать набор различных весовых функций или одну весовую функцию с различными параметрами. Для таких случаев удобно использовать полиморфный вызов
соответствующих методов для генерации весовых окон и получения списка их названий. В случкае отсутствия необходимости использования набора весовых функций
можно использовать статические методы, определённые для каждого класса весовой функции в данной библиотеке.

##### Публичные функции
**virtual const QString& name() const = 0**
> Возвращает название весового окна.

**virtual QRealSignal makeWindow(int size) = 0**
> Формирует весовое окно длиной `size`.

#### Класс QAlphaParametric
Для многих весовых окон при генерации можно задать параметр, влияющий на их характеристики. Данный класс предназначен для описания одного параметра типа `double`.

##### Публичные функции
**QAlphaParametric()**
> Конструктор по-умолчанию. Параметр приравнивается к 0.

**explicit QAlphaParametric(double alpha)**
> Создаёт экземпляр класса с параметром, равным `alpha`.

**virtual void setAlpha(double alpha)**
> Устанавливает значение параметра, равное  `alpha`.

**double alpha()**
> Возвращает значение параметра.

#### Класс QBartlettHannWindow
Класс для генерации весового окна Бартлетта-Ханна. Публично наследован от класса `QAbstractWindow`.

**QBartlettHannWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QBlackmanWindow
Класс для генерации весового окна Блэкмана. Публично наследован от класса `QAbstractWindow`.

**QBlackmanWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QBlackmanHarrisWindow
Класс для генерации весового окна Блэкмана-Харриса. Публично наследован от класса `QAbstractWindow`.

**QBlackmanHarrisWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QBohmanWindow
Класс для генерации весового окна Бомана. Публично наследован от класса `QAbstractWindow`.

**QBohmanWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QChebyshevWindow
Класс для генерации весового окна Чебышева. Публично наследован от классов `QAbstractWindow` и `QAlphaParametric`.

**QChebyshevWindow(double alpha = defaultAlphaValue)**
> Создаёт экземпляр класса весового окна с параметром, равным `alpha`.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static constexpr double defaultAlphaValue = 4.0**
> Значение параметра весового окна по-умолчанию.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QFlattopWindow
Класс для генерации весового окна с плоской вершиной. Публично наследован от класса `QAbstractWindow`.

**QFlattopWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QGaussianWindow
Класс для генерации весового окна Гаусса. Публично наследован от классов `QAbstractWindow` и `QAlphaParametric`.

**QGaussianWindow(double alpha = defaultAlphaValue)**
> Создаёт экземпляр класса весового окна с параметром, равным `alpha`.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static constexpr double defaultAlphaValue = 3.0**
> Значение параметра весового окна по-умолчанию.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QHammingWindow
Класс для генерации весового окна Хэмминга. Публично наследован от классов `QAbstractWindow` и `QAlphaParametric`.

**QHammingWindow(double alpha = defaultAlphaValue)**
> Создаёт экземпляр класса весового окна с параметром, равным `alpha`.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static constexpr double defaultAlphaValue = 0.53856**
> Значение параметра весового окна по-умолчанию.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QHannWindow
Класс для генерации весового окна Ханна. Публично наследован от класса `QAbstractWindow`.

**QHannWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QKaiserWindow
Класс для генерации весового окна Кайзера. Публично наследован от классов `QAbstractWindow` и `QAlphaParametric`.

**QKaiserWindow(double alpha = defaultAlphaValue)**
> Создаёт экземпляр класса весового окна с параметром, равным `alpha`.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static constexpr double defaultAlphaValue = 3.0**
> Значение параметра весового окна по-умолчанию.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QNuttallWindow
Класс для генерации весового окна Наталла. Публично наследован от класса `QAbstractWindow`.

**QNuttallWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QParzenWindow
Класс для генерации весового окна Парзена. Публично наследован от класса `QAbstractWindow`.

**QParzenWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QTriangularWindow
Класс для генерации треугольногго весового окна. Публично наследован от класса `QAbstractWindow`.

**QTriangularWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QTukeyWindow
Класс для генерации весового окна Тьюки. Публично наследован от классов `QAbstractWindow` и `QAlphaParametric`.

**QTukeyWindow(double alpha = defaultAlphaValue)**
> Создаёт экземпляр класса весового окна с параметром, равным `alpha`.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static constexpr double defaultAlphaValue = 0.5**
> Значение параметра весового окна по-умолчанию.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Класс QRectangleWindow
Класс для генерации прямоугольного весового окна. Публично наследован от класса `QAbstractWindow`.

**QRectangleWindow()**
> Конструктор по-умолчанию. Создаёт экземпляр класса весового окна.

**const QString& name() const override**
> Возвращает название весового окна.

**QRealSignal makeWindow(int size) override**
> Формирует весовое окно длиной `size`.

**static const QString windowName**
> Возвращает название весового окна.

**static QRealSignal generate(int size)**
> Формирует весовое окно длиной `size`.

#### Создание собственных весовых окон
Для создания собственного весового окна необходимо:
1. Объявить класс собственного весового окна, публично унаследовав его от класса `QAbstractWindow` и, при необходимости, от `QAlphaParametric`;
2. Переопределить метод `virtual QRealSignal makeWindow(int size)` объявленного класса;
3. Переопределить метод `virtual const QString& name() const` объявленного класса;
4. Рекомендуется задать имя для весового окна с помощью публичной статической константы `static const QString windowName` и использовать его в реализации метода в п.1;
5. Рекомендуется определить публичный статический метод `static QRealSignal generate(int size)` для формирования весового окна и использовать его в реализации метода в п.2.

Пункты 3 и 4 рекомендуется реализовать для возможности генерации весовых окон без создания экземпляра описанного класса.

### Алгоритмы
_Описание в разработке_
#### QRealSignal abs(const T& signal)
Модуль сигнала
#### QRealSignal arg(const QComplexSignal& signal)
Аргумент комплексного сигнала
#### double peak(const T& signal)
Пиковое значение сигнала
#### double rms(const T& signal)
Среднне квадратичное значение сигнала
#### auto correlation(const T1& signal1, const T2& signal2)
Корреляция двух сигналов. Результат число
#### auto crossCorrelation(const T1& signal1, const T2& signal2, bool keepBeginTransientProcess = true, bool keepEndTransientProcess = false)
Кросс-корреляция двух сигналов, результат - сигнал
#### auto autoCorrelation(const T& signal)
Автокорреляция сигнала, результат сигнал
#### auto convolution(const T1& signal1, const T2& signal2, bool keepBeginTransientProcess = true, bool keepEndTransientProcess = false)
Свёртка, резульат - сигнал
#### auto firFilter(const T1& coefficients, const T2& signal, bool keepBeginTransientProcess = true, bool keepEndTransientProcess = false)
КИХ фильтрация, резульат - сигнал
#### auto cicFilter(const T &signal, int order, int delay)
CIC фильтр (без умножителей), результат - сигнал
#### auto decimate(const T &signal, int factor, bool accumulate = false, bool average = false)
Децимация сигнала, результат - сигнал
#### QComplexSignal dft(const T &signal, int size, bool inverted = false)
Дискретное преобразование Фурье
____

## Модуль qrswaveform.h
_Описание в разработке_
