#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <algorithm>
#include <limits>

const int TOTAL_ROUNDS = 10;
const int INIT_POPULATION = 100;
const int INIT_WHEAT = 2800;
const int INIT_AREA = 1000;
const int WHEAT_EAT_PER_PERSON = 20;
const double PRICE_PLANT_ACRE = 0.5;
const int MAX_PLANT_PER_PERSON = 10;
const int MIN_PRICE_ACRE = 17;
const int MAX_PRICE_ACRE = 26;
const int MIN_WHEAT_FROM_ACRE = 1;
const int MAX_WHEAT_FROM_ACRE = 6;
const double MAX_EATEN_BY_MICE = 0.07;
const int MAX_ARRIVED = 50;
const double LOSE_IF_DEAD = 0.45;
const double PLAGUE_PROBABILITY = 0.15;

int playRound(int numRound, int& currentPopulation, int& currentWheat, int& currentArea, int& currentAreaFields, int& currentDead, int& currentArrived,
	bool& hadPlague, int& wheatFromAcre, int& wheatEaten, int& priceForAcre, double& totalDead);

std::string getFormPeople(int number);
std::string getFormBushels(int number);
std::string getFormAcres(int number);

int main() {
	srand(time(0));
	setlocale(LC_ALL, "");
	int numRound = 1;
	int currentPopulation = INIT_POPULATION;
	int currentWheat = INIT_WHEAT;
	int currentArea = INIT_AREA;
	int currentAreaFields = 0;
	int currentDead = 0;
	int currentArrived = 0;
	bool hadPlague = false;
	int wheatFromAcre = 0;
	int wheatEaten = 0;
	int priceForAcre = 0;
	double totalDead = 0;

	std::fstream saveFile;
	saveFile.open("savefile.txt", std::ios::in);
	if (saveFile.is_open()) {
		if (numRound > 0) {
			std::cout << "Загрузить предыдущее сохранение? Y/n ";
			std::string reply;
			std::cin >> reply;
			if (reply.front() == 'Y' || reply.front() == 'y') {
				saveFile >> numRound;
				saveFile >> currentPopulation;
				saveFile >> currentWheat;
				saveFile >> currentArea;
				saveFile >> currentAreaFields;
				saveFile >> currentDead;
				saveFile >> currentArrived;
				saveFile >> hadPlague;
				saveFile >> wheatFromAcre;
				saveFile >> wheatEaten;
				saveFile >> priceForAcre;
				saveFile >> totalDead;
				if (saveFile.fail() || numRound <= 0) {
					std::cout << "Ошибка в файле сохранения!\n";
					numRound = 1;
					currentPopulation = INIT_POPULATION;
					currentWheat = INIT_WHEAT;
				}
			}
		}
		saveFile.close();
	}
	int roundResult;
	while (numRound <= TOTAL_ROUNDS) {
		roundResult = playRound(numRound, currentPopulation, currentWheat, currentArea, currentAreaFields, currentDead, currentArrived,
			hadPlague, wheatFromAcre, wheatEaten, priceForAcre, totalDead);
		++numRound;
		if (roundResult == 1) {
			break;
		}
		if (roundResult == 2) {
			std::cout << "\nВаше правление привело город к гибели и разорению. Жители вымерли или разбрелись по лесам, и город превратился в руины.\n";
			break;
		}
	}
	if (roundResult == 0) {
		double P = totalDead / static_cast<double>(TOTAL_ROUNDS);
		double L = static_cast<double>(currentArea) / static_cast<double>(currentPopulation);
		if (P > 0.33 || L < 7.0) {
			std::cout << "\nИз-за вашей некомпетентности в управлении, народ устроил бунт и изгнал вас их города.Теперь вы вынуждены влачить жалкое существование в изгнании\n";
		}
		else if (P > 0.1 || L < 9.0) {
			std::cout << "\nВы правили железной рукой, подобно Нерону и Ивану Грозному. Народ вздохнул с облегчением, и никто больше не желает видеть вас правителем\n";
		}
		else if (P > 0.03 || L < 10.0) {
			std::cout << "\nВы справились вполне неплохо, у вас, конечно, есть недоброжелатели, но многие хотели бы увидеть вас во главе города снова\n";
		}
		else {
			std::cout << "\nФантастика! Карл Великий, Дизраэли и Джефферсон вместе не справились бы лучше\n";
		}
	}
	system("pause");
}

