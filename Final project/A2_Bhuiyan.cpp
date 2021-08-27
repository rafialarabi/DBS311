
#define _CRT_SECURE_NO_WARNINGS
#define MAX_CART 5

#include <iostream>
#include <occi.h>
#include <ctype.h>
#include <iomanip>

using oracle::occi::Environment;
using oracle::occi::Connection;

using namespace oracle::occi;
using namespace std;

struct ShoppingCart {
	int product_id;
	double price;
	int quantity;
};

double findProduct(Connection* conn, int product_id);
int find_customer(Connection* conn, int CUST_NO);
int addToCart(Connection* conn, struct ShoppingCart cart[]);
int checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount);
bool getUserChoiceOfYesOrNo();
void displayProducts(struct ShoppingCart cart[], int productCount);
int customerLogin(Connection* conn, int customerId);
void showYearOrder(Connection*);

void displayMenu() {
	cout << "*************Main Menu by Bhuiyan ********************" << endl;
	cout << "1)     Login\n0)     Exit\n";
}

int mainMenu()
{
	displayMenu();
	int inputVal = -1;
	bool takeInput = true;
	cout << "Enter an option (0-1): ";
	while (takeInput) {
		cin >> inputVal;
		if (cin.fail()) {
			cout << endl;
			displayMenu();
			cout << "You entered a wrong value. Enter an option (0-1): ";
			cin.clear();
			cin.ignore(2000, '\n');
			inputVal = -1;
		}
		else if (inputVal < 0 || inputVal > 1) {
			cout << endl;
			displayMenu();
			cout << "You entered a wrong value. Enter an option (0-1): ";
			cin.clear();
			cin.ignore(2000, '\n');
		}
		else {
			if (char(cin.get()) == '\n') {
				takeInput = !takeInput;
			}
		}
	}
	return inputVal;
}

