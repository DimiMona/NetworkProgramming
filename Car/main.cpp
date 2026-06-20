#include <Windows.h>
#include <iostream>
#include<conio.h>
#include <thread>
#include<mutex>
#include<chrono>


using std::cin;
using std::cout;
using std::endl;

using namespace std::chrono_literals;

#define ESCAPE 27
#define ENTER 13

#define MIN_TANK_CAPACITY 20
#define MAX_TANK_CAPACITY 120


class Tank
{
	const int CAPACITY;
	double fuel_level;

public:

	Tank(int capacity) :
		CAPACITY
		(
			capacity < MIN_TANK_CAPACITY ? MIN_TANK_CAPACITY :
			capacity > MAX_TANK_CAPACITY ? MAX_TANK_CAPACITY :
			capacity
		)
	{
		this->fuel_level = 0;

		cout << "Tank is ready " << this << endl;
	}

	~Tank()
	{
		cout << "Tank is over " << this << endl;
	}



	//приватные методы ................................................................................................................................
	// 
	// 

	double get_fuel_level()const
	{
		return fuel_level;
	}





	//заправка бака
	void Fill(double amount)
	{
		if (amount < 0) return;
		fuel_level += amount;
		if (fuel_level > CAPACITY) fuel_level = CAPACITY;
	}

	//расход топлива
	double give_fuel(double amount) // лучше перевести метод на TRY 
	{
		if (amount < 0) return fuel_level;
		fuel_level -= amount;
		if (fuel_level < 0) fuel_level = 0;

		return fuel_level;
	}

	void info()const
	{
		cout << "Capacity: \t " << CAPACITY << " liters. \n";
		cout << "Fuel level: \t " << fuel_level << " liters. \n";
	}


};




#define MIN_ENGIN_CONSUMPTION 4
#define MAX_ENGIN_CONSUMPTION 30


//движок 
class Engine
{
	const double CONSUMPTION; //расход на 100 км
	double consumption_per_second; //расход  за 1 секунду
	bool is_started;


public:
	Engine(double consumption) : CONSUMPTION
	(
		consumption < MIN_ENGIN_CONSUMPTION ? MIN_ENGIN_CONSUMPTION :
		consumption > MAX_ENGIN_CONSUMPTION ? MAX_ENGIN_CONSUMPTION :
		consumption
	)

	{
		consumption_per_second = CONSUMPTION * 3e-5;
		cout << "Engin is ready: " << this << endl;
		is_started = false;
	}
	double get_consumption_per_second()
	{
		return consumption_per_second;
	}
	~Engine()
	{
		cout << "Engin is over: " << this << endl;
	}

	void info()const
	{
		cout << "Consumption:\t\t" << CONSUMPTION << " liters/km \n";
		cout << "Consumption per sec.:\t" << consumption_per_second << " liters/sec. \n";
	}



	void start()
	{
		is_started = true;
	}

	void stop()
	{
		is_started = false;
	}

	bool started()const
	{
		return is_started;
	}

};
#define MAX_SPEED_LOW_LIMIT		60
#define MAX_SPEED_HIGH_LIMIT		400
class Car
{
	Engine engine;
	Tank tank;

	bool driver_inside; //как костыль - зачем машине знать о наличии водителя? 
	const int MAX_SPEED;
	int speed;
	int acceleration;
	struct CarThreads
	{
		std::mutex mutex;
		std::thread panel_thread;
		std::thread engine_idle_thread;
		std::thread free_wheeling_thread;
	}car_threads;



public:
	Car(double consumption, int capacity = 50, int max_speed=250) : 
		engine(consumption), 
		tank(capacity),
		MAX_SPEED
		(
			max_speed < MAX_SPEED_LOW_LIMIT ? MAX_SPEED_LOW_LIMIT :
			max_speed < MAX_SPEED_HIGH_LIMIT ? MAX_SPEED_HIGH_LIMIT :
			max_speed
		)
	{
		speed = 0;
		acceleration = MAX_SPEED / 10;
		driver_inside = false;
		cout << "Your car is ready to go, press Enter to get in " << this << endl;
	}

	~Car()
	{
		cout << "Car is over: " << this << endl;
	}


	void get_in() //костыль - машина сажает водителя внутрь
	{

		driver_inside = true;
		//panel();

		if (!car_threads.panel_thread.joinable())
		{
			car_threads.panel_thread = std::thread(&Car::panel, this);
		}
	}

