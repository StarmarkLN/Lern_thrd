//--- строка компиляции и построения + запуск -----------------------
//--- g++ -std=c++17 -O2 -Wall -pedantic -pthread xxx.cpp && ./a.out
//
//--- Не работает с C++98 
//--- Работает, начиная с C++11 
//-------------------------------------------------------------------


#include <chrono>
#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>

using namespace std::chrono_literals;

std::mutex cerr_mutex;
std::mutex cout_mutex;
std::mutex cout_guard;


void foo(int &val)
{
    //--- Переданное по ссылке значение val
    std::cout << "foo: " << val << std::endl;
}


void foo1(const std::string &message)
{
    //--- урок 5
    //--- mutex позволяет ограничить доступ к ресурсам в других потоках
    cout_guard.lock();
    std::cout << "thread " << std::this_thread::get_id() << ", message " << message << std::endl;

    //--- урок 6
    //--- не забыть разлочить мьютекс! Иначе зависнет программа
	cout_guard.unlock();
}


void foo3()
{
    std::this_thread::sleep_for(4s); // на самом деле что-то очень полезное и безопасное
    //--- Поэтому мьютекс надо ставить в конце - перед вызовом доступа к ресурсу
    std::lock_guard<std::mutex> lock(cout_mutex);    
    std::cout << "foo3" << std::endl;
}

//---------------------------------------------------------------------
//--- урок 8 Взаимные блокировки
//--- Причина зависания кроется в перекрёстной блокировке. 
//--- Когда оба потока начинают работать каждый из них блокирует свой mutex. 
//--- То есть t1 захватил cerr_mutex, а t2 - cout_mutex. 
//--- После вызова sleep_for потоки пытаются захватить их наоборот, 
//--- еще не освободив занятые. 
//--- Для того чтобы освободить свой mutex потоку приходится ждать 
//--- пока это сделает второй, а у второго ситуация ровно такая же.
//---
//--- Самое простое решение использовать 
//--- std::lock для захвата обоих mutex
//---------------------------------------------------------------------
void fo()
{
    //cerr_mutex.lock();
    std::lock(cerr_mutex, cout_mutex);

    std::cerr << "use cerr in fo" << std::endl;
    std::this_thread::sleep_for(1s);

    //cout_mutex.lock();
    std::cout << "use cout in fo" << std::endl;

    cout_mutex.unlock();
    cerr_mutex.unlock();
}

void br()
{
    //cout_mutex.lock();
    std::lock(cerr_mutex, cout_mutex);

    std::cout << "use cout in br" << std::endl;
    std::this_thread::sleep_for(1s);

    //cerr_mutex.lock();
    std::cerr << "use cerr in br" << std::endl;

    cerr_mutex.unlock();
    cout_mutex.unlock();
}

//--------------------------------------------------------------------

//--- урок 12 Не обработанные исключения в потоке
static std::exception_ptr eptr = nullptr;

void foexp()
{
    try
    {
		//--- попытка выкинуть исключение в потоке
		//--- но оно уйдет с помошью eptr
        throw std::runtime_error("foexp()");
    }
    catch (...)
    {
		//--- а тут формируется сообщение в главный поток
		//--- сообщение уйдет посредством указателя eptr
        eptr = std::current_exception();
    }
}


//--- пример с лямбдой 
    //--- лямбда принимает пару строк по ссылкам const std::string& 
	//--- и выводит пару const std::string 
	//--- благодаря типу auto возвращаемого результата
auto func = [](const std::string& first, const std::string& second)
{
    std::cout << first << second;
};//--- Обязательно ставить ';' в конце определения лямбды


//====================================================================

int main(int argc, char *argv[])
{
	if (__cplusplus == 201703L) std::cout << "C++17\n";		//--- для параметра -std=c++17	
    else if (__cplusplus == 201402L) std::cout << "C++14\n";//--- для параметра -std=c++14
    else if (__cplusplus == 201103L) std::cout << "C++11\n";//--- для параметра -std=c++15
    else if (__cplusplus == 199711L) std::cout << "C++98\n";//--- для параметра -std=c++98
    else std::cout << "pre-standard C++\n"; 				//--- для параметра -std=c++2a

    std::cout << "std::thread::hardware_concurrency() = " << std::thread::hardware_concurrency() << std::endl;

    int answer = 77;
    //--- урок 4
	//--- передавать по ссылке следует так std::ref(answer)
    std::thread t(foo, std::ref(answer));
	
    //--- уроки 1,2
    //--- Всегда следует в вызывающем поток оторвать или вызвать join
	//--- t.detach(); //--- После detach join даст ошибку
	//--- Поэтому надо проверять на joinable
    if (t.joinable())
    {
        t.join();
    }


	std::thread t1(foo1, "каждый");
    std::thread t2(foo1, "охотник");
    std::thread t3(foo1, "желает");
    foo1("знать");
    std::thread t4(foo1, "где");
    std::thread t5(foo1, "сидит");
    std::thread t6(foo1, "фазан");

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();


    //--- урок 7
	std::thread t31(foo3);
    std::thread t32(foo3);

    t31.join();
    t32.join();


	//--- урок 8 Взаимные блокировки
    std::thread tr1(fo);
    std::thread tr2(br);

    tr1.join();
    tr2.join();


	//--- урок 12 Не обработанные исключения в потоке
    //--- перехват исключения в потоке и передача информации о нём 
	//--- через экземпляр std::exception_ptr в родительский поток 
	//--- и повторного бросания уже за пределами породившего исключения потока.
	std::thread texp(foexp);
    texp.join();

    if (eptr)
    {
        try
        {
            std::rethrow_exception(eptr);
        }
        catch (const std::exception &e)
        {
            std::cout << "From " << e.what() << " Error!" << std::endl;
        }
    }


    //--- лямбда принимает пару строк по ссылкам const std::string& 
	//--- и выводит пару const std::string 
	//--- благодаря типу auto возвращаемого результата
	/*--- вынесли за main()
    auto func = [](const std::string& first, const std::string& second)
    {
        std::cout << first << second;
    };
	*/
    std::thread thread(func, "Hello, ", "threads!");
    thread.join();

    return 0;
}
