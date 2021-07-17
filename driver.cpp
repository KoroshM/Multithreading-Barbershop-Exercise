/** @file driver.cpp
 * @author Korosh Moosavi
 * @date 2021-05-10
 *
 * Modified version of driver.cpp provided by Prof. Dimpsey
 *
 * Shop.cpp file:
 * The Shop class is a monitor for barber and customer threads
 * Shop coordinates the interactions between these threads
 *   using various signals, condition variables, and a mutex
 *
 * Assumptions:
 * The driver using this class calls the below methods in an appropriate
 *   order (i.e. helloCustomer() -> byeCustomer() )
 * Driver validates parameters before passing them into these methods
 */

#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "Shop.h"

using namespace std;

void* barber(void*);
void* customer(void*);

// ThreadParam class
// This class is used as a way to pass more
// than one argument to a thread. 
class ThreadParam
{
public:
   ThreadParam(Shop* shop, int id, int service_time) :
      shop(shop),
      id(id),
      service_time(service_time) {};
   Shop* shop;
   int id;
   int service_time;
};

int main(int argc, char* argv[])
{

   // Read arguments from command line
   if (argc != 5) {
      cout << "Usage: num_barbers num_chairs num_customers service_time" << endl;
      return -1;
   }

   int num_barbers = atoi(argv[1]);
   int num_chairs = atoi(argv[2]);
   int num_customers = atoi(argv[3]);
   int service_time = atoi(argv[4]);

   for (int i = 1; i < argc; i++) // Check that all values are 1 or greater (except waiting chairs which can be 0)
   {
      if (i != 2 && atoi(argv[i]) < 1) {
         cout << "Invalid parameter: " << argv[i] << endl;
         cout << "Parameter must be greater than 0." << endl;
         return -1;
      }
   }

   if (num_chairs < 0) {
      cout << "Invalid parameter: " << num_chairs << endl;
      cout << "Parameter must be greater than or equal to 0." << endl;
      return -1;
   }

   //Many barbers, one shop, many customers
   pthread_t barber_threads[num_barbers];
   pthread_t customer_threads[num_customers];
   Shop shop(num_barbers, num_chairs);

   for (int i = 0; i < num_barbers; i++) {
      ThreadParam* barber_param = new ThreadParam(&shop, i, service_time); // Barber ID is used for indexing, so "+ 1" was removed
      pthread_create(&barber_threads[i], NULL, barber, barber_param);      //   It's added back right before printing
   }

   for (int i = 0; i < num_customers; i++) {
      usleep(rand() % 1000);
      ThreadParam* customer_param = new ThreadParam(&shop, i + 1, 0);
      pthread_create(&customer_threads[i], NULL, customer, customer_param);
   }

   // Wait for customers to finish and cancel barber
   for (int i = 0; i < num_customers; i++) {
      pthread_join(customer_threads[i], NULL);
   }

   for (int i = 0; i < num_barbers; i++) {
      pthread_cancel(barber_threads[i]);
   }

   cout << "# customers who didn't receive a service = " << shop.get_cust_drops() << endl;
   return 0;
}

void* barber(void* arg)
{
   ThreadParam* barber_param = (ThreadParam*)arg;
   Shop& shop = *barber_param->shop;
   int barbID = barber_param->id;
   int service_time = barber_param->service_time;
   delete barber_param;

   while (true) {
      shop.helloCustomer(barbID);                                          // Wait for a customer
      usleep(service_time);                                                // Perform haircut
      shop.byeCustomer(barbID);                                            // Receive payment & signal new customer
   }
   return nullptr;
}

void* customer(void* arg)
{
   ThreadParam* customer_param = (ThreadParam*)arg;
   Shop& shop = *customer_param->shop;
   int id = customer_param->id;
   delete customer_param;

   int barbID = shop.visitShop(id);                                        // Get the barber ID of an open chair
   if (barbID != -1) {                                                     // If customer got a seat proceed with transaction
      shop.leaveShop(id, barbID);
   }
   return nullptr;
}