int playRound(int numRound, int& currentPopulation, int& currentWheat, int& currentArea, int& currentAreaFields, int& currentDead, int& currentArrived,
	bool& hadPlague, int& wheatFromAcre, int& wheatEaten, int& priceForAcre, double& totalDead) {
	if (numRound > 1) {
		std::cout << "\nВыйти и сохранить текущий результат? Y/n ";
		std::string reply;
		std::cin >> reply;
		bool continueRound = true;
		if (reply.front() == 'Y' || reply.front() == 'y') {
			std::fstream saveFile;
			saveFile.open("savefile.txt", std::ios::out | std::ios::trunc);
			if (saveFile.is_open()) {
				saveFile << numRound << "\n" << currentPopulation << "\n" << currentWheat << "\n" << currentArea << "\n" << currentAreaFields << "\n" <<
					currentDead << "\n" << currentArrived << "\n" << hadPlague << "\n" << wheatFromAcre << "\n" << wheatEaten << "\n" << priceForAcre << "\n" << totalDead;
				if (saveFile.fail()) {
					std::cout << "Не удалось сохранить результат! Продолжить игру? Y/n ";
					std::cin >> reply;
					if (!(reply.front() == 'Y' || reply.front() == 'y')) {
						continueRound = false;
					}
				}
				else {
					continueRound = false;
				}
			}
			saveFile.close();
		}
		if (!continueRound) {
			return 1;
		}
	}

	priceForAcre = MIN_PRICE_ACRE + rand() % (MAX_PRICE_ACRE - MIN_PRICE_ACRE + 1);
	std::cout << "\nМой повелитель, соизволь поведать тебе\n";
	std::cout << "в году " << numRound << " твоего высочайшего правления\n";
	if (numRound > 1) {
		if (currentDead > 0) {
			std::cout << currentDead << " " << getFormPeople(currentDead) << " умерли с голоду";
			if (currentArrived > 0) {
				std::cout << ", и ";
			}
		}
		if (currentArrived > 0) {
			std::cout << currentArrived << " " << getFormPeople(currentArrived) << " прибыли в наш великий город";
		}
		if (currentDead > 0 || currentArrived > 0) {
			std::cout << ";\n";
		}
	}
	if (hadPlague) {
		std::cout << "Чума уничтожила половину населения;\n";
	}
	std::cout << "Население города сейчас составляет " << currentPopulation << " " << getFormPeople(currentPopulation) << ";\n";
	if (numRound > 1) {
		std::cout << "Мы собрали " << currentAreaFields * wheatFromAcre << " " << getFormBushels(currentAreaFields * wheatFromAcre) << " пшеницы, по "
			<< wheatFromAcre << " " << getFormBushels(wheatFromAcre) << " с акра;\n";
		std::cout << "Крысы истребили " << wheatEaten << " " << getFormBushels(wheatEaten) << " пшеницы, оставив "
			<< currentWheat << " " << getFormBushels(currentWheat) << " в амбарах;\n";
	}
	else {
		std::cout << "В амбарах сейчас " << currentWheat << " " << getFormBushels(currentWheat) << " пшеницы;\n";
	}
	std::cout << "Город сейчас занимает " << currentArea << " " << getFormAcres(currentArea) << ";\n";
	std::cout << "1 акр земли стоит сейчас " << priceForAcre << " " << getFormBushels(priceForAcre) << ".\n\n";

	std::cout << "Что пожелаешь, повелитель?\n";
	int newArea;
	int newToEat;
	int newToPlant;
	constexpr auto maxSize = std::numeric_limits<std::streamsize>::max();
	std::cout << "Сколько акров земли повелеваешь купить? ";
	std::cin >> newArea;
	while (std::cin.fail() || newArea < -currentArea) {
		std::cin.clear();
		std::cin.ignore(maxSize, '\n');
		std::cout << "О повелитель, мы не знаем таких чисел!\n";
		std::cout << "Сколько акров земли повелеваешь купить? ";
		std::cin >> newArea;
	}
	
	std::cout << "Сколько бушелей пшеницы повелеваешь съесть? ";	
	std::cin >> newToEat;
	while (std::cin.fail() || newToEat < 0) {
		std::cin.clear();
		std::cin.ignore(maxSize, '\n');
		std::cout << "О повелитель, мы не знаем таких чисел!\n";
		std::cout << "Сколько бушелей пшеницы повелеваешь съесть? ";
		std::cin >> newToEat;
	}
	
	std::cout << "Сколько акров земли повелеваешь засеять? ";	
	std::cin >> newToPlant;
	while (std::cin.fail() || newToPlant < 0 || newToPlant > currentArea) {
		std::cin.clear();
		std::cin.ignore(maxSize, '\n');
		std::cout << "О повелитель, мы не знаем таких чисел!\n";
		std::cout << "Сколько акров земли повелеваешь засеять? ";
		std::cin >> newToPlant;
	}

	while (currentWheat < newArea * priceForAcre + newToEat + round(static_cast<double>(newToPlant) * PRICE_PLANT_ACRE)) {
		std::cout << "О, повелитель, пощади нас! У нас только " << currentPopulation << " " << getFormPeople(currentPopulation)
			<< ", " << currentWheat << " " << getFormBushels(currentWheat) << " пшеницы и " << currentArea << " " << getFormAcres(currentArea) << " земли!\n";
		std::cout << "Сколько акров земли повелеваешь купить? ";
		std::cin >> newArea;
		while (std::cin.fail() || newArea < -currentArea) {
			std::cin.clear();
			std::cin.ignore(maxSize, '\n');
			std::cout << "О повелитель, мы не знаем таких чисел!\n";
			std::cout << "Сколько акров земли повелеваешь купить? ";
			std::cin >> newArea;
		}
		std::cout << "Сколько бушелей пшеницы повелеваешь съесть? ";
		std::cin >> newToEat;
		while (std::cin.fail() || newToEat < 0) {
			std::cin.clear();
			std::cin.ignore(maxSize, '\n');
			std::cout << "О повелитель, мы не знаем таких чисел!\n";
			std::cout << "Сколько бушелей пшеницы повелеваешь съесть? ";
			std::cin >> newToEat;
		}
		std::cout << "Сколько акров земли повелеваешь засеять? ";
		std::cin >> newToPlant;
		while (std::cin.fail() || newToPlant < 0 || newToPlant > currentArea) {
			std::cin.clear();
			std::cin.ignore(maxSize, '\n');
			std::cout << "О повелитель, мы не знаем таких чисел!\n";
			std::cout << "Сколько акров земли повелеваешь засеять? ";
			std::cin >> newToPlant;
		}
	}
		
	currentArea += newArea;
	currentWheat -= newArea * priceForAcre + newToEat;
	newToEat -= WHEAT_EAT_PER_PERSON * currentPopulation;
	if (newToEat < 0) {
		currentDead = ceil(-static_cast<double>(newToEat) / static_cast<double>(WHEAT_EAT_PER_PERSON));
		if (currentDead > round(LOSE_IF_DEAD * static_cast<double>(currentPopulation))) {
			return 2;
		}
		totalDead += static_cast<double>(currentDead) / static_cast<double>(currentPopulation);
		currentPopulation -= currentDead;
		if (currentPopulation <= 0) {
			return 2;
		}
	}
	else {
		currentWheat += newToEat;
		currentDead = 0;
	}
	newToPlant = std::min(newToPlant, MAX_PLANT_PER_PERSON * currentPopulation);
	currentWheat -= round(newToPlant * PRICE_PLANT_ACRE);
	currentAreaFields = newToPlant;
	wheatFromAcre = MIN_WHEAT_FROM_ACRE + rand() % (MAX_WHEAT_FROM_ACRE - MIN_WHEAT_FROM_ACRE + 1);
	currentWheat += wheatFromAcre * currentAreaFields;
	wheatEaten = static_cast<double>(rand()) / static_cast<double>(RAND_MAX / MAX_EATEN_BY_MICE) * currentWheat;
	currentWheat -= wheatEaten;
	currentArrived = std::clamp(currentDead / 2 + (5 - wheatFromAcre) * currentWheat / 600 + 1, 0, MAX_ARRIVED);
	currentPopulation += currentArrived;
	hadPlague = static_cast<double>(rand()) / static_cast<double>(RAND_MAX) < PLAGUE_PROBABILITY;
	if (hadPlague) {
		currentPopulation = floor(currentPopulation / 2.0);
	}
	if (currentPopulation <= 0) {
		return 2;
	}
	return 0;
}

std::string getFormPeople(int number) {
	int tempDigit = number % 10;
	if (tempDigit <= 1 || tempDigit >= 5) {
		return "человек";
	}
	tempDigit = (number % 100) / 10;
	if (tempDigit == 1) {
		return "человек";
	}
	return "человека";
}

std::string getFormBushels(int number) {
	int tempDigit = number % 10;
	if (tempDigit == 1) {
		tempDigit = (number % 100) / 10;
		if (tempDigit == 1) {
			return "бушелей";
		}
		else {
			return "бушель";
		}
	}
	if (tempDigit == 0 || tempDigit >= 5) {
		return "бушелей";
	}
	tempDigit = (number % 100) / 10;
	if (tempDigit == 1) {
		return "бушелей";
	}
	return "бушеля";
}

std::string getFormAcres(int number) {
	int tempDigit = number % 10;
	if (tempDigit == 1) {
		tempDigit = (number % 100) / 10;
		if (tempDigit == 1) {
			return "акров";
		}
		else {
			return "акр";
		}
	}
	if (tempDigit == 0 || tempDigit >= 5) {
		return "акров";
	}
	tempDigit = (number % 100) / 10;
	if (tempDigit == 1) {
		return "акров";
	}
	return "акра";
}