int main(void)
{
	Environment* env = nullptr;
	Connection* conn = nullptr;
	Statement* stmt = nullptr;
	ResultSet* rs = nullptr;

	int num;
	string str;
	string user = "dbs311_211b04";
	string pass = "31322178";
	string constr = "myoracle12c.senecacollege.ca:1521/oracle12c";

	try {

		env = Environment::createEnvironment(Environment::DEFAULT);
		conn = env->createConnection(user, pass, constr);

		bool ExistMenuDisplay = false;
		bool TurnOffSystem = false;
		int CustomerID = 0;

		//Create PL SQLS

		stmt = conn->createStatement();
		stmt->execute("create or replace PROCEDURE find_customer ( "
			"customer_id IN NUMBER,found OUT NUMBER "
			") AS "
			"first_name VARCHAR2(255 BYTE); "
			"BEGIN "
			"select CNAME INTO first_name from customers where CUST_NO=customer_id; "
			"found:=1; "
			"EXCEPTION "
			"WHEN no_data_found THEN "
			"found:=0; "
			"WHEN OTHERS THEN "
			"found:=0; "
			"END find_customer;"
		);

		stmt->execute(
			"CREATE OR REPLACE PROCEDURE find_product("
			" product_id IN NUMBER,"
			" price OUT products.prod_sell%TYPE"
			" ) AS"
			" BEGIN"
			" SELECT prod_sell INTO price FROM products WHERE prod_no=product_id; "
			" EXCEPTION"
			" WHEN no_data_found THEN"
			" price:=0;"
			" WHEN OTHERS THEN"
			" price:=0;"
			"END find_product;"
		);

		stmt->execute("CREATE OR REPLACE PROCEDURE add_order(customer_id IN NUMBER, new_order_id OUT NUMBER) AS"
			" BEGIN"
			" SELECT MAX(order_no) INTO new_order_id "
			" FROM orders; new_order_id:=new_order_id+1;"
			" INSERT INTO orders VALUES(new_order_id,36, customer_id, TO_CHAR(sysdate, 'DD-MON-YYYY'),null, null);"
			" END;"
		);


		stmt->execute("CREATE OR REPLACE PROCEDURE"
			" add_orderline(orderId IN orderlines.order_no % type,"
			" itemId IN orderlines.line_no % type,"
			" productId IN orderlines.prod_no % type,"
			" quantity IN orderlines.qty % type,"
			" price IN orderlines.price % type)"
			" AS"
			" BEGIN"
			" INSERT INTO orderlines"
			" VALUES(orderId, itemId, productId, price, quantity,null);"
			" END;"
		);
		conn->terminateStatement(stmt);
		stmt = nullptr;

		//start program
		do
		{
			while (!ExistMenuDisplay)
			{
				int loginORexit = mainMenu(); //display menu
				if (loginORexit == 0)
				{
					ExistMenuDisplay = true;
					TurnOffSystem = true;
					cout << "Thank you --- Good bye... \n";
				}
				else {
#pragma region Take-CustomerID
					cout << "Enter the customer ID: ";
					int inputVal = -1;
					bool takeInput = true;
					//getting valid input number only
					while (takeInput) {
						cin >> inputVal;
						if (cin.fail()) {
							cout << "You entered a wrong value. Enter number only: ";
							cin.clear();
							cin.ignore(2000, '\n');
							inputVal = -1;
						}
						else if (inputVal < 0) {
							cout << "You entered a wrong value. Enter number only: ";
							cin.clear();
							cin.ignore(2000, '\n');
						}
						else {
							if (char(cin.get()) == '\n') {
								takeInput = !takeInput;
							}
						}
					}

					//call procedure find_customer
					int result = find_customer(conn, inputVal);  //check login 1001
					if (result == 0)
						cout << "The customer does not exist.\n";
					else
					{
						ExistMenuDisplay = true;
						CustomerID = inputVal;
					}

#pragma endregion
				}
			}
			if (!TurnOffSystem)
			{
				ExistMenuDisplay = false;

				//Add Products to Cart
				cout << "-------------- Add Products to Cart --------------\n";
				struct ShoppingCart _cart[MAX_CART]{ 0 };
				int productCount = addToCart(conn, _cart);

				//display product
				cout << "------ - Ordered Products-----------------------------\n";
				displayProducts(_cart, productCount);
				checkout(conn, _cart, CustomerID, productCount);
			}

		} while (!TurnOffSystem);

		showYearOrder(conn);
		env->terminateConnection(conn);
	}
	catch (SQLException& sqlExcp) {
		cout << "error";
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}
	return 0;
}

int customerLogin(Connection* conn, int customerId)
{
	return find_customer(conn, customerId);
}

int find_customer(Connection* conn, int CUST_NO)
{
	try {
		//Statement* stmt = conn->createStatement("select cname from customers where cust_no=:1");
		int found = 0;// variable to store response from procedure
		Statement* stmt = conn->createStatement();
		stmt->setSQL("BEGIN"
			" find_customer(:1, :2);"
			" END;"
		);
		stmt->setNumber(1, CUST_NO);
		stmt->registerOutParam(2, Type::OCCIINT, sizeof(found));
		stmt->executeUpdate();
		found = stmt->getInt(2);

		conn->terminateStatement(stmt);
		stmt = nullptr;
		return found;
	}
	catch (SQLException& sqlExcp) {
		cout << "error";
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}
}

double findProduct(Connection* conn, int product_id)
{
	try {
		//"select prod_sell from products WHERE PROD_NO=:1");
		Statement* stmt = conn->createStatement();
		stmt->setSQL("BEGIN"
			" find_product(:1, :2);"
			" END;");
		stmt->setNumber(1, product_id);//40100
		double price = 0;
		stmt->registerOutParam(2, Type::OCCIDOUBLE, sizeof(price));
		stmt->executeUpdate();
		price = stmt->getDouble(2);
		conn->terminateStatement(stmt);
		stmt = nullptr;
		return price;
	}
	catch (SQLException& sqlExcp) {
		cout << "error";
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}
}

