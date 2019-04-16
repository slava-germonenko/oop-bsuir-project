#include "passenger.hpp"
#include <iostream>
#include "passenger_exception.hpp"
#include "connection.hpp"
#define RAND_MAX 10

int passenger::max_ID_ = 0;
std::vector<user> passenger::callback_drivers_;

void passenger_options::edit_credentials(const std::string& login)
{
	auto credits = ask_new_credentials();
	auto SQLtext = form_update_credits(credits, login);
	update(SQLtext);
}

credentials passenger_options::ask_new_credentials()
{
	credentials credits;
	std::cout << "Enter name: ";
	std::getline(std::cin, credits.name, '\n');
	std::cout << "Enter surname: ";
	std::getline(std::cin, credits.surname, '\n');
	return credits;
}

void passenger_options::update(const std::string& SQLtext)
{
	connection conn("Taxi.sqlite3");
	conn.execute(SQLtext);
}

void passenger_options::change_password(const std::string& login)
{
	auto password = ask_new_password();
	auto SQLtext = form_update_password(password, login);
	update(SQLtext);
}

std::string passenger_options::ask_new_password()
{
	std::string password;
	std::cout << "Enter new password: ";
	std::getline(std::cin, password, '\n');
	return password;
}

void passenger_options::order_taxi(const std::string& login)
{
	auto order = ask_order_info();
	auto driver = select_driver();
	order.cost = generate_id();
	order.order_id = generate_id();	
	if (ask_commit(order, driver))
		order.status = COMPLETED;
	else
		order.status = DECLINED;
	auto SQLtext = form_commit(order, driver.get_credential().name, login);
	insert(SQLtext);
	callback_drivers_.clear();
}

order_info passenger_options::ask_order_info()
{
	order_info order;
	std::cout << "Enter address: ";
	std::getline(std::cin, order.from_address, '\n');
	std::cout << "Enter where to go: ";
	std::getline(std::cin, order.to_address, '\n');
	return order;
}

user passenger_options::select_driver()
{
	connection conn("Taxi.sqlite3");
	conn.execute(form_select_drivers(), nullptr, select_drivers_callback);
	print_drivers_found();
	auto driver = pick_driver();
	return driver;
}

bool passenger_options::ask_commit(const order_info& info, const user& driver)
{
	std::string commit = "null";
	print_order_info(info, driver);
	while (commit_options.count(commit) == 0) {
		std::cout << "1) Commit || 2) Decline\nInput> ";
		std::getline(std::cin, commit, '\n');
	}
	return commit_options[commit];
}

void passenger_options::insert(const std::string& SQLtext)
{
	connection conn("Taxi.sqlite3");
	conn.execute(SQLtext);
}

void passenger_options::print_order_info(const order_info& info, const user& driver)
{
	std::cout << "From " << info.from_address << " to " << info.to_address << std::endl;
	std::cout << "Cost: " << info.cost << ", driver " << driver.get_credential() << std::endl;
}

void passenger_options::print_drivers_found()
{
	for (auto i = 0; i != callback_drivers_.size(); ++i)
		std::cout << i + 1 << ") " << callback_drivers_.at(i).get_credential() << std::endl;
}
user passenger_options::pick_driver()
{
	std::string pick;
	while (true)
	{
		try {
			std::cout << "Pick driver: ";
			std::getline(std::cin, pick, '\n');
			return callback_drivers_.at(std::stoi(pick) - 1);
		} catch (std::invalid_argument& e)	{
			std::cout << "Input error! Try again, please." << std::endl;
		} catch (std::out_of_range& e) {
			std::cout << "Index given is out of range! Try again, please." << std::endl;
		}
	}	
}

int passenger_options::generate_cost()
{
	return std::rand() + 5;
}

int passenger_options::generate_id()
{
	max_ID_ = -1;
	connection conn("Taxi.sqlite3");
	conn.execute(form_max_id_req(), nullptr, max_id_callback);
	if (max_ID_ == -1)
		throw bad_order_id();
	return max_ID_ + 1;
}

std::string passenger_options::form_update_credits(const credentials& credits, const std::string& login)
{
	return "UPDATE users SET name = '" + credits.name + "', surname = '" + credits.surname + "' WHERE login = '" + login + "'";
}

std::basic_string<char> passenger_options::form_update_password(const std::basic_string<char>& password, const std::basic_string<char>& login)
{
	return "UPDATE users SET password = '" + password + "' WHERE login = '" + login + "'";
}

std::string passenger_options::form_max_id_req()
{
	return "SELECT MAX(id) FROM orders";	
}

std::string passenger_options::form_select_drivers()
{
	return "SELECT name, surname, login, password FROM users WHERE role = 1";
}

std::string passenger_options::form_commit(const order_info& info, const std::string& driver, const std::string& passenger)
{
	return "INSERT INTO orders VALUES (" + std::to_string(info.order_id) + ", '" + driver + "', '" + passenger + "', '"\
			+ info.from_address + "', '" + info.to_address + "', " + std::to_string(info.status) + ")";
}

int passenger_options::max_id_callback(void* not_used, int count, char** value, char** column_name)
{
	max_ID_ = atoi(value[0]);
	return 0;
}

int passenger_options::select_drivers_callback(void* not_used, int count, char** value, char** column_name)
{
	credentials driver_credits;
	driver_credits.name = value[0];
	driver_credits.surname = value[1];
	logging_data data;
	data.login = value[2];
	data.password = "";
	callback_drivers_.emplace_back(data, driver_credits, DRIVER);
	return 0;
}

std::map<std::string, bool> passenger::commit_options = {

	{"1", true},
	{"y", true},	
	{"yes", true},
	{"Yes", true},
	{"commit", true},
	{"Commit", true},
	{"0", false},
	{"n", false},
	{"N", false},
	{"no", false},
	{"No", false}
};