	void get_out() //костыль - машина высаживает водителя.
	{
		driver_inside = false;
		if (car_threads.panel_thread.joinable()) car_threads.panel_thread.join();
		system("CLS");

		cout << "Your are out of the car" << endl;

	}
	void startup()
	{
		if (tank.give_fuel(0))
		{
			engine.start();
			if(!car_threads.engine_idle_thread.joinable())
			car_threads.engine_idle_thread = std::thread(&Car::engine_idle, this);
		}
	}
	void shutdown()
	{
		engine.stop();
		if (car_threads.engine_idle_thread.joinable())
			car_threads.engine_idle_thread.join();
	}
	void free_wheeling()
	{
		while (speed > 0)
		{
			speed--;
			//if (speed < 0)speed = 0;
			std::this_thread::sleep_for(1s);
		}
	}
	void engine_idle()
	{
		while (engine.started() && tank.give_fuel(engine.get_consumption_per_second()))
		{
			std::this_thread::sleep_for(1s);
		}
	}
	void accelerate()
	{
		if (engine.started())
		{
			speed += acceleration;
			if (speed > MAX_SPEED) speed = MAX_SPEED;
			if (!car_threads.free_wheeling_thread.joinable())
				car_threads.free_wheeling_thread = std::thread(&Car::free_wheeling, this);
			std::this_thread::sleep_for(1s);
		}
	}
	void slow_down()
	{
		if (speed > 0)
		{
			speed -= acceleration;
			if (speed < 0)speed = 0;
			if (speed == 0 && car_threads.free_wheeling_thread.joinable())
				car_threads.free_wheeling_thread.join();
			std::this_thread::sleep_for(1s);
		}
	}

	void control()
	{
		char key;

		do
		{

			key = 0;
				if(_kbhit())key = _getch(); //ловим нажатую клавишу

			switch (key)
			{
			case ENTER:
				if (driver_inside) get_out();
				else get_in();
				break;
			case'F':
			case'f':
				car_threads.mutex.lock();
				if (!driver_inside && !engine.started())
				{
					double amount;
					cout << "Введите объем топлива: "; cin >> amount;
					tank.Fill(amount);
				}
				else cout << "\nНужно заглушить двигатель и выйти из машины, у нас только самообслуживание";
				car_threads.mutex.unlock();
				break;
			case 'I':
			case 'i':
				if (driver_inside && !engine.started())startup();
				else if( driver_inside) shutdown();
				break;
			case 'W':
			case 'w':
				accelerate();
				break;
			case 'S':
			case 's':
				slow_down();
				break;
			case ESCAPE:
				get_out();
				shutdown();
			}
			if (speed < 0)speed = 0;
			if (speed == 0 && car_threads.free_wheeling_thread.joinable()) car_threads.free_wheeling_thread.join();
			if (tank.get_fuel_level() == 0 && engine.started())shutdown();



		} while (key != ESCAPE);
	}	

	void panel()
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_CURSOR_INFO cursor_info;
		GetConsoleCursorInfo(hConsole, &cursor_info);
		cursor_info.bVisible = FALSE;
		SetConsoleCursorInfo(hConsole, &cursor_info);
		
		CONSOLE_SCREEN_BUFFER_INFO  current_state;
		GetConsoleScreenBufferInfo(hConsole, &current_state);
		system("CLS");
		
		cout << "Fuel level:\t\tliters" << endl;
		cout << "Engine is " << endl;
		cout << "Speed: \t\tkm/h." << endl;

		while (driver_inside)
		{
			car_threads.mutex.lock();
			SetConsoleCursorPosition(hConsole, COORD{ 12,0 }); 
				cout << tank.get_fuel_level();
			//system("CLS");
			//cout << "Fuel level: " << tank.get_fuel_level() << " liters.\t";
			if (tank.get_fuel_level() < 5)
			{
				SetConsoleCursorPosition(hConsole, COORD{ 32,0 });

				SetConsoleTextAttribute(hConsole, 0x4F);
				cout << "LOW FUEL";
				SetConsoleTextAttribute(hConsole, 0x07);
			}
			cout << endl;
			SetConsoleCursorPosition(hConsole, COORD{ 12,1 });
			cout << (engine.started() ? "started" : "stopped");
			SetConsoleCursorPosition(hConsole, COORD{ 8,2 });
			cout.width(5);
			//cout << std::left;
			cout << speed;
			//cout << "Engine is " << (engine.started() ? "started" : "stopped") << endl;

			std::this_thread::sleep_for(100ms);
			car_threads.mutex.unlock();
		}
		cursor_info.bVisible = TRUE;
		SetConsoleCursorInfo(hConsole, &cursor_info);
	}


};




//#define TANK_CHECK
//#define ENGINE_CHECK
#define CAR_CHECK


void main()
{
	setlocale(LC_ALL, "Russian");


#ifdef TANK_CHECK
	Tank tank(40);

	int amount;

	while (true)
	{
		cout << "Введите объем топлива: "; cin >> amount;

		tank.Fill(amount);
		tank.info();
	}

#endif //TANK_CHECK;



#ifdef ENGINE_CHECK
	Engine engine(10);
	engine.info();
#endif // ENGINE_CHECK

#ifdef CAR_CHECK


	Car bmw(10, 70);
	bmw.control();


#endif // CAR_CHECK

}