void displayProducts(struct ShoppingCart cart[], int productCount)
{
	double total = 0;
	for (int i = 0; i < productCount; i++)
	{
		cout << "--- Item " << i + 1 << "   Product ID : " << cart[i].product_id << "   Price : " << cart[i].price << "  Quantity : " << cart[i].quantity << "\n";
		total = total + (cart[i].price * cart[i].quantity);
	}
	cout << "-------------------------------------------------------\n";
	cout << "Total : " << std::fixed << std::setprecision(2) << total << "\n";
	cout << "=======================================================\n";
}

bool getUserChoiceOfYesOrNo()
{
	bool Response = false;
	char _choice[10];
	//in.getline(_choice,10);
	cin >> _choice;
	while (!(_choice[0] == 'Y' || _choice[0] == 'y' || _choice[0] == 'N' || _choice[0] == 'n') || cin.fail() || strlen(_choice) > 1)
	{
		cout.clear();
		cout << "Wrong input. Please try again...";
		cout << "Would you like to checkout ? (Y / y or N / n) ";
		cin >> _choice;
	}
	if (_choice[0] == 'Y' || _choice[0] == 'y') {
		Response = true;
	}
	return Response;
}

int addToCart(Connection* conn, struct ShoppingCart cart[])
{
	//The customer can purchase up to five items in one order. 
	int cartList = 0;
	bool LoadCart = true;
	do
	{
		int inputVal = -1;
		double price = 0;
		bool takeInput;
		bool takeProductId = true;
		do {
			takeInput = true;
			cout << "Enter the product ID: ";
			while (takeInput)
			{
				cin >> inputVal;
				if (cin.fail()) {
					cout << "You entered a wrong value. Enter number only: ";
					cin.clear();
					cin.ignore(2000, '\n');
					inputVal = -1;
				}
				else if (inputVal < 0) {
					cout << "You entered a wrong value. Enter number only: ";
					cin.clear();
					cin.ignore(2000, '\n');
				}
				else {
					if (char(cin.get()) == '\n') {
						takeInput = !takeInput;
					}
				}
			}

			price = 0;
			price = findProduct(conn, inputVal);
			if (price == 0)
				cout << "Product not found\n";
			else
				takeProductId = false;
		} while (takeProductId);

		cart[cartList].product_id = inputVal;
		cart[cartList].price = price;
		cout << "Product Price: " << price;
		cout << "\nEnter the product Quantity: ";
		inputVal = -1;
		takeInput = true;
		//getting valid input number only
		while (takeInput) {
			cin >> inputVal;
			if (cin.fail()) {
				cout << "You entered a wrong value. Enter number only: ";
				cin.clear();
				cin.ignore(2000, '\n');
				inputVal = -1;
			}
			else if (inputVal < 0) {
				cout << "You entered a wrong value. Enter number only: ";
				cin.clear();
				cin.ignore(2000, '\n');
			}
			else {
				if (char(cin.get()) == '\n') {
					takeInput = !takeInput;
				}
			}
		} //input validation

		cart[cartList].quantity = inputVal;
		cartList++;
		inputVal = -1;

		if (cartList < MAX_CART)
		{
			cout << "Enter 1 to add more products or 0 to checkout:";
			takeInput = true;
		}
		else
		{
			cout << "Cart is full.\n";
			inputVal = 0;
		}

		//getting valid input number only
		while (takeInput) {
			cin >> inputVal;
			if (cin.fail()) {
				cout << "You entered a wrong value. Enter number (0-1) only: ";
				cin.clear();
				cin.ignore(2000, '\n');
				inputVal = -1;
			}
			else if (inputVal < 0 || inputVal > 1) {
				cout << "You entered a wrong value. Enter number (0-1) only: ";
				cin.clear();
				cin.ignore(2000, '\n');
			}
			else {
				if (char(cin.get()) == '\n') {
					takeInput = !takeInput;
				}
			}
		} //input validation

		if (inputVal == 0)
			LoadCart = false;
	} while (LoadCart && cartList < MAX_CART);
	return cartList;
}

int checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount)
{
	int orderNumber = 0;
	cout << "Would you like to checkout ? (Y / y or N / n) ";
	bool response = getUserChoiceOfYesOrNo();
	if (response)
	{
		//get next order number
		Statement* stmt = conn->createStatement();
		try {
			stmt->setSQL("BEGIN"
				" add_order(:1, :2);"
				" END;"
			);
			stmt->setNumber(1, customerId);
			stmt->registerOutParam(2, Type::OCCIINT, sizeof(orderNumber));
			stmt->executeUpdate();
			orderNumber = stmt->getInt(2);
		}
		catch (SQLException& sqlExcp) {
			cout << "error";
			cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
		}
		conn->terminateStatement(stmt);
		stmt = nullptr;
	}
	else
	{
		cout << "The order is cancelled.\n";
		return 0;
	}

	//add products to database
	Statement* stmt = conn->createStatement();
	for (auto i = 0; i < productCount; i++)
	{
		try {
			stmt->setSQL("BEGIN"
				" add_orderline(:1, :2, :3, :4, :5);"
				" END;"
			);
			stmt->setNumber(1, orderNumber);
			stmt->setNumber(2, i + 1); //item number
			stmt->setNumber(3, cart[i].product_id);
			stmt->setDouble(5, cart[i].price);
			stmt->setNumber(4, cart[i].quantity);
			stmt->executeUpdate();
		}
		catch (SQLException& sqlExcp) {
			cout << "error";
			cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
		}
	}
	conn->terminateStatement(stmt);
	cout << "The order is successfully completed.\n";

	return 1;
}

void showYearOrder(Connection* conn)
{
	try {
		Statement* stmt = conn->createStatement("SELECT"
			" cus.cname, orders.order_no,products.prod_name, (orderlines.price * orderlines.qty) FROM customers cus"
			" INNER JOIN orders ON cus.cust_no = orders.cust_no"
			" INNER JOIN orderlines ON orders.order_no = orderlines.order_no"
			" INNER JOIN products ON orderlines.prod_no = products.prod_no"
			" WHERE"
			" orders.order_dt like '%2021%'"
			" ORDER BY orders.order_no DESC");
		ResultSet* results = stmt->executeQuery();
		if (!results->next())
		{
			cout << "\nThere is no orders information to be displayed." << endl;
		}
		else
		{
			cout << endl;
			cout << "All orders of 2021\n";
			cout.width(30);
			cout << "Customer Name";
			cout.width(10);
			cout << "Order No";
			cout.width(20);
			cout << "Product Name";
			cout.width(15);
			cout.right;
			cout << "Total Dollars\n";
			do
			{
				cout.width(30);
				cout << results->getString(1); //cname
				cout.width(10);
				cout << results->getInt(2); //order_no
				cout.width(20);
				cout << results->getString(3); //prod_name
				cout.width(15);
				cout << std::fixed << std::setprecision(2) << results->getFloat(4) << '\n';
			} while (results->next());
		}

		//stmt->execute("ROLLBACK"); //user can rollback from here as well
		conn->terminateStatement(stmt);
		stmt = nullptr;

		cout << endl;
		cout << endl;
		cout << "Group Members\n";
		cout << "Saroj Shrestha 140486192 BB\n";
		cout << "Md Rafi Al Arabi Bhuiyan 147307193 BB\n";
		cout << "Mustafa Malik 120691191 BB\n";
		cout << "Syed Ahsan Sirat 149321192 BB\n";
	}
	catch (SQLException& sqlExcp) {
		cout << "error\n";
		cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
	}
}